// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <umod/umodpack.h>

#include "instruments.h"
#include "song.h"
#include "patterns.h"

int save_pack(const char *path)
{
    int ret = -1;

    FILE *f = fopen(path, "wb+");
    if (f == NULL)
        return -1;

    fprintf(f, "UMOD");

    uint32_t num_songs = song_total_number();
    fwrite(&num_songs, sizeof(num_songs), 1, f);

    uint32_t num_patterns = pattern_total_number();
    fwrite(&num_patterns, sizeof(num_patterns), 1, f);

    uint32_t num_instruments = instrument_total_number();
    fwrite(&num_instruments, sizeof(num_instruments), 1, f);

    // Leave this empty for now
    uint32_t *song_offset_array = malloc(num_songs * sizeof(uint32_t));
    uint32_t *pattern_offset_array = malloc(num_patterns * sizeof(uint32_t));
    uint32_t *instrument_offset_array = malloc(num_instruments * sizeof(uint32_t));

    if ((song_offset_array == NULL) || (pattern_offset_array == NULL) ||
        (instrument_offset_array == NULL))
        goto cleanup;

    uint32_t empty = 0;

    // Songs

    long song_offsets = ftell(f);
    for (uint32_t i = 0; i < num_songs; i++)
        fwrite(&empty, sizeof(empty), 1, f);

    // Patterns

    long pattern_offsets = ftell(f);
    for (uint32_t i = 0; i < num_patterns; i++)
        fwrite(&empty, sizeof(empty), 1, f);

    // Samples

    long instrument_offsets = ftell(f);
    for (uint32_t i = 0; i < num_instruments; i++)
        fwrite(&empty, sizeof(empty), 1, f);

    // Write songs

    for (uint32_t i = 0; i < num_songs; i++)
    {
        song_offset_array[i] = ftell(f);

        const int *data;
        size_t length;
        song_get(0, &data, &length);

        uint16_t length_ = length;
        fwrite(&length_, sizeof(length_), 1, f);

        for (size_t p = 0; p < length; p++)
        {
            uint8_t val = data[p];
            fwrite(&val, sizeof(val), 1, f);
        }

        // Align next element to 32 bit
        {
            long align = (4 - (ftell(f) & 3)) & 3;
            uint8_t val = 0;
            fwrite(&val, sizeof(val), align, f);
        }
    }

    // Write patterns

    for (uint32_t i = 0; i < num_patterns; i++)
    {
        pattern_offset_array[i] = ftell(f);

        int channels, rows;
        pattern_get_dimensions(i, &channels, &rows);

        uint8_t value;
        value = channels;
        fwrite(&value, sizeof(value), 1, f);
        value = rows;
        fwrite(&value, sizeof(value), 1, f);

        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < channels; c++)
            {
                int note, instrument, volume, effect, effect_params;
                pattern_step_get(i, r, c, &note, &instrument, &volume,
                                 &effect, &effect_params);

                uint8_t flags = 0;
                if (instrument != -1)
                    flags |= STEP_HAS_INSTRUMENT;
                if (note != -1)
                    flags |= STEP_HAS_NOTE;
                if (volume != -1)
                    flags |= STEP_HAS_VOLUME;
                if (effect != -1)
                    flags |= STEP_HAS_EFFECT;

                fwrite(&flags, sizeof(flags), 1, f);

                if (instrument != -1)
                {
                    value = instrument;
                    fwrite(&value, sizeof(value), 1, f);
                }
                if (note != -1)
                {
                    value = note;
                    fwrite(&value, sizeof(value), 1, f);
                }
                if (volume != -1)
                {
                    value = volume;
                    fwrite(&value, sizeof(value), 1, f);
                }
                if (effect != -1)
                {
                    value = effect;
                    fwrite(&value, sizeof(value), 1, f);
                    value = effect_params;
                    fwrite(&value, sizeof(value), 1, f);
                }
            }
        }

        // Align next element to 32 bit
        {
            long align = (4 - (ftell(f) & 3)) & 3;
            uint8_t val = 0;
            fwrite(&val, sizeof(val), align, f);
        }
    }

    // Write instruments

    for (uint32_t i = 0; i < num_instruments; i++)
    {
        instrument_offset_array[i] = ftell(f);

        int8_t *data;
        size_t size, loop_start, loop_length;
        int volume, finetune;

        instrument_get(i, &data, &size, &volume, &finetune,
                       &loop_start, &loop_length);

        int looping = 0;
        if (loop_length > 0)
            looping = 1;

        int loop_at_the_end = 1;
        if ((loop_start + loop_length) < size)
            loop_at_the_end = 0;

        uint32_t size_value;

        size_value = size;
        fwrite(&size_value, sizeof(size_value), 1, f);

        if ((looping == 1) && (loop_at_the_end == 0))
        {
            // If the loop isn't at the end, copy it to the end after the
            // complete waveform.
            size_value = size;
            fwrite(&size_value, sizeof(size_value), 1, f);
            size_value = size + loop_length;
            fwrite(&size_value, sizeof(size_value), 1, f);
        }
        else
        {
            size_value = loop_start;
            fwrite(&size_value, sizeof(size_value), 1, f);
            size_value = loop_start + loop_length;
            fwrite(&size_value, sizeof(size_value), 1, f);
        }

        uint8_t value;

        value = volume;
        fwrite(&value, sizeof(value), 1, f);
        value = finetune;
        fwrite(&value, sizeof(value), 1, f);

        for (size_t j = 0; j < size; j++)
        {
            int8_t sample = data[j];
            fwrite(&sample, sizeof(sample), 1, f);
        }

        // Write some more samples to help with mixer optimizations

        if (looping)
        {
            // Repeat loop

            if (loop_at_the_end)
            {
                // The only thing needed is to add the buffer

                size_t j = loop_start;

                for (int s = 0; s < UMODPACK_INSTRUMENT_EXTRA_SAMPLES; s++)
                {
                    int8_t sample = data[j];
                    fwrite(&sample, sizeof(sample), 1, f);

                    j++;

                    if (j == (loop_start + loop_length))
                        j = loop_start;
                }
            }
            else
            {
                // First, copy the loop again

                for (size_t j = loop_start; j <= (loop_start + loop_length); j++)
                {
                    int8_t sample = data[j];
                    fwrite(&sample, sizeof(sample), 1, f);
                }

                // Then, add the buffer

                size_t j = loop_start;

                for (int s = 0; s < UMODPACK_INSTRUMENT_EXTRA_SAMPLES; s++)
                {
                    int8_t sample = data[j];
                    fwrite(&sample, sizeof(sample), 1, f);

                    j++;

                    if (j == (loop_start + loop_length))
                        j = loop_start;
                }
            }
        }
        else
        {
            // Write zeroes as buffer

            for (int s = 0; s < UMODPACK_INSTRUMENT_EXTRA_SAMPLES; s++)
            {
                int8_t sample = 0;
                fwrite(&sample, sizeof(sample), 1, f);
            }
        }

        // Align next element to 32 bit
        {
            long align = (4 - (ftell(f) & 3)) & 3;
            uint8_t val = 0;
            fwrite(&val, sizeof(val), align, f);
        }
    }

    // Fill empty fields

    uint32_t offset;

    fseek(f, song_offsets, SEEK_SET);

    for (uint32_t i = 0; i < num_songs; i++)
    {
        offset = song_offset_array[i];
        fwrite(&offset, sizeof(offset), 1, f);
    }

    fseek(f, pattern_offsets, SEEK_SET);

    for (uint32_t i = 0; i < num_patterns; i++)
    {
        offset = pattern_offset_array[i];
        fwrite(&offset, sizeof(offset), 1, f);
    }

    fseek(f, instrument_offsets, SEEK_SET);

    for (uint32_t i = 0; i < num_instruments; i++)
    {
        offset = instrument_offset_array[i];
        fwrite(&offset, sizeof(offset), 1, f);
    }

    ret = 0;
cleanup:
    fclose(f);
    free(song_offset_array);
    free(pattern_offset_array);
    free(instrument_offset_array);

    return ret;
}
