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
// https://sites.google.com/site/musicgapi/technical-documents/wav-file-format

#pragma pack(push, 1)

// RIFF header
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
typedef struct {
    uint32_t chunk_id;          // "RIFF" == 0x46464952
    uint32_t chunk_size;        // File size - size of (chunk_id + chunk_size)
    uint32_t format;            // "WAVE" == 0x45564157
} riff_header;

// "fmt " subchunk
#define ID_FMT 0x20746D66
typedef struct {
    uint32_t subchunk_id;       // "fmt " == 0x20746D66
    uint32_t subchunk_size;     // 16 for PCM
    uint16_t audio_format;      // PCM = 1
    uint16_t num_channels;      // Mono = 1, Stereo = 2
    uint32_t sample_rate;       // Samples per second
    uint32_t byte_rate;         // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;       // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample;   // 8 bits = 8, 16 bits = 16, etc.
} fmt_subchunk;

// "data" subchunk
#define ID_DATA 0x61746164
typedef struct {
    uint32_t subchunk_id;       // "data" == 0x61746164
    uint32_t subchunk_size;     // Size of data following this entry
    // uint8_t data[]           // Sound data is stored here
} data_subchunk;

// "smpl" subchunk
#define ID_SMPL 0x6C706D73
typedef struct {
    uint32_t subchunk_id;           // "smpl" == 0x6C706D73
    uint32_t subchunk_size;         // Size of data following this entry
    uint32_t manufacturer;          // 0 - 0xFFFFFFFF
    uint32_t product;               // 0 - 0xFFFFFFFF
    uint32_t sample_period;         // 0 - 0xFFFFFFFF
    uint32_t midi_unity_note;       // 0 - 127
    uint32_t midi_pitch_fraction;   // 0 - 0xFFFFFFFF
    uint32_t smpte_format;          // 0, 24, 25, 29, 30
    uint32_t smpte_offset;          // 0 - 0xFFFFFFFF
    uint32_t num_sample_loops;      // 0 - 0xFFFFFFFF
    uint32_t sampler_data;          // 0 - 0xFFFFFFFF

    struct {
        uint32_t cue_point_id;      // 0 - 0xFFFFFFFF

        // Types:
        //   0 : Loop forward (normal)
        //   1 : Alternating loop (forward/backward, also known as Ping Pong)
        //   2 : Loop backward (reverse)
        //   3 - 31 : Reserved for future standard types
        //   32 - 0xFFFFFFFF : Sampler specific types (defined by manufacturer)
        uint32_t type;              // 0 - 0xFFFFFFFF

        uint32_t start;             // 0 - 0xFFFFFFFF
        uint32_t end;               // 0 - 0xFFFFFFFF
        uint32_t fraction;          // 0 - 0xFFFFFFFF
        uint32_t play_count;        // 0 - 0xFFFFFFFF
    } sample_loop[];

} smpl_subchunk;

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

    if (size < sizeof(riff_header))
    {
        printf("  File is too small.\n");
        goto cleanup;
    }

    // Check header of the WAV file

    riff_header *header = buffer;

    if (header->chunk_id != ID_RIFF)
    {
        printf("  Not a wav file: chunk ID: %" PRIx32 "\n", header->chunk_id);
        goto cleanup;
    }

    if ((header->chunk_size + sizeof(uint32_t) + sizeof(uint32_t)) != size)
    {
        printf("  Inconsistent sizes: 0x%" PRIx32 " 0x%zx\n",
               header->chunk_size, size);
        goto cleanup;
    }

    if (header->format != ID_WAVE)
    {
        printf("  Not a wav file: format: %" PRIx32 "\n", header->format);
        goto cleanup;
    }

    // Iterate through all subchunks

    void *subchunk = (uint32_t *)((uintptr_t)header + sizeof(riff_header));

    int format_read = 0;
    int data_read = 0;

    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    uint32_t data_size = 0;
    void *wav_data = NULL;
    uint32_t wav_loop_start = 0;
    uint32_t wav_loop_length = 0;

    while (1)
    {
        uintptr_t offset = (uintptr_t)subchunk - (uintptr_t)header;
        if (offset >= size)
            break;

        uint32_t subchunk_id = ((uint32_t *)subchunk)[0];
        uint32_t subchunk_size = ((uint32_t *)subchunk)[1];

        printf("  Subchunk at offset %lu (%" PRIu32 " bytes) [%c%c%c%c]\n",
               offset, subchunk_size,
               subchunk_id & 0xFF,
               (subchunk_id >> 8) & 0xFF,
               (subchunk_id >> 16) & 0xFF,
               (subchunk_id >> 24) & 0xFF);

        if (subchunk_id == ID_FMT)
        {
            format_read = 1;

            fmt_subchunk *fmt = subchunk;

            if (fmt->subchunk_size != 16)
            {
                printf("  Not a valid wav file: subchunk fmt size: %" PRIx32 "\n",
                    fmt->subchunk_size);
                goto cleanup;
            }

            if (fmt->audio_format != 1)
            {
                printf("  Not a valid wav file: audio format: %" PRIx16 "\n",
                       fmt->audio_format);
                goto cleanup;
            }

            if (fmt->num_channels != 1)
            {
                printf("  Only wav files with one channel are supported: num channels: %"
                       PRIx16 "\n",
                       fmt->num_channels);
                goto cleanup;
            }

            sample_rate = fmt->sample_rate;

            printf("    Sample rate: %" PRIu32 "\n", sample_rate);

            if (fmt->byte_rate !=
                (sample_rate * fmt->num_channels * fmt->bits_per_sample / 8))
            {
                printf("    Inconsistent byte rate\n");
                goto cleanup;
            }

            if (fmt->block_align != (fmt->num_channels * fmt->bits_per_sample / 8))
            {
                printf("    Inconsistent block align\n");
                goto cleanup;
            }

            bits_per_sample = fmt->bits_per_sample;

            printf("    Bits per sample: %" PRIu16 "\n", bits_per_sample);

            subchunk = (void *)((uintptr_t)subchunk + sizeof(fmt_subchunk));
        }
        else if (subchunk_id == ID_DATA)
        {
            if (format_read == 0)
            {
                printf("  Data subchunk found before format subchunk.\n");
                goto cleanup;
            }

            data_read = 1;

            data_size = subchunk_size;

            printf("    Data size: %" PRIu32 "\n", data_size);

            // Get pointer to the start of the waveform
            wav_data = (void *)((uintptr_t)subchunk + sizeof(data_subchunk));

            subchunk = (void *)((uintptr_t)wav_data + data_size);
        }
        else if (subchunk_id == ID_SMPL)
        {
            if (format_read == 0)
            {
                printf("  Sample subchunk found before format subchunk.\n");
                goto cleanup;
            }

            smpl_subchunk *smpl = subchunk;

            if (smpl->num_sample_loops != 1)
            {
                printf("  Expected one loop. Found %" PRIu32 "\n",
                       smpl->num_sample_loops);
                goto cleanup;
            }

            if (smpl->sample_loop[0].type != 0)
            {
                printf("  Only forward loops are supported. Value: %" PRIu32 "\n",
                       smpl->sample_loop[0].type);
                goto cleanup;
            }

            if (smpl->sample_loop[0].play_count != 0)
            {
                printf("  Only infinite loops are supported. Value: %" PRIu32 "\n",
                       smpl->sample_loop[0].play_count);
                goto cleanup;
            }

            uint32_t start = smpl->sample_loop[0].start;
            uint32_t end = smpl->sample_loop[0].end;

            printf("    Loop: %" PRIu32 " - %" PRIu32 "\n", start, end);

            if (end > data_size)
            {
                printf("    End is outside of bounds (size = %" PRIu32 "): Clamped.\n",
                       data_size);
                end = data_size;
            }

            if (start > end)
            {
                printf("    Start after end. Ignoring loop information.\n");
            }
            else
            {
                wav_loop_start = start;
                wav_loop_length = end - start;
            }

            subchunk = (void *)((uintptr_t)subchunk + subchunk_size
                                + sizeof(uint32_t) + sizeof(uint32_t));
        }
        else
        {
            printf("    Unknown subchunk (ignored)\n");
            subchunk = (void *)((uintptr_t)subchunk + subchunk_size
                                + sizeof(uint32_t) + sizeof(uint32_t));
        }
    }

    if (data_read == 0)
    {
        printf("  Data subchunk not found.\n");
        goto cleanup;
    }

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
                                           wav_loop_start, wav_loop_length,
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
                                           wav_loop_start, wav_loop_length,
                                           sample_rate);
    }
    else
    {
        printf("  Invalid bits per sample: %" PRIu16 "\n", bits_per_sample);
        goto cleanup;
    }

    printf("  Finished importing wav file.\n");

    ret = 0;
cleanup:
    free(buffer);
    return ret;
}
