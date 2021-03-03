// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

// Test calling SFX functions with invalid arguments.

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

    umod_handle laser_handle;

    // Try to play effects that don't exist
    // ------------------------------------

    if (UMOD_SFX_Play(100, UMOD_LOOP_DEFAULT) != UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Play(UINT32_MAX, UMOD_LOOP_DEFAULT) != UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    // Play a non-looping effect, wait until it ends, try to modify it
    // ---------------------------------------------------------------

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    if (laser_handle == UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(100);

    if (UMOD_SFX_IsPlaying(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(900);

    // Call functions that should fail

    if (UMOD_SFX_IsPlaying(laser_handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetVolume(laser_handle, 128) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetPanning(laser_handle, 128) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetFrequencyMultiplier(laser_handle, 1 << 16) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Release(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Stop(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(1000);

    // Play an effect, stop it, try to modify it
    // -----------------------------------------

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    if (laser_handle == UMOD_HANDLE_INVALID)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(100);

    if (UMOD_SFX_IsPlaying(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Stop(laser_handle) != 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(900);

    // Call functions that should fail

    if (UMOD_SFX_IsPlaying(laser_handle) == 1)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetVolume(laser_handle, 128) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetPanning(laser_handle, 128) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_SetFrequencyMultiplier(laser_handle, 1 << 16) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Release(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    if (UMOD_SFX_Stop(laser_handle) == 0)
    {
        printf("Line %d: Check failed\n", __LINE__);
        goto cleanup;
    }

    generate_ms(1000);

    // Test volumes outside of valid ranges (they should be clamped)
    // -------------------------------------------------------------

    // Reference
    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetVolume(laser_handle, 512);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetVolume(laser_handle, -128);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    // Reference
    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetMasterVolume(512);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetMasterVolume(-128);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    // Reference
    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetMasterVolume(256);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    generate_ms(500);

    // Test pannings outside of valid ranges (they should be clamped)
    // --------------------------------------------------------------

    // Reference
    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetPanning(laser_handle, 512);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    UMOD_SFX_SetPanning(laser_handle, -128);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    // Reference
    laser_handle = UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);
    generate_ms(200);
    UMOD_SFX_Stop(laser_handle);

    WAV_FileEnd();

    rc = 0;
cleanup:
    free(pack_buffer);
    return rc;
}
