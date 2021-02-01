// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#ifndef UMOD_DEFINITIONS_H__
#define UMOD_DEFINITIONS_H__

#ifdef __GBA__
# define ARM_CODE   __attribute__((target("arm")))
# define THUMB_CODE __attribute__((target("thumb")))
# define IWRAM_DATA __attribute__((section(".iwram")))
# define EWRAM_DATA __attribute__((section(".ewram")))
# define EWRAM_BSS  __attribute__((section(".sbss")))
# define IWRAM_CODE __attribute__((section(".iwram"), long_call))
# define EWRAM_CODE __attribute__((section(".ewram"), long_call))
#else
# define ARM_CODE
# define THUMB_CODE
# define IWRAM_DATA
# define EWRAM_DATA
# define EWRAM_BSS
# define IWRAM_CODE
# define EWRAM_CODE
#endif

#endif // UMOD_DEFINITIONS_H__
