// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "global.h"
#include "mod_channel.h"

static umod_loaded_pack loaded_pack;

static uint32_t global_sample_rate;

void UMOD_Init(uint32_t sample_rate)
{
    global_sample_rate = sample_rate;

    ModSetSampleRateConvertConstant(sample_rate);

    // This will load all the pointers to the mixer channels so that the song
    // volume can be changed.
    ModChannelResetAll();
    UMOD_Song_VolumeSet(256);

    UMOD_SFX_SetMasterVolume(256);
}

uint32_t GetGlobalSampleRate(void)
{
    return global_sample_rate;
}

int UMOD_LoadPack(const void *pack)
{
    if (global_sample_rate == 0)
        return -1;

    const umodpack_header *header = pack;

    if ((header->magic[0] != 'U') || (header->magic[1] != 'M') ||
        (header->magic[2] != 'O') || (header->magic[3] != 'D'))
    {
        return -2;
    }

    loaded_pack.data = pack;

    loaded_pack.num_songs = header->num_songs;
    loaded_pack.num_patterns = header->num_patterns;
    loaded_pack.num_instruments = header->num_instruments;

    // If there are songs, it is needed to at least have one pattern
    if ((loaded_pack.num_songs > 0) && (loaded_pack.num_patterns == 0))
        return -3;

    // Reject any file with no instruments
    if (loaded_pack.num_instruments == 0)
        return -4;

    uint32_t *read_ptr = (uint32_t *)((uintptr_t)pack + sizeof(umodpack_header));
    loaded_pack.offsets_songs = read_ptr;
    read_ptr += loaded_pack.num_songs;
    loaded_pack.offsets_patterns = read_ptr;
    read_ptr += loaded_pack.num_patterns;
    loaded_pack.offsets_samples = read_ptr;

    return 0;
}

umod_loaded_pack *GetLoadedPack(void)
{
    return &loaded_pack;
}

umodpack_instrument *InstrumentGetPointer(int index)
{
    uint32_t offset = loaded_pack.offsets_samples[index];
    uintptr_t instrument_address = (uintptr_t)loaded_pack.data;
    instrument_address += offset;

    return (umodpack_instrument *)instrument_address;
}
