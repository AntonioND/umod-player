// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int     *patterns;
    size_t  num_patterns;
} generic_song;

generic_song **songs;
int songs_total;
int songs_used;

static void clean_songs(void)
{
    for (int i = 0; i < songs_used; i++)
    {
        free(songs[i]->patterns);
        free(songs[i]);
    }

    free(songs);
}

static int new_song(void)
{
    if (songs == NULL)
    {
        songs_total = 8;
        songs_used = 0;
        songs = malloc(sizeof(generic_song *) * songs_total);

        if (songs == NULL)
            return -1;

        atexit(clean_songs);
    }

    if (songs_used == songs_total)
    {
        songs_total *= 2;
        songs = realloc(songs, sizeof(generic_song *) * songs_total);

        if (songs == NULL)
            return -1;
    }

    songs[songs_used] = calloc(sizeof(generic_song), 1);
    return songs_used++;
}

int song_add(size_t num_patterns)
{
    // Allocate new song

    int song_index = new_song();
    if (song_index < 0)
        return -1;

    generic_song *song = songs[song_index];
    song->num_patterns = num_patterns;

    song->patterns = malloc(num_patterns * sizeof(int));
    if (song->patterns == NULL)
        return -1;

    return song_index;
}

int song_set_pattern(int song_index, size_t pattern_pos, int pattern_index)
{
    if (song_index >= songs_used)
        return -1;

    generic_song *song = songs[song_index];

    if (song->num_patterns < pattern_pos)
        return -1;

    song->patterns[pattern_pos] = pattern_index;

    return 0;
}

int song_total_number(void)
{
    return songs_used;
}

int song_get(int song_index, const int **data, size_t *length)
{
    if (song_index >= songs_used)
        return -1;

    generic_song *song = songs[song_index];

    *data = song->patterns;
    *length = song->num_patterns;

    return 0;
}
