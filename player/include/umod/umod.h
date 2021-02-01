// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_UMOD_H__
#define UMOD_UMOD_H__

#include <stddef.h>
#include <stdint.h>

#define UMOD_SAMPLE_RATE    (32 * 1024)

int UMOD_LoadPack(void *pack);
int UMOD_PlaySong(uint32_t index);
void UMOD_Mix(uint8_t *left_buffer, uint8_t *right_buffer, size_t buffer_size);
int UMOD_IsPlayingSong(void);

#endif // UMOD_UMOD_H__
