# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

# Target that re-generates all references of all unit tests
add_custom_target(generate_references)

include(cmake/test.cmake)

add_subdirectory(mod)
add_subdirectory(sfx)
