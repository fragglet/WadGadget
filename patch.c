
/* PATCH.C */

#include <conio.h>
#include <io.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#include "patches.h"
#include "prints.h"
#include "setpal.h"
#include "wadview.h"

/* Die Texture-Patch-Routinen; gebraucht von WADVIEW */
/*
void Print(int x,int y,char h,char v,char string[]);
void ScreenAufbau(void);
void DisplayPatch(void);
void GetPatch(void);
void Choose64(void);
void PatchData(void);
void PatchSuchen();
*/
void Patches(void)
{
  long L,l,n,ll;
/*  int i,k,pepe,k2,k3,Arg; */
  int i,pepe,k2,k3;
  unsigned char Taste;

  Texes=1;
  Print(18,10,5,15," ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
  Print(18,11,5,15," ³      Reading Texture Resources...      ³ ");
  Print(18,12,5,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");

  Print(27,22,0,15,"          ");
  for (i=0;i<PNames;i++) PName[i].UsedInTex=999;

  l=Entry->RStart;
  fseek(f,l,0); _read(fileno(f),&n,4); TEntries=(int)n;
  for (L=0;L<n;L++) {
    i=(int)L;
    _read(fileno(f),&ll,4);
    TEntry[i].TStart=ll+l;
  }
  for (i=0;i<TEntries;i++) {
    fseek(f,TEntry[i].TStart,0);
    _read(fileno(f),&TEntry[i].TName,8); TEntry[i].TName[8]=0;
    _read(fileno(f),Puffer,12); _read(fileno(f),&pepe,2);
    _read(fileno(f),Puffer,4);
    for (k2=0;k2<pepe;k2++) {
      _read(fileno(f),&k3,2); _read(fileno(f),Puffer,8);
      PName[k3].UsedInTex=i;
	 }
  }
  ScreenAufbau(); DisplayPatch();
  for(;;) {
	 Taste=getch();
    if (Taste==27) break;
    if (Taste==73) { TPos-=21; if (TPos<0) { TPos=0; TCPos=2; } DisplayPatch(); Taste=0; }
	 if (Taste==81) { TPos+=21; if (TPos>n-22) { TPos=(int)n-22; TCPos=23; } DisplayPatch(); Taste=0; }
    if (Taste==72 && TCPos==2 && TPos>0) { TPos--; DisplayPatch(); Taste=0; }
	 if (Taste==80 && TCPos==23) { TPos++; if (TPos>n-22) TPos=(int)n-22; DisplayPatch(); Taste=0; }
    if (Taste==72 && TCPos==2 && TPos==0) Taste=0;
    if (Taste==80) { TCPos++; DisplayPatch(); Taste=0; }
    if (Taste==72) { TCPos--; DisplayPatch(); Taste=0; }
	 if (Taste==79) { TPos=(int)n-22; TCPos=23; DisplayPatch(); }
    if (Taste==71) { TPos=0; TCPos=2; DisplayPatch(); }
	 if (Taste==13) GetPatch();
	 if (Taste==60) Choose64();
	 if (Taste==59) PatchData();
    if ( ((Taste>94 && Taste<123) || (Taste>32 && Taste<58)) && SPos<8) {
      SuchStr[SPos]=Taste; SuchStr[SPos+1]=0;
		Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
		SPos++; if (SPos>0) PatchSuchen();
    }
	 if (Taste==8 && SPos>0) {
      SPos--; SuchStr[SPos]=0;
      Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      if (SPos>0) PatchSuchen();
    }
  }
  SuchStr[0]=0; SPos=0;
  Texes=0; ScreenAufbau(); Display();
}

void PatchSuchen(void)
{
  int i;
  for (i=0;i<TEntries;i++) {
	 if (strnicmp(SuchStr,TEntry[i].TName,SPos)==0) {
		TPos=i-10; TCPos=12; if (TPos<0) { TCPos=i+2; TPos=0; }
		if (TPos>TEntries-22) { TPos=TEntries-22; TCPos=24-(TEntries-i); }
		break;
	 }
  }
  DisplayPatch();
}

void DisplayPatch(void)
{
  char Word[15];
  int i;
/*  long l; */

  Print(2,1,1,15,"Num  Name            ³");
  Print(25,9,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  Print(22,10,1,15," ³               Texture-Patch-Directory                 ³");
  Print(25,11,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
  Print(25,17,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  for (i=12;i<17;i++) { Print(25,i,1,15,"³"); Print(77,i,1,15,"³"); }
  Print(26,12,1,11," RET - View texture       Esc - Abort             ");
  Print(26,13,1,11," F1  - List patches       F2  - Add texture patch ");
  Print(26,14,1,11,"                                                  ");
  Print(26,15,1,11,"                                                  ");
  Print(26,16,1,11,"                                                  ");
  Print(22,18,1,15," ³                                                       ³");
  Print(22,19,1,15," ³                                                       ³");
  Print(25,20,1,15,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿                                   ");
  Print(25,21,1,15,"³"); Print(26,21,1,11," Search string: "); Print(42,21,1,15,"³                                   ");
  Print(25,22,1,15,"³ "); Print(27,22,0,7,"              "); Print(41,22,1,15," ³                                   ");
  Print(25,23,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ                                   ");
  Print(27,22,0,15,SuchStr);
  Print(27,7,0,15,"Type:"); Print(55,7,0,15,"Patches:");
  Print(27,8,0, 7,"Texture data [                                   ");
  Print(1,1,1,1," ");
  for (i=0;i<22;i++) {
	 PrintInt(2,i+2,0,15,TPos+i+1);
	 memset(Word,32,15);
	 strcpy(Word,TEntry[i+TPos].TName);
	 Word[strlen(Word)]=32;
	 Word[14]=0;
	 Print(7,i+2,0,15,Word);
    Print(16,i+2,0,15,"      ");
  }
  PrintInt(2,TCPos,7,15,TPos+TCPos-1);
  memset(Word,32,15); strcpy(Word,TEntry[TPos+TCPos-2].TName);
  Word[strlen(Word)]=32; Word[15]=0; Print(7,TCPos,7,15,Word);
  fseek(f,(long)(TEntry[TPos+TCPos-2].TStart+12),0);
  _read(fileno(f),&Tx,2); _read(fileno(f),&Ty,2);
  Print(27,8,0, 7,"Texture data [                                   ");
  PrintInt2(41,8,0,7,Tx); Print(44,8,0,7,",");
  PrintInt2(45,8,0,7,Ty); Print(48,8,0,7,"]");
  _read(fileno(f),&i,2); _read(fileno(f),&i,2);
  _read(fileno(f),&Tn,2); PrintInt(55,8,0,7,Tn);
}

void PatchData(void)
{
  int xoff,yoff,i,k,x=0,y=0,PNum;

  Print(25,9,1,15,"ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´");
  for (i=10;i<23;i++) {
    Print(25,i,1,15,"³ "); Print(27,i,0,15,"                                                 ");
    Print(76,i,1,15," ³");
  }
  Print(25,23,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
  for (i=0;i<Tn;i++,y++) {
    _read(fileno(f),&xoff,2); _read(fileno(f),&yoff,2); _read(fileno(f),&PNum,2);
    if (y>12) { y=0; x+=10; }
    Print(27+x,10+y,0,15,PName[PNum].Name);
    _read(fileno(f),&k,2); _read(fileno(f),&k,2);
  }
  getch();
  DisplayPatch();
}

void GetPatch(void)
{
  int xoff,yoff,i,k,x,y,PNum;
  long lll,l;

  x=(320-Tx)/2; y=(200-Ty)/2;
  SwitchMode(1);
  DoomPalette();
  if (WinPal==2) Window(0xa000,0,x,y,Tx,Ty);
  for (i=0;i<Tn;i++) {
	 _read(fileno(f),&xoff,2); _read(fileno(f),&yoff,2);
	 _read(fileno(f),&PNum,2); l=ftell(f);
	 Entry=E; Entry+=PName[PNum].Num;
	 lll=Entry->RStart;
	 TakePic(lll,x,y,xoff,yoff,x+Tx-1,y+Ty-1,0xa000,0); fseek(f,l,0);
    _read(fileno(f),&k,2); _read(fileno(f),&k,2);
  }
  getch();
  SwitchMode(0); ScreenAufbau(); DisplayPatch();
}
