// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>

#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"

typedef struct {
    int volume;
    int left_panning; // 0...255
    int right_panning; // 0...255

    int left_volume; // combined volume * left_panning
    int right_volume; // combined volume * right_panning

    struct {
        uint8_t    *pointer; // Pointer to sample data
        size_t      size;
        size_t      loop_start;
        size_t      loop_end;

        uint32_t    position; // Position in the sample to read from
        uint64_t    elapsed_ticks; // 48.16. Do position++ when elapsed > period
        uint64_t    period; // 48.16
    } sample;

    uint32_t    handle; // Handle that was given to the owner of this channel
} mixer_channel_info;

static mixer_channel_info mixer_channel[MIXER_CHANNELS_MAX];

static uint32_t handle_counter;

// Flags that hold the status of every mixer channel. If 0, that channel is
// disabled. If 1, that channel is enabled.
static uint32_t used_channel_flags;

static inline uint32_t MixerChannelGetNewCounter(void)
{
    handle_counter++;

    if (handle_counter == 0)
        handle_counter++;

    return handle_counter;
}

static void MixerChannelUpdateVolumes(mixer_channel_info *ch)
{
    ch->left_volume = ch->volume * ch->left_panning;
    ch->right_volume = ch->volume * ch->right_panning;
}

// Returns a handler
uint32_t MixerChannelAllocate(void)
{
    for (uint32_t i = 0; i < MIXER_CHANNELS_MAX; i++)
    {
        if (used_channel_flags & (1 << i))
            continue;

        used_channel_flags |= (1 << i);

        uint32_t handle = (MixerChannelGetNewCounter() << 16) | i;

        mixer_channel_info *ch = &mixer_channel[i];
        ch->handle = handle;
        ch->left_panning = 128;
        ch->right_panning = 128;
        MixerChannelUpdateVolumes(ch);
        return handle;
    }

    // No channel found
    return MIXER_HANDLE_INVALID;
}

static int MixerChannelGetIndex(uint32_t handle)
{
    uint32_t channel = handle & 0xFFFF;

    // If the channel has a different handler, the handle is no longer valid
    if (mixer_channel[channel].handle != handle)
        return -1;

    return channel;
}

int MixerChannelIsPlaying(uint32_t handle)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return 0;

    if ((used_channel_flags & (1 << channel)) == 0)
        return 0;

    return 1;
}

int MixerChannelStop(uint32_t handle)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return 0;

    used_channel_flags &= ~(1 << channel);

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->sample.pointer = NULL;

    return 1;
}

int MixerChannelSetNotePeriod(uint32_t handle, uint64_t period) // 48.16
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->sample.position = 0;
    ch->sample.elapsed_ticks = 0;
    ch->sample.period = period;

    return 0;
}

int MixerChannelSetInstrument(uint32_t handle, void *instrument_pointer)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    if (instrument_pointer == NULL)
        return -1;

    // Get information from instrument struct

    umodpack_instrument *instrument = instrument_pointer;

    uint32_t size = instrument->size;
    uint32_t loop_start = instrument->loop_start;
    uint32_t loop_end = instrument->loop_end;
    uint8_t *pointer = &instrument->data[0];

    // Save data

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->sample.pointer = pointer;
    ch->sample.size = size;
    ch->sample.loop_start = loop_start;
    ch->sample.loop_end = loop_end;

    return 0;
}

int MixerChannelSetVolume(uint32_t handle, int volume)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->volume = volume;

    MixerChannelUpdateVolumes(ch);

    return 0;
}

int MixerChannelSetPanning(uint32_t handle, int panning)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->left_panning = 255 - panning;
    ch->right_panning = panning;

    MixerChannelUpdateVolumes(ch);

    return 0;
}

ARM_CODE IWRAM_CODE
void MixerMix(uint8_t *left_buffer, uint8_t *right_buffer, size_t buffer_size)
{
    // This function needs to mix all channels

    while (buffer_size > 0)
    {
        int32_t total_left = 0;
        int32_t total_right = 0;

        mixer_channel_info *ch = &mixer_channel[0];

        for (int channel = 0 ; channel < MIXER_CHANNELS_MAX; channel++, ch++)
        {
            //if ((used_channel_flags & (1 << channel)) == 0)
            //    continue;

            if (ch->sample.pointer == NULL)
                continue;

            int value = ch->sample.pointer[ch->sample.position]; // -128..127

            total_left += value * ch->left_volume;
            total_right += value * ch->right_volume;

            // Increment sample pointer if needed

            ch->sample.elapsed_ticks += (uint64_t)1 << 32; // 1.0 in 32.32 format
            while (ch->sample.elapsed_ticks >= ch->sample.period)
            {
                ch->sample.elapsed_ticks -= ch->sample.period;
                ch->sample.position++;

                if (ch->sample.position >= ch->sample.size)
                {
                    ch->sample.position = 0;

                    // TODO: loop_start loop_end
                }
            }
        }

        // Total = number of channels * (value * volume * panning)
        // It is needed to divide between number of channels * max volume * max
        // panning

        static_assert(MIXER_CHANNELS_MAX == (1 << 3));
        total_left >>= 3 + 8 + 8;
        total_right >>= 3 + 8 + 8;

        *left_buffer++ = total_left;
        *right_buffer++ = total_right;
        buffer_size--;
    }
}