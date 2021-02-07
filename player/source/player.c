// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "mod_channel.h"

typedef struct {
    void       *data;
    uint32_t    num_songs;
    uint32_t    num_patterns;
    uint32_t    num_instruments;
    uint32_t   *offsets_songs;
    uint32_t   *offsets_patterns;
    uint32_t   *offsets_samples;
} umod_loaded_pack;

static umod_loaded_pack loaded_pack;

int UMOD_LoadPack(void *pack)
{
    umodpack_header *header = pack;

    if ((header->magic[0] != 'U') || (header->magic[1] != 'M') ||
        (header->magic[2] != 'O') || (header->magic[3] != 'D'))
    {
        return -1;
    }

    loaded_pack.data = pack;

    loaded_pack.num_songs = header->num_songs;
    loaded_pack.num_patterns = header->num_patterns;
    loaded_pack.num_instruments = header->num_instruments;

    if ((loaded_pack.num_songs == 0) ||
        (loaded_pack.num_patterns == 0) ||
        (loaded_pack.num_instruments == 0))
    {
        return -1;
    }

    uint32_t *read_ptr = (uint32_t *)((uintptr_t)pack + sizeof(umodpack_header));
    loaded_pack.offsets_songs = read_ptr;
    read_ptr += loaded_pack.num_songs;
    loaded_pack.offsets_patterns = read_ptr;
    read_ptr += loaded_pack.num_patterns;
    loaded_pack.offsets_samples = read_ptr;

    return 0;
}

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
        loaded_song.samples_per_tick = UMOD_SAMPLE_RATE / hz;
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

    loaded_song.playing = 1;

    return 0;
}

static void *InstrumentGetPointer(int index)
{
    uint32_t offset = loaded_pack.offsets_samples[index];
    uintptr_t instrument_address = (uintptr_t)loaded_pack.data;
    instrument_address += offset;

    return (void *)instrument_address;
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
        ModChannelUpdateAllTick(loaded_song.current_ticks);
        return;
    }

    loaded_song.current_ticks = 0;

    if (loaded_song.current_row == loaded_song.pattern_rows)
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

    //printf("%d/%d : ", loaded_song.current_row, loaded_song.pattern_rows);

    for (int c = 0; c < loaded_song.pattern_channels; c++)
    {
        uint8_t flags = *loaded_song.pattern_position++;

        if (flags & STEP_HAS_INSTRUMENT)
        {
            uint8_t instrument = *loaded_song.pattern_position++;
            //printf("I: %u ", instrument);

            void *instrument_pointer = InstrumentGetPointer(instrument);
            ModChannelSetInstrument(c, instrument_pointer);
        }

        if (flags & STEP_HAS_NOTE)
        {
            uint8_t note = *loaded_song.pattern_position++;
            //printf("N: %u ", note);
            ModChannelSetNote(c, note);
        }

        if (flags & STEP_HAS_VOLUME)
        {
            uint8_t volume = *loaded_song.pattern_position++;
            //printf("V: %u ", volume);
            ModChannelSetVolume(c, volume);
        }

        if (flags & STEP_HAS_EFFECT)
        {
            uint8_t effect = *loaded_song.pattern_position++;
            uint8_t effect_params = *loaded_song.pattern_position++;
            //printf("E: %u %u ", effect, effect_params);

            if (effect == EFFECT_SET_SPEED)
            {
                SetSpeed(effect_params);
            }
            else if (effect == EFFECT_PATTERN_BREAK)
            {
                loaded_song.current_pattern++;

                if (loaded_song.current_pattern == loaded_song.length)
                {
                    loaded_song.playing = 0;
                    ModChannelResetAll();
                    return;
                }
                else
                {
                    ReloadPatternData();
                    SeekRow(effect_params);
                    loaded_song.current_row--;
                }
            }
            else if (effect == EFFECT_JUMP_TO_PATTERN)
            {
                jump_to_pattern = effect_params;
            }
            else
            {
                ModChannelSetEffect(c, effect, effect_params);
            }
        }
        else
        {
            ModChannelSetEffect(c, EFFECT_NONE, 0);
        }
    }

    //printf("\n");

    ModChannelUpdateAllTick(loaded_song.current_ticks);

    ModChannelUpdateAllRow();

    if (jump_to_pattern >= 0)
    {
        loaded_song.current_pattern = jump_to_pattern;
        ReloadPatternData();
        loaded_song.current_row = 0;
    }
    else
    {
        loaded_song.current_row++;
    }
}

void UMOD_Mix(uint8_t *left_buffer, uint8_t *right_buffer, size_t buffer_size)
{
    while (buffer_size > 0)
    {
        if (loaded_song.samples_left_for_tick == 0)
        {
            UMOD_Tick();
            loaded_song.samples_left_for_tick = loaded_song.samples_per_tick;
        }

        if (loaded_song.playing == 0)
        {
            memset(left_buffer, 0, buffer_size);
            memset(right_buffer, 0, buffer_size);
            return;
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

int UMOD_IsPlayingSong(void)
{
    return loaded_song.playing;
}
