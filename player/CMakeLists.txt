# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021-2022 Antonio Niño Díaz

umod_search_source_files(source PLAYER_SOURCES)
get_filename_component(INCLUDE_PATH "include" ABSOLUTE)

add_subdirectory(source)
