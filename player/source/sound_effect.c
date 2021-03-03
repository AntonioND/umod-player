// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "global.h"
#include "mixer_channel.h"

typedef struct {

    // Pointer to the mixer channel that is being used for this SFX
    mixer_channel_info *ch;

    // Pointer to the instrument being played in the channel
    umodpack_instrument *instrument;

    // Handle that was given to the owner of this channel
    umod_handle handle;

    // Set to 1 when the effect is released: Flagged as low priority and
    // available if any SFX channel is needed and all other channels are being
    // used.
    int released;

} sfx_channel_info;

// TODO: Use only MIXER_SFX_CHANNELS
static sfx_channel_info sfx_channel[MIXER_CHANNELS_MAX];

// A handle is formed by two uint16_t values packed in one uint32_t. The top
// uint16_t is a counter that increments by one whenever a new handle is
// requested. The lower uint16_t is the mixer channel the handle corresponds to.
// Each mixer channel keeps track of the last handle that was returned for that
// channel.
//
// If a SFX is requested in a channel, it ends, and another SFX is played in the
// same channel, the handles won't be the same, so it can't be cancelled with
// the old handle, only with the new one.
static uint32_t handle_counter;

static umod_handle SFX_GenerateHandle(uint32_t channel)
{
    handle_counter++;

    if (handle_counter == 0)
        handle_counter++;

    umod_handle handle = (handle_counter << 16) | channel;

    return handle;
}

// Returns a channel number. On error, it returns -1
static int SFX_MixerChannelAllocate(void)
{
    // First, look for any free channel.

    for (int i = MOD_CHANNELS_MAX; i < MIXER_CHANNELS_MAX; i++)
    {
        mixer_channel_info *ch = MixerChannelGetFromIndex(i);

        if (MixerChannelIsPlaying(ch))
            continue;

        return i;
    }

    // Now, as all channels are being used, check if any of them has been
    // released.

    for (int i = MOD_CHANNELS_MAX; i < MIXER_CHANNELS_MAX; i++)
    {
        sfx_channel_info *sfx = &sfx_channel[i];

        if (sfx->released == 0)
            continue;

        mixer_channel_info *ch = MixerChannelGetFromIndex(i);

        MixerChannelStop(ch);

        return i;
    }

    // No channel found
    return -1;
}

static sfx_channel_info *SFX_MixerChannelGet(umod_handle handle)
{
    if (handle == UMOD_HANDLE_INVALID)
        return NULL;

    uint32_t channel = handle & 0xFFFF;

    // TODO: Check if channel is within range.

    sfx_channel_info *sfx = &sfx_channel[channel];

    // If the channel has a different handler, the handle is no longer valid
    if (sfx->handle != handle)
        return NULL;

    return sfx;
}

// ============================================================================
//                              SFX API
// ============================================================================

void UMOD_SFX_SetMasterVolume(int volume)
{
    if (volume > 256)
        volume = 256;
    else if (volume < 0)
        volume = 0;

    // Refresh volume of all channels
    for (int i = MOD_CHANNELS_MAX; i < MIXER_CHANNELS_MAX; i++)
    {
        mixer_channel_info *mixer_ch = MixerChannelGetFromIndex(i);
        MixerChannelSetMasterVolume(mixer_ch, volume);
    }
}

umod_handle UMOD_SFX_Play(uint32_t index, umod_loop_type loop_type)
{
    umod_loaded_pack *loaded_pack = GetLoadedPack();

    if (index >= loaded_pack->num_instruments)
        return -1;

    int channel = SFX_MixerChannelAllocate();

    if (channel == -1)
        return -1;

    umod_handle handle = SFX_GenerateHandle(channel);

    if (handle == UMOD_HANDLE_INVALID)
        return -1;

    sfx_channel_info *sfx = &sfx_channel[channel];

    // Save handle to be able to verify that the sound being played in channel X
    // is the sound the handle corresponds to.

    sfx->handle = handle;

    // Set as high priority.

    sfx->released = 0;

    // Save pointer to mixer channel for easier access.

    mixer_channel_info *ch = MixerChannelGetFromIndex(channel);
    assert(ch != NULL);

    sfx->ch = ch;

    // Save the original instrument in order to be able to return to the
    // default values (frequency, etc)

    umodpack_instrument *instrument_pointer = InstrumentGetPointer(index);
    sfx_channel[channel].instrument = instrument_pointer;

    MixerChannelSetInstrument(ch, instrument_pointer);

    // Calculate note period

    uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

    // 32.32 / 64.0 = 32.32
    uint64_t period = (sample_rate << 32) / instrument_pointer->frequency;

    MixerChannelSetNotePeriod(ch, period); // 32.32

    // Set remaining values of the mixer channel

    MixerChannelSetVolume(ch, 255);
    MixerChannelSetPanning(ch, 128);

    MixerChannelStart(ch);

    // Override loop values if needed

    if (loop_type == UMOD_LOOP_DISABLE)
    {
        MixerChannelSetLoop(ch, UMOD_LOOP_DISABLE, 0, 0);
    }
    else if (loop_type == UMOD_LOOP_ENABLE)
    {
        // If the original instrument didn't have loop information, loop
        // everything. If the original instrument had loop information, this
        // isn't needed, the mixer has already been given the information when
        // assigning the instrument.
        if (instrument_pointer->loop_start == instrument_pointer->loop_end)
        {
            MixerChannelSetLoop(ch, UMOD_LOOP_ENABLE,
                                0, instrument_pointer->size);
        }
    }

    return handle;
}

int UMOD_SFX_SetVolume(umod_handle handle, int volume)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return -1;

    MixerChannelSetVolume(sfx->ch, volume);

    return 0;
}

int UMOD_SFX_SetPanning(umod_handle handle, int panning)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return -1;

    MixerChannelSetPanning(sfx->ch, panning);

    return 0;
}

// Multiplier in format 16.16
int UMOD_SFX_SetFrequencyMultiplier(umod_handle handle, uint32_t multiplier)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return -1;

    uint32_t frequency = (multiplier * (uint64_t)sfx->instrument->frequency) >> 16;

    uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

    // 32.32 / 64.0 = 32.32
    uint64_t period = (sample_rate << 32) / frequency;

    MixerChannelSetNotePeriodPorta(sfx->ch, period); // 32.32

    return 0;
}

int UMOD_SFX_Release(umod_handle handle)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return -1;

    sfx->released = 1;

    return 0;
}

int UMOD_SFX_IsPlaying(umod_handle handle)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return 0;

    assert(sfx->ch);

    return MixerChannelIsPlaying(sfx->ch);
}

int UMOD_SFX_Stop(umod_handle handle)
{
    sfx_channel_info *sfx = SFX_MixerChannelGet(handle);

    if (sfx == NULL)
        return -1;

    MixerChannelStop(sfx->ch);

    return 0;
}
