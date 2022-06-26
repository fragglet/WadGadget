/* IMPEXP.C */

#include <io.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "impexp.h"
#include "gifpcx.h"
#include "prints.h"
#include "setpal.h"
#include "wadview.h"


/* Import-Export-Routinen fr WADVIEW/NWT */



void Write2Iwad(void)
{
  long L,L2,L3,ll;

  L=Entry->RLength;
  fa=fopen(DateiName1,"rb");
  if (fa==NULL) { ScreenAufbau(); Display(); Error(2); return; }
  L2=filelength(fileno(fa));
  if (L2>L) {
    Entry->RStart=DirPos;
	 fseek(f,8,0); DirPos+=L2; _write(fileno(f),&DirPos,4);
  }
  L3=Entry->RStart; Entry->RLength=L2;
  fseek(f,L3,0); ll=L2;
  for (;;) {
    if (L2>=32000) {
      _read(fileno(fa),Puffer,32000); _write(fileno(f),Puffer,32000); L2-=32000;
    } else {
      _read(fileno(fa),Puffer,(int)L2); _write(fileno(f),Puffer,(int)L2); break;
    }
  }
  L2=ll;
  if (L2>L) {
    Entry=E;
    for (L3=0;L3<Entries;L3++) {
		ll=Entry->RStart; _write(fileno(f),&ll,4);
		ll=Entry->RLength; _write(fileno(f),&ll,4);
      _write(fileno(f),Entry->RName,8);
      Entry++;
    }
  }
  fclose(fa); Entry=E; Entry+=(Pos+CPos-2);
}

void Write2Pwad(void)
{
/*
  int i,x,y,k,ay,b,kk;
  int NewX,NewY,NOx,NOy;
  unsigned long L[320],LL=1,LL2,L2,L3,ll;
  char ident[5]="PWAD";
  char SchonDa=0;
  unsigned int Off,ii;
*/

  int i;
  unsigned long LL=1,LL2,L2,L3,ll;
  char ident[5]="PWAD";
  char SchonDa=0;

  fb=fopen(DateiName1,"rb+");
  if (fb==NULL) { ScreenAufbau(); Display(); Error(2); return; }

  fa=fopen(DateiName2,"rb+");
  if (fa==NULL) fa=fopen(DateiName2,"wb+"); else SchonDa=1;
  _write(fileno(fa),ident,4);
  if (SchonDa) {
	 _read(fileno(fa),&LL,4); L3=LL; LL++; fseek(fa,4,0); PEntries=(int)L3;
  }
  _write(fileno(fa),&LL,4);

  LL2=filelength(fileno(fb)); LL=LL2; LL+=12;
  if (SchonDa) {
	 _read(fileno(fa),&LL,4); L2=LL; LL+=LL2; fseek(fa,8,0);
  }
  _write(fileno(fa),&LL,4);

  if (SchonDa) {
	 fseek(fa,L2,0); PEntry=PE;
    for (LL=0;LL<L3;LL++) {
		_read(fileno(fa),&ll,4); PEntry->RStart=ll;
		_read(fileno(fa),&ll,4); PEntry->RLength=ll;
      _read(fileno(fa),PEntry->RName,8);
      PEntry->RName[8]=0;
      PEntry++;
	 }
    fseek(fa,L2,0); PEntry=PE;
  }

  LL=ftell(fa);
  ll=LL2;
  for (;;) {
    if (LL2>=32000) {
      _read(fileno(fb),Puffer,32000); _write(fileno(fa),Puffer,32000); LL2-=32000;
    } else {
      _read(fileno(fb),Puffer,(int)LL2); _write(fileno(fa),Puffer,(int)LL2); break;
    }
  }
  LL2=ll; PEntries++;
  if (SchonDa) {
    for (L2=0;L2<L3;L2++) {
		for (i=0;i<10;i++) if (strcmp(SpSigs[i],Entry->RName)==0) { i=1; break; }
		if (strcmp(Entry->RName,PEntry->RName)!=0 || i==1) {
	ll=PEntry->RStart; _write(fileno(fa),&ll,4);
	ll=PEntry->RLength; _write(fileno(fa),&ll,4);
	_write(fileno(fa),PEntry->RName,8);
      } else {
	PEntries--;
      }
      PEntry++;
    }
  }
  _write(fileno(fa),&LL,4); _write(fileno(fa),&LL2,4); _write(fileno(fa),Entry->RName,8);
  if (SchonDa) { LL=(long)PEntries; fseek(fa,4,0); _write(fileno(fa),&LL,4); fseek(fa,0,2); }
  fflush(fa); fclose(fa); fclose(fb);
}

unsigned int ReadVoc(void)
{
  int i,k,i2;
  char a;

  fc=fopen(DateiName1,"rb"); if (fc==0) return 0;
  _read(fileno(fc),&VocKopf,sizeof(VocKopf));
  fseek(fc,(long)VocKopf.DataStart,0);
  _read(fileno(fc),&a,1);
  if (a==1) {
	 _read(fileno(fc),&i,2);    /* VOC Laenge lesen */
	 _read(fileno(fc),&a,1);    /* 3tes byte der Laenge lesen, nicht werten */
	 _read(fileno(fc),&k,2);    /* SR & PACK lesen, ebenfalls nicht werten */
    i-=2;                      /* definitive L„nge des samples */
    if (i<0) { fclose(fc); return 0; }        /* Wenn, dann Fehler */
    for (i2=i,k=0;;) {
      if (i>=32000) {
	_read(fileno(fc),Puffer,32000); memcpy(MK_FP(B1Seg,B1Off+k),Puffer,32000); i-=32000; k+=32000;
      } else {
	_read(fileno(fc),Puffer,i); memcpy(MK_FP(B1Seg,B1Off+k),Puffer,i); i=0; break;
      }
    }
    fclose(fc); return i2;
  } else {
    fclose(fc); return 0;
  }
}

unsigned int ReadWav(void)
{
  unsigned int i,i2,k;
  long l;

  fc=fopen(DateiName1,"rb"); if (fc==0) return 0;
  _read(fileno(fc),&WavKopf,sizeof(WavKopf));
  _read(fileno(fc),Puffer,16);        /* 16 bytes bullshit */
  _read(fileno(fc),&l,4);              /* Laenge der daten */
  if (l>64000) { fclose(fc); return 0; }
  i=(unsigned int)l;
  if (i==0) { fclose(fc); return 0; }
  for (i2=i,k=0;;) {
    if (i>=32000) {
      _read(fileno(fc),Puffer,32000); memcpy(MK_FP(B1Seg,B1Off+k),Puffer,32000); i-=32000; k+=32000;
    } else {
      _read(fileno(fc),Puffer,i); memcpy(MK_FP(B1Seg,B1Off+k),Puffer,i); i=0; break;
    }
  }
  fclose(fc); return i2;
}

void ImportVoc2Raw(void)
{
  int i,k;
  unsigned int ii;

  Box(1);
  Print(29,10,5,15," WAV/VOC > RAW ");
  Print(32,11,5,15,"Infile:              Outfile:");
  Entry=E; Entry+=(Pos+CPos-2);
  strcpy(DateiName1,Entry->RName); strcat(DateiName1,Endung[SExport+1]);
  strcpy(DateiName2,Entry->RName); strcat(DateiName2,".RAW");
  Print(61,11,5,0,DateiName2); Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus7;
    i=Eingabe2(61,11,5,0); if (i<0) goto Aus7;
  }
  CWeg();

  Entry=E; Entry+=(Pos+CPos-2);

  for (i=0,k=0;i<strlen(DateiName1);i++) {
     if (DateiName1[i]==46) {
       if (DateiName1[i+1]==119 || DateiName1[i+1]==87) { k=1; ii=ReadWav(); break; }
       if (DateiName1[i+1]==118 || DateiName1[i+1]==86) { k=1; ii=ReadVoc(); break; }
     }
  }
  if (!k || ii==0) {
    ScreenAufbau(); Display(); Error(2); return;
  }
  SaveVocData(ii); unlink(DateiName2); rename("NWT.TMP",DateiName2);
Aus7:
  ScreenAufbau(); Display();
}

void ImportVoc2Pwad(void)
{
  int i,k;
  unsigned int ii;

  Box(1);
  Print(29,10,5,15," WAV/VOC > PWAD ");
  Print(32,11,5,15,"Infile:              Outfile:");
  Entry=E; Entry+=(Pos+CPos-2);
  strcpy(DateiName1,Entry->RName); strcat(DateiName1,Endung[SExport+1]);
  strcpy(DateiName2,Entry->RName); strcat(DateiName2,".WAD");
  if (PWadName[0]!=0) strcpy(DateiName2,PWadName);
  if (PutIn1==2 && Marked>0) strcpy(DateiName2,OneName);
  Print(61,11,5,0,DateiName2); Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus2;
    i=Eingabe2(61,11,5,0); if (i<0) goto Aus2;
  }
  strcpy(PWadName,DateiName2); CWeg();

  Entry=E; Entry+=(Pos+CPos-2);

  for (i=0,k=0;i<strlen(DateiName1);i++) {
     if (DateiName1[i]==46) {
       if (DateiName1[i+1]==119 || DateiName1[i+1]==87) { k=1; ii=ReadWav(); break; }
       if (DateiName1[i+1]==118 || DateiName1[i+1]==86) { k=1; ii=ReadVoc(); break; }
     }
  }
  if (!k || ii==0) {
    ScreenAufbau(); Display(); Error(2); return;
  }
  SaveVocData(ii);

  strcpy(DateiName1,"NWT.TMP"); Write2Pwad();

Aus2:
  ScreenAufbau(); Display();
}

void ImportVoc2Iwad(void)
{
  int i,k;
  unsigned int ii;

  Box(1);
  Print(29,10,5,15," WAV/VOC > IWAD ");
  Print(32,11,5,15,"Infile:");
  Entry=E; Entry+=(Pos+CPos-2);
  strcpy(DateiName1,Entry->RName); strcat(DateiName1,Endung[SExport+1]);
  Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus6;
  }
  CWeg();

  Entry=E; Entry+=(Pos+CPos-2);

  for (i=0,k=0;i<strlen(DateiName1);i++) {
	  if (DateiName1[i]==46) {
		 if (DateiName1[i+1]==119 || DateiName1[i+1]==87) { k=1; ii=ReadWav(); break; }
		 if (DateiName1[i+1]==118 || DateiName1[i+1]==86) { k=1; ii=ReadVoc(); break; }
	  }
  }
  if (!k || ii==0) {
	 ScreenAufbau(); Display(); Error(2); return;
  }
  SaveVocData(ii);

  strcpy(DateiName1,"NWT.TMP"); Write2Iwad();

Aus6:
  ScreenAufbau(); Display();
}

void ImportGif2Pwad(void)
{
  int x,y,i=14,k,ii;
  int NewX,NewY,NOx,NOy;

  if (ViewAble==1) Box(3); else Box(1);
  Print(29,10,5,15," GIF/PCX > PWAD ");
  Print(32,11,5,15,"Infile:              Outfile:");
  if (ViewAble==1) {
    Print(32,12,5,15,"Size X:               Size Y:");
    Print(30,13,5,15,"X-Offset:             Y-Offset:");
  }
  Entry=E; Entry+=(Pos+CPos-2);
  if (ViewAble==2) {
    if (Ox<0) Ox=0;
    if (Oy<0) Oy=0;
  }
  NewX=Rx; NewY=Ry; NOx=Ox; NOy=Oy;
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,Endung[GExport-1]);
  strcpy(DateiName2,Entry->RName); CheckForVile(2); strcat(DateiName2,".WAD");
  if (PWadName[0]!=0) strcpy(DateiName2,PWadName);
  if (PutIn1==2 && Marked>0) strcpy(DateiName2,OneName);
  Print(61,11,5,0,DateiName2); Print(39,11,5,0,DateiName1);
  if (ViewAble==1) {
    PrintInt3(39,12,5,0,Rx); PrintInt3(39,13,5,0,Ox);
    PrintInt3(61,12,5,0,Ry); PrintInt3(61,13,5,0,Oy);
  }
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus1;
    i=Eingabe2(61,11,5,0); if (i<0) goto Aus1;
  }
  strcpy(Puffer,DateiName1);
  if (ViewAble==1) {
    itoa(NewX,DateiName1,10); i=Eingabe1(39,12,5,0); if (i<0) goto Aus1;
    NewX=atoi(DateiName1);
    itoa(NewY,DateiName1,10); i=Eingabe1(61,12,5,0); if (i<0) goto Aus1;
    NewY=atoi(DateiName1);
	 if (Ox>0 && Oy>0) {
      NOx=NewX/2-1; NOy=NewY-5; PrintInt3(39,13,5,0,NOx); PrintInt3(61,13,5,0,NOy);
    }
    if (Marked==0) {
      itoa(NOx,DateiName1,10); i=Eingabe1(39,13,5,0); if (i<0) goto Aus1;
      NOx=atoi(DateiName1);
      itoa(NOy,DateiName1,10); i=Eingabe1(61,13,5,0); if (i<0) goto Aus1;
      NOy=atoi(DateiName1);
    }
  }
  strcpy(DateiName1,Puffer); strcpy(DatName1,DateiName1);
  strcpy(PWadName,DateiName2); CWeg();

  x=NewX; y=NewY;
  Entry=E; Entry+=(Pos+CPos-2);

  for (i=0,k=0;i<strlen(DatName1);i++) {
     if (DatName1[i]==46) {
       if (DatName1[i+1]==103 || DatName1[i+1]==71) { k=1; ii=ReadGif(); break; }
       if (DatName1[i+1]==112 || DatName1[i+1]==80) { k=1; ii=LadeVGAPcx(B1Seg,B1Off); break; }
     }
  }
  if (!k || ii<0) {
    ScreenAufbau(); Display(); Error(2); return;
  }
  SaveData(x,y,NOx,NOy);

  strcpy(DateiName1,"NWT.TMP"); Write2Pwad();

Aus1:
  ScreenAufbau(); Display();
}

void ImportRaw2Pwad(void)
{
  int i;

  Box(1);
  Print(29,10,5,15," RAW > PWAD ");
  Print(32,11,5,15,"Infile:              Outfile:");
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,".RAW");
  strcpy(DateiName2,Entry->RName); CheckForVile(2); strcat(DateiName2,".WAD");
  if (PWadName[0]!=0) strcpy(DateiName2,PWadName);
  if (PutIn1==2 && Marked>0) strcpy(DateiName2,OneName);
  Print(39,11,5,0,DateiName1); Print(61,11,5,0,DateiName2);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus3;
    i=Eingabe2(61,11,5,0); if (i<0) goto Aus3;
    CWeg();
  }
  Print(39,11,5,0,DateiName1); Print(61,11,5,0,DateiName2);
  strcpy(PWadName,DateiName2);

  Write2Pwad();

Aus3:
  ScreenAufbau(); Display();
}

void Export2Pwad(void)
{
  int i;
  unsigned long LL,LL2,L2,L3,ll;
  char ident[5]="PWAD";
  char SchonDa=0;

  Box(1);
  Print(29,10,5,15," Export >PWAD ");
  Print(31,11,5,15,"Outfile:");
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,".WAD");
  if (PWadName[0]!=0) strcpy(DateiName1,PWadName);
  if (PutIn1==2 && Marked>0) strcpy(DateiName1,OneName);
  Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus4;
    CWeg();
  }
  strcpy(PWadName,DateiName1);

  fa=fopen(DateiName1,"rb+");
  if (fa==NULL) fa=fopen(DateiName1,"wb"); else SchonDa=1;
  if (fa==NULL) { ScreenAufbau(); Display(); Error(3); return; }
  _write(fileno(fa),ident,4); LL=1;
  if (SchonDa) {
	 _read(fileno(fa),&LL,4); L3=LL; LL++; fseek(fa,4,0); PEntries=(int)L3;
  }
  _write(fileno(fa),&LL,4);

  LL2=Entry->RLength; LL=LL2; LL+=12;
  if (SchonDa) {
	 _read(fileno(fa),&LL,4); L2=LL; LL+=LL2; fseek(fa,8,0);
  }
  _write(fileno(fa),&LL,4);

  if (SchonDa) {
    fseek(fa,L2,0); PEntry=PE;
    for (i=0;i<PEntries;i++) {
		_read(fileno(fa),&ll,4); PEntry->RStart=ll;
		_read(fileno(fa),&ll,4); PEntry->RLength=ll;
      _read(fileno(fa),PEntry->RName,8);
      PEntry->RName[8]=0;
      PEntry++;
    }
    fseek(fa,L2,0); PEntry=PE;
  }

  LL=ftell(fa); ll=Entry->RStart; fseek(f,ll,0); ll=LL2;
  for (;;) {
    if (LL2>=32000) {
      _read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); LL2-=32000;
    } else {
      _read(fileno(f),Puffer,(int)LL2); _write(fileno(fa),Puffer,(int)LL2); break;
    }
  }
  LL2=ll; PEntries++;
  if (SchonDa) {
    for (L2=0;L2<L3;L2++) {
		for (i=0;i<10;i++) if (strcmp(SpSigs[i],Entry->RName)==0) { i=1; break; }
      if (strcmp(Entry->RName,PEntry->RName)!=0 || i==1) {
	ll=PEntry->RStart; _write(fileno(fa),&ll,4);
	ll=PEntry->RLength; _write(fileno(fa),&ll,4);
	_write(fileno(fa),PEntry->RName,8);
		} else {
	PEntries--;
      }
      PEntry++;
    }
  }
  _write(fileno(fa),&LL,4);
  _write(fileno(fa),&LL2,4);
  _write(fileno(fa),Entry->RName,8);
  if (SchonDa) { LL=(long)PEntries; fseek(fa,4,0); _write(fileno(fa),&LL,4); fseek(fa,0,2); }
  fflush(fa); fclose(fa);
Aus4:
/*  ScreenAufbau(); Display(); */
;
}

void SaveVoc(void)
{
/*
  int i,samplerate,format=SExport;
  unsigned int samples;
  long l,l2,l3;
  char a,b;
*/

  int i,samplerate,format=SExport;
  unsigned int samples;
  long l;
  char a,b;

  if (!PlayAble) { Error(7); return; }
  l=Entry->RStart;
  strcpy(VocKopf.Titel,"Creative Voice File\x1A");
  VocKopf.DataStart=0x001A; VocKopf.Version=0x010A;
  VocKopf.Id=0x1129;
  WavKopf.Riff[0]=82; WavKopf.Riff[1]=73; WavKopf.Riff[2]=70; WavKopf.Riff[3]=70;
  WavKopf.Laenge=0; strcpy(WavKopf.Wave,"WAVEfmt"); WavKopf.Wave[7]=32;
  WavKopf.Laenge2=16; WavKopf.format=1; WavKopf.kanals=1;
  Box(1);
  Print(29,10,5,15," Export WAV/VOC ");
  Print(31,11,5,15,"Outfile:");
  strcpy(DateiName1,Entry->RName); strcat(DateiName1,Endung[SExport+1]);
  Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus8;
    CWeg();
  }
  fa=fopen(DateiName1,"wb+"); if (fa==NULL) { Error(3); return; }
  fseek(f,l,0); _read(fileno(f),&i,2); _read(fileno(f),&samplerate,2);
  _read(fileno(f),&samples,2); _read(fileno(f),&i,2);
  for (i=0;i<strlen(DateiName1);i++) {
     if (DateiName1[i]==46) {
       if (DateiName1[i+1]==118 || DateiName1[i+1]==86) { format=2; break; }
       if (DateiName1[i+1]==119 || DateiName1[i+1]==87) { format=1; break; }
     }
  }
  if (format==2) {
	 _write(fileno(fa),&VocKopf,sizeof(VocKopf));
	 a=1; _write(fileno(fa),&a,1);                    /* Typ */
	 samples+=2; _write(fileno(fa),&samples,2);       /* Laenge (int!) */
	 b=0; _write(fileno(fa),&b,1);                    /* 3.byte der Laenge */
    a=(char)256-(1000000/samplerate);
	 _write(fileno(fa),&a,1);                 /* samplerate */
	 _write(fileno(fa),&b,1);                 /* pack=0 */
    samples-=2;
  }
  if (format==1) {
	 _write(fileno(fa),&WavKopf,sizeof(WavKopf));
	 l=(long)samplerate; _write(fileno(fa),&l,4);     /* samplerate */
	 l=(long)samplerate; _write(fileno(fa),&l,4);     /* samplerate 2 */
	 i=1; _write(fileno(fa),&i,2);                    /* bytes per sample */
	 i=8; _write(fileno(fa),&i,2);                    /* bits per sample */
	 a=100; _write(fileno(fa),&a,1); a=97; _write(fileno(fa),&a,1);
	 a=116; _write(fileno(fa),&a,1); a=97; _write(fileno(fa),&a,1); /* data */
	 l=(long)samples; _write(fileno(fa),&l,4);        /* sample-laenge */
  }
  for (;;) {
    if (samples>=32000) {
		_read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); samples-=32000;
    } else {
      _read(fileno(f),Puffer,(long)samples); _write(fileno(fa),Puffer,(long)samples); break;
    }
  }
  b=0; if (format==1) b=127;
  _write(fileno(fa),&b,1);                   /* nochmal Blocktyp 0 - Ende */
				  /* oder zur rundung bei wav ein 127 ran */
  if (format==1) {
    l=ftell(fa); fseek(fa,4,0);
	 l-=8; _write(fileno(fa),&l,4);
  }
  fflush(fa); fclose(fa);
Aus8:
  ScreenAufbau(); Display();
}

void SaveRaw(void)
{
  int i;
  long l,l2,l3;

  l=Entry->RStart; l2=Entry->RLength;
  Box(1);
  Print(29,10,5,15," Export RAW ");
  Print(31,11,5,15,"Outfile:");
  strcpy(DateiName1,Entry->RName); CheckForVile(1);
  strcat(DateiName1,".RAW");
  Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus9;
    CWeg();
  }
  fa=fopen(DateiName1,"wb+"); if (fa==NULL) { Error(3); return; }
  fseek(f,l,0);
  for (l3=l2;;) {
    if (l3>=32000) {
      _read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); l3-=32000;
    } else {
      _read(fileno(f),Puffer,(int)l3); _write(fileno(fa),Puffer,(int)l3); break;
    }
  }
  fflush(fa); fclose(fa);
Aus9:
  ScreenAufbau(); Display();
}

void SavePic(void)
{
  long l;
  unsigned int i;
  int ii;

  if (ViewAble==0) { Error(4); return; }
  l=Entry->RStart;
  if (ViewAble==2) { Ox=0; Oy=0; }
  Box(3);
  Print(29,10,5,15," Export GIF/PCX ");
  Print(31,11,5,15,"Outfile:");
  Print(32,12,5,15,"Size X:               Size Y:");
  Print(30,13,5,15,"X-Offset:             Y-Offset:");
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,Endung[GExport-1]);
  PrintInt3(39,12,5,0,Rx); PrintInt3(39,13,5,0,Ox); Print(39,11,5,0,DateiName1);
  PrintInt3(61,12,5,0,Ry); PrintInt3(61,13,5,0,Oy);
  if (Marked==0) {
    ii=Eingabe1(39,11,5,0); if (ii<0) goto Aus10;
    CWeg();
  }
  for (i=0;i<64000;i++) pokeb(B1Seg,B1Off+i,247);
  TakePic(l,0,0,0,0,320,200,B1Seg,B1Off);
  DoomPal[741]=0; DoomPal[742]=63; DoomPal[743]=63;
  for (i=0;i<strlen(DateiName1);i++) {
     if (DateiName1[i]==46) {
       if (DateiName1[i+1]==103 || DateiName1[i+1]==71) { SaveGif(Rx,Ry); break; }
       if (DateiName1[i+1]==112 || DateiName1[i+1]==80) { SavePcx(Rx,Ry,B1Seg,B1Off); break; }
     }
  }
  if (WinPal==1) {
    DoomPal[741]=0; DoomPal[742]=0; DoomPal[743]=0;
  } else {
    DoomPal[741]=0; DoomPal[742]=63; DoomPal[743]=63;
  }
Aus10:
  ScreenAufbau(); Display();
}

void ImportRaw2Iwad(void)
{
  int i;
/* int i,NewX,NewY,NOx,NOy;
	int x,y,k,ii;
	long L,L2,L3,ll;
*/

  Box(1);
  Print(29,10,5,15," RAW > IWAD ");
  Print(32,11,5,15,"Infile:");
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,".RAW");
  Print(39,11,5,0,DateiName1);
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus5;
    CWeg();
  }

  Write2Iwad();
Aus5:
  ScreenAufbau(); Display();
}

void ImportGif2Iwad(void)
{
  int i=14,NewX,NewY,NOx,NOy;
  int x,y,k,ii;
/*  long L,L2,L3,ll; */

  if (Entry->RStart==0) ViewAble=1;
  if (ViewAble==1) Box(3); else Box(1);
  Print(29,10,5,15," GIF/PCX > IWAD ");
  Print(32,11,5,15,"Infile:");
  if (ViewAble==1) {
    Print(32,12,5,15,"Size X:               Size Y:");
    Print(30,13,5,15,"X-Offset:             Y-Offset:");
  }
/*  Entry=E; Entry+=(Pos+CPos-2); */
  if (ViewAble==2) {
    if (Ox<0) Ox=0;
    if (Oy<0) Oy=0;
  }
  NewX=Rx; NewY=Ry; NOx=Ox; NOy=Oy;
  if (Entry->RStart==0) { NewX=0; NewY=0; NOx=0; NOy=0; Ox=0; Oy=0; Rx=0; Ry=0; }
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,Endung[GExport-1]);
  Print(39,11,5,0,DateiName1);
  if (ViewAble==1) {
    PrintInt3(39,12,5,0,Rx); PrintInt3(39,13,5,0,Ox);
    PrintInt3(61,12,5,0,Ry); PrintInt3(61,13,5,0,Oy);
  }
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus12;
  }
  strcpy(Puffer,DateiName1);
  if (ViewAble==1 && Marked==0) {
    itoa(NewX,DateiName1,10); i=Eingabe1(39,12,5,0); if (i<0) goto Aus12;
    NewX=atoi(DateiName1); if (NewX<=0) goto Aus12;
	 itoa(NewY,DateiName1,10); i=Eingabe1(61,12,5,0); if (i<0) goto Aus12;
    NewY=atoi(DateiName1); if (NewY<=0) goto Aus12;
	 if (Ox>0 && Oy>0) {
      NOx=NewX/2-1; NOy=NewY-5; PrintInt3(39,13,5,0,NOx); PrintInt3(61,13,5,0,NOy);
    }
    itoa(NOx,DateiName1,10); i=Eingabe1(39,13,5,0); if (i<0) goto Aus12;
    NOx=atoi(DateiName1);
    itoa(NOy,DateiName1,10); i=Eingabe1(61,13,5,0); if (i<0) goto Aus12;
    NOy=atoi(DateiName1);
  }
  if (NewX==64 && NewY==64 && Entry->RStart==0) ViewAble=2;
  if (Marked==0) {
	 strcpy(DateiName1,Puffer);
	 strcpy(DatName1,DateiName1); CWeg();
  }

  x=NewX; y=NewY;

  for (i=0,k=0;i<strlen(DatName1);i++) {
     if (DatName1[i]==46) {
       if (DatName1[i+1]==103 || DatName1[i+1]==71) { k=1; ii=ReadGif(); break; }
       if (DatName1[i+1]==112 || DatName1[i+1]==80) { k=1; ii=LadeVGAPcx(B1Seg,B1Off); break; }
     }
  }
  if (!k || ii<0) {
    ScreenAufbau(); Display(); Error(2); return;
  }
  SaveData(x,y,NOx,NOy);

  strcpy(DateiName1,"NWT.TMP"); Write2Iwad();
Aus12:
  CWeg();
  if ( (NewX<=0 || NewY<=0) && i>=0 ) Error(20);
  ScreenAufbau(); Display();
}

void ImportGif2Raw(void)
{
/*
  int i=14,NewX,NewY,NOx,NOy;
  int x,y,k,ay,ii,b,kk;
  unsigned int Off;
*/

  int i=14,NewX,NewY,NOx,NOy;
  int x,y,k,ii;

  if (ViewAble==1) Box(3); else Box(1);
  Print(29,10,5,15," GIF/PCX > RAW ");
  Print(32,11,5,15,"Infile:              Outfile:");
  if (ViewAble==1) {
    Print(32,12,5,15,"Size X:               Size Y:");
    Print(30,13,5,15,"X-Offset:             Y-Offset:");
  }
  Entry=E; Entry+=(Pos+CPos-2);
  if (ViewAble==2) {
    if (Ox<0) Ox=0;
    if (Oy<0) Oy=0;
  }
  NewX=Rx; NewY=Ry; NOx=Ox; NOy=Oy;
  strcpy(DateiName1,Entry->RName); CheckForVile(1); strcat(DateiName1,Endung[GExport-1]);
  strcpy(DateiName2,Entry->RName); CheckForVile(2); strcat(DateiName2,".RAW");
  Print(39,11,5,0,DateiName1); Print(61,11,5,0,DateiName2);
  if (ViewAble==1) {
    PrintInt3(39,12,5,0,Rx); PrintInt3(39,13,5,0,Ox);
    PrintInt3(61,12,5,0,Ry); PrintInt3(61,13,5,0,Oy);
  }
  if (Marked==0) {
    i=Eingabe1(39,11,5,0); if (i<0) goto Aus13;
    i=Eingabe2(61,11,5,0); if (i<0) goto Aus13;
  }
  strcpy(Puffer,DateiName1);
  if (ViewAble==1 && Marked==0) {
    itoa(NewX,DateiName1,10); i=Eingabe1(39,12,5,0); if (i<0) goto Aus13;
    NewX=atoi(DateiName1);
	 itoa(NewY,DateiName1,10); i=Eingabe1(61,12,5,0); if (i<0) goto Aus13;
    NewY=atoi(DateiName1);
	 if (Ox>0 && Oy>0) {
      NOx=NewX/2-1; NOy=NewY-5; PrintInt3(39,13,5,0,NOx); PrintInt3(61,13,5,0,NOy);
    }
    itoa(NOx,DateiName1,10); i=Eingabe1(39,13,5,0); if (i<0) goto Aus13;
    NOx=atoi(DateiName1);
    itoa(NOy,DateiName1,10); i=Eingabe1(61,13,5,0); if (i<0) goto Aus13;
    NOy=atoi(DateiName1);
  }
  strcpy(DateiName1,Puffer); strcpy(DatName1,DateiName1); CWeg();

  x=NewX; y=NewY;

  for (i=0,k=0;i<strlen(DatName1);i++) {
     if (DatName1[i]==46) {
       if (DatName1[i+1]==103 || DatName1[i+1]==71) { k=1; ii=ReadGif(); break; }
       if (DatName1[i+1]==112 || DatName1[i+1]==80) { k=1; ii=LadeVGAPcx(B1Seg,B1Off); break; }
     }
  }
  if (!k || ii<0) {
    ScreenAufbau(); Display(); Error(2); return;
  }
  SaveData(x,y,NOx,NOy); unlink(DateiName2); rename("NWT.TMP",DateiName2);
Aus13:
  ScreenAufbau(); Display();
}

void SaveVocData(unsigned int Laenge)
{
  unsigned int i;
  fc=fopen("NWT.TMP","wb+");
  i=3; _write(fileno(fc),&i,2);
  i=11025; _write(fileno(fc),&i,2);
  _write(fileno(fc),&Laenge,2);
  i=0; _write(fileno(fc),&i,2);
  for (i=0;;) {
	 if (Laenge>=32000) {
		memcpy(Puffer,MK_FP(B1Seg,B1Off+i),32000); _write(fileno(fc),Puffer,32000); Laenge-=32000; i+=32000;
	 } else {
		memcpy(Puffer,MK_FP(B1Seg,B1Off+i),Laenge); _write(fileno(fc),Puffer,Laenge); Laenge=0; break;
	 }
  }
  fclose(fc);
}

void SaveData(int x,int y,int xoff,int yoff)
{
/*
  int i,k,ay,ii,b,kk;
  char str[5],Spalte[200],a,End=0;
  unsigned long L[320];
*/

  int i,k,ay,ii,kk;
  char Spalte[200],End=0;
  unsigned long L[320];

  fc=fopen("NWT.TMP","wb+");

  if (ViewAble==2) {
    for (i=0;i<64;i++) {
      for (k=0;k<64;k++) Spalte[k]=peekb(B1Seg,B1Off+k+i*320);
      _write(fileno(fc),&Spalte,64);
    }
    fclose(fc); return;
  }

  _write(fileno(fc),&x,2); _write(fileno(fc),&y,2);
  _write(fileno(fc),&xoff,2); _write(fileno(fc),&yoff,2);

  for (i=0;i<x;i++) _write(fileno(fc),&L[0],4);

  k=0; i=0;
  for (i=0;i<x;i++) {
    for (k=0;k<y;k++) Spalte[k]=peekb(B1Seg,B1Off+i+k*320);
    L[i]=(long)ftell(fc);
    for (End=0,kk=0;;) {
      for (k=kk;k<y;k++) {
	if (Spalte[k]==247 && k==y-1) { ii=255; fputc(ii,fc); End=1; break; }
	if (Spalte[k]!=247) { ay=k; fputc(k,fc); break; }
      }
      if (End) break;
      for (ii=0,k=ay;k<y;k++) {
	if (Spalte[k]==247) { ii=k-ay; fputc(ii,fc); break; }
      }
      if (ii==0) { ii=y-ay; fputc(ii,fc); End=1; }
      fputc(Spalte[ay],fc);
      for (k=ay;k<ay+ii;k++) {
	fputc(Spalte[k],fc);
      }
      fputc(Spalte[k-1],fc);
      if (End) { ii=255; fputc(ii,fc); break; }
      kk=k;
    }
  }
  fseek(fc,8,0); for (i=0;i<x;i++) _write(fileno(fc),&L[i],4);
  fflush(fc); fclose(fc);
}
