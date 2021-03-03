// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_MIXER_CHANNEL_H__
#define UMOD_MIXER_CHANNEL_H__

#include <stddef.h>
#include <stdint.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "mod_channel.h"

#define MIXER_SFX_CHANNELS      (4)
#define MIXER_CHANNELS_MAX      (MOD_CHANNELS_MAX + MIXER_SFX_CHANNELS)

typedef struct {
    int master_volume;
    int volume;         // 0...255
    int left_panning;   // 0...255
    int right_panning;  // 0...255

    // The following variables store the current overall volume to save time
    // during the mixing routine.
    int left_volume;    // (master_volume * volume * left_panning) >> 8 = 0...65535
    int right_volume;   // (master_volume * volume * right_panning) >> 8 = 0...65535

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

} mixer_channel_info;

// Direct access functions

mixer_channel_info *MixerChannelGetFromIndex(uint32_t index);
void MixerChannelRefreshVolumes(mixer_channel_info *ch);
int MixerChannelIsPlaying(mixer_channel_info *ch);
int MixerChannelStart(mixer_channel_info *ch);
int MixerChannelStop(mixer_channel_info *ch);
int MixerChannelSetSampleOffset(mixer_channel_info *ch, uint32_t offset);
int MixerChannelSetNotePeriod(mixer_channel_info *ch, uint64_t period); // 32.32
int MixerChannelSetNotePeriodPorta(mixer_channel_info *ch, uint64_t period); // 32.32
int MixerChannelSetInstrument(mixer_channel_info *ch, umodpack_instrument *instrument_pointer);
int MixerChannelSetLoop(mixer_channel_info *ch, umod_loop_type loop_type,
                        size_t loop_start, size_t loop_end);
int MixerChannelSetVolume(mixer_channel_info *ch, int volume);
int MixerChannelSetMasterVolume(mixer_channel_info *ch, int volume);
int MixerChannelSetPanning(mixer_channel_info *ch, int panning);

// Mixer function

// If mix_song is 1, the song will be mixed. If not, the channels assigned to
// the song will be skipped.
void MixerMix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size,
              int mix_song);

#endif // UMOD_MIXER_CHANNEL_H__
