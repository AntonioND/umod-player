// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int     note;
    int     instrument;
    int     volume;
    int     effect;
    int     effect_params;
} generic_step;

// 4 bytes per channel in each row
typedef struct {
    generic_step    *steps;
    int             channels;
    int             rows;
} generic_pattern;

generic_pattern **patterns;
int patterns_total;
int patterns_used;

static void clean_patterns(void)
{
    for (int i = 0; i < patterns_used; i++)
    {
        free(patterns[i]->steps);
        free(patterns[i]);
    }

    free(patterns);
}

static int new_pattern(void)
{
    if (patterns == NULL)
    {
        patterns_total = 8;
        patterns_used = 0;
        patterns = malloc(sizeof(generic_pattern *) * patterns_total);

        if (patterns == NULL)
            return -1;

        atexit(clean_patterns);
    }

    if (patterns_used == patterns_total)
    {
        patterns_total *= 2;
        patterns = realloc(patterns, sizeof(generic_pattern *) * patterns_total);

        if (patterns == NULL)
            return -1;
    }

    patterns[patterns_used] = calloc(sizeof(generic_pattern), 1);
    return patterns_used++;
}

int pattern_add(int channels, int rows)
{
    if ((channels < 1) || (rows < 1))
        return -1;

    // Allocate new pattern

    int pattern_index = new_pattern();
    if (pattern_index < 0)
        return -1;

    generic_pattern *pattern = patterns[pattern_index];
    pattern->channels = channels;
    pattern->rows = rows;

    pattern->steps = calloc(sizeof(generic_step), channels * rows);
    if (pattern->steps == NULL)
        return -1;

    return pattern_index;
}

int pattern_step_set(int pattern_index,
                     int row, int channel,
                     int note, int instrument, int volume,
                     int effect, int effect_params)
{
    if (pattern_index >= patterns_used)
        return -1;

    generic_pattern *pattern = patterns[pattern_index];

    generic_step *step = &pattern->steps[row * pattern->channels + channel];

    step->note = note;
    step->instrument = instrument;
    step->volume = volume;
    step->effect = effect;
    step->effect_params = effect_params;

    return 0;
}

int pattern_total_number(void)
{
    return patterns_used;
}

int pattern_get_dimensions(int pattern_index, int *channels, int *rows)
{
    if (pattern_index >= patterns_used)
        return -1;

    generic_pattern *pattern = patterns[pattern_index];

    *channels = pattern->channels;
    *rows = pattern->rows;

    return 0;
}

int pattern_step_get(int pattern_index,
                     int row, int channel,
                     int *note, int *instrument, int *volume,
                     int *effect, int *effect_params)
{
    if (pattern_index >= patterns_used)
        return -1;

    generic_pattern *pattern = patterns[pattern_index];

    generic_step *step = &pattern->steps[row * pattern->channels + channel];

    *note = step->note;
    *instrument = step->instrument;
    *volume = step->volume;
    *effect = step->effect;
    *effect_params = step->effect_params;

    return 0;
}
