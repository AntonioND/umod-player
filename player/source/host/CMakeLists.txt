# SPDX-License-Identifier: MIT
#
# Copyright (c) 2020-2022 Antonio Niño Díaz

umod_toolchain_sdl2()

# Define library target
# ---------------------

add_library(umod_player STATIC)

# Source code, include directories and global definitions
# -------------------------------------------------------

target_sources(umod_player PRIVATE ${PLAYER_SOURCES})
target_include_directories(umod_player PUBLIC SYSTEM ${INCLUDE_PATH})
