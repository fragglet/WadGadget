/*
 *	Name:		Adlib interface module -- Header Include File
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

#ifndef __ADLIB_H_
#define __ADLIB_H_


#include "global.h"

#define ADLIBPORT 0x388
#define SBPORT 0x228
#define SBPROPORT 0x220
#define ADLIBCHANNELS  9
#define SBPROCHANNELS  18
#define OUTPUTCHANNELS 18

/* Global type declarations */


extern WORD AdlibPort;
extern WORD AdlibChannels;
extern WORD AdlibStereo;

#ifdef __cplusplus
  extern "C" {
#endif

BYTE WriteReg(WORD reg, BYTE data);
void WriteChannel(BYTE regbase, BYTE channel, BYTE data1, BYTE data2);
void WriteValue(BYTE regbase, BYTE channel, BYTE value);
BYTE convertVolume(BYTE data, WORD volume);
BYTE panVolume(WORD volume, int pan);
void WriteFreq(BYTE channel, WORD freq, BYTE octave, BYTE keyon);
void WriteVolume(BYTE channel, BYTE data[16], WORD volume);
void WritePan(BYTE channel, BYTE data[16], int pan);
void WriteInstrument(BYTE channel, BYTE data[16]);
void InitAdlib(WORD port, WORD stereo);
void DeinitAdlib(void);
void SetMixer(BYTE index, BYTE data);
int  GetMixer(BYTE index);
int  DetectMixer(WORD port);
int  DetectAdlib(WORD port);
int  DetectSBProII(WORD port);
int  DetectBlaster(WORD *port, BYTE *irq, BYTE *dma, BYTE *type);

#ifdef __cplusplus
  }
#endif

#endif // __ADLIB_H_
