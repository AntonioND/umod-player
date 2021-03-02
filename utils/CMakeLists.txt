# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

add_library(utils)

search_source_files(. FILES_SOURCE)

target_sources(utils PRIVATE ${FILES_SOURCE})

target_include_directories(utils PUBLIC .)
