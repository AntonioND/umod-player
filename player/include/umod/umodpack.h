// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_UMODPACK_H__
#define UMOD_UMODPACK_H__

#include <stdint.h>

typedef struct {
    char        magic[4]; // "UMOD"
    uint32_t    num_songs;
    uint32_t    num_patterns;
    uint32_t    num_instruments;
    //uint32_t  offset_to_song[NUM_SONGS]
    //uint32_t  offset_to_pattern[NUM_PATTERNS]
    //uint32_t  offset_to_instrument[NUM_INSTRUMENTS]
} umodpack_header;

typedef struct {
    char        num_of_patterns;
    uint8_t     pattern_index[];
} umodpack_song;

typedef struct {
    uint8_t     channels;
    uint8_t     rows;
    uint8_t     data[];
} umodpack_pattern;

typedef struct {
    uint32_t    size;
    uint32_t    loop_start;
    uint32_t    loop_end;
    uint8_t     volume;
    uint8_t     finetune;
    uint8_t     data[];
} umodpack_instrument;

// Pattern step flags

#define STEP_HAS_INSTRUMENT     (1 << 0)
#define STEP_HAS_NOTE           (1 << 1)
#define STEP_HAS_VOLUME         (1 << 2)
#define STEP_HAS_EFFECT         (1 << 3)

// Note: 0 = C0, 1 = C#0, etc
// Instrument: TODO
// Volume: 0 - 255
// Effect: Effect type (1 byte) | Effect parameters (1 byte)

// Effect types

#define EFFECT_NONE                 0
#define EFFECT_SET_SPEED            1
#define EFFECT_SET_PANNING          2
#define EFFECT_CUT_NOTE             3
#define EFFECT_PATTERN_BREAK        4
#define EFFECT_JUMP_TO_PATTERN      5
#define EFFECT_ARPEGGIO             6
#define EFFECT_VOLUME_SLIDE         7
#define EFFECT_PORTA_UP             8
#define EFFECT_PORTA_DOWN           9
#define EFFECT_FINE_PORTA_UP        10
#define EFFECT_FINE_PORTA_DOWN      11
#define EFFECT_FINE_VOLUME_SLIDE    12
#define EFFECT_SAMPLE_OFFSET        13
#define EFFECT_PORTA_TO_NOTE        14
#define EFFECT_PORTA_VOL_SLIDE      15
#define EFFECT_VIBRATO              16
#define EFFECT_VIBRATO_VOL_SLIDE    17
#define EFFECT_TREMOLO              18
#define EFFECT_RETRIG_NOTE          19
// TODO: Reorder effect numbers

#endif // UMOD_UMODPACK_H__
