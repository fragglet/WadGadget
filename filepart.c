#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

#include "filepart.h"
#include "prints.h"
#include "setpal.h"

extern Datdings *Datai,*Da;
extern int Dateien,AltPos,Pos,AltCPos,CPos,SPos;
extern char WildCard[],DatName1[],SuchStr[],Sort;


int SortIt(const void *D1, const void *D2)
{
  if (Sort==1)
	 return(strcmp(((Datdings *)D1)->FileName,((Datdings *)D2)->FileName));
  if (Sort==2)
	 return( (((Datdings *)D1)->Date<((Datdings *)D2)->Date) ? -1:1 );
  return 0;
}



int GetDir(char *Wild)
{
  struct ffblk fblock;
  int letzter;
/*  int k; */
  Dateien=0; Datai=Da;

  strcpy(WildCard,Wild);
  letzter=findfirst("*.*",&fblock,16);
  while (!letzter || Dateien>=500) {
    if (fblock.ff_attrib==16) {
      if (fblock.ff_name[0]==46 && strlen(fblock.ff_name)==1) {
	;
      } else {
	strcpy(Datai->FileName,fblock.ff_name);
	Datai->Date=0; Dateien++; Datai++;
      }
    }
    letzter=findnext(&fblock);
  }
  letzter=findfirst(Wild,&fblock,0);
  while (!letzter || Dateien>=500) {
	 strcpy(Datai->FileName,fblock.ff_name);
    Datai->Date=(long)fblock.ff_fdate*100000; Datai->Date+=(long)fblock.ff_ftime;
    Dateien++; Datai++;
    letzter=findnext(&fblock);
  }
  Datai=Da; CWeg();
  if (Dateien==0) return -1;
  qsort(Datai,Dateien,sizeof(Datdings),SortIt); ListFiles();
  return 0;
}



void ListFiles(void)
{
/*  int i,k; */
  unsigned char Taste=0;
  AltPos=Pos; AltCPos=CPos;
  Pos=0; CPos=2; DisplayFiles();
  for(;;) {
    Taste=getch();
    if (Taste==27) { DatName1[0]=0; break; }
    if (Taste==73) { Pos-=21; if (Pos<0) { Pos=0; CPos=2; } DisplayFiles(); Taste=0; }
    if (Taste==81) { Pos+=21; if (Pos>Dateien-22) { Pos=Dateien-22; CPos=23; if (Pos<0) { Pos=0; CPos=Dateien+1; } } DisplayFiles(); Taste=0; }
    if (Taste==72 && CPos==2 && Pos>0) { Pos--; DisplayFiles(); Taste=0; }
    if (Taste==80 && CPos==23) { Pos++; if (Pos>Dateien-22) Pos=Dateien-22; DisplayFiles(); Taste=0; }
    if (Taste==72 && CPos==2 && Pos==0) Taste=0;
    if (Taste==80 && CPos<Dateien+1) { CPos++; DisplayFiles(); Taste=0; }
    if (Taste==72) { CPos--; DisplayFiles(); Taste=0; }
    if (Taste==79) { Pos=Dateien-22; CPos=23; if (Pos<0) { Pos=0; CPos=Dateien+1; } DisplayFiles(); }
    if (Taste==71) { Pos=0; CPos=2; DisplayFiles(); }
    if ( ((Taste>94 && Taste<123) || (Taste>32 && Taste<58)) && SPos<8) {
      SuchStr[SPos]=Taste; SuchStr[SPos+1]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      SPos++; if (SPos>0) DateiSuchen();
    }
    if (Taste==8 && SPos>0) {
      SPos--; SuchStr[SPos]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      if (SPos>0) DateiSuchen();
    }
	 if (Taste==13) { strcpy(DatName1,Datai->FileName); break; }
  }
  if (Datai->Date==0 && strlen(DatName1)>0) { chdir(Datai->FileName); GetDir(WildCard); }
  Pos=AltPos; CPos=AltCPos; Dateien=0;
  DisplayFiles(); SuchStr[0]=0; SPos=0; CWeg();
}



void DisplayFiles(void)
{
  char Word[17];
/*  long l; */
  int i;

  Print(27,22,0,15,SuchStr); Print(18,1,1,1,"     ");

  if (Dateien==0) {
	 for (i=0;i<22;i++) Print(2,i+2,0,15,"                    ");
	 return;
  }

  Datai=Da; Datai+=Pos;
  for (i=0;i<22;i++) {
	 if (i>=Dateien)
		Print(2,i+2,0,15,"                    ");
	 else {
		PrintInt(2,i+2,0,15,Pos+i+1);
		memset(Word,32,16); strcpy(Word,Datai->FileName);
		Word[strlen(Word)]=32; Word[15]=0;
		Print(7,i+2,0,15,Word);
		if (Datai->Date==0) Print(19,i+2,0,15,"DIR");
		Datai++;
	 }
  }
  PrintInt(2,CPos,7,15,Pos+CPos-1);
  Datai=Da; Datai+=Pos+CPos-2;
  memset(Word,32,16);
  strcpy(Word,Datai->FileName);
  Word[strlen(Word)]=32; Word[15]=0;
  Print(7,CPos,7,15,Word);
  if (Datai->Date==0) Print(19,CPos,7,15,"DIR");
}

void DateiSuchen(void)
{
  int i;
  Datai=Da;
  for (i=0;i<Dateien;i++) {
	 if (strnicmp(SuchStr,Datai->FileName,SPos)==0) {
		Pos=i-10; CPos=12; if (Pos<0 || Dateien<22) { CPos=i+2; Pos=0; break; }
		if (Pos>Dateien-22) { Pos=Dateien-22; CPos=Dateien-i; CPos=24-CPos; }
		break;
	 }
	 Datai++;
  }
  DisplayFiles();
}

