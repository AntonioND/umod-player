# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

function(test_mod_wav mod_name)

    # Generate file names

    get_filename_component(base_name ${mod_name} NAME_WE)

    set(REF_MOD "${CMAKE_CURRENT_SOURCE_DIR}/${base_name}.mod")
    set(REF_TAR_BZ "${CMAKE_CURRENT_SOURCE_DIR}/${base_name}.wav.tar.bz")
    set(REF_WAV "${CMAKE_CURRENT_BINARY_DIR}/${base_name}.wav")
    set(REF_PACK "${CMAKE_CURRENT_BINARY_DIR}/${base_name}_pack.bin")
    set(REF_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${base_name}_header.h")
    set(GEN_WAV "${CMAKE_CURRENT_BINARY_DIR}/${base_name}_generated.wav")

    # Add test to CTest

    set(CMD1 "${CMAKE_COMMAND} -E tar -xf ${REF_TAR_BZ}")
    set(CMD2 "$<TARGET_FILE:umod_packer> ${REF_PACK} ${REF_HEADER} ${REF_MOD}")
    set(CMD3 "$<TARGET_FILE:umod_renderer>  ${REF_PACK} ${GEN_WAV}")
    set(CMD4 "${CMAKE_COMMAND} -E compare_files ${REF_WAV} ${GEN_WAV}")

    add_test(NAME ${base_name}_mod_test
        COMMAND ${CMAKE_COMMAND}
                    -DCMD1=${CMD1}
                    -DCMD2=${CMD2}
                    -DCMD3=${CMD3}
                    -DCMD4=${CMD4}
                    -P ${CMAKE_SOURCE_DIR}/tests/cmake/runcommands.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add target to autogenerate the new compressed file to be used as reference

    set(CMD1 "$<TARGET_FILE:umod_packer> ${REF_PACK} ${REF_HEADER} ${REF_MOD}")
    set(CMD2 "$<TARGET_FILE:umod_renderer>  ${REF_PACK} ${REF_WAV}")
    set(CMD3 "${CMAKE_COMMAND} -E tar -cfj ${REF_TAR_BZ} ${REF_WAV}")

    add_custom_target(${base_name}_mod_test_generate
        COMMAND ${CMAKE_COMMAND}
                    -DCMD1=${CMD1}
                    -DCMD2=${CMD2}
                    -DCMD3=${CMD3}
                    -P ${CMAKE_SOURCE_DIR}/tests/cmake/runcommands.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_dependencies(generate_references ${base_name}_mod_test_generate)

endfunction()


function(test_sfx_wav)

    # Generate file names

    get_filename_component(directory_name ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

    set(BINARY_NAME "test_sfx_${directory_name}")

    set(REF_TAR_BZ "${CMAKE_CURRENT_SOURCE_DIR}/reference.wav.tar.bz")

    set(REF_WAV "${CMAKE_CURRENT_BINARY_DIR}/reference.wav")
    set(REF_PACK "${CMAKE_CURRENT_BINARY_DIR}/pack.bin")
    set(REF_HEADER "${CMAKE_CURRENT_BINARY_DIR}/pack_header.h")
    set(GEN_WAV "${CMAKE_CURRENT_BINARY_DIR}/generated.wav")

    # Build source files

    add_executable(${BINARY_NAME})
    compiler_flags_sdl2(${BINARY_NAME})
    linker_flags_sdl2(${BINARY_NAME})

    search_source_files(. FILES_SOURCE)

    target_sources(${BINARY_NAME} PRIVATE ${FILES_SOURCE})

    target_link_libraries(${BINARY_NAME} umod_player utils)

    # Create pack file

    search_audio_files(. FILES_AUDIO)

    add_custom_command(
        OUTPUT ${REF_PACK} ${REF_HEADER}
        COMMAND $<TARGET_FILE:umod_packer> ${REF_PACK} ${REF_HEADER} ${FILES_AUDIO}
        DEPENDS ${FILES_AUDIO}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add output source files to the target

    message(VERBOSE "${directory_name}: Adding ${FILES_AUDIO}")

    target_sources(${BINARY_NAME} PRIVATE ${REF_HEADER})
    target_include_directories(${BINARY_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    set_source_files_properties(${REF_PACK} ${REF_HEADER} PROPERTIES GENERATED 1)

    # Add test to CTest

    set(CMD1 "${CMAKE_COMMAND} -E tar -xf ${REF_TAR_BZ}")
    set(CMD2 "$<TARGET_FILE:${BINARY_NAME}> ${GEN_WAV}")
    set(CMD3 "${CMAKE_COMMAND} -E compare_files ${REF_WAV} ${GEN_WAV}")

    add_test(NAME ${directory_name}_sfx_test
        COMMAND ${CMAKE_COMMAND}
                    -DCMD1=${CMD1}
                    -DCMD2=${CMD2}
                    -DCMD3=${CMD3}
                    -P ${CMAKE_SOURCE_DIR}/tests/cmake/runcommands.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add target to autogenerate the new compressed file to be used as reference

    set(CMD1 "$<TARGET_FILE:${BINARY_NAME}> ${REF_WAV}")
    set(CMD2 "${CMAKE_COMMAND} -E tar -cfj ${REF_TAR_BZ} ${REF_WAV}")

    add_custom_target(${directory_name}_sfx_test_generate
        COMMAND ${CMAKE_COMMAND}
                    -DCMD1=${CMD1}
                    -DCMD2=${CMD2}
                    -P ${CMAKE_SOURCE_DIR}/tests/cmake/runcommands.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_dependencies(generate_references ${directory_name}_sfx_test_generate)

endfunction()
