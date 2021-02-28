// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_UMOD_H__
#define UMOD_UMOD_H__

#include <stddef.h>
#include <stdint.h>

void UMOD_Init(uint32_t sample_rate);
int UMOD_LoadPack(const void *pack);
void UMOD_Mix(int8_t *left_buffer, int8_t *right_buffer, size_t buffer_size);

// Song API
// ========

int UMOD_PlaySong(uint32_t index);
int UMOD_IsPlayingSong(void);
void UMOD_Song_VolumeSet(int volume); // 0 (0.0) - 256 (1.0)

// SFX API
// =======

#define UMOD_HANDLE_INVALID     0

typedef uint32_t umod_handle;

typedef enum {
    // WAV files may contain loop information or not. If a WAV file contains
    // loop information, this value will let the mixer loop the sample as
    // expected. If it doesn't have information, it will be played once.
    UMOD_LOOP_DEFAULT = 0,

    // In WAV files with no loop information, this tells the mixer to loop the
    // whole waveform. In WAV files with loop information, it does nothing.
    UMOD_LOOP_ENABLE  = 1,

    // This value disables looping even in WAV effects that have loop
    // information. Note that it isn't possible to enable looping afterwards.
    UMOD_LOOP_DISABLE = 2
} umod_loop_type;

// Set master volume for all the SFX channels. Values: 0 - 256.
void UMOD_SFX_SetMasterVolume(int volume);

umod_handle UMOD_SFX_Play(uint32_t index, umod_loop_type loop_type);
int UMOD_SFX_Stop(umod_handle handle);

#endif // UMOD_UMOD_H__
