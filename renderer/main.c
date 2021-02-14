// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>

#include <umod/umod.h>

#include "file.h"
#include "wav_utils.h"

#define SAMPLE_RATE (32 * 1024)

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid number of arguments\n");
        return -1;
    }

    // Load file

    void *pack_buffer = NULL;
    size_t pack_size;

    file_load(argv[1], &pack_buffer, &pack_size);
    if (pack_size == 0)
        goto cleanup;

    // Play music until the song ends, while saving it to a WAV

    UMOD_Init(SAMPLE_RATE);

    UMOD_LoadPack(pack_buffer);
    UMOD_PlaySong(0);

    WAV_FileStart(argv[2], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto cleanup;

    int frames = 0;

    while (UMOD_IsPlayingSong())
    {
#define SIZE (SAMPLE_RATE / 60)
        uint8_t left[SIZE], right[SIZE];
        UMOD_Mix(&left[0], &right[0], SIZE);

        uint8_t buffer[SIZE * 2];
        for (int i = 0; i < SIZE; i++)
        {
            buffer[i * 2 + 0] = left[i];
            buffer[i * 2 + 1] = right[i];
        }

        WAV_FileStream(buffer, sizeof(buffer));

        frames++;

        if (frames > 60 * 60 * 10) // 10 minutes limit
            break;
    }

    WAV_FileEnd();

cleanup:
    free(pack_buffer);
    return 0;
}
