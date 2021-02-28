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

    // Handle that was given to the owner of this channel
    umod_handle handle;

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

static inline uint32_t MixerChannelGetNewCounter(void)
{
    handle_counter++;

    if (handle_counter == 0)
        handle_counter++;

    return handle_counter;
}

// Returns a handler
static umod_handle MixerChannelAllocate(void)
{
    for (uint32_t i = MOD_CHANNELS_MAX; i < MIXER_CHANNELS_MAX; i++)
    {
        mixer_channel_info *ch = MixerChannelGetFromIndex(i);

        if (MixerChannelIsPlaying(ch))
            continue;

        sfx_channel_info *sfx = &sfx_channel[i];

        umod_handle handle = (MixerChannelGetNewCounter() << 16) | i;

        sfx->handle = handle;

        ch->left_panning = 127;
        ch->right_panning = 128;
        MixerChannelRefreshVolumes(ch);

        return handle;
    }

    // No channel found
    return UMOD_HANDLE_INVALID;
}

mixer_channel_info *MixerChannelGet(umod_handle handle)
{
    uint32_t channel = handle & 0xFFFF;

    // If the channel has a different handler, the handle is no longer valid
    if (sfx_channel[channel].handle != handle)
        return NULL;

    mixer_channel_info *ch = MixerChannelGetFromIndex(channel);

    return ch;
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

    umod_handle handle = MixerChannelAllocate();

    if (handle != UMOD_HANDLE_INVALID)
    {
        mixer_channel_info *ch = MixerChannelGet(handle);

        assert(ch != NULL);

        umodpack_instrument *instrument_pointer = InstrumentGetPointer(index);

        uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

        MixerChannelSetInstrument(ch, instrument_pointer);

        // 32.32 / 64.0 = 32.32
        uint64_t period = (sample_rate << 32) / instrument_pointer->frequency;

        MixerChannelSetNotePeriod(ch, period); // 32.32

        MixerChannelSetVolume(ch, 255);

        MixerChannelStart(ch);

        if (loop_type != UMOD_LOOP_DEFAULT)
            MixerChannelSetLoop(ch, loop_type);
    }

    return handle;
}

int UMOD_SFX_SetVolume(umod_handle handle, int volume)
{
    mixer_channel_info *ch = MixerChannelGet(handle);

    if (ch == NULL)
        return -1;

    MixerChannelSetVolume(ch, volume);

    return 0;
}

int UMOD_SFX_SetPanning(umod_handle handle, int panning)
{
    mixer_channel_info *ch = MixerChannelGet(handle);

    if (ch == NULL)
        return -1;

    MixerChannelSetPanning(ch, panning);

    return 0;
}

int UMOD_SFX_Stop(umod_handle handle)
{
    mixer_channel_info *ch = MixerChannelGet(handle);

    if (ch == NULL)
        return -1;

    MixerChannelStop(ch);

    return 0;
}
