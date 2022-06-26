
/* SETPAL.C */

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#include "setpal.h"
#include "filepart.h"
#include "prints.h"

extern unsigned char ClearPal[],ScreenPal[],DoomPal[];
extern unsigned char HereticPal[],WinPal,WhichPal;

extern char DateiName1[],DatName1[], DateiName2[];

void CWeg(void)
{
  asm mov ah,2
  asm mov dh,1
  asm mov dl,1
  asm int 10h
}

void SetPal(unsigned int _Seg,unsigned int _Off)
{
  asm mov ax,_Seg
  asm mov es,ax

  asm mov ah,10h
  asm mov al,12h
  asm mov bx,0
  asm mov cx,256

  asm mov dx,_Off
  asm int 10h
}

void TextAusgabe(int x,int y,unsigned char _f, char *str)
{
  int i;
  unsigned char k;
  gotoxy(x,y);
  for (i=0;i<strlen(str);i++) {
	 k=str[i];
    asm mov ah,14
    asm mov al,k
    asm mov bl,_f
    asm int 10h
  }
}

void Mode3(void)
{
  asm mov ah,0
  asm mov al,3
  asm int 10h
}

void SwitchMode(int i)
{
/*  unsigned int _Seg,_Off,k; */
  unsigned int _Seg,_Off;

  if (i==1) {
    asm mov ah,0
    asm mov al,13h
    asm int 10h

    _Seg=FP_SEG(ClearPal);
    _Off=FP_OFF(ClearPal);

    asm mov ah,10h
    asm mov al,12h
    asm mov bx,0
    asm mov cx,256
    asm mov es,_Seg
    asm mov dx,_Off
    asm int 10h
  } else {
    asm mov ah,0
    asm mov al,3
    asm int 10h

    _Seg=FP_SEG(ScreenPal);
    _Off=FP_OFF(ScreenPal);

    asm mov ah,10h
    asm mov al,12h
    asm mov bx,0
    asm mov cx,256
    asm mov es,_Seg
    asm mov dx,_Off
    asm int 10h
  }
}

void DoomPalette(void)
{
  unsigned int _Seg,_Off;
  if (WhichPal==0) {
    _Seg=FP_SEG(DoomPal);
    _Off=FP_OFF(DoomPal);
  } else {
    _Seg=FP_SEG(HereticPal);
    _Off=FP_OFF(HereticPal);
  }

  asm mov ah,10h
  asm mov al,12h
  asm mov bx,0
  asm mov cx,256
  asm mov es,_Seg
  asm mov dx,_Off
  asm int 10h
}

void Move(unsigned int _Seg1,unsigned int _Off1,unsigned int _Seg2,unsigned int _Off2,unsigned int Laenge)
{
  asm   push cx
  asm   push ds
  asm   push es
  asm   push si
  asm   push di
  asm   pushf

  asm   mov cx,_Seg1
  asm   mov ds,cx
  asm   mov cx,_Seg2
  asm   mov es,cx
  asm   cld

  asm   mov si,_Off1
  asm   mov di,_Off2
  asm   mov cx,Laenge
  asm   repnz
  asm   movsw

  asm   popf
  asm   pop di
  asm   pop si
  asm   pop es
  asm   pop ds
  asm   pop cx
}

int Eingabe1(int x,int y,int h,int v)
{
  char Taste;
  int i;

  i=strlen(DateiName1);
  gotoxy(x+i+1,y+1); Taste=getch();
  if (Taste>31 && Taste<123) i=0;
  for (;;) {
    if (Taste>31 && Taste<123 && i<12) { DateiName1[i]=Taste; DateiName1[i+1]=0; Print(x,y,h,v,"            "); Print(x,y,h,v,DateiName1); i++; }
    if (Taste==13) break;
    if (Taste==8 && i>0) { i--; DateiName1[i]=0; Print(x,y,h,v,"            "); Print(x,y,h,v,DateiName1); }
    if (Taste==27) return -1;
    gotoxy(x+1+i,y+1); Taste=getch();
  }
  CWeg();
  if (strchr(DateiName1,42)==NULL) return 0; else {
    i=GetDir(DateiName1);
    if (i==0 && strlen(DatName1)>0) {
		strcpy(DateiName1,DatName1); Print(x,y,h,v,DateiName1); return 0;
    } else return -1;
  }
}

int Eingabe2(int x,int y,int h,int v)
{
  char Taste;
  int i;

  i=strlen(DateiName2);
  gotoxy(x+i+1,y+1); Taste=getch();
  if (Taste>31 && Taste<123) i=0;
  for (;;) {
    if (Taste>31 && Taste<123 && i<12) { DateiName2[i]=Taste; DateiName2[i+1]=0; Print(x,y,h,v,"            "); Print(x,y,h,v,DateiName2); i++; }
    if (Taste==13) break;
    if (Taste==8 && i>0) { i--; DateiName2[i]=0; Print(x,y,h,v,"            "); Print(x,y,h,v,DateiName2); }
    if (Taste==27) return -1;
    gotoxy(x+1+i,y+1); Taste=getch();
  }
  CWeg();
  if (strchr(DateiName2,42)==NULL) return 0; else {
	 i=GetDir(DateiName2);
	 if (i==0 && strlen(DatName1)>0) {
		strcpy(DateiName2,DatName1); Print(x,y,h,v,DateiName2); return 0;
	 } else return -1;
  }
}

void Window(unsigned int Seg,unsigned int Offset,int xo,int yo,int x,int y)
{
  int i,k;
  if (WhichPal==0) k=4; else k=35;
  for (i=1;i<=x;i++) { pokeb(Seg,Offset+xo+i-1+(yo-1)*320,k); pokeb(Seg,Offset+xo+i-1+(yo+y)*320,k); }
  for (i=1;i<=y;i++) { pokeb(Seg,Offset+xo-1+(yo+i-1)*320,k); pokeb(Seg,Offset+xo+x+(yo+i-1)*320,k); }
  if (WinPal==2) {
    for (i=0;i<x;i++)
      for (k=0;k<y;k++) pokeb(Seg,Offset+xo+i+(yo+k)*320,247);
  }
}
