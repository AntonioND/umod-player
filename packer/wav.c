// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <umod/umodpack.h>

#include "file.h"
#include "instruments.h"

// Information taken from:
//
// https://web.archive.org/web/20040317073101/http://ccrma-www.stanford.edu/courses/422/projects/WaveFormat/

#pragma pack(push, 1)
typedef struct {

    // RIFF header

    uint32_t chunk_id;          // "RIFF" == 0x46464952
    uint32_t chunk_size;        // File size - size of (chunk_id + chunk_size)
    uint32_t format;            // "WAVE" == 0x45564157

    // "fmt " subchunk

    uint32_t subchunk_1_id;     // "fmt " == 0x20746D66
    uint32_t subchunk_1_size;   // 16 for PCM
    uint16_t audio_format;      // PCM = 1
    uint16_t num_channels;      // Mono = 1, Stereo = 2
    uint32_t sample_rate;       // Samples per second
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;       // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample;   // 8 bits = 8, 16 bits = 16, etc.

    // "data" subchunk

    uint32_t subchunk_2_id;     // "data" == 0x61746164
    uint32_t subchunk_2_size;   // Size of data following this entry
    // uint8_t data[]           // Sound data is stored here

} wav_header_t;
#pragma pack(pop)

int add_wav(const char *path, int *instrument_index)
{
    assert(path != NULL);
    assert(instrument_index != NULL);

    int ret = -1;

    // Load file to memory

    printf("Reading WAV file...\n");

    void *buffer;
    size_t size;

    file_load(path, &buffer, &size);

    if (size < sizeof(wav_header_t))
    {
        printf("File is too small.\n");
        goto cleanup;
    }

    // Check header of the WAV file

    wav_header_t *header = buffer;

    if (header->chunk_id != 0x46464952)
    {
        printf("Not a wav file: chunk ID: %" PRIx32 "\n", header->chunk_id);
        goto cleanup;
    }

    if ((header->chunk_size + sizeof(uint32_t) + sizeof(uint32_t)) !=
        (header->subchunk_2_size + sizeof(wav_header_t)))
    {
        printf("Inconsistent sizes: %" PRIx32 " %" PRIx32 "\n",
               header->chunk_size, header->subchunk_2_size);
        goto cleanup;
    }

    if (header->format != 0x45564157)
    {
        printf("Not a wav file: format: %" PRIx32 "\n", header->format);
        goto cleanup;
    }

    if (header->subchunk_1_id != 0x20746D66)
    {
        printf("Not a wav file: subchunk 1 ID: %" PRIx32 "\n",
               header->subchunk_1_id);
        goto cleanup;
    }

    if (header->subchunk_1_size != 16)
    {
        printf("Not a wav file: subchunk 1 size: %" PRIx32 "\n",
               header->subchunk_1_size);
        goto cleanup;
    }

    if (header->audio_format != 1)
    {
        printf("Not a wav file: audio format: %" PRIx16 "\n",
               header->audio_format);
        goto cleanup;
    }

    if (header->num_channels != 1)
    {
        printf("Only wav files with one channel are supported: num channels: %" PRIx16 "\n",
               header->num_channels);
        goto cleanup;
    }

    uint32_t sample_rate = header->sample_rate;

    printf("  Sample rate: %" PRIu32 "\n", sample_rate);

    if (header->byte_rate !=
        (sample_rate * header->num_channels * header->bits_per_sample / 8))
    {
        printf("Inconsistent byte rate\n");
        goto cleanup;
    }

    if (header->block_align != (header->num_channels * header->bits_per_sample / 8))
    {
        printf("Inconsistent block align\n");
        goto cleanup;
    }

    uint16_t bits_per_sample = header->bits_per_sample;

    printf("  Bits per sample: %" PRIu16 "\n", bits_per_sample);

    if (header->subchunk_2_id != 0x61746164)
    {
        printf("Not a wav file: subchunk 2 ID: %" PRIx32 "\n", header->subchunk_2_id);
        goto cleanup;
    }

    uint32_t data_size = header->subchunk_2_size - sizeof(wav_header_t);

    printf("  Size: %" PRIu32 "\n", data_size);

    // Get pointer to the start of the waveform
    void *wav_data = (void *)((uintptr_t)header + sizeof(wav_header_t));

    if (bits_per_sample == 8)
    {
        // Samples are stored as unsigned bytes: 0 - 255

        uint8_t *waveform = wav_data;
        size_t waveform_samples = data_size;

        // The samples of the pack file are 8-bit signed (-128 - 127)

        for (size_t i = 0; i < waveform_samples; i++)
            waveform[i] = waveform[i] - 128;

        *instrument_index = instrument_add((int8_t *)waveform, waveform_samples,
                                           255, 0, // volume, finetune
                                           0 , 0, // loop start, length
                                           sample_rate);
    }
    else if (bits_per_sample == 16)
    {
        // Samples are stored as signed integers: -32768 - 32767

        uint16_t *waveform = wav_data;
        size_t waveform_samples = data_size / 2;

        // The samples of the pack file are 8-bit signed (-128 - 127)

        int8_t *waveform_dst = (int8_t *)waveform;

        for (size_t i = 0; i < waveform_samples; i++)
            waveform_dst[i] = waveform[i] >> 8;

        *instrument_index = instrument_add(waveform_dst, waveform_samples,
                                           255, 0, // volume, finetune
                                           0 , 0,  // loop start, length
                                           sample_rate);
    }
    else
    {
        printf("Invalid bits per sample: %" PRIu16 "\n", bits_per_sample);
        goto cleanup;
    }

    printf("Finished importing wav file.\n");

    ret = 0;
cleanup:
    free(buffer);
    return ret;
}
