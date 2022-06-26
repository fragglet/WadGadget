/*
 *	Name:		Adlib Interface Module
 *	Version:	1.40
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Oct-30-1994
 *	Compiler:	Borland C++ 3.1
 *
 */

/*
 * Revision History:
 *
 *	Aug-8-1994	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-9-1994	V1.10	V.Arnost
 *		Added stereo capabilities
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo capabilities made functional
 *	Aug-24-1994	V1.30	V.Arnost
 *		Added Adlib and SB Pro II detection
 *	Oct-30-1994	V1.40	V.Arnost
 *		Added BLASTER variable parsing
 */

#include <dos.h>
#include <stdlib.h>
#include "adlib.h"

#undef inportb

WORD AdlibPort = ADLIBPORT;
WORD AdlibChannels = ADLIBCHANNELS;
WORD AdlibStereo = 0;

/*
 * Direct write to any Adlib/SB Pro II FM synthetiser register.
 *   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
 *         of reg is zero, data go to port AdlibPort, otherwise to AdlibPort+2
 *   data - register value to be written
 */
BYTE WriteReg(WORD reg, BYTE data)
{
asm	mov	dx,AdlibPort
asm	mov	ax,reg
asm	or	ah,ah
asm	jz	out1
asm	inc	dx
asm	inc	dx
out1:
asm	out	dx,al
asm	mov	cx,6
loop1:
asm	in	al,dx
asm	loop	loop1

asm	inc	dx
asm	mov	al,data
asm	out	dx,al
asm	dec	dx
asm	mov	cx,36
loop2:
asm	in	al,dx
asm	loop	loop2
    return _AL;
}

/*
 * Write to an operator pair. To be used for register bases of 0x20, 0x40,
 * 0x60, 0x80 and 0xE0.
 */
void WriteChannel(BYTE regbase, BYTE channel, BYTE data1, BYTE data2)
{
    static BYTE adlib_op[] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
    static BYTE sbpro_op[] = { 0,  1,  2,   6,  7,  8,  12, 13, 14,
			      18, 19, 20,  24, 25, 26,  30, 31, 32};
    static WORD rg[] = {0x000,0x001,0x002,0x003,0x004,0x005,
			0x008,0x009,0x00A,0x00B,0x00C,0x00D,
			0x010,0x011,0x012,0x013,0x014,0x015,
			0x100,0x101,0x102,0x103,0x104,0x105,
			0x108,0x109,0x10A,0x10B,0x10C,0x10D,
			0x110,0x111,0x112,0x113,0x114,0x115};

    if (AdlibStereo)
    {
	register WORD reg = sbpro_op[channel];
	WriteReg(rg[reg]+regbase, data1);
	WriteReg(rg[reg+3]+regbase, data2);
    } else {
	register WORD reg = regbase+adlib_op[channel];
	WriteReg(reg, data1);
	WriteReg(reg+3, data2);
    }
}

/*
 * Write to channel a single value. To be used for register bases of
 * 0xA0, 0xB0 and 0xC0.
 */
void WriteValue(BYTE regbase, BYTE channel, BYTE value)
{
    static WORD ch[] = {0x000,0x001,0x002,0x003,0x004,0x005,0x006,0x007,0x008,
			0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108};
    register WORD chan;

    if (AdlibStereo)
	chan = ch[channel];
    else
	chan = channel;
    WriteReg(regbase + chan, value);
}

/*
 * Write frequency/octave/keyon data to a channel
 */
void WriteFreq(BYTE channel, WORD freq, BYTE octave, BYTE keyon)
{
    WriteValue(0xA0, channel, (BYTE)freq);
    WriteValue(0xB0, channel, *((BYTE *)&freq+1) | (octave << 2) | (keyon << 5));
}

/*
 * Adjust volume value (register 0x40)
 */
BYTE convertVolume(BYTE data, WORD volume)
{
    static BYTE volumetable[128] = {
	0x00,0x01,0x03,0x05,0x06,0x08,0x0A,0x0B,
	0x0D,0x0E,0x10,0x11,0x13,0x14,0x16,0x17,
	0x19,0x1A,0x1B,0x1D,0x1E,0x20,0x21,0x22,
	0x24,0x25,0x27,0x29,0x2B,0x2D,0x2F,0x31,
	0x32,0x34,0x36,0x37,0x39,0x3B,0x3C,0x3D,
	0x3F,0x40,0x42,0x43,0x44,0x45,0x47,0x48,
	0x49,0x4A,0x4B,0x4C,0x4D,0x4F,0x50,0x51,
	0x52,0x53,0x54,0x54,0x55,0x56,0x57,0x58,
	0x59,0x5A,0x5B,0x5C,0x5C,0x5D,0x5E,0x5F,
	0x60,0x60,0x61,0x62,0x63,0x63,0x64,0x65,
	0x65,0x66,0x67,0x67,0x68,0x69,0x69,0x6A,
	0x6B,0x6B,0x6C,0x6D,0x6D,0x6E,0x6E,0x6F,
	0x70,0x70,0x71,0x71,0x72,0x72,0x73,0x73,
	0x74,0x75,0x75,0x76,0x76,0x77,0x77,0x78,
	0x78,0x79,0x79,0x7A,0x7A,0x7B,0x7B,0x7B,
	0x7C,0x7C,0x7D,0x7D,0x7E,0x7E,0x7F,0x7F};
    WORD n;

    if (volume > 127)
	volume = 127;
    n = 63 - (data & 63);
    n = (n*(int)volumetable[volume]) >> 7;
    n = 63 - n;
    return n | (data & 0xC0);
}

BYTE panVolume(WORD volume, int pan)
{
    if (pan >= 0)
	return volume;
    else
	return (volume * (pan+64)) / 64;
}

/*
 * Write volume data to a channel
 */
void WriteVolume(BYTE channel, BYTE data[16], WORD volume)
{
    WriteChannel(0x40, channel, ((data[6] & 1) ?
	(convertVolume(data[5], volume) | data[4]) : (data[5] | data[4])),
	convertVolume(data[12], volume) | data[11]);
}

/*
 * Write pan (balance) data to a channel
 */
void WritePan(BYTE channel, BYTE data[16], int pan)
{
    BYTE bits;
    if (pan < -36) bits = 0x10;		/* left */
    else if (pan > 36) bits = 0x20;	/* right */
    else bits = 0x30;			/* both */

    WriteValue(0xC0, channel, data[6] | bits);
}

/*
 * Write an instrument to a channel
 *
 * Instrument layout:
 *
 *   Operator1  Operator2  Descr.
 *    data[0]    data[7]   reg. 0x20 - tremolo/vibrato/sustain/KSR/multi
 *    data[1]    data[8]   reg. 0x60 - attack rate/decay rate
 *    data[2]    data[9]   reg. 0x80 - sustain level/release rate
 *    data[3]    data[10]  reg. 0xE0 - waveform select
 *    data[4]    data[11]  reg. 0x40 - key scale level
 *    data[5]    data[12]  reg. 0x40 - output level
 *          data[6]        reg. 0xC0 - feedback/AM-FM (both operators)
 */
void WriteInstrument(BYTE channel, BYTE data[16])
{
/*    WriteChannel(0x80, channel, 0x0F, 0x0F); */
    WriteChannel(0x40, channel, 0x3F, 0x3F);		/* no volume */
    WriteChannel(0x20, channel, data[0], data[7]);
    WriteChannel(0x60, channel, data[1], data[8]);
    WriteChannel(0x80, channel, data[2], data[9]);
    WriteChannel(0xE0, channel, data[3], data[10]);
    WriteValue  (0xC0, channel, data[6] | 0x30);
}

/*
 * Initialize hardware upon startup
 */
void InitAdlib(WORD port, WORD stereo)
{
    WORD i;

    AdlibPort = port;
    if ( (AdlibStereo = stereo) != 0)
    {
	AdlibChannels = SBPROCHANNELS;
	WriteReg(0x105, 0x01);		/* enable YMF262/OPL3 mode */
	WriteReg(0x104, 0x00);		/* disable 4-operator mode   */
	WriteReg(0x01, 0x20);		/* enable Waveform Select      */
	WriteReg(0x08, 0x40);		/* turn off CSW mode             */
	WriteReg(0xBD, 0x00);		/* set vibrato/tremolo depth to low, set melodic mode */
    } else {
	AdlibChannels = ADLIBCHANNELS;
	WriteReg(0x01, 0x20);		/* enable Waveform Select */
	WriteReg(0x08, 0x40);		/* turn off CSW mode        */
	WriteReg(0xBD, 0x00);		/* set vibrato/tremolo depth to low, set melodic mode */
    }
    for(i = 0; i < AdlibChannels; i++)
    {
	WriteChannel(0x40, i, 0x3F, 0x3F);	/* turn off volume */
	WriteValue(0xB0, i, 0);			/* KEY-OFF           */
    }
}

/*
 * Deinitialize hardware before shutdown
 */
void DeinitAdlib(void)
{
    WORD i;

    for(i = 0; i < AdlibChannels; i++)
    {
	WriteChannel(0x40, i, 0x3F, 0x3F);	/* turn off volume */
	WriteValue(0xB0, i, 0);			/* KEY-OFF*/
    }
    if (AdlibStereo)
    {
	WriteReg(0x105, 0x00);		/* disable YMF262/OPL3 mode */
	WriteReg(0x104, 0x00);		/* disable 4-operator mode */
	WriteReg(0x01, 0x20);		/* enable Waveform Select */
	WriteReg(0x08, 0x00);		/* turn off CSW mode */
	WriteReg(0xBD, 0x00);		/* set vibrato/tremolo depth to low, set melodic mode */
    } else {
	WriteReg(0x01, 0x20);		/* enable Waveform Select */
	WriteReg(0x08, 0x00);		/* turn off CSW mode */
	WriteReg(0xBD, 0x00);		/* set vibrato/tremolo depth to low, set melodic mode */
    }
}

/*
 * Write to a mixer register -- SP Pro only
 */
void SetMixer(BYTE index, BYTE data)
{
    if (AdlibStereo) {
asm	mov	dx,AdlibPort
asm	add	dx,4			/* port 224h - Mixer register index */
asm	mov	al,index
asm	out	dx,al
asm	inc	dx			/* port 225h - Mixer data */
asm	mov	al,data
asm	out	dx,al
    }
}

/*
 * Read from a mixer register -- SP Pro only
 */
int GetMixer(BYTE index)
{
    if (AdlibStereo)
    {
asm	mov	dx,AdlibPort
asm	add	dx,4			/* port 224h - Mixer register index */
asm	mov	al,index
asm	out	dx,al
asm	inc	dx			/* port 225h - Mixer data */
asm	in	al,dx
asm	xor	ah,ah
    } else
	_AX = -1;
    return _AX;
}

/*
 * Detect SB Pro mixer
 */
int DetectMixer(WORD port)
{
    WORD origPort = AdlibPort;
    WORD origStereo = AdlibStereo;
    WORD origvolume, volume1, volume2;

    AdlibPort = port;
    AdlibStereo = 1;
    origvolume = GetMixer(0x26);	/* FM Volume */
    SetMixer(0x26, origvolume ^ 0xAA);
    volume1 = GetMixer(0x26) & 0xEE;
    SetMixer(0x26, origvolume ^ 0x44);
    volume2 = GetMixer(0x26) & 0xEE;
    SetMixer(0x26, origvolume);
    AdlibPort = origPort;
    AdlibStereo = origStereo;

    return ((volume1 ^ 0xAA) == (volume2 ^ 0x44));
}

/*
 * Detect Adlib card
 */
int DetectAdlib(WORD port)
{
    WORD origPort = AdlibPort;
    WORD stat1, stat2, i;

    AdlibPort = port;
    WriteReg(0x04, 0x60);
    WriteReg(0x04, 0x80);
    stat1 = inportb(port) & 0xE0;
    WriteReg(0x02, 0xFF);
    WriteReg(0x04, 0x21);
    for (i = 512; --i; )
	inportb(port);
    stat2 = inportb(port) & 0xE0;
    WriteReg(0x04, 0x60);
    WriteReg(0x04, 0x80);
    AdlibPort = origPort;

    return (stat1 == 0 && stat2 == 0xC0);
}

/*
 * Detect Sound Blaster Pro II (with OPL3)
 */
int DetectSBProII(WORD port)
{
    if (!DetectAdlib(port))
	return 0;
    if (!DetectMixer(port))
	return 0;
    if (DetectAdlib(port+2))
	return 0;
    return 1;
}

/*
 * Parse BLASTER environment variable
 */
int DetectBlaster(WORD *port, BYTE *irq, BYTE *dma, BYTE *type)
{
    const char *blaster = getenv("BLASTER");
    if (!blaster) return 0;

    while (*blaster)
    {
	char *endpos = NULL;
	long value;

	switch (*blaster++) {
	    case 'A':	/* base I/O address */
		value = strtol(blaster, &endpos, 16);
		if (port) *port = value;
		break;
	    case 'I':	/* IRQ number */
		value = strtol(blaster, &endpos, 0);
		if (irq) *irq = value;
		break;
	    case 'D':	/* DMA number */
		value = strtol(blaster, &endpos, 0);
		if (dma) *dma = value;
		break;
	    case 'T':	/* card type */
		value = strtol(blaster, &endpos, 0);
		if (type) *type = value;
		break;
	    /* ignore anything else (spaces, ...) */
	}
	if (endpos)
	    blaster = endpos;
    }
    return 1;
}
