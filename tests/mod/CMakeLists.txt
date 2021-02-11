# SPDX-License-Identifier: MIT
#
# Copyright (c) 2021 Antonio Niño Díaz

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
test_mod_wav("cut_note.mod")

# Test effect Bxy and some corner cases like having it twice in the same row.
test_mod_wav("jump_to_pattern.mod")

# Test effects Bxy and Dxy in the same row.
test_mod_wav("jump_to_pattern_plus_pattern_break.mod")

# Tests effects 3xy and 5xy.
test_mod_wav("porta_to_note.mod")

# Tests effects 1xy, 2xy, E1x and E2x.
test_mod_wav("porta_up_down.mod")

# Tests effects 8xy and E8x.
test_mod_wav("panning.mod")

# Test effect Dxy and some corner cases like having it twice in the same row.
test_mod_wav("pattern_break.mod")

# Tests full range of notes from C-3 to B-8.
test_mod_wav("range_test.mod")

# Tests behaviour of effect 9xy.
test_mod_wav("sample_offset.mod")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
test_mod_wav("sample_that_loops.mod")

# Tests effects 4xy and 6xy.
test_mod_wav("vibrato.mod")

# Test Cxy effect. Test that ECx effect sets the volume to 0 and the channel can
# be made to sound again with a Cxy effect. Also, test samples with different
# default volumes.
test_mod_wav("volume.mod")

# Tests effects Axy, EAx, EBx.
test_mod_wav("volume_slide.mod")
