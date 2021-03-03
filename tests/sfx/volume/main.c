// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

// Test volume and panning functions.

#include <stdlib.h>
#include <stdio.h>

#include <umod/umod.h>

#include "file.h"
#include "wav_utils.h"

#include "pack_header.h"

#define SAMPLE_RATE (32 * 1024)

void generate_ms(int ms)
{
    for (int t = 0; t < ms; t++)
    {
#define SIZE (SAMPLE_RATE / 1000)

        int8_t left[SIZE], right[SIZE];
        UMOD_Mix(&left[0], &right[0], SIZE);

        uint8_t buffer[SIZE * 2];
        for (int i = 0; i < SIZE; i++)
        {
            buffer[i * 2 + 0] = left[i] + 128;
            buffer[i * 2 + 1] = right[i] + 128;
        }

        WAV_FileStream(buffer, sizeof(buffer));
    }
}

int main(int argc, char *argv[])
{
    int rc = -1;

    if (argc != 2)
    {
        printf("Invalid number of arguments\n");
        return -1;
    }

    // Load file

    void *pack_buffer = NULL;
    size_t pack_size;

    file_load("pack.bin", &pack_buffer, &pack_size);
    if (pack_size == 0)
        goto cleanup;

    // Initialize library

    UMOD_Init(SAMPLE_RATE);

    int ret = UMOD_LoadPack(pack_buffer);
    if (ret != 0)
    {
        printf("UMOD_LoadPack() failed\n");
        goto cleanup;
    }

    WAV_FileStart(argv[1], SAMPLE_RATE);
    if (!WAV_FileIsOpen())
        goto cleanup;

    umod_handle handle = UMOD_HANDLE_INVALID;

    // Per-SFX volume

    UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);

    generate_ms(500);

    handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);

    if (UMOD_SFX_SetVolume(handle, 64) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(500);

    // Global SFX volume

    UMOD_SFX_SetMasterVolume(128);

    UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);

    generate_ms(500);

    handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);

    if (UMOD_SFX_SetVolume(handle, 64) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(500);

    // Test panning

    UMOD_SFX_SetMasterVolume(256);

    handle = UMOD_SFX_Play(SFX_HELICOPTER_WAV, UMOD_LOOP_ENABLE);
    generate_ms(2000);

    for (int i = 0; i < 256; i++) // 256 not included
    {
        if (UMOD_SFX_SetPanning(handle, i) != 0)
        {
            printf("Line %d: Check failed\n", __LINE__);
            goto cleanup;
        }

        generate_ms(40);
    }

    if (UMOD_SFX_Stop(handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    WAV_FileEnd();

    rc = 0;
cleanup:
    free(pack_buffer);
    return rc;
}
