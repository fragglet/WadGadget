
#ifndef __DSP_ROUT_H_
#define __DSP_ROUT_H_


#include "global.h"

typedef unsigned char byte;
typedef unsigned int  word;

#define JA   1
#define NEIN 0


extern unsigned int BASE;        /* I/O-Basisadresse                              */

BYTE lowbyte(WORD adresse);
BYTE highbyte(WORD adresse);
BYTE lies_dsp(void);
void schreib_dsp(BYTE wert);
void init_dsp(void);
void interrupt neu_int8(__CPPARGS);
int direkt_soundausgabe(void far *anf_adr,unsigned int Laenge,unsigned int freq);


#endif