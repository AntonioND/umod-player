# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

add_executable(umod_renderer)

search_source_files(. FILES_SOURCE)

target_sources(umod_renderer PRIVATE ${FILES_SOURCE})

target_link_libraries(umod_renderer umod_player)
