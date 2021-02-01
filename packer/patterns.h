// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef PATTERNS_H__
#define PATTERNS_H__

#include <stddef.h>

int pattern_add(int channels, int rows);

int pattern_step_set(size_t pattern_index,
                     int row, int channel,
                     int note, int instrument, int volume,
                     int effect, int effect_params);

int pattern_total_number(void);

int pattern_get_dimensions(int pattern_index, int *channels, int *rows);

int pattern_step_get(int pattern_index,
                     int row, int channel,
                     int *note, int *instrument, int *volume,
                     int *effect, int *effect_params);

#endif // PATTERNS_H__
