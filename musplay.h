#ifndef __MUSPLAY_H_
#define __MUSPLAY_H_

#include <stdio.h>
#include "global.h"

int readMUS(FILE *fm);
int readINS(FILE *fm);
void WriteFrequency(BYTE Adlibchannel, WORD note, int pitch, BYTE keyOn);
int OccupyChannel(WORD i, WORD channel, BYTE note, int volume, BYTE *instr,
		  WORD flag);
int ReleaseChannel(WORD i, WORD killed);
int FindFreeChannel(WORD flag);
void playNote(WORD channel, BYTE note, int volume);
void releaseNote(WORD channel, BYTE note);
void pitchWheel(WORD channel, int pitch);
void changeControl(WORD channel, BYTE controller, int value);
BYTE *playTick(BYTE *data);
BYTE *delayTicks(BYTE *data, DWORD *delaytime);
void playMusic(void);
void CMOSwrite(BYTE reg, BYTE value);
int CMOSread(BYTE reg);
void interrupt newint70h_handler(void);
int SetupTimer(void);
int ShutdownTimer(void);
void playMUS(void);
int detectHardware(void);
int dispatchOption(char *option);
int PlayMUSFile(void);

#endif


