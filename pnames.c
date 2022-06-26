/* PNAMES.C */

#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "pnames.h"
#include "prints.h"
#include "setpal.h"
#include "texture.h"
#include "wadview.h"

void EditPNames(void)
{
  int i,k;
  long l;

  EdPName=1; Pos=0; CPos=2; Marking=1;
  ScreenAufbau(); DisplayPNames(); ListPNames();
  Marking=0; EdPName=0; Pos=AltPos; CPos=AltCPos;
  Entry=E;
  for (i=0,l=0;l<Entries;l++) {
    if (strnicmp("PNAMES",Entry->RName,6)==0) { i=1; break; }
    Entry++;
  }
  if (i) {
    l=Entry->RStart; fseek(f,l,0);
    _read(fileno(f),&k,2); PNames=k; _read(fileno(f),&i,2);
    for (i=0;i<k;i++) {
      _read(fileno(f),&PName[i].Name,8);
      PName[i].Name[8]=0;
    }
    for (i=0;i<PNames;i++) {
      Entry=E+Entries-1;
      for (l=Entries-1,k=Entries-1;l>=0;l--,k--) {
	if (stricmp(PName[i].Name,Entry->RName)==0) { PName[i].Num=k; break; }
	Entry--;
      }
    }
  }
  ScreenAufbau(); Display();
}

void ListPNames(void)
{
  int i,k,AltPos2,AltCPos2;
  unsigned char Taste=0,Ente=0;

  for(;;) {
    Taste=getch();
    if (Taste==27) { Marked=0; break; }
    if (Taste==60) {                    /* Go edit Texture, nur wenn... */
      if (!EdPName) {
	if (!Marked) { Error(9); DisplayPNames(); } else break;
      } else {
	DeleteMarked();              /* Neue PNames markieren ! */
	Print(27,10,5,15," ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
	Print(27,11,5,15," ³   Now mark all graphic resources you want   ³ ");
	Print(27,12,5,15," ³       as new PNames. Graphics ONLY!         ³ ");
	Print(27,13,5,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");
	getch();
	AltPos2=Pos; AltCPos2=CPos; Pos=0; CPos=2; GetNewPName=1;
	ScreenAufbau(); Display(); MainSchleife();
	if (Marked>0) {
	  Entry=E;
	  for (k=0,i=0;i<Entries;i++) {
	    if (Entry->Mark==1) {
	      strcpy(PName[PNames+k].Name,Entry->RName);
	      PName[PNames+k].Num=i; k++;
	    }
	    Entry++;
	  }
	  PNames+=Marked;
	}
	DeleteMarked();
	Pos=AltPos2; CPos=AltCPos2; GetNewPName=0; ScreenAufbau(); DisplayPNames();
      }
	 }
    if (Taste==45 && EdPName) {         /* F3 - Delete PName(s) */
      if (Marked>0) {
	for (i=0;i<PNames;i++) {
	  Entry=E+PName[i].Num;
	  if (Entry->Mark==1) PName[i].Num=9999;
	}
      } else {
	PName[Pos+CPos-2].Num=9999;
      }
      DeletePNames();
      if (Pos>PNames-22) Pos=PNames-22;
      if (Pos<0) { Pos=0; CPos=2; }
      if (Pos+CPos-2>=PNames) CPos=PNames-Pos+2;
      if (CPos-2>=PNames) CPos=PNames+1;
      DisplayPNames(); Taste=0;
      Taste=0; DisplayPNames();
    }
    if (Taste==68 && EdPName) {         /* F10 - Save PName-Resource */
      Print(27,10,5,15," Ú Save PNames Data (WAD/RAW) ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
      Print(27,11,5,15," ³  Outfile:                                   ³ ");
      Print(27,12,5,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");
      strcpy(DateiName2,"PNAMES.RAW"); Print(39,11,5,0,DateiName2);
      for (;;) {
	if (Eingabe2(39,11,5,0)<0) break; else CWeg();
	if (!strchr(DateiName2,'.')) strcat(DateiName2,".RAW");
	if (SavePNameRes()) { Taste=0; break; }
      }
      ScreenAufbau(); DisplayPNames();
      if (Taste==0) break;
    }
    if (Taste==73) { Pos-=21; if (Pos<0) { Pos=0; CPos=2; } DisplayPNames(); Taste=0; }
    if (Taste==81) {
      Pos+=21; if (Pos>PNames) Pos-=21;
		if (Pos>PNames-22) { Pos=PNames-22; CPos=23; }
      if (Pos<0) { Pos=0; CPos=2; }
      DisplayPNames(); Taste=0;
    }
    if (Taste==72 && CPos==2 && Pos>0) { Pos--; DisplayPNames(); Taste=0; }
    if (Taste==80 && CPos==23) { Pos++; if (Pos>PNames-22) Pos=PNames-22; DisplayPNames(); Taste=0; }
    if (Taste==72 && CPos==2 && Pos==0) Taste=0;
    if (Taste==80 && Pos+CPos<PNames+1) { CPos++; DisplayPNames(); Taste=0; }
    if (Taste==72) { CPos--; DisplayPNames(); Taste=0; }
    if (Taste==79) { Pos=PNames-22; CPos=23; if (Pos<0) { Pos=0; CPos=PNames+1; } DisplayPNames(); }
    if (Taste==71) { Pos=0; CPos=2; DisplayPNames(); }

    if (Taste==13) {
      i=Pos; k=CPos; Pos=PName[Pos+CPos-2].Num-CPos+2;
      GetPic(); Pos=i; CPos=k; DisplayPNames();
    }
    if ( ((Taste>94 && Taste<123) || (Taste>32 && Taste<58)) && SPos<8) {
      SuchStr[SPos]=Taste; SuchStr[SPos+1]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      SPos++; if (SPos>0) PNameSuchen();
    }
    if (Taste==8 && SPos>0) {
      SPos--; SuchStr[SPos]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      if (SPos>0) PNameSuchen();
    }
    if (Taste==32 && Marking==1 && Entry->RLength!=0) {
      Ente=0;
      if (Entry->Mark==1) { Ente=1; Entry->Mark=0; Marked--; }
      if (!Ente && Entry->Mark==0 && Marked<64) { Entry->Mark=1; Marked++; }
      if (Pos+CPos-1<=PNames && CPos<24) CPos++;
      if (CPos==24) { Pos++; CPos=23; } if (Pos>PNames-22) Pos=PNames-22;
      DisplayPNames();
    }
    if (Taste==68 && Marking==1 && !EdPName) {
      DeleteMarked(); DisplayPNames();
	 }
  }
  SuchStr[0]=0; SPos=0;
}

void DeleteMarked(void)
{
  int i;
  Entry=E; Marked=0;
  for (i=0;i<Entries;i++) {
	 if (Entry->Mark==1) Entry->Mark=0;
	 Entry++;
  }
}

void DeletePNames(void)
{
  int i,k;

  for (i=0;i<PNames;i++) {
    if (PName[i].Num==9999) {
      for (k=i;k<PNames-1;k++) {
	strcpy(PName[k].Name,PName[k+1].Name); PName[k].Num=PName[k+1].Num; PName[k].UsedInTex=PName[k+1].UsedInTex;
      }
      i--; PNames--;
    }
  }
}

void DisplayPNames(void)
{
  char Word[15];
  int i,k;
/*  int kk; */
  long l;

  Print(27,22,0,15,SuchStr);
  for (i=0;i<22;i++) {
    if (i>=PNames)
      Print(2,i+2,0,15,"                    ");
    else {
      Entry=E; Entry+=PName[Pos+i].Num;
      if (Entry->Mark==1) k=1; else k=0;
      if (k==0) PrintInt(2,i+2,0,15,Pos+i+1); else PrintInt(2,i+2,HGR,15,Pos+i+1);
      memset(Word,32,15);
      strcpy(Word,Entry->RName);
      Word[strlen(Word)]=32; Word[14]=0;
      if (k==0) Print(7,i+2,0,15,Word); else Print(7,i+2,HGR,15,Word);
      l=Entry->RLength;
      if (k==0) PrintZahl(16,i+2,0,15,l); else PrintZahl(16,i+2,HGR,15,l);
    }
  }
  PrintInt(2,CPos,7,15,Pos+CPos-1);
  Entry=E; Entry+=PName[Pos+CPos-2].Num;
  memset(Word,32,15);
  strcpy(Word,Entry->RName);
  Word[strlen(Word)]=32; Word[14]=0;
  Print(7,CPos,7,15,Word);
  l=Entry->RLength;
  PrintZahl(16,CPos,7,15,l);
  DisplayInfo();
  i=PName[Pos+CPos-2].UsedInTex; if (i<999) Print(55,8,0,7,TEntry[PName[Pos+CPos-2].UsedInTex].TName);
}
