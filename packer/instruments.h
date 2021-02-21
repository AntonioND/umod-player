// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef INSTRUMENTS_H__
#define INSTRUMENTS_H__

#include <stddef.h>
#include <stdint.h>

// volume = 0-255
int instrument_add(int8_t *data, size_t size, int volume, int finetune,
                   size_t loop_start, size_t loop_length);

int instrument_total_number(void);

int instrument_get(int instrument_index, int8_t **data, size_t *size,
                   int *volume, int *finetune,
                   size_t *loop_start, size_t *loop_length);

int instrument_get_volume(int instrument_index, int *volume);

#endif // INSTRUMENTS_H__
