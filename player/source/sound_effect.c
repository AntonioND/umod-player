// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdint.h>

#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "player.h"

int SFX_Play(mixer_channel_info *ch, umodpack_instrument *instrument_pointer)
{
    uint64_t sample_rate = (uint64_t)GetGlobalSampleRate();

    MixerModChannelSetInstrument(ch, instrument_pointer);

    // 32.32 / 64.0 = 32.32
    uint64_t period = (sample_rate << 32) / instrument_pointer->frequency;

    MixerModChannelSetNotePeriod(ch, period); // 32.32

    MixerModChannelSetVolume(ch, 255);

    MixerModChannelStart(ch);

    return 0;
}
