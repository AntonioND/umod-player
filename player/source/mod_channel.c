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
    int         note;
    int32_t     amiga_period;

    int         volume;

    umodpack_instrument    *instrument_pointer;

    int         panning; // 0...255 = left...right

    int         effect;
    int         effect_params;

    int         arpeggio_tick;

    int         vibrato_tick;
    int         vibrato_args;

    int         tremolo_tick;
    int         tremolo_args;

    int         retrig_tick;

    int32_t     porta_to_note_target_amiga_period;
    int         porta_to_note_speed;

    int16_t    *vibrato_wave_table;
    int         vibrato_retrigger;

    int16_t    *tremolo_wave_table;
    int         tremolo_retrigger;

    int                     delayed_note;
    int                     delayed_volume;
    umodpack_instrument    *delayed_instrument;

    uint32_t    sample_offset; // Used for "Set Offset" effect

    uint32_t    mixer_channel_handle;
} mod_channel_info;

static mod_channel_info mod_channel[MOD_CHANNELS_MAX];

// Taken from FMODDOC.TXT
static int16_t vibrato_tremolo_wave_sine[64] = {
       0,   24,   49,   74,   97,  120,  141,  161,
     180,  197,  212,  224,  235,  244,  250,  253,
     255,  253,  250,  244,  235,  224,  212,  197,
     180,  161,  141,  120,   97,   74,   49,   24,
       0,  -24,  -49,  -74,  -97, -120, -141, -161,
    -180, -197, -212, -224, -235, -244, -250, -253,
    -255, -253, -250, -244, -235, -224, -212, -197,
    -180, -161, -141, -120,  -97,  -74,  -49,  -24
};

// for (int i = 0; i < 64; i++)
//     printf("%d, ", 256 - (i + 1) * 8);
static int16_t vibrato_tremolo_wave_ramp[64] = {
     248,  240,  232,  224,  216,  208,  200,  192,
     184,  176,  168,  160,  152,  144,  136,  128,
     120,  112,  104,   96,   88,   80,   72,   64,
      56,   48,   40,   32,   24,   16,    8,    0,
      -8,  -16,  -24,  -32,  -40,  -48,  -56,  -64,
     -72,  -80,  -88,  -96, -104, -112, -120, -128,
    -136, -144, -152, -160, -168, -176, -184, -192,
    -200, -208, -216, -224, -232, -240, -248, -256
};

// for (int i = 0; i < 64; i++)
//     printf("%d, ", i < 32 ? 255 : -255);
static int16_t vibrato_tremolo_wave_square[64] = {
     255,  255,  255,  255,  255,  255,  255,  255,
     255,  255,  255,  255,  255,  255,  255,  255,
     255,  255,  255,  255,  255,  255,  255,  255,
     255,  255,  255,  255,  255,  255,  255,  255,
    -255, -255, -255, -255, -255, -255, -255, -255,
    -255, -255, -255, -255, -255, -255, -255, -255,
    -255, -255, -255, -255, -255, -255, -255, -255,
    -255, -255, -255, -255, -255, -255, -255, -255
};

// for (int i = 0; i < 64; i++)
//     printf("%d, ", (rand() & 511) - 256);
static int16_t vibrato_tremolo_wave_random[64] = {
     103,  198, -151, -141, -175,   -1, -182,  -20,
      41,  -51,  -70,  171,  242,   -5,  227,   70,
    -132,  -62, -172,  248,   27,  232,  231,  141,
     118,   90,   46, -157, -205,  159,  -55,  154,
     102,   50, -243,  183, -207, -168,  -93,   90,
      37,   93,    5,   23, -168,  -23, -162,  -44,
     171,  -78,  -51,  -58, -101,  -76, -172, -239,
    -242,  130, -140, -191,   33,   61,  220, -121
};

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

static uint32_t ModNoteToAmigaPeriod(int note_index, int finetune)
{
    int octave = note_index / 12;
    int note = note_index % 12;

    uint32_t amiga_period = finetuned_period_table[finetune][note] >> octave;

    return amiga_period;
}

static uint64_t convert_constant;

void ModSetSampleRateConvertConstant(uint32_t sample_rate)
{
    convert_constant = ((uint64_t)sample_rate << 34) / 14318181;
}

// Returns the number of ticks needed to increase the sample read pointer in an
// instrument. For example, if it returns 4.5, the pointer in the instrument
// must be increased after every 4.5 samples that the global mixer generates.
//
// It returns a fixed point number in 32.32 format.
//
// This function is in ARM because ARM has support for long multiplies, unlike
// Thumb, so it is faster.
ARM_CODE
static uint64_t ModGetSampleTickPeriod(int note_index, int finetune)
{
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
    //
    //                                                Sample Rate * 4
    //   Period (Sample) = (Amiga Period [Octave 0] * ---------------) >> octave
    //                                                    14318181
    //
    //   As the returned value needs to be 32.32 fixed point:
    //
    //   Period (Sample) [Fixed Point] = Period (Sample) << 32
    //
    //                                                Sample Rate << 34
    //   Period (Sample) = (Amiga Period [Octave 0] * -----------------) >> octave
    //                                                    14318181

    uint64_t sample_tick_period = (amiga_period * convert_constant) >> octave;

    return sample_tick_period;
}

ARM_CODE
static uint64_t ModGetSampleTickPeriodFromAmigaPeriod(uint32_t amiga_period)
{
    uint64_t sample_tick_period = amiga_period * convert_constant;

    return sample_tick_period;
}

static uint32_t ModChannelAllocateMixer(mod_channel_info *mod_ch)
{
    uint32_t handle = MixerChannelAllocate();
    mod_ch->mixer_channel_handle = handle;

    return handle;
}

void ModChannelSetNote(int channel, int note)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];
    mod_ch->note = note;

    mod_ch->vibrato_tick = 0;

    uint32_t handle;

    if (mod_ch->mixer_channel_handle == MIXER_HANDLE_INVALID)
    {
        handle = ModChannelAllocateMixer(mod_ch);

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
        MixerChannelSetVolume(handle, mod_ch->volume);

        int finetune = 0;
        if (mod_ch->instrument_pointer != NULL)
            finetune = mod_ch->instrument_pointer->finetune;

        // TODO: Finetune from effect
        uint64_t period = ModGetSampleTickPeriod(mod_ch->note, finetune);
        MixerChannelSetNotePeriod(handle, period);

        uint32_t amiga_period = ModNoteToAmigaPeriod(mod_ch->note, 0);
        mod_ch->amiga_period = amiga_period;
    }
    else
    {
        handle = mod_ch->mixer_channel_handle;

        int finetune = 0;
        if (mod_ch->instrument_pointer != NULL)
            finetune = mod_ch->instrument_pointer->finetune;

        // TODO: Finetune from effect
        uint64_t period = ModGetSampleTickPeriod(note, finetune);
        MixerChannelSetNotePeriod(handle, period);

        uint32_t amiga_period = ModNoteToAmigaPeriod(note, 0);
        mod_ch->amiga_period = amiga_period;
    }
}

void ModChannelSetVolume(int channel, int volume)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->volume = volume;

    uint32_t handle;

    if (mod_ch->mixer_channel_handle == MIXER_HANDLE_INVALID)
    {
        handle = ModChannelAllocateMixer(mod_ch);

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
        MixerChannelSetVolume(handle, mod_ch->volume);

        int finetune = 0;
        if (mod_ch->instrument_pointer != NULL)
            finetune = mod_ch->instrument_pointer->finetune;

        // TODO: Finetune from effect
        uint64_t period = ModGetSampleTickPeriod(mod_ch->note, finetune);
        MixerChannelSetNotePeriod(handle, period);

        uint32_t amiga_period = ModNoteToAmigaPeriod(mod_ch->note, 0);
        mod_ch->amiga_period = amiga_period;
    }
    else
    {
        handle = mod_ch->mixer_channel_handle;

        MixerChannelSetVolume(handle, mod_ch->volume);
    }
}

void ModChannelSetInstrument(int channel, void *instrument_pointer)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->instrument_pointer = instrument_pointer;

    uint32_t handle;

    if (mod_ch->mixer_channel_handle == MIXER_HANDLE_INVALID)
    {
        handle = ModChannelAllocateMixer(mod_ch);

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
        MixerChannelSetVolume(handle, mod_ch->volume);

        int finetune = 0;
        if (mod_ch->instrument_pointer != NULL)
            finetune = mod_ch->instrument_pointer->finetune;

        // TODO: Finetune from effect
        uint64_t period = ModGetSampleTickPeriod(mod_ch->note, finetune);
        MixerChannelSetNotePeriod(handle, period);

        uint32_t amiga_period = ModNoteToAmigaPeriod(mod_ch->note, 0);
        mod_ch->amiga_period = amiga_period;
    }
    else
    {
        handle = mod_ch->mixer_channel_handle;

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
    }
}

void ModChannelSetEffectDelayNote(int channel, int effect_params, int note,
                                  int volume, void *instrument)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    mod_ch->effect = EFFECT_DELAY_NOTE;
    mod_ch->effect_params = effect_params;
    mod_ch->delayed_note = note;
    mod_ch->delayed_volume = volume;
    mod_ch->delayed_instrument = instrument;
}

void ModChannelSetEffect(int channel, int effect, int effect_params, int note)
{
    assert(channel < MOD_CHANNELS_MAX);

    mod_channel_info *mod_ch = &mod_channel[channel];

    if (mod_ch->mixer_channel_handle == MIXER_HANDLE_INVALID)
    {
        uint32_t handle = ModChannelAllocateMixer(mod_ch);

        MixerChannelSetInstrument(handle, mod_ch->instrument_pointer);
        MixerChannelSetVolume(handle, mod_ch->volume);

        int finetune = 0;
        if (mod_ch->instrument_pointer != NULL)
            finetune = mod_ch->instrument_pointer->finetune;

        // TODO: Finetune from effect
        uint64_t period = ModGetSampleTickPeriod(mod_ch->note, finetune);
        MixerChannelSetNotePeriod(handle, period);

        uint32_t amiga_period = ModNoteToAmigaPeriod(mod_ch->note, 0);
        mod_ch->amiga_period = amiga_period;
    }

    if ((mod_ch->effect == EFFECT_ARPEGGIO) && (effect != EFFECT_ARPEGGIO))
    {
        // Make sure that after an arpeggio effect, if there is no new arpeggio
        // effect, the note always goes back to the base frequency. This happens
        // only if no new note has been specified.
        if (note < 0)
        {
            uint32_t handle = mod_ch->mixer_channel_handle;

            int finetune = 0;
            if (mod_ch->instrument_pointer != NULL)
                finetune = mod_ch->instrument_pointer->finetune;

            // TODO: Finetune from effect
            uint64_t period = ModGetSampleTickPeriod(mod_ch->note, finetune);
            MixerChannelSetNotePeriod(handle, period);

            uint32_t amiga_period = ModNoteToAmigaPeriod(mod_ch->note, 0);
            mod_ch->amiga_period = amiga_period;
        }
    }

    mod_ch->effect = effect;
    mod_ch->effect_params = effect_params;

    if (effect == EFFECT_NONE)
    {
        return;
    }
    else if (effect == EFFECT_SET_PANNING)
    {
        uint32_t handle = mod_ch->mixer_channel_handle;
        mod_ch->panning = effect_params;
        MixerChannelSetPanning(handle, mod_ch->panning);
    }
    else if (effect == EFFECT_ARPEGGIO)
    {
        mod_ch->arpeggio_tick = 0;
    }
    else if (effect == EFFECT_PORTA_TO_NOTE)
    {
        if (effect_params != 0)
        {
            uint32_t amiga_period = ModNoteToAmigaPeriod(note, 0);

            mod_ch->porta_to_note_target_amiga_period = amiga_period;
            mod_ch->porta_to_note_speed = effect_params;
        }
    }
    else if ((effect == EFFECT_VIBRATO) || (effect == EFFECT_VIBRATO_VOL_SLIDE))
    {
        if ((note >= 0) && (mod_ch->vibrato_retrigger != 0))
            mod_ch->vibrato_tick = 0;

        if (effect == EFFECT_VIBRATO)
        {
            if (effect_params != 0)
                mod_ch->vibrato_args = effect_params;
        }
    }
    else if (effect == EFFECT_TREMOLO)
    {
        if ((note >= 0) && (mod_ch->tremolo_retrigger != 0))
            mod_ch->tremolo_tick = 0;

        if (effect_params != 0)
            mod_ch->tremolo_args = effect_params;
    }
    else if (effect == EFFECT_RETRIG_NOTE)
    {
        mod_ch->retrig_tick = 0;
    }
    else if (effect == EFFECT_VIBRATO_WAVEFORM)
    {
        mod_ch->vibrato_retrigger = 1;
        if (effect_params & (1 << 2))
            mod_ch->vibrato_retrigger = 0;

        effect_params &= 3;

        if (effect_params == 0)
            mod_ch->vibrato_wave_table = &vibrato_tremolo_wave_sine[0];
        else if (effect_params == 1)
            mod_ch->vibrato_wave_table = &vibrato_tremolo_wave_ramp[0];
        else if (effect_params == 2)
            mod_ch->vibrato_wave_table = &vibrato_tremolo_wave_square[0];
        else if (effect_params == 3)
            mod_ch->vibrato_wave_table = &vibrato_tremolo_wave_random[0];
    }
    else if (effect == EFFECT_TREMOLO_WAVEFORM)
    {
        mod_ch->tremolo_retrigger = 1;
        if (effect_params & (1 << 2))
            mod_ch->tremolo_retrigger = 0;

        effect_params &= 3;

        if (effect_params == 0)
            mod_ch->tremolo_wave_table = &vibrato_tremolo_wave_sine[0];
        else if (effect_params == 1)
            mod_ch->tremolo_wave_table = &vibrato_tremolo_wave_ramp[0];
        else if (effect_params == 2)
            mod_ch->tremolo_wave_table = &vibrato_tremolo_wave_square[0];
        else if (effect_params == 3)
            mod_ch->tremolo_wave_table = &vibrato_tremolo_wave_random[0];
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

            continue;
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

            int finetune = 0;
            if (ch->instrument_pointer != NULL)
                finetune = ch->instrument_pointer->finetune;

            // TODO: Finetune from effect
            uint64_t period = ModGetSampleTickPeriod(note, finetune);
            MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);

            uint32_t amiga_period = ModNoteToAmigaPeriod(note, 0);
            ch->amiga_period = amiga_period;

            continue;
        }
        else if (ch->effect == EFFECT_FINE_VOLUME_SLIDE)
        {
            if (tick_number == 0)
            {
                int volume = ch->volume + (int8_t)ch->effect_params;

                if (volume > 255)
                    volume = 255;

                if (volume < 0)
                    volume = 0;

                if (volume != ch->volume)
                {
                    ch->volume = volume;
                    MixerChannelSetVolume(handle, volume);
                }
            }

            continue;
        }
        else if (ch->effect == EFFECT_PORTA_UP)
        {
            if (tick_number > 0)
            {
                ch->amiga_period -= (uint8_t)ch->effect_params;
                if (ch->amiga_period < 1)
                    ch->amiga_period = 1;

                uint64_t period;
                period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
            }

            continue;
        }
        else if (ch->effect == EFFECT_FINE_PORTA_UP)
        {
            if (tick_number == 0)
            {
                ch->amiga_period -= (uint8_t)ch->effect_params;
                if (ch->amiga_period < 1)
                    ch->amiga_period = 1;

                uint64_t period;
                period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
            }

            continue;
        }
        else if (ch->effect == EFFECT_PORTA_DOWN)
        {
            if (tick_number > 0)
            {
                ch->amiga_period += (uint8_t)ch->effect_params;

                uint64_t period;
                period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
            }

            continue;
        }
        else if (ch->effect == EFFECT_FINE_PORTA_DOWN)
        {
            if (tick_number == 0)
            {
                ch->amiga_period += (uint8_t)ch->effect_params;

                uint64_t period;
                period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
            }

            continue;
        }
        else if (ch->effect == EFFECT_SAMPLE_OFFSET)
        {
            if (tick_number == 0)
            {
                uint32_t offset = ch->effect_params << 8;

                if (offset == 0)
                {
                    // Reload the last value used. If this effect was used
                    // before, there will be an offset. If not, it will be 0.
                    offset = ch->sample_offset;
                }
                else
                {
                    ch->sample_offset = offset;
                }

                MixerChannelSetSampleOffset(handle, offset);
            }

            continue;
        }
        else if (ch->effect == EFFECT_TREMOLO)
        {
            if (tick_number > 0)
            {
                int speed = ch->tremolo_args >> 4;
                int depth = ch->tremolo_args & 0xF;

                ch->tremolo_tick = (ch->tremolo_tick + speed) & 63;

                int sine = ch->tremolo_wave_table[ch->tremolo_tick];

                // Divide by 64, but multiply by 4 (Volume goes from 0 to 255,
                // but it goes from 0 to 64 in the MOD format).
                int value = (sine * depth) >> (6 - 2);

                int volume = ch->volume + value;

                if (volume < 0)
                    volume = 0;
                else if (volume > 255)
                    volume = 255;

                MixerChannelSetVolume(ch->mixer_channel_handle, volume);
            }

            continue;
        }
        else if (ch->effect == EFFECT_RETRIG_NOTE)
        {
            if (ch->retrig_tick >= ch->effect_params)
            {
                ch->retrig_tick = 0;
                MixerChannelSetSampleOffset(handle, 0);
            }

            ch->retrig_tick++;

            continue;
        }
        else if (ch->effect == EFFECT_DELAY_NOTE)
        {
            if (ch->effect_params == tick_number)
            {
                if (ch->delayed_instrument != NULL)
                    ModChannelSetInstrument(c, ch->delayed_instrument);

                if (ch->delayed_note != -1)
                    ModChannelSetNote(c, ch->delayed_note);

                if (ch->delayed_volume != -1)
                    ModChannelSetVolume(c, ch->delayed_volume);

                ch->effect = EFFECT_NONE;
            }

            continue;
        }

        if ((ch->effect == EFFECT_VIBRATO) ||
            (ch->effect == EFFECT_VIBRATO_VOL_SLIDE))
        {
            if (tick_number > 0)
            {
                int speed = ch->vibrato_args >> 4;
                int depth = ch->vibrato_args & 0xF;

                ch->vibrato_tick = (ch->vibrato_tick + speed) & 63;

                int sine = ch->vibrato_wave_table[ch->vibrato_tick];

                int value = (sine * depth) >> 7; // Divide by 128

                uint64_t period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period + value);
                MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
            }
        }

        if ((ch->effect == EFFECT_VOLUME_SLIDE) ||
            (ch->effect == EFFECT_PORTA_VOL_SLIDE) ||
            (ch->effect == EFFECT_VIBRATO_VOL_SLIDE))
        {
            if (tick_number > 0)
            {
                int volume = ch->volume + (int8_t)ch->effect_params;

                if (volume > 255)
                    volume = 255;

                if (volume < 0)
                    volume = 0;

                if (volume != ch->volume)
                {
                    ch->volume = volume;
                    MixerChannelSetVolume(handle, volume);
                }
            }
        }

        if ((ch->effect == EFFECT_PORTA_TO_NOTE) ||
            (ch->effect == EFFECT_PORTA_VOL_SLIDE))
        {
            if (tick_number != 0)
            {
                int32_t target = ch->porta_to_note_target_amiga_period;

                if (target > ch->amiga_period)
                {
                    ch->amiga_period += ch->porta_to_note_speed;
                    if (target < ch->amiga_period)
                        ch->amiga_period = target;

                    uint64_t period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                    MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
                }
                else if (target < ch->amiga_period)
                {
                    ch->amiga_period -= ch->porta_to_note_speed;
                    if (target > ch->amiga_period)
                        ch->amiga_period = target;

                    uint64_t period = ModGetSampleTickPeriodFromAmigaPeriod(ch->amiga_period);
                    MixerChannelSetNotePeriod(ch->mixer_channel_handle, period);
                }
            }
        }
    }
}
