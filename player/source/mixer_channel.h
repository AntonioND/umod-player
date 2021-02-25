// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_MIXER_CHANNEL_H__
#define UMOD_MIXER_CHANNEL_H__

#include <stddef.h>
#include <stdint.h>

#include <umod/umodpack.h>

#include "mod_channel.h"

#define MIXER_SFX_CHANNELS      (4)
#define MIXER_CHANNELS_MAX      (MOD_CHANNELS_MAX + MIXER_SFX_CHANNELS)

#define MIXER_HANDLE_INVALID    0

typedef struct {
    int volume;         // 0...255
    int left_panning;   // 0...255
    int right_panning;  // 0...255

    int left_volume;    // volume * left_panning = 0...65535
    int right_volume;   // volume * right_panning = 0...65535

#define STATE_STOP 0
#define STATE_PLAY 1
#define STATE_LOOP 2

    // Set to loop after the first time the sample is played, if loop_start > 4
    // Note that in the mod file it would be 2, but the size is divided by 2
    // in the file, and it is multiplied by 2 by the packer.
    int play_state;

    struct {
        int8_t     *pointer;    // Pointer to sample data (signed 8 bit)
        uint32_t    size;       // 20.12
        uint32_t    loop_start; // 20.12
        uint32_t    loop_end;   // 20.12

        uint32_t    position;   // 20.12 Position in the sample to read from
        uint32_t    position_inc_per_sample; // 20.12
    } sample;

    uint32_t    handle; // Handle that was given to the owner of this channel
} mixer_channel_info;

// Handles API

uint32_t MixerChannelAllocate(void);
mixer_channel_info *MixerChannelGet(uint32_t handle);

// Direct access API

mixer_channel_info *MixerModChannelGet(uint32_t c);
int MixerModChannelIsPlaying(mixer_channel_info *ch);
int MixerModChannelStart(mixer_channel_info *ch);
int MixerModChannelStop(mixer_channel_info *ch);
int MixerModChannelSetSampleOffset(mixer_channel_info *ch, uint32_t offset);
int MixerModChannelSetNotePeriod(mixer_channel_info *ch, uint64_t period); // 32.32
int MixerModChannelSetNotePeriodPorta(mixer_channel_info *ch, uint64_t period); // 32.32
int MixerModChannelSetInstrument(mixer_channel_info *ch, umodpack_instrument *instrument_pointer);
int MixerModChannelSetVolume(mixer_channel_info *ch, int volume);
int MixerModChannelSetPanning(mixer_channel_info *ch, int panning);

// Mixer function

void MixerMix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size);

#endif // UMOD_MIXER_CHANNEL_H__
