# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021-2022 Antonio Niño Díaz

umod_toolchain_sdl2()

add_executable(umod_packer)
compiler_flags_sdl2(umod_packer)
linker_flags_sdl2(umod_packer)

search_source_files(. FILES_SOURCE)

target_sources(umod_packer PRIVATE ${FILES_SOURCE})

target_link_libraries(umod_packer umod_player)
