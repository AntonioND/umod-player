// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

// Example of sound streaming with double buffer and DMA. It sets up both DMA
// channels, and sends the values of DMA A to the left speaker and DMA B to the
// right speaker. Note that the GBA only has one speaker, it is needed to use
// headphones to hear the difference.

#include <stdio.h>
#include <string.h>

#include <gba.h>

#include <umod/umod.h>

#include "pack_bin.h"
#include "pack_info.h"

// Buffer size needs to be a multiple of 16 (the amount of bytes copied to the
// FIFO whenever it gets data from DMA).
//
// Timer Reload = Clocks per frame / buffer size = 280896 / buffer size
// It needs to be an exact division.
//
// Sample rate = Buffer size * FPS = Buffer size * CPU Freq / Clocks per frame
// Sample rate = (Buffer size * 16 * 1024 * 1024) / 280896
//
// Valid combinations:
//
// Sample Rate | Timer Reload | Buffer Size
// ------------+--------------+------------
// 10512.04    | 1596         | 176
// 13378.96    | 1254         | 224
// 18157.16    | 924          | 304
// 21024.08    | 798          | 352
// 26757.92    | 627          | 448
// 31536.12    | 532          | 528
// 36314.32    | 462          | 608
// 40136.88    | 418          | 672

#define SAMPLE_RATE         31536

#define TICKS_PER_RELOAD    532
#define RELOAD_VALUE        (65536 - TICKS_PER_RELOAD)

#define BUFFER_SIZE         528

#define DMA_TIMER_INDEX     0 // Timer 0 controls the transfer rate of DMA A/B

static int current_dma_buffer = 0;

ALIGN(32) int8_t wave_a[BUFFER_SIZE * 2];
ALIGN(32) int8_t wave_b[BUFFER_SIZE * 2];

IWRAM_CODE void Audio_DMA_Retrigger(void)
{
    uint32_t flags = DMA_DST_FIXED | DMA_SRC_INC |
                     DMA_REPEAT | DMA32 | DMA_SPECIAL | DMA_ENABLE;

    // Stop the channels first
    REG_DMA1CNT = 0;
    REG_DMA2CNT = 0;

    // Enable them again
    REG_DMA1CNT = flags;
    REG_DMA2CNT = flags;
}

IWRAM_CODE void vbl_handler(void)
{
    // The buffer swap needs to be done right at the beginning of the VBL
    // interrupt handler so that the timing is always the same in each frame.

    if (current_dma_buffer == 0)
        Audio_DMA_Retrigger();
}

void Audio_DMA_Setup(void)
{
    // Setup sound, timer and DMA

    // The sound hardware needs to be enabled to write to any other register.
    REG_SOUNDCNT_X = 1 << 7;
    // DMA A to the left, DMA B to the right
    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDB_VOL_100 |
                     SNDA_L_ENABLE | SNDB_R_ENABLE |
                     SNDA_RESET_FIFO | SNDB_RESET_FIFO;

    REG_TM0CNT_H = 0; // Stop
    REG_TM0CNT_L = RELOAD_VALUE;
    REG_TM0CNT_H = TIMER_START;

    // Stop the channels first
    REG_DMA1CNT = 0;
    REG_DMA2CNT = 0;

#define PTR_REG_FIFO_A  ((uint32_t volatile *)0x40000A0)
#define PTR_REG_FIFO_B  ((uint32_t volatile *)0x40000A4)

    REG_DMA1SAD = (uintptr_t)&wave_a[0];
    REG_DMA1DAD = (uintptr_t)PTR_REG_FIFO_A;

    REG_DMA2SAD = (uintptr_t)&wave_b[0];
    REG_DMA2DAD = (uintptr_t)PTR_REG_FIFO_B;
}

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);
    irqSet(IRQ_VBLANK, vbl_handler);

    UMOD_Init(SAMPLE_RATE);
    UMOD_LoadPack(pack_bin);

    consoleDemoInit();

    iprintf("Sample rate: %d\n"
            "Samples per frame: %d\n"
            "Ticks per reload: %d\n",
            SAMPLE_RATE,
            BUFFER_SIZE,
            TICKS_PER_RELOAD);

    Audio_DMA_Setup();

    UMOD_PlaySong(SONG_KAOS_OCH_DEKADENS_MOD);

    umod_handle helicopter_handle = UMOD_HANDLE_INVALID;

    while (1)
    {
        if (current_dma_buffer == 1)
            UMOD_Mix(wave_a, wave_b, BUFFER_SIZE);
        else
            UMOD_Mix(&wave_a[BUFFER_SIZE], &wave_b[BUFFER_SIZE], BUFFER_SIZE);

        current_dma_buffer ^= 1;

        scanKeys();

        int keys_pressed = keysDown();

        if (keys_pressed & KEY_A)
            UMOD_SFX_Play(SFX_LASER2_1_WAV, UMOD_LOOP_DEFAULT);

        if (keys_pressed & KEY_B)
        {
            if (helicopter_handle == 0)
            {
                helicopter_handle = UMOD_SFX_Play(SFX_HELICOPTER_WAV, UMOD_LOOP_ENABLE);
            }
            else
            {
                UMOD_SFX_Stop(helicopter_handle);
                helicopter_handle = 0;
            }
        }

        VBlankIntrWait();
    }
}
