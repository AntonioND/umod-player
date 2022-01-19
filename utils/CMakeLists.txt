# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021-2022 Antonio Niño Díaz

add_library(utils)

umod_search_source_files(. FILES_SOURCE)

target_sources(utils PRIVATE ${FILES_SOURCE})

target_include_directories(utils PUBLIC .)
