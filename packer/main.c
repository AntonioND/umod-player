// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <stdio.h>
#include <string.h>

#include "mod.h"
#include "save_pack.h"

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
        return add_mod(path);
    }
    else
    {
        printf("Invalid extension\n");
    }

    return -1;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Not enough arguments.\n");
        return -1;
    }

    const char *save_file = argv[1];

    argc--; // Skip argv[0]
    argv++;

    argc--; // Skip argv[1]
    argv++;

    for (int i = 0; i < argc; i++)
    {
        printf("LOADING: %s\n", argv[i]);
        int ret = add_file(argv[i]);
        if (ret != 0)
            return ret;
    }

    return save_pack(save_file);
}
