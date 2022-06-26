/* GIFPCX.C */

#include <conio.h>
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include "gifpcx.h"
#include "sg.h"
#include "wadview.h"

/* GIF & PCX-Routinen fÅr WADVIEW */

int LadeVGAPcx(unsigned int Seg,unsigned int Off)
{
/*  unsigned char Buffer[1024] */
  unsigned char Faktor,Farbe;
  long DateiLaenge,DateiPos;
  int i,x,y,AnzBytes,MaxBytes=1024,lauf;

  fa=fopen(DatName1,"rb+");
  if (fa==0) return -1;
  DateiLaenge=filelength(fileno(fa));
  i=_read(fileno(fa),&PcxKopf,68); if (i<0) { fclose(fa); return -1; }
  DateiPos=128; fseek(fa,128,0);
  x=0; y=0;
  PcxKopf.x=PcxKopf.r-PcxKopf.l+1;
  PcxKopf.y=PcxKopf.b-PcxKopf.t+1;
  for (;;) {
    if (DateiPos<=DateiLaenge-MaxBytes-769) AnzBytes=MaxBytes; else AnzBytes=(int)DateiLaenge-DateiPos-769;
    if (AnzBytes>0) _read(fileno(fa),Puffer,AnzBytes); DateiPos+=AnzBytes;
    if (AnzBytes>0) {
      for (lauf=0;lauf<AnzBytes;) {
	if (Puffer[lauf]>=193) {
	  if (lauf<AnzBytes-1) {
	    Faktor=Puffer[lauf]-192;
	    Farbe=Puffer[lauf+1];
	    lauf+=2;
	  } else {
	    Faktor=Puffer[lauf]-192;
	    if (DateiPos<=DateiLaenge-MaxBytes-769) AnzBytes=MaxBytes; else AnzBytes=DateiLaenge-DateiPos-769;
	    if (AnzBytes>0) _read(fileno(fa),Puffer,AnzBytes); DateiPos+=AnzBytes;
	    if (AnzBytes>0) {
	      Farbe=Puffer[0]; lauf=1;
	    }
	  }
	} else {
	  Faktor=1; Farbe=Puffer[lauf]; lauf++;
	}
	if (AnzBytes>0) {
	  for (i=0;i<Faktor;i++) {
	    pokeb(Seg,Off+x+y*320,Farbe);
	    if (x==PcxKopf.x-1) {
	      x=0; y++;
	    } else x++;
	  }
	}
      }
    }
    if (DateiPos>=DateiLaenge-769) break;
  }
  fclose(fa);
  return 0;
}

void SaveGif(int xx,int yy)
{
  int i;
  for (i=0;i<768;i++) ClearPal[i]=(DoomPal[i]<<2)+3;
  create_gif(xx,yy,256,ClearPal,0,DateiName1);
  put_image(xx,yy,0,0,0,0,ClearPal);
  close_gif();
  memset(ClearPal,0,768);
}

void SavePcx(int xx,int yy,unsigned int Seg,unsigned int Off)
{
/*  unsigned char Buffer[2048]; */
  int lauf=0,x=0,y=0,AnzBytes,i;
  unsigned char fak,farbe,aktfarbe;

  Pcxkopf.Kennung=10; Pcxkopf.Version=5;
  Pcxkopf.Kodierung=1; Pcxkopf.BpP=8; Pcxkopf.a=1; Pcxkopf.BpZ=xx;
  Pcxkopf.x1=0; Pcxkopf.x2=xx-1; Pcxkopf.y1=0; Pcxkopf.y2=yy-1;
  Pcxkopf.SizeX=xx; Pcxkopf.SizeY=yy;
  fa=fopen(DateiName1,"wb+");

  _write(fileno(fa),&Pcxkopf,sizeof(Pcxkopf));
  gotoxy(1,1);
  for (y=0;y<yy;y++) {
    lauf=0; fak=0;
    farbe=peekb(Seg,Off+1+y*320);
    for (x=0;x<xx;x++) {
      aktfarbe=peekb(Seg,Off+x+y*320);
      if (fak==63 || farbe!=aktfarbe || x==xx-1) {
	if (fak>1 || (fak==1 && farbe>=192)) {
	  Puffer[lauf]=fak+192; Puffer[lauf+1]=farbe; lauf+=2;
	} else {
	  if (fak==1 && farbe<192) {
	    Puffer[lauf]=farbe; lauf++;
	  }
	}
	fak=1; farbe=aktfarbe;
      } else fak++;
    }
    if (fak>1 || (fak==1 && farbe>=192)) {
      Puffer[lauf]=fak+192; Puffer[lauf+1]=farbe; lauf+=2;
    } else {
      if (fak==1 && farbe<192) {
	Puffer[lauf]=farbe; lauf++;
      }
    }
    AnzBytes=lauf;
    _write(fileno(fa),Puffer,AnzBytes);
  }

  Puffer[0]=12; lauf=0;
  for (i=0;i<256;i++) {
    Puffer[lauf+1]=(DoomPal[lauf]<<2)+3;
    Puffer[lauf+2]=(DoomPal[lauf+1]<<2)+3;
    Puffer[lauf+3]=(DoomPal[lauf+2]<<2)+3;
    lauf+=3;
  }
  _write(fileno(fa),Puffer,769);
  fflush(fa); fclose(fa);
}

int ReadGif(void)
{
  int i;
  unsigned char a;

  gifx=0; gify=0;
  ff=fopen(DatName1,"rb");
  if (ff==0) return -1;
  _read(fileno(ff),&gifkopf,sizeof(gifkopf));
  if (gifkopf.byte!=44) for (;;) { _read(fileno(ff),&a,1); if (a==44) break; }
  _read(fileno(ff),&gifkopf2,sizeof(gifkopf2));
  i=decoder(gifkopf2.gx); fclose(ff);
  return(i);
}

int get_byte(void)
{
  return(fgetc(ff));
}

int out_line(unsigned char pixels[],int linelen)
{
  int i;
  for (i=0;i<linelen;i++) {
    pokeb(B1Seg,B1Off+gifx+gify*320,pixels[i]);
    gifx++;
    if (gifx+1>gifkopf2.gx) { gifx=0; gify++; }
    if (gify+1>gifkopf2.gy) break;
  }
  return(0);
}

int curr_size;
int clear;
int ending;
int newcodes;
int top_slot;
int slot;
int navail_bytes = 0;
int nbits_left = 0;
unsigned char b1;
unsigned char byte_buff[257];
unsigned char *pbytes;

long code_mask[13] = { 0,0x0001, 0x0003,0x0007, 0x000F,0x001F, 0x003F,0x007F, 0x00FF,0x01FF, 0x03FF,0x07FF, 0x0FFF };
unsigned char stack[MAX_CODES + 1];
unsigned char suffix[MAX_CODES + 1];
unsigned int  prefix[MAX_CODES + 1];

int init_exp(int size)
{
  curr_size = size + 1;
  top_slot = 1 << curr_size;
  clear = 1 << size;
  ending = clear + 1;
  slot = newcodes = ending + 1;
  navail_bytes = nbits_left = 0;
  return(0);
}

int get_next_code(void)
{
  int i, x;
  unsigned long ret;

  if (nbits_left == 0) {
	 if (navail_bytes <= 0) {
		pbytes = byte_buff;
	if ((navail_bytes = get_byte()) < 0) {
	  return(navail_bytes);
	} else {
	  if (navail_bytes) {
		 for (i = 0; i < navail_bytes; ++i) {
			if ((x = get_byte()) < 0) return(x);
			byte_buff[i] = x;
		 }
	  }
	}
	 }
	 b1 = *pbytes++; nbits_left = 8; --navail_bytes;
  }
  ret = b1 >> (8 - nbits_left);
  while (curr_size > nbits_left) {
	 if (navail_bytes <= 0) {
		pbytes = byte_buff;
		if ((navail_bytes = get_byte()) < 0) {
	return(navail_bytes);
		} else {
	if (navail_bytes) {
	  for (i = 0; i < navail_bytes; ++i) {
	    if ((x = get_byte()) < 0) return(x);
	      byte_buff[i] = x;
	  }
	}
		}
	 }
	 b1 = *pbytes++; ret |= b1 << nbits_left; nbits_left += 8; --navail_bytes;
  }
  nbits_left -= curr_size; ret &= code_mask[curr_size];
  return((int)(ret));
}

int decoder(int linewidth)
{
  register unsigned char *sp, *bufptr;
  unsigned char *buf;
  register int code, fc, oc, bufcnt;
  int c, size, ret;

  if ((size = get_byte()) < 0) return(size);
  if (size < 2 || 9 < size) return(BAD_CODE_SIZE);
  init_exp(size);

  oc = fc = 0;

  if ((buf = (unsigned char *)malloc(linewidth + 1)) == NULL) return(OUT_OF_MEMORY);
  sp = stack; bufptr = buf; bufcnt = linewidth;

  while ((c = get_next_code()) != ending) {
    if (c < 0) {
      free(buf); return(0);
	 }
    if (c == clear) {
      curr_size = size + 1;
      slot = newcodes;
      top_slot = 1 << curr_size;
      while ((c = get_next_code()) == clear) ;
      if (c == ending) break;
      if (c >= slot) c = 0;
      oc = fc = c;
      *bufptr++ = c;
      if (--bufcnt == 0) {
	if ((ret = out_line(buf, linewidth)) < 0) {
	  free(buf);
	  return(ret);
	}
	bufptr = buf;
	bufcnt = linewidth;
      }
    } else {
      code = c;
      if (code >= slot) {
	if (code > slot) ++bad_code_count;
	code = oc; *sp++ = fc;
      }
      while (code >= newcodes) {
	*sp++ = suffix[code];
	code = prefix[code];
      }
      *sp++ = code;
      if (slot < top_slot) {
	suffix[slot] = fc = code;
	prefix[slot++] = oc;
	oc = c;
      }
      if (slot >= top_slot)
	if (curr_size < 12) {
	  top_slot <<= 1;
	  ++curr_size;
	}
      while (sp > stack) {
	*bufptr++ = *(--sp);
	if (--bufcnt == 0) {
	  if ((ret = out_line(buf, linewidth)) < 0) {
	    free(buf);
	    return(ret);
	  }
	  bufptr = buf;
	  bufcnt = linewidth;
	}
      }
    }
  }
  ret = 0;
  if (bufcnt != linewidth) ret = out_line(buf, (linewidth - bufcnt));
  free(buf);
  return(ret);
}

