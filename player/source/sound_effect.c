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

// ============================================================================
//                              SFX API
// ============================================================================

void UMOD_SFX_VolumeSet(int volume)
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

int UMOD_SFX_Stop(umod_handle handle)
{
    mixer_channel_info *ch = MixerChannelGet(handle);

    if (ch == NULL)
        return -1;

    MixerChannelStop(ch);

    return 0;
}
