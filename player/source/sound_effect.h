// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_SOUND_EFFECT_H__
#define UMOD_SOUND_EFFECT_H__

#include <stdint.h>

#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "player.h"

int SFX_Play(mixer_channel_info *ch, umodpack_instrument *instrument_pointer);

#endif // UMOD_SOUND_EFFECT_H__
