# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

cmake_minimum_required(VERSION 3.13)
project(umod)
enable_language(C)

set(C_STANDARD 11)
set(C_EXTENSIONS OFF)
set(C_STANDARD_REQUIRED ON)

# Global compiler-specific options

include(cmake/compiler_flags.cmake)

# Macro that searches all the source files in the specified directory in 'dir'
# and saves them in 'var'

macro(search_source_files dir var)
    file(GLOB ${var} CONFIGURE_DEPENDS ${dir}/*.c ${dir}/*.h)
endmacro()

# Add components

add_subdirectory(player)
add_subdirectory(packer)
add_subdirectory(renderer)
