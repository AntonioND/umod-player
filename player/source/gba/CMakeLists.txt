# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

include(${CMAKE_SOURCE_DIR}/cmake/toolchain_gba.cmake)

# Define library target
# ---------------------

add_library(umod_player_gba STATIC)

# Source code, include directories and global definitions
# -------------------------------------------------------

target_sources(umod_player_gba PRIVATE ${PLAYER_SOURCES})
target_include_directories(umod_player_gba PUBLIC ${INCLUDE_PATH})
