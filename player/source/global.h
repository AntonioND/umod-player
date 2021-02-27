// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_GLOBAL_H__
#define UMOD_GLOBAL_H__

#include <stdint.h>

typedef struct {
    const void *data;
    uint32_t    num_songs;
    uint32_t    num_patterns;
    uint32_t    num_instruments;
    uint32_t   *offsets_songs;
    uint32_t   *offsets_patterns;
    uint32_t   *offsets_samples;
} umod_loaded_pack;

uint32_t GetGlobalSampleRate(void);
umod_loaded_pack *GetLoadedPack(void);
umodpack_instrument *InstrumentGetPointer(int index);

#endif // UMOD_GLOBAL_H__
