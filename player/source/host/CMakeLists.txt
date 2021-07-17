# SPDX-License-Identifier: MIT
#
# Copyright (c) 2020-2021 Antonio Niño Díaz

include(${CMAKE_SOURCE_DIR}/cmake/toolchain_host.cmake)

# Define library target
# ---------------------

add_library(umod_player STATIC)

# Source code, include directories and global definitions
# -------------------------------------------------------

target_sources(umod_player PRIVATE ${PLAYER_SOURCES})
target_include_directories(umod_player PUBLIC SYSTEM ${INCLUDE_PATH})
