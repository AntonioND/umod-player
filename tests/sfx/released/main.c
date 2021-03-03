// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

// Test the effect of releasing channels.

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

    umod_handle helicopter_handle;
    umod_handle laser_handle;

    // First, test that a non-released SFX can't be cancelled automatically
    // --------------------------------------------------------------------

    // Start sound that can't be cancelled

    helicopter_handle = UMOD_SFX_Play(SFX_HELICOPTER_WAV, UMOD_LOOP_ENABLE);
    if (helicopter_handle == UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }
    generate_ms(1000);

    // Fill the other channels

    for (int i = 0; i < UMOD_SFX_CHANNELS - 1; i++)
    {
        laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
        if (laser_handle == UMOD_HANDLE_INVALID)
        {
            printf("Line %d: Check failed (iteration %d)\n", __LINE__, i);
            goto cleanup;
        }
        generate_ms(50);
    }

    // Try to play another sound effect and fail

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    if (laser_handle != UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }
    generate_ms(50);

    if (UMOD_SFX_IsPlaying(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Stop(helicopter_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(1000);

    // Second, test that a released SFX can be cancelled automatically
    // ---------------------------------------------------------------

    // Start sound that can be cancelled

    helicopter_handle = UMOD_SFX_Play(SFX_HELICOPTER_WAV, UMOD_LOOP_ENABLE);
    if (helicopter_handle == UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Release(helicopter_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(1000);

    // Fill the other channels

    for (int i = 0; i < UMOD_SFX_CHANNELS - 1; i++)
    {
        laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
        if (laser_handle == UMOD_HANDLE_INVALID)
        {
            printf("Line %d: Check failed (iteration %d)\n", __LINE__, i);
            goto cleanup;
        }
        generate_ms(50);
    }

    // The original effect should still be playing

    if (UMOD_SFX_IsPlaying(helicopter_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    // Try to play another sound effect and succeed

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    if (laser_handle == UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }
    generate_ms(50);

    // The original effect should have been cancelled

    if (UMOD_SFX_IsPlaying(helicopter_handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    // Any other effect should fail because there are no more released channels.

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    if (laser_handle != UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(1000);

    WAV_FileEnd();

    rc = 0;
cleanup:
    free(pack_buffer);
    return rc;
}
