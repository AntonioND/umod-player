// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int8_t     *data;
    size_t      size;
    int         volume;
    int         finetune;
    size_t      loop_start;
    size_t      loop_length;
    uint32_t    frequency;  // Default playback frequency (for WAV files)
} generic_instrument;

generic_instrument **instruments;
int instruments_total;
int instruments_used;

static void clean_instruments(void)
{
    for (int i = 0; i < instruments_used; i++)
    {
        free(instruments[i]->data);
        free(instruments[i]);
    }

    free(instruments);
}

static int new_instrument(void)
{
    if (instruments == NULL)
    {
        instruments_total = 8;
        instruments_used = 0;
        instruments = malloc(sizeof(generic_instrument *) * instruments_total);

        if (instruments == NULL)
            return -1;

        atexit(clean_instruments);
    }

    if (instruments_used == instruments_total)
    {
        instruments_total *= 2;
        instruments = realloc(instruments,
                              sizeof(generic_instrument *) * instruments_total);

        if (instruments == NULL)
            return -1;
    }

    instruments[instruments_used] = calloc(sizeof(generic_instrument), 1);
    return instruments_used++;
}

int instrument_add(int8_t *data, size_t size, int volume, int finetune,
                   size_t loop_start, size_t loop_length, uint32_t frequency)
{
    // Fix instrument

    if (size == 0)
    {
        printf("Empty");
        return -1;
    }

    if (loop_start > size)
    {
        loop_start = 0;
        loop_length = 0;
        printf("Invalid loop arguments for instrument: Loop disabled");
    }

    if ((loop_start + loop_length) > size)
    {
        loop_length = size - loop_start;
        printf("Invalid loop length for instrument: Loop clamped");
    }

    // Check if this instrument is already in the list

    for (int index = 0; index < instruments_used; index++)
    {
        generic_instrument *instrument = instruments[index];

        if ((instrument->size != size) || (instrument->volume != volume) ||
            (instrument->finetune != finetune) ||
            (instrument->loop_start != loop_start) ||
            (instrument->loop_length != loop_length) ||
            (instrument->frequency != frequency))
        {
            continue;
        }

        int different = 0;

        for (size_t i = 0; i < size; i++)
        {
            if (instrument->data[i] != data[i])
            {
                different = 1;
                break;
            }
        }

        if (different)
            continue;

        // Everything matches, don't allocate a new instrument, return index of
        // this one.

        printf("Repeated: Index %d", index);

        return index;
    }

    // Allocate new instrument

    int instrument_index = new_instrument();
    if (instrument_index < 0)
        return -1;

    printf("New: Index %d", instrument_index);

    generic_instrument *instrument = instruments[instrument_index];
    instrument->size = size;

    instrument->volume = volume;
    instrument->finetune = finetune;
    instrument->loop_start = loop_start;
    instrument->loop_length = loop_length;
    instrument->frequency = frequency;

    instrument->data = malloc(size);
    if (instrument->data == NULL)
        return -1;

    for (size_t i = 0; i < size; i++)
        instrument->data[i] = data[i];

    return instrument_index;
}

int instrument_total_number(void)
{
    return instruments_used;
}

int instrument_get(int instrument_index, int8_t **data, size_t *size,
                   int *volume, int *finetune,
                   size_t *loop_start, size_t *loop_length,
                   uint32_t *frequency)
{
    if (instrument_index >= instruments_used)
        return -1;

    generic_instrument *instrument = instruments[instrument_index];

    *size = instrument->size;
    *volume = instrument->volume;
    *finetune = instrument->finetune;
    *loop_start = instrument->loop_start;
    *loop_length = instrument->loop_length;
    *data = instrument->data;
    *frequency = instrument->frequency;

    return 0;
}

int instrument_get_volume(int instrument_index, int *volume)
{
    if (instrument_index >= instruments_used)
        return -1;

    generic_instrument *instrument = instruments[instrument_index];

    *volume = instrument->volume;

    return 0;
}
