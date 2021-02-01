# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

add_executable(umod_packer)

search_source_files(. FILES_SOURCE)

target_sources(umod_packer PRIVATE ${FILES_SOURCE})

target_link_libraries(umod_packer umod_player)
