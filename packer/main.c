// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdio.h>
#include <string.h>

#include "mod.h"
#include "save_header.h"
#include "save_pack.h"
#include "wav.h"

int add_file(const char *path)
{
    size_t len = strlen(path);
    const char *extension = NULL;

    for (size_t i = len - 1; i > 0; i--)
    {
        if (path[i] == '.')
        {
            extension = &path[i + 1];
            break;
        }
    }

    if (extension == NULL)
    {
        printf("Extension not found\n");
        return -1;
    }

    if (strcmp(extension, "mod") == 0)
    {
        int song_index;
        int ret = add_mod(path, &song_index);
        printf("Saved [%s] to song index %d\n", path, song_index);

        if (ret == 0)
            header_add_song(path, song_index);

        return ret;
    }
    else if (strcmp(extension, "wav") == 0)
    {
        int instrument_index;
        int ret = add_wav(path, &instrument_index);
        printf("Saved [%s] to instrument index %d\n", path, instrument_index);

        if (ret == 0)
            header_add_sfx(path, instrument_index);

        return ret;
    }
    else
    {
        printf("Invalid extension\n");
    }

    return -1;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Not enough arguments.\n\n");

        printf("Usage: %s [output pack].bin [output header].h <audio files>\n\n",
               argv[0]);

        printf("Supported formats:\n"
               "\n"
               "  MOD: 4 and 8 channels\n"
               "  WAV: PCM (no ADPCM)\n"
               "\n");

        return -1;
    }

    const char *save_file = argv[1];
    const char *header_file = argv[2];

    argc--; // Skip argv[0]
    argv++;

    argc--; // Skip argv[1] = save_file
    argv++;

    argc--; // Skip argv[2] = header_file
    argv++;

    header_start(header_file);

    for (int i = 0; i < argc; i++)
    {
        printf("[*] LOADING: %s\n", argv[i]);
        int ret = add_file(argv[i]);
        if (ret != 0)
            return ret;
    }

    header_end();

    return save_pack(save_file);
}
