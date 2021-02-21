// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>

#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"

static mixer_channel_info mixer_channel[MIXER_CHANNELS_MAX];

// Flags that hold the status of every mixer channel. If 0, that channel is
// disabled. If 1, that channel is enabled.
static uint32_t used_channel_flags;

static void MixerChannelUpdateVolumes(mixer_channel_info *ch)
{
    ch->left_volume = ch->volume * ch->left_panning;
    ch->right_volume = ch->volume * ch->right_panning;
}

// Handles API
// ===========

static uint32_t handle_counter;

static inline uint32_t MixerChannelGetNewCounter(void)
{
    handle_counter++;

    if (handle_counter == 0)
        handle_counter++;

    return handle_counter;
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
        ch->left_panning = 127;
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

    ch->play_state = STATE_STOP;

    return 1;
}

int MixerChannelSetSampleOffset(uint32_t handle, uint32_t offset)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    if (offset >= (ch->sample.size >> 12))
    {
        // Fail if the position is out of bounds. Stop channel.
        MixerChannelStop(handle);
        return -1;
    }

    ch->sample.position = offset << 12;

    return 0;
}

int MixerChannelSetNotePeriod(uint32_t handle, uint64_t period) // 32.32
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    if (period == 0) // TODO: Make sure this makes sense
    {
        ch->play_state = STATE_STOP;
        return -1;
    }

    ch->sample.position = 0; // 20.12

    // 20.44 / 32.32 = 52.12 = 20.12
    ch->sample.position_inc_per_sample = ((uint64_t)1 << 44) / period;

    ch->play_state = STATE_PLAY;

    return 0;
}

int MixerChannelSetNotePeriodPorta(uint32_t handle, uint64_t period) // 32.32
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    mixer_channel_info *ch = &mixer_channel[channel];

    if (period == 0) // TODO: Make sure this makes sense
    {
        ch->play_state = STATE_STOP;
        return -1;
    }

    // 20.44 / 32.32 = 52.12 = 20.12
    ch->sample.position_inc_per_sample = ((uint64_t)1 << 44) / period;

    return 0;
}

int MixerChannelSetInstrument(uint32_t handle, umodpack_instrument *instrument_pointer)
{
    int channel = MixerChannelGetIndex(handle);

    if (channel == -1)
        return -1;

    if (instrument_pointer == NULL)
        return -1;

    // Get information from instrument struct

    umodpack_instrument *instrument = instrument_pointer;

    uint64_t size = instrument->size;
    uint64_t loop_start = instrument->loop_start;
    uint64_t loop_end = instrument->loop_end;
    int8_t *pointer = &instrument->data[0];

    // Save data

    mixer_channel_info *ch = &mixer_channel[channel];

    ch->sample.pointer = pointer;
    ch->sample.size = size << 12;
    ch->sample.loop_start = loop_start << 12;
    ch->sample.loop_end = loop_end << 12;

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

// Direct access API
// =================

mixer_channel_info *MixerModChannelGet(uint32_t c)
{
    assert(c < MIXER_CHANNELS_MAX);

    mixer_channel_info *ch = &mixer_channel[c];

    return ch;
}

int MixerModChannelIsPlaying(mixer_channel_info *ch)
{
    assert(ch != NULL);

    if (ch->play_state == STATE_STOP)
        return 0;

    return 1;
}

int MixerModChannelStop(mixer_channel_info *ch)
{
    assert(ch != NULL);

    ch->play_state = STATE_STOP;

    return 1;
}

int MixerModChannelSetSampleOffset(mixer_channel_info *ch, uint32_t offset)
{
    assert(ch != NULL);

    if (offset >= (ch->sample.size >> 12))
    {
        // Fail if the position is out of bounds. Stop channel.
        ch->play_state = STATE_STOP;
        return -1;
    }

    ch->sample.position = offset << 12;

    return 0;
}

int MixerModChannelSetNotePeriod(mixer_channel_info *ch, uint64_t period) // 32.32
{
    assert(ch != NULL);

    if (period == 0) // TODO: Make sure this makes sense
    {
        ch->play_state = STATE_STOP;
        return -1;
    }

    ch->sample.position = 0; // 20.12

    // 20.44 / 32.32 = 52.12 = 20.12
    ch->sample.position_inc_per_sample = ((uint64_t)1 << 44) / period;

    ch->play_state = STATE_PLAY;

    return 0;
}

int MixerModChannelSetNotePeriodPorta(mixer_channel_info *ch, uint64_t period) // 32.32
{
    assert(ch != NULL);

    if (period == 0) // TODO: Make sure this makes sense
    {
        ch->play_state = STATE_STOP;
        return -1;
    }

    // 20.44 / 32.32 = 52.12 = 20.12
    ch->sample.position_inc_per_sample = ((uint64_t)1 << 44) / period;

    return 0;
}

int MixerModChannelSetInstrument(mixer_channel_info *ch, umodpack_instrument *instrument_pointer)
{
    assert(ch != NULL);

    if (instrument_pointer == NULL)
        return -1;

    // Get information from instrument struct

    umodpack_instrument *instrument = instrument_pointer;

    uint64_t size = instrument->size;
    uint64_t loop_start = instrument->loop_start;
    uint64_t loop_end = instrument->loop_end;
    int8_t *pointer = &instrument->data[0];

    // Save data

    ch->sample.pointer = pointer;
    ch->sample.size = size << 12;
    ch->sample.loop_start = loop_start << 12;
    ch->sample.loop_end = loop_end << 12;

    return 0;
}

int MixerModChannelSetVolume(mixer_channel_info *ch, int volume)
{
    assert(ch != NULL);

    ch->volume = volume;

    MixerChannelUpdateVolumes(ch);

    return 0;
}

int MixerModChannelSetPanning(mixer_channel_info *ch, int panning)
{
    assert(ch != NULL);

    ch->left_panning = 255 - panning;
    ch->right_panning = panning;

    MixerChannelUpdateVolumes(ch);

    return 0;
}

// Mixer function
// ==============

#define UNROLLED_LOOP_ITERATIONS    16

ARM_CODE IWRAM_CODE
void MixerMix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size)
{
    // Get list of all active channels

    int active_channels = 0;
    mixer_channel_info *active_ch[MIXER_CHANNELS_MAX];

    for (int channel = 0; channel < MIXER_CHANNELS_MAX; channel++)
    {
        mixer_channel_info *ch = &mixer_channel[channel];

        //if ((used_channel_flags & (1 << channel)) == 0)
        //    continue;

        if (ch->play_state == STATE_STOP)
            continue;

        if (ch->sample.pointer == NULL)
            continue;

        // Make sure that the unrolled iterations won't go over the extra
        // samples appended to the end of the instrument waveform.
        assert(ch->sample.position_inc_per_sample <
               (UMODPACK_INSTRUMENT_EXTRA_SAMPLES / UNROLLED_LOOP_ITERATIONS) << 12);

        active_ch[active_channels++] = ch;
    }

    // Mix active channels

    while (buffer_size >= UNROLLED_LOOP_ITERATIONS)
    {
        for (int c = 0; c < UNROLLED_LOOP_ITERATIONS; c++)
        {
            int32_t total_left = 0;
            int32_t total_right = 0;

            for (int i = 0; i < active_channels; i++)
            {
                mixer_channel_info *ch = active_ch[i];

                // -128..127
                int32_t value = ch->sample.pointer[ch->sample.position >> 12];
                ch->sample.position += ch->sample.position_inc_per_sample;

                total_left += value * ch->left_volume;
                total_right += value * ch->right_volume;
            }

            // Total = sample * number of channels * volume * panning
            //       -128...127         8            0...255  0...255
            //
            // The result needs to be scaled down and clamped to -128...127
            //
            // Divide by volume, panning first. Then, divide by a number smaller
            // than the number of channels. 4 seems to be a good number to keep the
            // volume up.

            static_assert(MIXER_CHANNELS_MAX == 8);
            total_left >>= 2 + 8 + 8;  // 4 * max volume * max panning
            total_right >>= 2 + 8 + 8; // 4 * max volume * max panning

            if (total_left < -128)
                total_left = -128;
            if (total_right < -128)
                total_right = -128;
            if (total_left > 127)
                total_left = 127;
            if (total_right > 127)
                total_right = 127;

            *left_buffer++ = total_left;
            *right_buffer++ = total_right;
        }

        buffer_size -= UNROLLED_LOOP_ITERATIONS;

        for (int i = 0; i < active_channels; i++)
        {
            mixer_channel_info *ch = active_ch[i];

            if (ch->play_state == STATE_PLAY)
            {
                if (ch->sample.position >= ch->sample.size)
                {
                    if (ch->sample.loop_end == ch->sample.loop_start)
                    {
                        ch->sample.position = 0;
                        ch->play_state = STATE_STOP;

                        // Remove this channel from the list
                        for (int j = i; j < active_channels - 1; j++)
                            active_ch[j] = active_ch[j + 1];

                        active_channels--;

                        break;
                    }
                    else
                    {
                        uint64_t len = ch->sample.size - ch->sample.loop_start;

                        ch->sample.position -= len;

                        ch->play_state = STATE_LOOP;
                    }
                }
            }
            else // if (ch->play_state == STATE_LOOP)
            {
                while (ch->sample.position >= ch->sample.loop_end)
                {
                    uint64_t len = ch->sample.loop_end - ch->sample.loop_start;

                    ch->sample.position -= len;
                }
            }
        }
    }

    if (buffer_size == 0)
        return;

    while (buffer_size > 0)
    {
        int32_t total_left = 0;
        int32_t total_right = 0;

        for (int i = 0; i < active_channels; i++)
        {
            mixer_channel_info *ch = active_ch[i];

            // -128..127
            int32_t value = ch->sample.pointer[ch->sample.position >> 12];
            ch->sample.position += ch->sample.position_inc_per_sample;

            total_left += value * ch->left_volume;
            total_right += value * ch->right_volume;
        }

        // Total = sample * number of channels * volume * panning
        //       -128...127         8            0...255  0...255
        //
        // The result needs to be scaled down and clamped to -128...127
        //
        // Divide by volume, panning first. Then, divide by a number smaller
        // than the number of channels. 4 seems to be a good number to keep the
        // volume up.

        static_assert(MIXER_CHANNELS_MAX == 8);
        total_left >>= 2 + 8 + 8;  // 4 * max volume * max panning
        total_right >>= 2 + 8 + 8; // 4 * max volume * max panning

        if (total_left < -128)
            total_left = -128;
        if (total_right < -128)
            total_right = -128;
        if (total_left > 127)
            total_left = 127;
        if (total_right > 127)
            total_right = 127;

        *left_buffer++ = total_left;
        *right_buffer++ = total_right;
        buffer_size--;
    }

    for (int i = 0; i < active_channels; i++)
    {
        mixer_channel_info *ch = active_ch[i];

        if (ch->play_state == STATE_PLAY)
        {
            if (ch->sample.position >= ch->sample.size)
            {
                if (ch->sample.loop_end == ch->sample.loop_start)
                {
                    ch->sample.position = 0;
                    ch->play_state = STATE_STOP;

                    // Remove this channel from the list
                    for (int j = i; j < active_channels - 1; j++)
                        active_ch[j] = active_ch[j + 1];

                    active_channels--;

                    break;
                }
                else
                {
                    uint64_t len = ch->sample.size - ch->sample.loop_start;

                    ch->sample.position -= len;

                    ch->play_state = STATE_LOOP;
                }
            }
        }
        else // if (ch->play_state == STATE_LOOP)
        {
            while (ch->sample.position >= ch->sample.loop_end)
            {
                uint64_t len = ch->sample.loop_end - ch->sample.loop_start;

                ch->sample.position -= len;
            }
        }
    }
}
