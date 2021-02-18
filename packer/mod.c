// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <umod/umodpack.h>

#include "file.h"
#include "instruments.h"
#include "patterns.h"
#include "song.h"

// The UMOD pack format has volume from 0 to 255, MOD only reaches 64.
#define MOD_VOLUME_SCALE    (256 / 64)

// MOD format constants

#define MOD_ROWS    64 // Rows per pattern

// MOD format structures

static inline uint16_t swap16(uint16_t v)
{
    return ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
}

#pragma pack(push, 1)
typedef struct {
    char        name[22];
    uint16_t    length;
    uint8_t     fine_tune;          // 4 lower bits
    uint8_t     volume;             // 0-64
    uint16_t    loop_point;
    uint16_t    loop_length;        // Loop if length > 1
} mod_instrument;

typedef struct {
    char            name[20];
    mod_instrument  instrument[31];
    uint8_t         song_length;        // Length in patterns
    uint8_t         unused;             // Set to 127, used by Noisetracker
    uint8_t         pattern_table[128]; // 0..63
    char            identifier[4];
    // Followed by patterns, followed by instruments
} mod_header;
#pragma pack(pop)

// Amiga periods for each note. They are the values written to the note field
// in the MOD patterns.
const uint16_t mod_period[6 * 12] = {
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 907,
     856,  808,  762,  720,  678,  640,  604,  570,  538,  508, 480, 453,
     428,  404,  381,  360,  339,  320,  302,  285,  269,  254, 240, 226,
     214,  202,  190,  180,  170,  160,  151,  143,  135,  127, 120, 113,
     107,  101,   95,   90,   85,   80,   75,   71,   67,   63,  60,  56,
      53,   50,   47,   45,   42,   40,   37,   35,   33,   31,  30,  28
};

// Returns a note index from a MOD period. Returns -1 on error.
int get_note_index_from_period(uint16_t period)
{
    if (period < mod_period[(6 * 12) - 1])
    {
        printf("Note too high!\n");
        return -1;
    }
    else if (period > mod_period[0])
    {
        printf("Note too low!\n");
        return -1;
    }

    for (int i = 0; i < 6 * 12; i++)
    {
        if (period == mod_period[i])
            return i;
    }

    // Couldn't find exact match... get nearest value?

    uint16_t nearest_value = 0xFFFF;
    int nearest_index = 0;

    for (int i = 0; i < 6 * 12; i++)
    {
        int test_distance = abs(((int)period) - ((int)mod_period[i]));
        int nearest_distance = abs(((int)period) - nearest_value);

        if (test_distance < nearest_distance)
        {
            nearest_value = mod_period[i];
            nearest_index = i;
        }
    }

    return nearest_index;
}

int add_mod(const char *path)
{
    int ret = -1;

    // Load file to memory

    printf("Reading MOD file...\n");

    void *buffer;
    size_t size;

    file_load(path, &buffer, &size);

    if (size < sizeof(mod_header))
    {
        printf("File is too small.\n");
        goto cleanup;
    }

    // Used to keep track of the converted instruments and patterns
    int instrument_index[31];
    int pattern_index[128];

    mod_header *header = buffer;

    char mod_name[21] = { 0 };
    memcpy(mod_name, header->name, sizeof(header->name));

    char identifier[5] = { 0 };
    memcpy(identifier, header->identifier, sizeof(header->identifier));

    printf("  Size:     %zu bytes\n", size);
    printf("  Name:     '%s'\n", mod_name);
    printf("  ID:       '%s'\n", identifier);

    int channels;

    if (strcmp(identifier, "M.K.") == 0)
    {
        channels = 4;
    }
    else if (strcmp(identifier, "6CHN") == 0)
    {
        channels = 6;
    }
    else if (strcmp(identifier, "8CHN") == 0)
    {
        channels = 8;
    }
    else
    {
        printf("Invalid identifier\n");
        goto cleanup;
    }

    printf("  Channels: %d\n", channels);
    printf("  Length:   %" PRIu8 " patterns\n", header->song_length);

    int song_index = song_add(header->song_length);

    printf("  Pattern list:\n");

    uint8_t max_pattern_index = 0;

    for (int i = 0; i < header->song_length; i++)
    {
        uint8_t index = header->pattern_table[i];

        if ((i % 16) == 0)
            printf("      ");

        printf("%2d", index);

        if (((i % 16) == 15) || (i == (header->song_length - 1)))
            printf("\n");
        else
            printf(", ");

        if (index > max_pattern_index)
            max_pattern_index = index;
    }
    printf("  Number of patterns: %" PRIu8 "\n", max_pattern_index + 1);

    uint8_t *pattern_data = (uint8_t *)header;
    pattern_data += sizeof(mod_header);

    uint8_t *instrument_data = pattern_data;
    instrument_data += channels * (max_pattern_index + 1) * channels * MOD_ROWS;

    // Save instrument data

    printf("  Instruments:\n");

    for (int i = 0; i < 31; i++)
    {
        // Get instrument information

        mod_instrument *instrument = &header->instrument[i];

        char instrument_name[23] = { 0 };
        memcpy(instrument_name, instrument->name, sizeof(instrument->name));

        size_t instrument_size = 2 * (size_t)swap16(instrument->length);
        size_t instrument_loop_point = 2 * (size_t)swap16(instrument->loop_point);
        size_t instrument_loop_length = 2 * (size_t)swap16(instrument->loop_length);

        printf("    %2d: Name: '%s'\n", i, instrument_name);

#if 0
        if (instrument_size > 0)
        {
            printf("        Length: %zu\n", instrument_size);
            printf("        Fine tune: %" PRIu8 "\n", instrument->fine_tune);
            printf("        Volume: %" PRIu8 "\n", instrument->volume);
            printf("        Loop point: %zu\n", instrument_loop_point);
            printf("        Loop length: %zu\n", instrument_loop_length);
        }
#endif

        if (instrument_loop_length < 4)
            instrument_loop_length = 0;

        // Pointer to instrument data start and end

        uint8_t *instrument_pointer = instrument_data;

        uint8_t *instrument_next = instrument_pointer + instrument_size;

        if (instrument_next > ((uint8_t *)buffer +  size))
        {
            size_t overflow = instrument_next - ((uint8_t *)buffer +  size);
            printf("File instrument region overflowed: %zu bytes\n", overflow);
            goto cleanup;
        }

        // Add instrument to list

        if (instrument_size > 0)
        {
            // TODO: This is logarithmic
            int volume = instrument->volume * MOD_VOLUME_SCALE;
            if (volume > 255)
                volume = 255;

            instrument_index[i] = instrument_add(instrument_pointer,
                                                 instrument_size,
                                                 volume, instrument->fine_tune,
                                                 instrument_loop_point,
                                                 instrument_loop_length);
        }

        // Point to next instrument data

        instrument_data = instrument_next;
    }

    // Save pattern data

    for (int i = 0; i <= max_pattern_index; i++)
    {
        pattern_index[i] = pattern_add(channels, MOD_ROWS);

        for (int r = 0; r < MOD_ROWS; r++)
        {
            for (int c = 0; c < channels; c++)
            {
                uint8_t bytes[4];
                bytes[0] = *pattern_data++;
                bytes[1] = *pattern_data++;
                bytes[2] = *pattern_data++;
                bytes[3] = *pattern_data++;

                int instrument_number = (bytes[0] & 0xF0)
                                      | ((bytes[2] >> 4) & 0xF);

                int instrument_period_value = (((uint16_t)bytes[0] & 0xF) << 8)
                                             | (uint16_t)bytes[1];

                int effect_number = bytes[2] & 0xF;
                int effect_params = bytes[3];

                // Convert from MOD values to generic values

                int note = -1;
                if (instrument_period_value > 0)
                    note = get_note_index_from_period(instrument_period_value);

                int volume = -1;

                int converted_effect = -1;
                int converted_effect_param = -1;

                switch (effect_number)
                {
                    case 0x0: // Arpeggio
                    {
                        // Ignore if the parameter is 0 (this is used as NOP)
                        if (effect_params == 0)
                            break;

                        converted_effect = EFFECT_ARPEGGIO;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x1: // Porta up
                    {
                        converted_effect = EFFECT_PORTA_UP;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x2: // Porta down
                    {
                        converted_effect = EFFECT_PORTA_DOWN;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x3: // Porta to note
                    {
                        converted_effect = EFFECT_PORTA_TO_NOTE;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x4: // Vibrato
                    {
                        converted_effect = EFFECT_VIBRATO;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x5: // Porta + Volume slide
                    {
                        converted_effect = EFFECT_PORTA_TO_NOTE;
                        converted_effect_param = 0;

                        int dec = effect_params & 0xF;
                        int inc = effect_params >> 4;

                        // If both are set, ignore effect
                        if ((dec > 0) && (inc > 0))
                            break;

                        converted_effect = EFFECT_PORTA_VOL_SLIDE;

                        if (inc > 0)
                            converted_effect_param = inc * MOD_VOLUME_SCALE;
                        if (dec > 0)
                            converted_effect_param = -dec * MOD_VOLUME_SCALE;

                        break;
                    }
                    case 0x6: // Vibrato + Volume slide
                    {
                        converted_effect = EFFECT_VIBRATO;
                        converted_effect_param = 0;

                        int dec = effect_params & 0xF;
                        int inc = effect_params >> 4;

                        // If both are set, ignore effect
                        if ((dec > 0) && (inc > 0))
                            break;

                        converted_effect = EFFECT_VIBRATO_VOL_SLIDE;

                        if (inc > 0)
                            converted_effect_param = inc * MOD_VOLUME_SCALE;
                        if (dec > 0)
                            converted_effect_param = -dec * MOD_VOLUME_SCALE;

                        break;
                    }
                    case 0x7: // Tremolo
                    {
                        converted_effect = EFFECT_TREMOLO;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x8: // Pan
                    {
                        // 0x00 = left, 0xFF = right
                        converted_effect = EFFECT_SET_PANNING;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0x9: // Sample Offset
                    {
                        converted_effect = EFFECT_SAMPLE_OFFSET;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0xA: // Volume slide
                    {
                        int dec = effect_params & 0xF;
                        int inc = effect_params >> 4;

                        // If both are set, ignore effect
                        if ((dec > 0) && (inc > 0))
                            break;

                        converted_effect = EFFECT_VOLUME_SLIDE;
                        if (inc > 0)
                            converted_effect_param = inc * MOD_VOLUME_SCALE;
                        if (dec > 0)
                            converted_effect_param = -dec * MOD_VOLUME_SCALE;

                        break;
                    }
                    case 0xB: // Jump to pattern
                    {
                        converted_effect = EFFECT_JUMP_TO_PATTERN;
                        converted_effect_param = effect_params;
                        break;
                    }
                    case 0xC: // Set volume
                    {
                        volume = effect_params * MOD_VOLUME_SCALE;
                        if (volume > 255)
                            volume = 255;
                        break;
                    }
                    case 0xD: // Pattern break
                    {
                        converted_effect = EFFECT_PATTERN_BREAK;
                        if (effect_params < 0x40)
                            converted_effect_param = effect_params;
                        else
                            converted_effect_param = 0;

                        break;
                    }
                    case 0xE:
                    {
                        effect_number = effect_params >> 4;
                        effect_params &= 0xF;

                        switch (effect_number)
                        {
                            case 0x0: // Set filter
                            {
                                printf("Effect not supported: E0x (Set Filter)\n");
                                // For now there is no intention of implementing
                                // this effect in the future, as it has to do
                                // with a hardware filter of the Amiga.
                                break;
                            }
                            case 0x1: // Fine porta up
                            {
                                converted_effect = EFFECT_FINE_PORTA_UP;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0x2: // Fine porta down
                            {
                                converted_effect = EFFECT_FINE_PORTA_DOWN;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0x4: // Vibrato waveform
                            {
                                if (effect_params > 8)
                                {
                                    printf("Effect EC4 (Vibrato Waveform): Invalid param: %d\n",
                                           effect_params);
                                    effect_params = 0;
                                }

                                converted_effect = EFFECT_VIBRATO_WAVEFORM;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0x7: // Tremolo waveform
                            {
                                if (effect_params > 8)
                                {
                                    printf("Effect EC7 (Tremolo Waveform): Invalid param: %d\n",
                                           effect_params);
                                    effect_params = 0;
                                }

                                converted_effect = EFFECT_TREMOLO_WAVEFORM;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0x8: // 16 pos panning
                            {
                                // Input:    0x0 = left, 0xF = right
                                // Required: 0x00 = left, 0xFF = right
                                converted_effect = EFFECT_SET_PANNING;
                                converted_effect_param = (effect_params * 255) / 15;
                                break;
                            }
                            case 0x9: // Retrig note
                            {
                                converted_effect = EFFECT_RETRIG_NOTE;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0xA: // Fine volume slide up
                            {
                                int value = effect_params * MOD_VOLUME_SCALE;
                                converted_effect = EFFECT_FINE_VOLUME_SLIDE;
                                converted_effect_param = value;
                                break;
                            }
                            case 0xB: // Fine volume slide down
                            {
                                int value = effect_params * MOD_VOLUME_SCALE;
                                converted_effect = EFFECT_FINE_VOLUME_SLIDE;
                                converted_effect_param = -value;
                                break;
                            }
                            case 0xC: // Cut note
                            {
                                converted_effect = EFFECT_CUT_NOTE;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0xD: // Delay note
                            {
                                if (effect_params == 0)
                                {
                                    printf("Effect ED0 (Delay Note): Invalid param: 0\n");
                                    break;
                                }
                                converted_effect = EFFECT_DELAY_NOTE;
                                converted_effect_param = effect_params;
                                break;
                            }
                            case 0xF: // Invert loop
                            {
                                printf("Effect not supported: EFx (Invert Loop)\n");
                                // For now there is no intention of implementing
                                // this effect in the future, unless there are
                                // songs that actually use it.
                                break;
                            }

                            case 0x3: // Glissando control
                            case 0x5: // Set finetune
                            case 0x6: // Pattern loop
                            case 0xE: // Pattern delay
                            default:
                            {
                                printf("Effect not supported: E%X%X\n",
                                       effect_number, effect_params);
                                break;
                            }
                        }
                        break;
                    }
                    case 0xF: // Set speed
                    {
                        // Ignore if the parameter is 0
                        if (effect_params == 0)
                            break;

                        converted_effect = EFFECT_SET_SPEED;
                        converted_effect_param = effect_params;
                        break;
                    }
                    default:
                    {
                        printf("Effect not supported: %X%02X\n",
                               effect_number, effect_params);
                        break;
                    }
                }

                int instrument = -1;
                if (instrument_number > 0)
                {
                    instrument = instrument_index[instrument_number - 1];

                    if (volume == -1)
                    {
                        // If there is an instrument, but no volume command, set
                        // the volume to the default instrument volume.

                        instrument_get_volume(instrument, &volume);
                    }
                }

                pattern_step_set(pattern_index[i], r, c,
                                 note, instrument, volume,
                                 converted_effect, converted_effect_param);
            }
        }
    }

    // Save pattern order

    for (int i = 0; i < header->song_length; i++)
    {
        uint8_t index = header->pattern_table[i];

        int generic_pattern_index = pattern_index[index];

        song_set_pattern(song_index, i, generic_pattern_index);
    }

    ret = 0;
cleanup:
    free(buffer);
    return ret;
}
