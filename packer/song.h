// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef SONG_H__
#define SONG_H__

#include <stddef.h>

int song_add(size_t num_patterns);

int song_set_pattern(int song_index, size_t pattern_pos, int pattern_index);

int song_total_number(void);

int song_get(int song_index, const int **data, size_t *length);

#endif // SONG_H__
