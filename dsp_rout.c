/* DSP_ROUT.C */

#include <conio.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>

#include "dsp_rout.h"

#undef inportb
#undef outportb


unsigned int BASE=0x220;        /* I/O-Basisadresse                              */


byte far *zgr;
unsigned int counter,fehler=0;

byte lowbyte(word adresse)
{
   return(adresse & 255);
}

byte highbyte(word adresse)
{
   return((adresse>>8) & 255);
}

byte lies_dsp(void)
{
   int i,error=JA;

   for(i=0;i<10000;i++)
   {  if(inportb((int)BASE+0x0E) & 128)
      {  error = NEIN;
	 break;
      }
   }
   if (error) fehler=1;
   return(inportb(BASE+0x0A));
}

void schreib_dsp(byte wert)
{
   int i,error=JA;

   for(i=0;i<10000;i++)
   {  if(!(inportb(BASE+0x0C) & 128))
      {  error = NEIN;
	 break;
      }
   }
   if(error) fehler=1;
   outportb(BASE+0x0C,wert);
}

void init_dsp(void)
{
   int i,error=JA;

   outportb(BASE+0x06,1);
   delay(1);
   outportb(BASE+0x06,0);
   for(i=0;i<100;i++)
   {  if(inportb(BASE+0x0A) == 0xAA)
      {  error = NEIN;
		 break;
	  }
   }
   if (error) fehler=2;
}

void interrupt neu_int8(__CPPARGS)
{
   byte wert=0;

   wert = *zgr++;
   schreib_dsp(0x10);
   schreib_dsp(wert);
   counter++;
   outportb(0x20,0x20);
}

int direkt_soundausgabe(void far *anf_adr,unsigned int Laenge,unsigned int freq)
{

	void interrupt (*alt_int8) (__CPPARGS);
   word timer_start;
/*   char c=0;  */

   init_dsp();
   schreib_dsp(0x0D1);                   /* Speaker an                  */
   alt_int8 = getvect(0x08);
   timer_start = 0x00077;                /* Sample-Freq. = 10 KHz       */
   timer_start = freq;
   outportb(0x40, lowbyte(timer_start));
   outportb(0x40,highbyte(timer_start));
   setvect(0x08,neu_int8);
   zgr = (byte far *)anf_adr; counter=0;
   if (fehler) {
     timer_start = 0x0FFFF;      /* Timer auf Originalwert      */
     outportb(0x40, lowbyte(timer_start));
     outportb(0x40,highbyte(timer_start));
     setvect(0x08,alt_int8);
     return(fehler);
   }
   for (;;) {
     if (kbhit()) { getch(); break; }
     if (counter>=Laenge) break;
   }
   timer_start = 0x0FFFF;                /* Timer auf Originalwert      */
   outportb(0x40, lowbyte(timer_start));
   outportb(0x40,highbyte(timer_start));
   setvect(0x08,alt_int8);
   schreib_dsp(0x0D3);                   /* Speaker aus                 */
   if (fehler) { return(fehler); }
   return 0;
}
