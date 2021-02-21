// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdint.h>
#include <stdio.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "mod_channel.h"

typedef struct {
    const void *data;
    uint32_t    num_songs;
    uint32_t    num_patterns;
    uint32_t    num_instruments;
    uint32_t   *offsets_songs;
    uint32_t   *offsets_patterns;
    uint32_t   *offsets_samples;
} umod_loaded_pack;

static umod_loaded_pack loaded_pack;

static uint32_t global_sample_rate;

void UMOD_Init(uint32_t sample_rate)
{
    global_sample_rate = sample_rate;

    ModSetSampleRateConvertConstant(sample_rate);
}

int UMOD_LoadPack(const void *pack)
{
    const umodpack_header *header = pack;

    if ((header->magic[0] != 'U') || (header->magic[1] != 'M') ||
        (header->magic[2] != 'O') || (header->magic[3] != 'D'))
    {
        return -1;
    }

    loaded_pack.data = pack;

    loaded_pack.num_songs = header->num_songs;
    loaded_pack.num_patterns = header->num_patterns;
    loaded_pack.num_instruments = header->num_instruments;

    // If there are songs, it is needed to at least have one pattern
    if ((loaded_pack.num_songs > 0) && (loaded_pack.num_patterns == 0))
        return -1;

    // Reject any file with no instruments
    if (loaded_pack.num_instruments == 0)
        return -2;

    uint32_t *read_ptr = (uint32_t *)((uintptr_t)pack + sizeof(umodpack_header));
    loaded_pack.offsets_songs = read_ptr;
    read_ptr += loaded_pack.num_songs;
    loaded_pack.offsets_patterns = read_ptr;
    read_ptr += loaded_pack.num_patterns;
    loaded_pack.offsets_samples = read_ptr;

    return 0;
}

static umodpack_instrument *InstrumentGetPointer(int index)
{
    uint32_t offset = loaded_pack.offsets_samples[index];
    uintptr_t instrument_address = (uintptr_t)loaded_pack.data;
    instrument_address += offset;

    return (umodpack_instrument *)instrument_address;
}

// ============================================================================
//                              Song API
// ============================================================================

typedef struct {
    int         playing;

    uint8_t    *pattern_indices; // Pointer to list of pattern indices
    int         length;
    int         current_pattern; // From 0 to song length

    void       *pattern_pointer;
    int         pattern_channels;
    int         pattern_rows;

    size_t      samples_per_tick; // Increment tick every X samples
    size_t      samples_left_for_tick;

    int         song_speed; // Ticks to advance to next row
    int         current_ticks; // Ticks elapsed in this row
    int         current_row;
    uint8_t    *pattern_position; // Current position inside pattern
} song_state;

static song_state loaded_song;

static void ReloadPatternData(void)
{
    int current_pattern = loaded_song.current_pattern;
    int pattern_index = loaded_song.pattern_indices[current_pattern];
    uint32_t pattern_offset = loaded_pack.offsets_patterns[pattern_index];

    //printf("Playing pattern index %d\n", pattern_index);

    uintptr_t pattern_address = (uintptr_t)loaded_pack.data + pattern_offset;

    loaded_song.pattern_pointer = (void *)pattern_address;

    umodpack_pattern *pattern = loaded_song.pattern_pointer;

    loaded_song.pattern_channels = pattern->channels;
    loaded_song.pattern_rows = pattern->rows;
    loaded_song.pattern_position = &pattern->data[0];
}

static void SetSpeed(int speed)
{
    if (speed == 0)
        return;

    if (speed >= 0x20)
    {
        // Default is 125 BPM -> 50 Hz
        int hz = (2 * speed) / 5;
        loaded_song.samples_per_tick = global_sample_rate / hz;
        loaded_song.samples_left_for_tick = loaded_song.samples_per_tick;
    }
    else
    {
        loaded_song.song_speed = speed;
    }
}

int UMOD_PlaySong(uint32_t index)
{
    if (index >= loaded_pack.num_songs)
        return -1;

    if (loaded_song.playing)
    {
        loaded_song.playing = 0;

        // TODO: Stop song
        ModChannelResetAll();
    }

    // The default initial speed is 6 at 125 BPM
    SetSpeed(6);
    SetSpeed(125);

    // Force an update in the first step, which hopefully sets the real speed
    loaded_song.samples_left_for_tick = 0;
    loaded_song.current_ticks = loaded_song.song_speed;

    loaded_song.current_row = 0;

    uint32_t song_offset = loaded_pack.offsets_songs[index];
    uintptr_t song_address = (uintptr_t)loaded_pack.data + song_offset;

    loaded_song.length = *(uint16_t *)song_address;
    loaded_song.pattern_indices = (void *)(song_address + 2);

    loaded_song.current_pattern = 0;

    ReloadPatternData();

    ModChannelResetAll();

    for (int c = 0; c < MOD_CHANNELS_MAX; c++)
    {
        // Reset panning
        ModChannelSetEffect(c, EFFECT_SET_PANNING, 128, -1);

        // Reset waveforms of vibrato and tremolo effects
        ModChannelSetEffect(c, EFFECT_VIBRATO_WAVEFORM, 0, -1);
        ModChannelSetEffect(c, EFFECT_TREMOLO_WAVEFORM, 0, -1);
    }

    loaded_song.playing = 1;

    return 0;
}

static int SeekRow(int row)
{
    loaded_song.current_row = row;

    while (row > 0)
    {
        for (int c = 0; c < loaded_song.pattern_channels; c++)
        {
            uint8_t flags = *loaded_song.pattern_position++;
            if (flags & STEP_HAS_NOTE)
                loaded_song.pattern_position++;
            if (flags & STEP_HAS_INSTRUMENT)
                loaded_song.pattern_position++;
            if (flags & STEP_HAS_VOLUME)
                loaded_song.pattern_position++;
            if (flags & STEP_HAS_EFFECT)
                loaded_song.pattern_position += 2;
        }
        row--;
    }

    return 0;
}

ARM_CODE IWRAM_CODE
static void UMOD_Tick(void)
{
    loaded_song.current_ticks++;

    if (loaded_song.current_ticks < loaded_song.song_speed)
    {
        ModChannelUpdateAllTick_TN(loaded_song.current_ticks);
        return;
    }

    loaded_song.current_ticks = 0;

    if (loaded_song.current_row >= loaded_song.pattern_rows)
    {
        loaded_song.current_pattern++;
        loaded_song.current_row = 0;

        if (loaded_song.current_pattern >= loaded_song.length)
        {
            loaded_song.playing = 0;
            ModChannelResetAll();
            return;
        }
        else
        {
            ReloadPatternData();
        }
    }

    int jump_to_pattern = -1;
    int pattern_break = -1;

    //printf("%d/%d : ", loaded_song.current_row, loaded_song.pattern_rows);
    //setvbuf(stdout, 0, _IONBF, 0);

    for (int c = 0; c < loaded_song.pattern_channels; c++)
    {
        uint8_t flags = *loaded_song.pattern_position++;

        int instrument = -1;
        int note = -1;
        int volume = -1;
        int effect = -1;
        int effect_params = -1;

        if (flags & STEP_HAS_INSTRUMENT)
            instrument = *loaded_song.pattern_position++;

        if (flags & STEP_HAS_NOTE)
            note = *loaded_song.pattern_position++;

        if (flags & STEP_HAS_VOLUME)
            volume = *loaded_song.pattern_position++;

        if (flags & STEP_HAS_EFFECT)
        {
            effect = *loaded_song.pattern_position++;
            effect_params = *loaded_song.pattern_position++;
        }

        if (effect == EFFECT_DELAY_NOTE)
        {
            umodpack_instrument *instrument_pointer = NULL;
            if (instrument != -1)
                instrument_pointer = InstrumentGetPointer(instrument);

            ModChannelSetEffectDelayNote(c, effect_params, note, volume,
                                         instrument_pointer);
            continue;
        }

        // This is only reached if the effect isn't Delay Note

        if (instrument != -1)
        {
            umodpack_instrument *instrument_pointer = InstrumentGetPointer(instrument);
            ModChannelSetInstrument(c, instrument_pointer);

            if (volume == -1)
                volume = instrument_pointer->volume;
        }

        if (note != -1)
        {
            if ((effect != EFFECT_PORTA_TO_NOTE) &&
                (effect != EFFECT_PORTA_VOL_SLIDE))
            {
                ModChannelSetNote(c, note);
            }
        }

        if (volume != -1)
        {
            ModChannelSetVolume(c, volume);
        }

        if (effect == -1)
        {
            ModChannelSetEffect(c, EFFECT_NONE, 0, -1);
        }
        else
        {
            if (effect == EFFECT_SET_SPEED)
            {
                SetSpeed(effect_params);
            }
            else if (effect == EFFECT_PATTERN_BREAK)
            {
                pattern_break = effect_params;
            }
            else if (effect == EFFECT_JUMP_TO_PATTERN)
            {
                jump_to_pattern = effect_params;
            }
            else
            {
                ModChannelSetEffect(c, effect, effect_params, note);
            }
        }
    }

    //printf("\n");

    ModChannelUpdateAllTick_T0();

    if (jump_to_pattern >= 0)
    {
        loaded_song.current_pattern = jump_to_pattern;
        if (loaded_song.current_pattern < loaded_song.length)
        {
            ReloadPatternData();
            loaded_song.current_row = 0;
        }
        else
        {
            // The next time that it's time to increment the row number,
            // overflow pattern, which will reach the end of the song.
            loaded_song.current_row = loaded_song.pattern_rows;
        }
    }
    else if (pattern_break >= 0)
    {
        loaded_song.current_pattern++;

        if (loaded_song.current_pattern >= loaded_song.length)
        {
            // The next time that it's time to increment the row number,
            // overflow pattern, which will reach the end of the song.
            loaded_song.current_row = loaded_song.pattern_rows;
        }
        else
        {
            ReloadPatternData();
            SeekRow(pattern_break);
        }
    }
    else
    {
        loaded_song.current_row++;
    }
}

int UMOD_IsPlayingSong(void)
{
    return loaded_song.playing;
}

// ============================================================================
//                              Mixer API
// ============================================================================

void UMOD_Mix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size)
{
    while (buffer_size > 0)
    {
        if (loaded_song.playing == 0)
        {
            MixerMix(left_buffer, right_buffer, buffer_size);
            break;
        }
        else
        {
            if (loaded_song.samples_left_for_tick == 0)
            {
                UMOD_Tick();
                loaded_song.samples_left_for_tick = loaded_song.samples_per_tick;
            }

            if (buffer_size >= loaded_song.samples_left_for_tick)
            {
                size_t size = loaded_song.samples_left_for_tick;

                MixerMix(left_buffer, right_buffer, size);
                left_buffer += size;
                right_buffer += size;
                buffer_size -= size;

                loaded_song.samples_left_for_tick = 0;
            }
            else // if (buffer_size < loaded_song.samples_left_for_tick)
            {
                MixerMix(left_buffer, right_buffer, buffer_size);

                loaded_song.samples_left_for_tick -= buffer_size;

                return;
            }
        }
    }
}
