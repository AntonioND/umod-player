// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_SOUND_EFFECT_H__
#define UMOD_SOUND_EFFECT_H__

#include <stdint.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"

int SFX_Play(mixer_channel_info *ch, umodpack_instrument *instrument_pointer);
int SFX_Loop(mixer_channel_info *ch, umod_loop_type loop_type);
int SFX_Stop(mixer_channel_info *ch);

#endif // UMOD_SOUND_EFFECT_H__
