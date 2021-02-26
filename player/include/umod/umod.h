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

int UMOD_PlaySong(uint32_t index);
int UMOD_IsPlayingSong(void);

// SFX API

typedef enum {
    UMOD_LOOP_DEFAULT = 0,
    UMOD_LOOP_ENABLE  = 1,
    UMOD_LOOP_DISABLE = 2
} umod_loop_type;

uint32_t UMOD_SFX_Play(uint32_t index, umod_loop_type loop_type);
int UMOD_SFX_Stop(uint32_t handle);

#endif // UMOD_UMOD_H__
