// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_MOD_CHANNEL_H__
#define UMOD_MOD_CHANNEL_H__

#define MOD_CHANNELS_MAX    16

void ModSetSampleRateConvertConstant(uint32_t sample_rate);

void ModChannelResetAll(void);
void ModChannelSetNote(int channel, int note);
void ModChannelSetVolume(int channel, int volume);
void ModChannelSetInstrument(int channel, void *instrument_pointer);
void ModChannelSetEffect(int channel, int effect, int effect_params, int note);
void ModChannelSetEffectDelayNote(int channel, int effect_params, int note,
                                  int volume, void *instrument);
void ModChannelUpdateAllTick(int tick_number);

#endif // UMOD_MOD_CHANNEL_H__
