// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef SAVE_HEADER_H__
#define SAVE_HEADER_H__

int header_start(const char *path);
int header_add_song(const char *path, int song_index);
int header_add_sfx(const char *path, int instrument_index);
int header_end(void);

#endif // SAVE_HEADER_H__
