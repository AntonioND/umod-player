// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <umod/umod.h>
#include <umod/umodpack.h>

#include "definitions.h"
#include "mixer_channel.h"
#include "mod_channel.h"

typedef struct {
    int     note;
    int     volume;
    void   *instrument_pointer;
    int     panning; // 0...255 = left...right
    int     effect;
    int     effect_params;
    int     arpeggio_tick;

    int     mixer_channel_handle;
} mod_channel_info;

static mod_channel_info mod_channel[MOD_CHANNELS_MAX];

void ModChannelReset(int channel)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info * ch = &mod_channel[channel];

    MixerChannelStop(ch->mixer_channel_handle);

    ch->note = -1;
    ch->volume = -1;
    ch->instrument_pointer = NULL;
    ch->effect = EFFECT_NONE;
    ch->effect_params = -1;
    ch->panning = 128; // Middle

    ch->mixer_channel_handle = 0;
}

void ModChannelResetAll(void)
{
    for (int i = 0; i < MOD_CHANNELS_MAX; i++)
        ModChannelReset(i);
}

// Table taken from:
//
//   https://github.com/OpenMPT/openmpt/blob/818b2c101d2256a430291ddcbcb47edd7e762308/soundlib/Tables.cpp#L272-L290
//
static uint16_t finetuned_period_table[16][12] = {
    // Values for octave 0. Divide by 2 to get octave 1, by 4 to get octave 2...
    //  C    C#    D     D#    E     F     F#    G     G#    A     A#    B
    { 1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960, 907 },
    { 1700, 1604, 1514, 1430, 1348, 1274, 1202, 1134, 1070, 1010,  954, 900 },
    { 1688, 1592, 1504, 1418, 1340, 1264, 1194, 1126, 1064, 1004,  948, 894 },
    { 1676, 1582, 1492, 1408, 1330, 1256, 1184, 1118, 1056,  996,  940, 888 },
    { 1664, 1570, 1482, 1398, 1320, 1246, 1176, 1110, 1048,  990,  934, 882 },
    { 1652, 1558, 1472, 1388, 1310, 1238, 1168, 1102, 1040,  982,  926, 874 },
    { 1640, 1548, 1460, 1378, 1302, 1228, 1160, 1094, 1032,  974,  920, 868 },
    { 1628, 1536, 1450, 1368, 1292, 1220, 1150, 1086, 1026,  968,  914, 862 },
    { 1814, 1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960 },
    { 1800, 1700, 1604, 1514, 1430, 1350, 1272, 1202, 1134, 1070, 1010, 954 },
    { 1788, 1688, 1592, 1504, 1418, 1340, 1264, 1194, 1126, 1064, 1004, 948 },
    { 1774, 1676, 1582, 1492, 1408, 1330, 1256, 1184, 1118, 1056,  996, 940 },
    { 1762, 1664, 1570, 1482, 1398, 1320, 1246, 1176, 1110, 1048,  988, 934 },
    { 1750, 1652, 1558, 1472, 1388, 1310, 1238, 1168, 1102, 1040,  982, 926 },
    { 1736, 1640, 1548, 1460, 1378, 1302, 1228, 1160, 1094, 1032,  974, 920 },
    { 1724, 1628, 1536, 1450, 1368, 1292, 1220, 1150, 1086, 1026,  968, 914 }
};

// Returns the number of ticks needed to increase the sample read pointer in an
// instrument. For example, if it returns 4.5, the pointer in the instrument
// must be increased after every 4.5 samples that the global mixer generates.
//
// It returns a fixed point number in 32.32 format.
ARM_CODE
static uint64_t ModGetSampleTickPeriod(int note_index, int finetune)
{
    // TODO: Mark this function as ARM (because of the 64 bit multiply)

    int octave = note_index / 12;
    int note = note_index % 12;

    uint32_t amiga_period = finetuned_period_table[finetune][note];

    // To convert from this to Hz, do:
    //
    //   Frequency (Hz) [PAL Value]  = 7093789.2 / (Amiga Period * 2)
    //   Frequency (Hz) [NTSC Value] = 7159090.5 / (Amiga Period * 2)
    //
    // It looks like the NTSC value is the most popular one to use.
    //
    // Then, to convert to sample tick period:
    //
    //   Period (Samples) = Sample Rate / Frequency
    //
    // Result:
    //
    //   Period (Samples) = (Sample Rate * Amiga Period * 2) / 7159090.5
    //
    //   Period (Samples) = Amiga Period * ((Sample Rate * 2) / 7159090.5)
    //
    // On the other hand, to calculate the period of an arbitrary octave from
    // the values of octave 0 it is needed to divide by 2:
    //
    //   C1 = C0 / 2 ; C2 = C0 / 4 ; etc
    //                                                Sample Rate * 2
    //   Period (Sample) = (Amiga Period [Octave 0] * ---------------) >> octave
    //                                                   7159090.5

    const uint64_t constant = (((uint64_t)UMOD_SAMPLE_RATE * 2) << 32) / 7159090.5;

    uint64_t sample_tick_period = (amiga_period * constant) >> octave;

    return sample_tick_period;
}

void ModChannelSetNote(int channel, int note)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->note = note;

    if (!MixerChannelIsPlaying(mod_ch->mixer_channel_handle))
    {
        uint32_t handle = MixerChannelAllocate();
        mod_ch->mixer_channel_handle = handle;

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
        MixerChannelSetVolume(handle, mod_ch->volume);
    }

    if (mod_ch->mixer_channel_handle != MIXER_HANDLE_INVALID)
    {
        // TODO: Finetune (either default from sample, or current one of effect)
        uint64_t period = ModGetSampleTickPeriod(note, 0);
        MixerChannelSetNotePeriod(mod_ch->mixer_channel_handle, period);
    }
}

void ModChannelSetVolume(int channel, int volume)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->volume = volume;

    uint32_t handle = mod_ch->mixer_channel_handle;
    MixerChannelSetVolume(handle, mod_ch->volume);
}

void ModChannelSetInstrument(int channel, void *instrument_pointer)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->instrument_pointer = instrument_pointer;

    uint32_t handle = mod_ch->mixer_channel_handle;
    MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
}

void ModChannelSetEffect(int channel, int effect, int effect_params)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->effect = effect;
    mod_ch->effect_params = effect_params;

    if (effect == EFFECT_NONE)
    {
        return;
    }
    else if (effect == EFFECT_SET_PANNING)
    {
        mod_ch->panning = effect_params;
        uint32_t handle = mod_ch->mixer_channel_handle;
        MixerChannelSetPanning(handle, mod_ch->panning);
    }
    else if (effect == EFFECT_ARPEGGIO)
    {
        mod_ch->arpeggio_tick = 0;
    }

    // TODO
}

void ModChannelUpdateAllTick(int tick_number)
{
    for (size_t c = 0; c < MOD_CHANNELS_MAX; c++)
    {
        mod_channel_info *ch = &mod_channel[c];

        uint32_t handle = ch->mixer_channel_handle;

        if (handle == MIXER_HANDLE_INVALID)
            continue;

        if (ch->effect == EFFECT_CUT_NOTE)
        {
            if (ch->effect_params == tick_number)
            {
                ch->effect = EFFECT_NONE;
                MixerChannelSetVolume(handle, 0);
            }
        }
        else if (ch->effect == EFFECT_ARPEGGIO)
        {
            int note = ch->note;

            if (ch->arpeggio_tick == 0)
            {
                ch->arpeggio_tick++;
            }
            else if (ch->arpeggio_tick == 1)
            {
                note += ch->effect_params >> 4;
                ch->arpeggio_tick++;
            }
            else if (ch->arpeggio_tick == 2)
            {
                note += ch->effect_params & 0xF;
                ch->arpeggio_tick = 0;
            }

            // TODO: Finetune
            uint64_t period = ModGetSampleTickPeriod(note, 0);
            MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
        }

        // TODO : Other effects
    }
}

void ModChannelUpdateAllRow(void)
{
    // TODO
}