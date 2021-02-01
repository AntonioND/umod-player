# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

add_library(umod_player)

search_source_files(source FILES_SOURCE)

target_sources(umod_player PRIVATE ${FILES_SOURCE})

target_include_directories(umod_player PUBLIC include)
