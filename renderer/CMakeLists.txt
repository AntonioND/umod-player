# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021-2022 Antonio Niño Díaz

umod_toolchain_sdl2()

add_executable(umod_renderer)
umod_compiler_flags_sdl2(umod_renderer)
umod_linker_flags_sdl2(umod_renderer)

umod_search_source_files(. FILES_SOURCE)

target_sources(umod_renderer PRIVATE ${FILES_SOURCE})

target_link_libraries(umod_renderer umod_player utils)
