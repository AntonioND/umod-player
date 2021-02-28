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

// Set master volume for all the song channels. Values: 0 - 256.
void UMOD_Song_SetMasterVolume(int volume);

// Plays the specified song (MOD_xxx defines, etc). It stops the currently
// played song if any. It returns 0 on success.
int UMOD_Song_Play(uint32_t index);

// It returns 1 if there is currently a song being played, 0 otheriwse.
int UMOD_Song_IsPlaying(void);

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

// Play SFX that corresponds to the specified SFX_xxx define. It returns a
// handle that can be used to modify this effect while it is being played. In
// the case of one-shot sounds, the handle stops being valid after the sound
// ends.
//
// Note that using an invalid handle won't crash any function, but it won't
// affect any other effect being played in the same channel.
//
// This function returns UMOD_HANDLE_INVALID if the SFX doesn't exist, or if
// there are no available channels.
umod_handle UMOD_SFX_Play(uint32_t index, umod_loop_type loop_type);

// Set volume for the specified effect. Values: 0 - 256. Returns 0 on success.
// It can fail if the handle is invalid or if the SFX has already finished.
int UMOD_SFX_SetVolume(umod_handle handle, int volume);

// Set panning for the specified effect. Values: 0 (left) - 255 (right). Returns
// 0 on success. It can fail if the handle is invalid or if the SFX has already
// finished.
int UMOD_SFX_SetPanning(umod_handle handle, int panning);

// Stop playing the specified sound. Returns 0 on success. It can fail if the
// handle is invalid or if the SFX has already finished.
int UMOD_SFX_Stop(umod_handle handle);

#endif // UMOD_UMOD_H__
