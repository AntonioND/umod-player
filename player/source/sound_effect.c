// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>

#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "player.h"

int SFX_Play(mixer_channel_info *ch, umodpack_instrument *instrument_pointer)
{
    assert(ch != NULL);
    assert(instrument_pointer != NULL);

    uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

    MixerModChannelSetInstrument(ch, instrument_pointer);

    // 32.32 / 64.0 = 32.32
    uint64_t period = (sample_rate << 32) / instrument_pointer->frequency;

    MixerModChannelSetNotePeriod(ch, period); // 32.32

    MixerModChannelSetVolume(ch, 255);

    MixerModChannelStart(ch);

    return 0;
}

int SFX_Loop(mixer_channel_info *ch, umod_loop_type loop_type)
{
    assert(ch != NULL);

    MixerModChannelSetLoop(ch, loop_type);

    return 0;
}

int SFX_Stop(mixer_channel_info *ch)
{
    assert(ch != NULL);

    MixerModChannelStop(ch);

    return 0;
}
