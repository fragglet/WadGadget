/* PRINTS.C */

#include <conio.h>
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prints.h"
#include "setpal.h"
#include "wadview.h"


//extern unsigned char MakeTex,Marking,Texes,EdPName,GetNewPName,SuchStr[],*Puffer;
//extern unsigned char DateiName1[],DateiName2[];
//extern unsigned int B2Seg,B2Off,MarkedEntry[];
//extern FILE *f;
//extern char WinPal,HGR;
//extern int ViewAble,Pos,CPos,NewTx,NewTy,NTCount,Marked;
//extern Entr *Entry,*E;
//extern unsigned long Entries;
//extern long NTKilo;

long DPos;

void Display64(void)
{
  char Word[15];
  int i;
/*  int k,kk; */
  long l;

  Print(27,22,0,15,SuchStr);
  for (i=0;i<22;i++) {
    if (i>=Marked)
      Print(2,i+2,0,15,"                    ");
    else {
      Entry=E; Entry+=MarkedEntry[Pos+i];
      PrintInt(2,i+2,0,15,Pos+i+1);
      memset(Word,32,15);
      strcpy(Word,Entry->RName);
      Word[strlen(Word)]=32; Word[14]=0;
      Print(7,i+2,0,15,Word);
      l=Entry->RLength;
      PrintZahl(16,i+2,0,15,l);
      Entry++;
    }
  }
  PrintInt(2,CPos,7,15,Pos+CPos-1);
  Entry=E; Entry+=MarkedEntry[Pos+CPos-2];
  memset(Word,32,15);
  strcpy(Word,Entry->RName);
  Word[strlen(Word)]=32; Word[14]=0;
  Print(7,CPos,7,15,Word);
  l=Entry->RLength;
  PrintZahl(16,CPos,7,15,l);
  Print(27,20,1,15,"Used patches:"); PrintInt3(40,20,1,15,NTCount);
  Print(53,20,1,15,"Used kb (max 64):"); PrintZahl(70,20,1,15,NTKilo);
  DisplayInfo();
}


void CheckForVile(unsigned char i)
{
  char Dummy[15];

  if (i==1) strcpy(Dummy,DateiName1);
  if (i==2) strcpy(Dummy,DateiName2);
  if (strnicmp("VILE[",Dummy,5)==0) Dummy[4]=49;
  if (strnicmp("VILE\\",Dummy,5)==0) Dummy[4]=50;
  if (strnicmp("VILE]",Dummy,5)==0) Dummy[4]=51;
  if (i==1) strcpy(DateiName1,Dummy);
  if (i==2) strcpy(DateiName2,Dummy);
}


void Box(int h)
{
  int i;
  Print(27,10,5,15," ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
  for (i=0;i<h;i++) {
	 Print(27,11+i,5,15," ³                                             ³ ");
  }
  Print(27,11+h,5,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");
}


void DisplayEndoom(void)
{
  int i;
  long l,l2;

  Mode3();
  l=Entry->RStart; l2=Entry->RLength; fseek(f,l,0);
  for (i=0;i<l2;i++) pokeb(0xb800,i,fgetc(f));
  gotoxy(1,25); getch();
  SwitchMode(0); ScreenAufbau(); Display();
}


void HexDump(void)
{
  long l,lll;
  int i;
  char Taste;

  DPos=0;
  Entry=E; Entry+=(Pos+CPos-2);
  lll=Entry->RLength;
  Print(0,0,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
  Print(0,1,1,15,"³ Hex dump                                                                     ³");
  Print(58,1,1,15,Entry->RName);
  Print(73,1,1,15,"bytes");
  for (i=2;i<24;i++) {
    Print(0,i,1,15,"³ ");
    Print(2,i,0,15,"                                                                            ");
    Print(78,i,1,15," ³");
  }
  Print(0,24,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  Print(3,3,0,15,"Offset  Hex                                              Ascii");
  Print(1,1,1,1," ");
  PrintZahl(66,1,1,15,lll);
  DumpIt(); l=(long)lll/16; if (l*16<lll) l++;
  l*=16;
  for (;;) {
    CWeg();
    Taste=getch();
	 if (Taste==72 && DPos>0) { DPos-=16; DumpIt(); }
	 if (Taste==80 && DPos+320<=l) {
      DPos+=16; if (DPos+304>l) DPos=l-304;
      DumpIt();
    }
	 if (Taste==71 && DPos>0) { DPos=0; DumpIt(); }
	 if (Taste==79 && DPos+320<l) { DPos=l-304; if (DPos<0) DPos=0; DumpIt(); }
	 if (Taste==81 && DPos+320<l) { DPos+=288; if (DPos+304>l) DPos=l-304; if (DPos<0) DPos=0; DumpIt(); }
	 if (Taste==73 && DPos>0) { DPos-=288; if (DPos<0) DPos=0; DumpIt(); }
    if (Taste==27) break;
  }
  ScreenAufbau();
  Display();
}

void DumpIt(void)
{
  int i,k;
  long l,ll,lll,l2;

  ll=Entry->RStart; lll=Entry->RLength;
  ll+=DPos;
  fseek(f,ll,0);
  _read(fileno(f),Puffer,320);
  for (i=0,k=0,l=0;k<304;i++,k++,l++) {
	 if (l+DPos>=lll) break;
	 if (i==0) {
		Print(2,k/16+4,0,15,"                                                                            ");
		l2=(long)l+DPos; PrintZahl(3,k/16+4,0,15,l2);
	 }
	 if (Puffer[k]!=0) {
		PrintHex(i*3+11,k/16+4,0,15,Puffer[k]);
	 } else {
		Print(i*3+11,k/16+4,0,15,"00");
	 }
	 if (Puffer[k]>32) pokeb(0xb800,120+((k/16+4)*160)+i*2,Puffer[k]);
	 if (i==15) i=-1;
  }
}

void Print(int x,int y,char h,char v,char Text[])
{
  int i;
  for (i=0;i<strlen(Text);i++) {
    pokeb(0xb800,(x+i)*2+y*160+1,h*16+v); pokeb(0xb800,(x+i)*2+y*160,Text[i]);
  }
}

void PrintZahl(int x,int y,char h,char v,long Zahl)
{
  char Num[10];
  ltoa(Zahl,Num,10);
  if (Zahl<10) x+=5;
  if (Zahl<100 && Zahl>9) x+=4;
  if (Zahl<1000 && Zahl>99) x+=3;
  if (Zahl<10000 && Zahl>999) x+=2;
  if (Zahl<100000 && Zahl>9999) x+=1;
  Print(x,y,h,v,Num);
}

void PrintInt(int x,int y,char h,char v,int Zahl)
{
  char Num[10];
  itoa(Zahl,Num,10);
  if (Zahl<10) { Print(x,y,h,v,Num); Print(x+1,y,h,v,"    "); }
  if (Zahl>9 && Zahl<100) { Print(x,y,h,v,Num); Print(x+2,y,h,v,"   "); }
  if (Zahl>99 && Zahl<1000) { Print(x,y,h,v,Num); Print(x+3,y,h,v,"  "); }
  if (Zahl>999 && Zahl<10000) { Print(x,y,h,v,Num); Print(x+4,y,h,v," "); }
}

void PrintInt2(int x,int y,char h,char v,int Zahl)
{
  char Num[10];
  itoa(Zahl,Num,10);
  if (Zahl<10) Print(x+2,y,h,v,Num);
  if (Zahl>9 && Zahl<100) Print(x+1,y,h,v,Num);
  if (Zahl>99 && Zahl<1000) Print(x,y,h,v,Num);
}

void PrintInt3(int x,int y,char h,char v,int Zahl)
{
  char Num[10];
  itoa(Zahl,Num,10);
  Print(x,y,h,v,Num);
}

void PrintHex(int x,int y,char h,char v,int Zahl)
{
  int i;
  char Num[3];
/*  char Num2[3]; */
  itoa(Zahl,Num,16);
  if (strlen(Num)==1) { Num[1]=Num[0]; Num[0]=48; Num[2]=0; }
  for (i=0;i<strlen(Num);i++) if (Num[i]>96 && Num[i]<123) Num[i]-=32;
  Print(x,y,h,v,Num);
}

void Error(int i)
{
  char Fehlers[21][45]={
  " ³     Can't show/play this resource!     ³ "," ³          Error reading file!           ³ ",
  " ³          Error writing file!           ³ "," ³   Can't export. No GIF/WAV resource!   ³ ",
  " ³      Error playing sound resource!     ³ "," ³     SoundBlaster base adress error!    ³ ",
  " ³   Can't import. No GIF/WAV resource!   ³ "," ³      Can't find texture-resource!      ³ ",
  " ³ Editing impossible - no pnames marked! ³ "," ³     Can't save. No texture edited!     ³ ",
  " ³Can't add.Reached maximum of 64 patches!³ "," ³   Can't add. Reached maximum of 64k!   ³ ",
  " ³      Can't find PNames resource!       ³ "," ³  AdLib not found. Can't play MUS-data. ³ ",
  " ³    Error writing/opening MUS-data.     ³ "," ³ Error reading instr.data GENMIDI.OP2 ! ³ ",
  " ³ Warning: This resource already exists! ³ "," ³ Can't del/ren/ins! Restore IWad first! ³ ",
  " ³Do not mark more than one unknown entry!³ "," ³             Invalid size!              ³ ",
  " ³    Can't edit. No graphic resource!    ³ "};
  Print(18,10,4,15," ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
  Print(18,11,4,15,Fehlers[i-1]);
  Print(18,12,4,15," ³"); Print(20,12,4,0,"             Hit any key...              ");
  Print(60,12,4,15,"³ ");
  Print(18,13,4,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");
  getch(); ScreenAufbau();
  if (MakeTex==0 && Marking==0 && Texes==0) { Display(); DisplayInfo(); }
}

void TakePic(long lll,int xo,int yo,int wxo,int wyo,int mx,int my,unsigned int Seg,unsigned int Offset)
{
/*
  unsigned long Longs[320],ll,l;
  int x,y,p=0,oldy,q=0;
  unsigned char Bytes[512],a,b,Taste,End=0;
  int i,k;
*/

  unsigned long Longs[320],ll;
  int x,y,p=0,oldy,q=0;
  unsigned char Bytes[512],a,b,End=0;
  int i,k;

  if (ViewAble==2) {
	 if (WinPal==2) Window(0xa000,0,128,68,64,64);
	 fseek(f,lll,0);
	 for (i=0;i<64;i++) {
		_read(fileno(f),&Bytes,64);
      for (k=0;k<64;k++) pokeb(Seg,Offset+xo+k+i*320+yo*320,Bytes[k]);
    }
  } else {
    fseek(f,lll,0);
	 _read(fileno(f),&x,2); _read(fileno(f),&y,2);
	 _read(fileno(f),&i,2); _read(fileno(f),&i,2);
	 for (i=0;i<x;i++) _read(fileno(f),&Longs[i],4);
    if (xo==-1000) {
      xo=(int)(320-x)/2;
      yo=(int)(200-y)/2;
    }
	 oldy=0; if (WinPal==2 && !Texes) Window(0xa000,0,xo,yo,x,y);

    for (k=0;k<x;k++) {
      if (wxo+xo+k>mx) break;
      ll=(long)Longs[k]+lll;
      fseek(f,ll,0);
		_read(fileno(f),&Bytes,500);
      for (p=0;;) {
	b=Bytes[p]; if (b==255) break;
	q=oldy+b; p++; a=Bytes[p];
	p+=2;
	for (i=0;i<a;i++,q++) {
	  if (wyo+q+yo>my) { End=1; break; }
	  if (wxo+xo+k>=xo && wyo+yo+q>=yo) pokeb(Seg,Offset+wxo+xo+k+(wyo+q+yo)*320,Bytes[p]); p++;
	}
	if (End) break;
	p++;
      }
      q=oldy;
    }
  }
}

void PushPic(unsigned int Seg,unsigned int Off,int xo,int yo,int wxo,int wyo,int x,int y,int mx,int my)
{
  int i,k;
  unsigned char kk;
  for (i=0;i<x;i++) {
    if (wxo+xo+i>mx) break;
    for (k=0;k<y;k++) {
      kk=peekb(B2Seg,B2Off+i+k*320);
      if (wyo+k+yo>my) break;
		if (wxo+xo+i>=xo && wyo+yo+k>=yo && kk!=247) pokeb(Seg,Off+wxo+xo+i+(wyo+k+yo)*320,kk);
    }
  }
}

void GetPic(void)
{
/*
  unsigned long Longs[320],ll,l,lll;
  unsigned int x,y,oldy,Seg,Off;
  unsigned char Bytes[512],a,b,Taste;
  int i,k,xo,yo;
*/

  unsigned long l,lll;

  Entry=E; Entry+=(Pos+CPos-2);

  l=Entry->RLength; lll=Entry->RStart;
  SwitchMode(1);

  if (ViewAble==2 && (l==4096 || l==4160) ) {
	 TakePic(lll,128,68,0,0,320,200,0xa000,0);
  } else {
	 TakePic(lll,-1000,-1000,0,0,320,200,0xa000,0);
  }
  DoomPalette();
  getch();
  SwitchMode(0);
  ScreenAufbau(); if (MakeTex==0 && Marking==0 && Texes==0) Display();
}

void ScreenAufbau(void)
{
  int i;
  CWeg();
  Print(0,0,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
  Print(0,1,1,15,"³ Num  Name       Size ³                                                       ³");
  for (i=2;i<24;i++) {
    Print(0,i,1,15,"³ ");
    Print(22,i,1,15," ³                                                       ³");
  }
  Print(0,24,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  Print(28,2,1,11,"ğ NWT - NewWadTool for Doom, Doom 2 & Heretic ğ");
  Print(36,3,1,9, "      v1.3 1/95 by TiC");
  Print(25,5,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
  Print(25,6,1,15,"³                                                   ³");
  for (i=0;i<2;i++) {
    Print(25,7+i,1,15,"³ ");
    Print(27,7+i,0,15,"                                                 ");
    Print(76,7+i,1,15," ³");
  }
  Print(25,9,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  Print(27,6,1,11,"IWAD size: "); PrintZahl(38,6,1,11,(long)filelength(fileno(f)));
  Print(47,6,1,11,"bytes"); PrintInt(72,6,1,11,Entries);
  Print(63,6,1,11,"Entries: ");
  Print(25,11,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
  Print(25,19,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  for (i=12;i<19;i++) { Print(25,i,1,15,"³"); Print(77,i,1,15,"³"); }
  if (MakeTex==0 && Marking==0 && Texes==0 && EdPName==0) {
    Print(42,10,1,15,"Resource-Directory");
    Print(26,12,1,11," F1 - View Hex Dump     RET - Show/play resource   ");
    Print(26,13,1,11," F2 - Export RAW        F6  - Import RAW >PWAD     ");
    Print(26,14,1,11," F3 - Export GIF/WAV    F7  - Import GIF/WAV >IWAD ");
    Print(26,15,1,11," F4 - Export >PWAD      F8  - Import GIF/WAV >PWAD ");
    Print(26,16,1,11," F5 - Import RAW >IWAD  F9  - Import GIF/WAV >RAW  ");
/*    Print(26,18,1, 7," SPACE - Mark resource F10 - Unmark all ESC - Exit "); */
	 Print(26,18,1, 7," SPACE-Mark  F10-Unmark all  INS-Insert '-'-Delete ");
    Print(44,20,1, 7," ALT_E-Edit R.");   Print(62,20,1, 7," ALT_S-Setup");
    Print(44,21,1, 7," ALT_R-Rename R."); Print(62,21,1, 7," ALT_L-Load WAD");
    Print(44,22,1, 7," ALT_T-Textures");  Print(62,22,1, 7," ALT_C-DOS Shell");
    Print(44,23,1, 7," ALT_P-PNames");    Print(62,23,1, 7," ESC  -Exit");
  }
  if (EdPName && !GetNewPName) {
    Print(36,10,1,15,"Viewing/Editing PNames resource");
    Print(26,12,1,11," RET - View resource   SPACE - Mark resource");
    Print(26,13,1,11," F2  - Add pname(s)      '-' - Delete pname(s)");
    Print(26,14,1,11," ESC - Abort             F10 - Save & Exit");
  }
  if (GetNewPName) {
    Print(33,10,1,15,"Resource-Directory, Select new PNames");
    Print(26,12,1,11," RET - View resource     ESC - Done");
    Print(26,14,1,11," SPACE - Mark resource   F10 - Unmark all");
    Print(28,18,1,15,"Mark only graphic resources you want as PNames");
  }
  if (MakeTex) {
    Print(34,10,1,15,"Editing new texture patch [");
    PrintInt(61,10,1,15,NewTx); Print(64,10,1,15,",");
    PrintInt(65,10,1,15,NewTy); Print(68,10,1,15,"]");
    Print(26,12,1,11," RET   - View resource   F2  - View texture       ");
    Print(26,13,1,11," F1    - List patches    F3  - Clear texture      ");
    Print(26,14,1,11," ESC   - Abort editing   F10 - Save & Exit        ");
    Print(26,15,1,11,"                                                  ");
    Print(26,16,1,11," SPACE - Add resource to texture                  ");
  }
  if (Marking && Texes) {
    Print(43,10,1,15,"PNames-Directory");
    Print(26,12,1,11," RET   - View resource");
    Print(26,13,1,11," SPACE - Mark resource      F10 - Unmark all      ");
    Print(26,15,1,11," ESC   - Abort              F2  - Edit Patch");
  }
  if (MakeTex==0) {
    Print(25,20,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
    Print(25,21,1,15,"³"); Print(26,21,1,11," Search string:"); Print(42,21,1,15,"³");
    Print(25,22,1,15,"³ "); Print(27,22,0,7,"              "); Print(41,22,1,15," ³");
    Print(25,23,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  }
  Print(1,1,1,1," ");
}

void Display(void)
{
  char Word[15];
  int i,k;
  long l;

  Print(27,22,0,15,SuchStr);
  Entry=E; Entry+=Pos;
  for (i=0;i<22;i++) {
	 if (i>=Entries)
		Print(2,i+2,0,15,"                    ");
	 else {
		if (Entry->Mark==1) k=1; else k=0;
		if (k==0) PrintInt(2,i+2,0,15,Pos+i+1); else PrintInt(2,i+2,HGR,15,Pos+i+1);
		memset(Word,32,15);
		strcpy(Word,Entry->RName);
		Word[strlen(Word)]=32; Word[14]=0;
		if (k==0) Print(7,i+2,0,15,Word); else Print(7,i+2,HGR,15,Word);
		l=Entry->RLength;
		if (k==0) PrintZahl(16,i+2,0,15,l); else PrintZahl(16,i+2,HGR,15,l);
		Entry++;
	 }
  }
  PrintInt(2,CPos,7,15,Pos+CPos-1);
  Entry=E; Entry+=(Pos+CPos-2);
  memset(Word,32,15);
  strcpy(Word,Entry->RName);
  Word[strlen(Word)]=32; Word[14]=0;
  Print(7,CPos,7,15,Word);
  l=Entry->RLength;
  PrintZahl(16,CPos,7,15,l);
  DisplayInfo();
}
