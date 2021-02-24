// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdio.h>
#include <string.h>

static FILE *header = NULL;
static char header_guard_name[2048]; // TODO: Don't hardcode this

static void generate_define_name(const char *path, char *name)
{
    size_t len = strlen(path);

    size_t name_start = len;
    while (name_start > 0)
    {
        if ((path[name_start] == '/') || (path[name_start] == '\\'))
        {
            name_start++; // Skip folder separator
            break;
        }
        name_start--;
    }

    while (name_start < len)
    {
        char c = path[name_start];

        if ((c >= 'A') && (c <= 'Z'))
        {
            // Leave as it is
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            // To uppercase
            c = c + 'A' - 'a';
        }
        else if ((c >= '0') && (c <= '9'))
        {
            // Leave as it is
        }
        else
        {
            // Any other symbol is converted to an underscore
            c = '_';
        }

        *name++ = c;

        name_start++;
    }

    *name = '\0';
}

int header_start(const char *path)
{
    if (header != NULL)
        return -1;

    header = fopen(path, "w");
    if (header == NULL)
    {
        printf("Failed to open: %s\n", path);
        return -1;
    }

    generate_define_name(path, &header_guard_name[0]);

    fprintf(header, "#ifndef %s__\n#define %s__\n\n",
            header_guard_name, header_guard_name);

    return 0;
}

int header_add_song(const char *path, int song_index)
{
    char define[2048]; // TODO: Don't hardcode this
    generate_define_name(path, &define[0]);

    fprintf(header, "#define SONG_%s %d\n", define, song_index);

    return 0;
}

int header_add_sfx(const char *path, int instrument_index)
{
    char define[2048]; // TODO: Don't hardcode this
    generate_define_name(path, &define[0]);

    fprintf(header, "#define SFX_%s %d\n", define, instrument_index);

    return 0;
}

int header_end(void)
{
    if (header == NULL)
        return -1;

    fprintf(header, "\n#endif // %s__\n", header_guard_name);

    fclose(header);
    return 0;
}
