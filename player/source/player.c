// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "global.h"
#include "mixer_channel.h"
#include "mod_channel.h"

// ============================================================================
//                              Song API
// ============================================================================

typedef struct {

#define STATE_STOPPED   0
#define STATE_PAUSED    1
#define STATE_PLAYING   2

    int         state;

    uint16_t   *pattern_indices; // Pointer to list of pattern indices
    int         length;
    int         current_pattern; // From 0 to song length

    umodpack_pattern   *pattern_pointer;
    int                 pattern_channels;
    int                 pattern_rows;

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
    umod_loaded_pack *loaded_pack = GetLoadedPack();

    int current_pattern = loaded_song.current_pattern;
    int pattern_index = loaded_song.pattern_indices[current_pattern];
    uint32_t pattern_offset = loaded_pack->offsets_patterns[pattern_index];

    //printf("Playing pattern index %d\n", pattern_index);

    uintptr_t pattern_address = (uintptr_t)loaded_pack->data + pattern_offset;

    umodpack_pattern *pattern = (umodpack_pattern *)pattern_address;

    loaded_song.pattern_pointer = pattern;

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
        uint32_t global_sample_rate = GetGlobalSampleRate();

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

void UMOD_Song_Stop(void)
{
    if (loaded_song.state == STATE_STOPPED)
        return;

    ModChannelResetAll();

    loaded_song.state = STATE_STOPPED;
}

int UMOD_Song_Play(uint32_t index)
{
    umod_loaded_pack *loaded_pack = GetLoadedPack();

    if (index >= loaded_pack->num_songs)
        return -1;

    if (loaded_song.state != STATE_STOPPED)
    {
        loaded_song.state = STATE_STOPPED;

        ModChannelResetAll();
    }

    // The default initial speed is 6 at 125 BPM
    SetSpeed(6);
    SetSpeed(125);

    // Force an update in the first step, which hopefully sets the real speed
    loaded_song.samples_left_for_tick = 0;
    loaded_song.current_ticks = loaded_song.song_speed;

    loaded_song.current_row = 0;

    uint32_t song_offset = loaded_pack->offsets_songs[index];
    uintptr_t song_address = (uintptr_t)loaded_pack->data + song_offset;

    umodpack_song *song = (umodpack_song *)song_address;

    loaded_song.length = song->num_of_patterns;
    loaded_song.pattern_indices = &(song->pattern_index[0]);

    loaded_song.current_pattern = 0;

    ReloadPatternData();

    ModChannelResetAll();

    for (int c = 0; c < UMOD_SONG_CHANNELS; c++)
    {
        // Reset panning
        ModChannelSetEffect(c, EFFECT_SET_PANNING, 128, -1);

        // Reset waveforms of vibrato and tremolo effects
        ModChannelSetEffect(c, EFFECT_VIBRATO_WAVEFORM, 0, -1);
        ModChannelSetEffect(c, EFFECT_TREMOLO_WAVEFORM, 0, -1);
    }

    loaded_song.state = STATE_PLAYING;

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
            loaded_song.state = STATE_STOPPED;
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
        {
            instrument = *loaded_song.pattern_position++;
            instrument |= ((uint16_t)*loaded_song.pattern_position++) << 8;
        }

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

int UMOD_Song_IsPlaying(void)
{
    if (loaded_song.state == STATE_PLAYING)
        return 1;

    return 0;
}

int UMOD_Song_IsPaused(void)
{
    if (loaded_song.state == STATE_PAUSED)
        return 1;

    return 0;
}

int UMOD_Song_Pause(void)
{
    if (loaded_song.state != STATE_PLAYING)
        return -1;

    loaded_song.state = STATE_PAUSED;

    return 1;
}

int UMOD_Song_Resume(void)
{
    if (loaded_song.state != STATE_PAUSED)
        return -1;

    loaded_song.state = STATE_PLAYING;

    return 1;
}

// ============================================================================
//                              Mixer API
// ============================================================================

void UMOD_Mix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size)
{
    while (buffer_size > 0)
    {
        if (loaded_song.state != STATE_PLAYING)
        {
            // If the song isn't being played, it isn't needed to call
            // UMOD_Tick(), so just call the mixer to fill all the buffer.
            MixerMix(left_buffer, right_buffer, buffer_size, 0);
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

                MixerMix(left_buffer, right_buffer, size, 1);
                left_buffer += size;
                right_buffer += size;
                buffer_size -= size;

                loaded_song.samples_left_for_tick = 0;
            }
            else // if (buffer_size < loaded_song.samples_left_for_tick)
            {
                MixerMix(left_buffer, right_buffer, buffer_size, 1);

                loaded_song.samples_left_for_tick -= buffer_size;

                return;
            }
        }
    }
}
