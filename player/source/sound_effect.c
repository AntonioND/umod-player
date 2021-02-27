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

int SFX_Play(mixer_channel_info *ch, umodpack_instrument *instrument_pointer)
{
    assert(ch != NULL);
    assert(instrument_pointer != NULL);

    uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

    MixerChannelSetInstrument(ch, instrument_pointer);

    // 32.32 / 64.0 = 32.32
    uint64_t period = (sample_rate << 32) / instrument_pointer->frequency;

    MixerChannelSetNotePeriod(ch, period); // 32.32

    MixerChannelSetVolume(ch, 255);

    MixerChannelStart(ch);

    return 0;
}

int SFX_Loop(mixer_channel_info *ch, umod_loop_type loop_type)
{
    assert(ch != NULL);

    MixerChannelSetLoop(ch, loop_type);

    return 0;
}

int SFX_Stop(mixer_channel_info *ch)
{
    assert(ch != NULL);

    MixerChannelStop(ch);

    return 0;
}

// ============================================================================
//                              SFX API
// ============================================================================

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

        SFX_Play(ch, instrument_pointer);

        if (loop_type != UMOD_LOOP_DEFAULT)
            SFX_Loop(ch, loop_type);
    }

    return handle;
}

int UMOD_SFX_Stop(umod_handle handle)
{
    mixer_channel_info *ch = MixerChannelGet(handle);

    if (ch == NULL)
        return -1;

    SFX_Stop(ch);

    return 0;
}
