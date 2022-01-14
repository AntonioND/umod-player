# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021-2022 Antonio Niño Díaz

cmake_minimum_required(VERSION 3.13)
project(umod)
enable_language(C)

set(C_STANDARD 11)
set(C_EXTENSIONS OFF)
set(C_STANDARD_REQUIRED ON)

# Detect if this project is a submodule

get_directory_property(PROJECT_HAS_PARENT PARENT_DIRECTORY)
set(PROJECT_IS_SUBMODULE ${PROJECT_HAS_PARENT})

# Macro that searches all the source files in the specified directory in 'dir'
# and saves them in 'var'

macro(search_source_files dir var)
    file(GLOB ${var} CONFIGURE_DEPENDS ${dir}/*.c ${dir}/*.h)
endmacro()

# Macro that searches all the audio files in the specified directory in 'dir'
# and saves them in 'var'

macro(search_audio_files dir var)
    file(GLOB ${var} CONFIGURE_DEPENDS ${dir}/*.mod ${dir}/*.wav)
endmacro()

# Global build options

option(BUILD_GBA "Build GBA binaries in addition to PC binaries" OFF)

option(USE_DEVKITARM "Use devkitARM to build GBA binaries" ON)

# Toolchain selection macros

include(cmake/compiler_flags.cmake)
include(cmake/toolchain_gba.cmake)
include(cmake/toolchain_sdl2.cmake)

# Add components

add_subdirectory(player)
add_subdirectory(packer)
add_subdirectory(renderer)
add_subdirectory(utils)

# If this project is being used as a module within another project, remove all
# testing from the build.
if(NOT PROJECT_IS_SUBMODULE)
    enable_testing()

    add_subdirectory(tests)
endif()
