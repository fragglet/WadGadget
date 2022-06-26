/* MUSPLAY.C */

/* asm .386 */
extern unsigned char MUSisPlaying;
extern unsigned char InstrSindDa;
/*
 *	Name:		MUS File Player
 *	Version:	1.50
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
 *		Some minor changes to improve sound quality. Tried to add
 *		stereo sound capabilities, but failed to -- my SB Pro refuses
 *		to switch to stereo mode.
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo sound fixed. Now works also with Sound Blaster Pro II
 *		(chip OPL3 -- gives 18 "stereo" (ahem) channels).
 *		Changed code to handle properly notes without volume.
 *		(Uses previous volume on given channel.)
 *		Added cyclic channel usage to avoid annoying clicking noise.
 *	Aug-17-1994	V1.30	V.Arnost
 *		Completely rewritten time synchronization. Now the player runs
 *		on IRQ 8 (RTC Clock - 1024 Hz).
 *	Aug-28-1994	V1.40	V.Arnost
 *		Added Adlib and SB Pro II detection.
 *		Fixed bug that caused high part of 32-bit registers (EAX,EBX...)
 *		to be corrupted.
 *	Oct-30-1994	V1.50	V.Arnost
 *		Tidied up the source code
 *		Added C key - invoke COMMAND.COM
 *		Added RTC timer interrupt flag check (0000:04A0)
 *		Added BLASTER environment variable parsing
 *		FIRST PUBLIC RELEASE
 */
#include <alloc.h>
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "adlib.h"
#include "musplay.h"

/*
#define VERSION	   "1.50"
#define COPYRIGHT  "MUS File Player  Version "VERSION"  (c) 1994 QA-Software"
*/

/* Global type declarations */

/*
#ifdef __386__
 typedef unsigned long DWORD;
#else
 typedef unsigned char BYTE;
 typedef unsigned int  WORD;
 typedef unsigned long DWORD;
#endif
*/

/*
#if __BORLANDC__ >= 0x300    BC 3.1: in you want to compile in 386 mode,
#define FASTCALL _fastcall   undef this #define
#else
#define FASTCALL
#endif
*/

#define CHANNELS 16		/* total channels 0..CHANNELS-1 */
#define PERCUSSION 15		/* percussion channel */

#define INSTRSIZE 36		/* instrument size (in bytes) */

struct MUSheader {
	char	ID[4];		/* identifier "MUS" 0x1A */
	WORD	scoreLen;
	WORD	scoreStart;
	WORD	channels;
	WORD	dummy1;
	WORD    instrCnt;
	WORD	dummy2;
/*	WORD	instruments[]; */
} header;

WORD	SBProPort = SBPROPORT;
WORD	channelInstr[CHANNELS];		/* instrument # */
BYTE	channelVolume[CHANNELS];	/* volume */
BYTE	channelLastVolume[CHANNELS];	/* last volume */
signed char channelPan[CHANNELS];	/* pan, 0=normal */
signed char channelPitch[CHANNELS];	/* pitch wheel, 0=normal */
DWORD	Adlibtime[OUTPUTCHANNELS];	/* Adlib channel start time */
WORD	Adlibchannel[OUTPUTCHANNELS];	/* Adlib channel & note # */
BYTE	Adlibnote[OUTPUTCHANNELS];	/* Adlib channel note */
BYTE   *Adlibinstr[OUTPUTCHANNELS];	/* Adlib channel instrument address */
BYTE	Adlibvolume[OUTPUTCHANNELS];	/* Adlib channel volume */
signed char Adlibfinetune[OUTPUTCHANNELS];/* Adlib 2nd channel pitch difference */
#ifdef NOIRQ
DWORD	speed = 6832;
#endif

void interrupt (*oldint70h)(__CPPARGS);
WORD far *timerstack = NULL;
WORD far *timerstackend = NULL;
volatile WORD far *timersavestack = NULL;
volatile DWORD	MUStime;
volatile BYTE  *MUSdata;
volatile DWORD	MUSticks;
volatile WORD	playingAtOnce = 0;
volatile WORD	playingPeak = 0;
volatile WORD	playingChannels = 0;

BYTE   *score;
BYTE   *instruments;

/* Command-line parameters */
char   *musname = NULL;
char   *instrname = "GENMIDI.OP2";
int	help = 0;
int	singlevoice = 0;
int	stereo = 1;
char   *execCmd = NULL;
int	loopForever = 0;

int readMUS(FILE *fm)
{
    if (fread(&header, sizeof(BYTE), sizeof header, fm) != sizeof header)
    {
/*	puts("Unexpected end of file."); */
	return -1;
    }

    if (header.ID[0] != 'M' ||
	header.ID[1] != 'U' ||
	header.ID[2] != 'S' ||
	header.ID[3] != 0x1A)
    {
/*	puts("Bad file."); */
	return -1;
    }

    fseek(fm, header.scoreStart, SEEK_SET);

	 if ( (score = (BYTE *)malloc(header.scoreLen)) == NULL)
    {
/*	puts("Not enough memory."); */
	return -1;
    }

    if (fread(score, sizeof(BYTE), header.scoreLen, fm) != header.scoreLen)
    {
/*	puts("Unexpected end of file."); */
	return -1;
    }

    return 0;
}

int readINS(FILE *fm)
{
    char hdr[8];
    static char masterhdr[8] = {'#','O','P','L','_','I','I','#'};

    if (fread(&hdr, sizeof(BYTE), sizeof hdr, fm) != sizeof hdr)
    {
/*	puts("Unexpected end of instrument file."); */
	return -1;
    }
    if (memcmp(hdr, masterhdr, sizeof hdr))
    {
/*	puts("Bad instrument file."); */
	return -1;
    }
	 if ( (instruments = (BYTE *)calloc(175, INSTRSIZE)) == NULL)
    {
/*	puts("Not enough memory."); */
	return -1;
    }
    if (fread(instruments, INSTRSIZE, 175, fm) != 175)
    {
/*	puts("Unexpected end of instrument file."); */
	return -1;
    }
    return 0;
}

static WORD freqtable[] = {
	172, 183, 194, 205, 217, 230, 244, 258, 274, 290, 307, 326,
	345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,
	690, 731, 774, 820, 869, 921, 975, 1023, 1023, 1023, 1023, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
static char octavetable[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1};

void WriteFrequency(BYTE Adlibchannel, WORD note, int pitch, BYTE keyOn)
{
    WORD freq = freqtable[note];
    WORD octave = octavetable[note];

    if (pitch > 0)
    {
	int freq2;
	int octave2 = octavetable[note+1] - octave;

	if (octave2)
	{
	    octave++;
	    freq >>= 1;
	}
	freq2 = freqtable[note+1] - freq;
	freq += (freq2 * pitch) / 64;
    } else
    if (pitch < 0)
    {
	int freq2;
	int octave2 = octave - octavetable[note-1];

	if (octave2)
	{
	    octave--;
	    freq <<= 1;
	}
	freq2 = freq - freqtable[note-1];
	freq -= (freq2 * -pitch) / 64;
    }
    WriteFreq(Adlibchannel, freq, octave, keyOn);
}

int OccupyChannel(WORD i, WORD channel, BYTE note, int volume, BYTE *instr,
		  WORD flag)
{
    playingChannels++;
    Adlibchannel[i] = (channel << 8) | note | (flag << 15);
    Adlibtime[i] = MUStime;
    if (volume == -1)
	volume = channelLastVolume[channel];
    else
	channelLastVolume[channel] = volume;
    volume = channelVolume[channel] * (Adlibvolume[i] = volume) / 127; /* 127 */
    if (instr[0] & 1)
	note = instr[3];
    else if (channel == PERCUSSION)
	note = 60;			/* C-5 */
    if (flag && (instr[0] & 4))
	Adlibfinetune[i] = instr[2] - 0x80;
    else
	Adlibfinetune[i] = 0;
    if (flag)
	instr += 16+4;
    else
	instr += 4;
    WriteInstrument(i, Adlibinstr[i] = instr);
    WritePan(i, instr, channelPan[channel]);
    WriteVolume(i, instr, volume);
    Adlibnote[i] = note += instr[14] + 12;
    WriteFrequency(i, note, Adlibfinetune[i]+channelPitch[channel], 1);
    return i;
}

int ReleaseChannel(WORD i, WORD killed)
{
    WORD channel = (Adlibchannel[i] >> 8) & 0x7F;
    playingChannels--;
    WriteFrequency(i, Adlibnote[i], Adlibfinetune[i]+channelPitch[channel], 0);
    Adlibchannel[i] = 0xFFFF;
    if (killed)
    {
	WriteChannel(0x80, i, 0x0F, 0x0F);	/* release rate - fastest */
	WriteChannel(0x40, i, 0x3F, 0x3F);	/* no volume */
    }
    return i;
}

int FindFreeChannel(WORD flag)
{
    static WORD last = 0xFFFF;
    WORD i;
    WORD latest = 0xFFFF;
    DWORD latesttime = MUStime;

    for(i = 0; i < AdlibChannels; i++)
    {
	if (++last == AdlibChannels)
	    last = 0;
	if (Adlibchannel[last] == 0xFFFF)
	    return last;
    }

    if (flag & 1)
	return -1;

    for(i = 0; i < AdlibChannels; i++)
    {
	if ((Adlibchannel[i] & 0x8000))
	{
	    ReleaseChannel(i, -1);
	    return i;
	} else
	    if (Adlibtime[i] < latesttime)
	    {
		latesttime = Adlibtime[i];
		latest = i;
	    }
    }

    if ( !(flag & 2) && latest != 0xFFFF)
    {
	ReleaseChannel(latest, -1);
	return latest;
    }
    return -1;
}

/* code 1: play note */
void playNote(WORD channel, BYTE note, int volume)
{
    int i; /* orignote = note; */
    BYTE *instr, instrnumber;

    if (channel == PERCUSSION)
    {
	if (note < 35 || note > 81)
	    return;			/* wrong percussion number */
	instrnumber = (128-35) + note;
    } else
	instrnumber = channelInstr[channel];
    instr = &instruments[instrnumber*INSTRSIZE];
    if ( (i = FindFreeChannel((channel == PERCUSSION) ? 2 : 0)) != -1)
    {
	OccupyChannel(i, channel, note, volume, instr, 0);
	if (!singlevoice && instr[0] == 4)
	{
	    if ( (i = FindFreeChannel((channel == PERCUSSION) ? 3 : 1)) != -1)
		OccupyChannel(i, channel, note, volume, instr, 1);
	}
    }
}

void releaseNote(WORD channel, BYTE note)
{
    WORD i;
    channel = (channel << 8) | note;
    for(i = 0; i < AdlibChannels; i++)
	if ((Adlibchannel[i] & 0x7FFF) == channel)
	{
	    ReleaseChannel(i, 0);
	}
}

void pitchWheel(WORD channel, int pitch)
{
    WORD i;

    channelPitch[channel] = pitch;
    for(i = 0; i < AdlibChannels; i++)
	if (((Adlibchannel[i] >> 8) & 0x7F) == channel)
	{
	    Adlibtime[i] = MUStime;
	    WriteFrequency(i, Adlibnote[i], Adlibfinetune[i]+pitch, 1);
	}
}

void changeControl(WORD channel, BYTE controller, int value)
{
    WORD i;
    switch (controller) {
	case 0:	/* change instrument */
	    channelInstr[channel] = value;
	    break;
	case 3:	/* change volume */
	    channelVolume[channel] = value;
	    for(i = 0; i < AdlibChannels; i++)
		if (((Adlibchannel[i] >> 8) & 0x7F) == channel)
		{
		    Adlibtime[i] = MUStime;
		    WriteVolume(i, Adlibinstr[i], value * Adlibvolume[i] / 127);  /* 127 */
		}
	    break;
	case 4:	/* change pan (balance) */
	    channelPan[channel] = value -= 64;
	    for(i = 0; i < AdlibChannels; i++)
		if (((Adlibchannel[i] >> 8) & 0x7F) == channel)
		{
		    Adlibtime[i] = MUStime;
		    WritePan(i, Adlibinstr[i], value);
		}
	    break;
    }
}

BYTE *playTick(BYTE *data)
{
    for(;;) {
	BYTE command = (*data >> 4) & 7;
	BYTE channel = *data & 0x0F;
	BYTE last = *data & 0x80;
	data++;

	switch (command) {
	    case 0:	/* release note   */
		playingAtOnce--;
		releaseNote(channel, *data++);
		break;
	    case 1: {	/* play note        */
		BYTE note = *data++;
		playingAtOnce++;
		if (playingAtOnce > playingPeak)
		    playingPeak = playingAtOnce;
		if (note & 0x80)	/* note with volume */
		    playNote(channel, note & 0x7F, *data++);
		else
		    playNote(channel, note, -1);
		} break;
	    case 2:	/* pitch wheel */
		pitchWheel(channel, *data++ - 0x80);
		break;
	    case 4:	/* change control */
		changeControl(channel, data[0], data[1]);
		data += 2;
		break;
	    case 6:	/* end */
		return NULL;
	    case 5:	/* ???   */
	    case 7:	/* ???     */
		break;
	    case 3:	/* set tempo ??? -- ignore it */
		data++;
		break;
	}
	if (last)
	    break;
    }
    return data;
}

BYTE *delayTicks(BYTE *data, DWORD *delaytime)
{
    DWORD time = 0;

    do {
	time <<= 7;
	time += *data & 0x7F;
    } while (*data++ & 0x80);

    *delaytime = time;
    return data;
}

/* ######################### Hier war es ############################### */

void playMusic(void)
{
    BYTE keyflags = *(BYTE far *)MK_FP(0x0040, 0x0018);

    playingPeak = playingAtOnce;
    if (!MUSdata) return;
    if (!MUSticks || keyflags & 4)
    {
	MUSdata = playTick((BYTE *)MUSdata);
	if (MUSdata == NULL)
	{
	    if (loopForever)
		MUSdata = score;
	    return;
	}
	MUSdata = delayTicks((BYTE *)MUSdata, &(DWORD)MUSticks);
	MUSticks = (MUSticks * 75) / 10;
/*	MUSticks *= 8; */
	MUStime += MUSticks;
    }
    MUSticks--;
/*    MUStime++; */
}

void CMOSwrite(BYTE reg, BYTE value)
{
   asm mov	al,reg
   asm out	70h,al
   asm jmp	delay1
delay1:	asm jmp	delay2
delay2: asm     mov     al,value
   asm     out	71h,al
}

int CMOSread(BYTE reg)
{
asm	mov	al,reg
asm	out	70h,al
asm	jmp	delay1
delay1:	asm jmp	delay2
delay2:
asm	in	al,71h
asm	xor	ah,ah
    return _AX;
}

void interrupt newint70h_handler(__CPPARGS)
{
    static WORD count = 0;

    if ( ! (CMOSread(0x0C) & 0x40) )
    {
	(*oldint70h)();
	return;
    }

    if (!count)
    {
	count++;
asm	    mov	WORD PTR timersavestack[2],ss
asm	    mov	WORD PTR timersavestack[0],sp
asm	    mov	ss,WORD PTR timerstackend[2]
asm	    mov	sp,WORD PTR timerstackend[0]
asm	    db 66h;
asm         pusha	/* *386* pushad */
	playMusic();
asm	    db 66h;
asm         popa	/* *386* popad */
asm	    nop			/* workaround against 386 POPAD bug */
asm	    mov	ss,WORD PTR timersavestack[2]
asm	    mov	sp,WORD PTR timersavestack[0]
	count--;
    }

asm	mov	al,20h			/* end-of-interrupt */
asm	out	0A0h,al
asm	jmp	delay1
delay1:	asm out	20h,al
}

int SetupTimer(void)
{
    if ( *(BYTE far *)MK_FP(0x0000, 0x04A0) )		/* is the timer busy? */
    	return -1;
	 if ( (timerstack = (WORD *)calloc(0x100, sizeof(WORD))) == NULL)
	return -1;
    *(BYTE far *)MK_FP(0x0000, 0x04A0) = 1;	/* mark the timer as busy */
    timerstackend = &timerstack[0x100];
    oldint70h = getvect(0x70);
    setvect(0x70, newint70h_handler);
    CMOSwrite(0x0B, CMOSread(0x0B) | 0x40);	/* enable periodic interrupt */
asm	in	al,0A1h		/* enable IRQ 8 */
asm	and	al,0FEh
asm	out	0A1h,al
    return 0;
}

int ShutdownTimer(void)
{
    CMOSwrite(0x0B, CMOSread(0x0B) & ~0x40);	/* disable periodic interrupt */
asm	in	al,0A1h		/* disable IRQ 8 */
asm	or	al,1
asm	out	0A1h,al
    setvect(0x70, oldint70h);
    free(timerstack);
    *(BYTE far *)MK_FP(0x0000, 0x04A0) = 0;	/* mark the timer as unused */
    return 0;
}

void playMUS(void)
{
    DWORD lasttime;
    WORD i;

    if (stereo)
	InitAdlib(SBProPort, 1);
    else
	InitAdlib(ADLIBPORT, 0);

    setmem(Adlibchannel, sizeof Adlibchannel, 0xFF);
    for (i = 0; i < CHANNELS; i++)
    {
	channelVolume[i] = 127; 	/* default volume 127 (full volume) */
	channelLastVolume[i] = 100;
    }

    MUSdata = score;
    MUSticks = 0;
    MUStime = 0;
	 lasttime = 0xFFFFFFFFL;

    if (SetupTimer())
    {
/*	printf("FATAL ERROR: Cannot initialize 1024 Hz timer. Aborting.\n"); */
	return;
    }
/*
    if (execCmd)
	system(execCmd);
*/
    for(;;) {
	if (!MUSdata)		/* no more music */
	    break;
	if (bioskey(1))
	{
	    WORD key = bioskey(0);
	    if (key == 0x011B)		/* Esc - exit to DOS */
		break;                  /* Naja, aus der Routine... :-) */
	    else if ((BYTE)key == 32) {
		MUSisPlaying=1; return;
	    }                           /* Im HGR laufen lassen ? */
	}
	if (lasttime != MUStime)
	{
	    DWORD playtime = ((lasttime = MUStime)*1000)/1024;
/*	    cprintf("\rPlaying ... %2ld:%02ld:%03ld  %2d (%2d)", playtime / 60000,
		(playtime / 1000) % 60, playtime % 1000, playingAtOnce, playingChannels); */
	}
    }
    ShutdownTimer();
    DeinitAdlib();
    MUSisPlaying=0;
}

int detectHardware(void)
{
    DetectBlaster(&SBProPort, NULL, NULL, NULL);
    if (!DetectAdlib(ADLIBPORT))
	return -1;
    return DetectSBProII(SBProPort);
}

int dispatchOption(char *option)
{
    switch (*option) {
	case '?':
	case 'H':		/* help */
	    help = 1;
	    break;
	case 'B':		/* instrument bank */
	    instrname = option+1;
	    break;
	case '1':		/* single voice */
	    singlevoice = 1;
	    break;
	case 'M':		/* mono mode    */
	    stereo = 0;
	    break;
	case 'C':		/* command      */
	    execCmd = option+1;
	    break;
	case 'L':		/* loop         */
	    loopForever = 1;
	    break;
	default:
	    return -1;
    }
    return 0;
}

int PlayMUSFile(void)
{
    FILE *fm;

    if (MUSisPlaying) {
      ShutdownTimer();
      DeinitAdlib();
    }

    switch (detectHardware()) {
	case -1:
/*	    puts("Adlib not detected. Exiting."); */
	    return 4;
	case 0:
	    stereo = 0;
/*	    puts("Adlib detected (9 channels mono)"); */
	    break;
	case 1:
/*	    puts("Sound Blaster Pro II detected (18 channels stereo)"); */
	    break;
    }

/*    if ( (fm = fopen(musname, "rb")) == NULL) */
    if ( (fm = fopen("NWT.TMP", "rb")) == NULL)
    {
/*	printf("Can't open file %s\n", musname); */
	return 5;
    } else
/*	printf("Reading file %s ...\n", musname); */
      ;

    if (readMUS(fm))
    {
	fclose(fm);
	return 6;
    }
    fclose(fm);

    if (!InstrSindDa) {
      if ( (fm = fopen(instrname, "rb")) == NULL) return 7;
      if (readINS(fm)) {
	fclose(fm); return 8;
      }
      fclose(fm);
    }
    InstrSindDa=1;

    playMUS();

    if (!MUSisPlaying) {
      free(score); /* free(instruments); */
    }
    return 0;
}
