// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_MIXER_CHANNEL_H__
#define UMOD_MIXER_CHANNEL_H__

#include <stddef.h>
#include <stdint.h>

#define MIXER_CHANNELS_MAX      8

#define MIXER_HANDLE_INVALID    0

uint32_t MixerChannelAllocate(void);
int MixerChannelIsPlaying(uint32_t handle);
int MixerChannelStop(uint32_t handle);

int MixerChannelSetSampleOffset(uint32_t handle, uint32_t offset);
int MixerChannelSetNotePeriod(uint32_t handle, uint64_t period); // 48.16
int MixerChannelSetInstrument(uint32_t handle, void *instrument_pointer);
int MixerChannelSetVolume(uint32_t handle, int volume);
int MixerChannelSetPanning(uint32_t handle, int panning);

void MixerMix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size);

#endif // UMOD_MIXER_CHANNEL_H__
