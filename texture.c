/* TEXTURE.C */

#include <io.h>
#include <stdio.h>
#include <string.h>

#include "pnames.h"
#include "prints.h"
#include "texture.h"
#include "wadview.h"



void SaveTexture(char *DateiName1, char *DateiName2)
{
  /* D1 = Texture-Name   D2 = Output-File-Name */

  int i,k,dummy,d2;
  char format=0,SchonDa=0,Flag=0,TDa=0;
  char ident[5]="PWAD";
  long L,LPos[500],AnzRes=0,ResBeg,ll,n,l,TS;

  for (i=0;i<strlen(DateiName1);i++) if (DateiName1[i]>=97 && DateiName1[i]<=123) DateiName1[i]-=32;
  for (i=0;i<strlen(DateiName2);i++) if (DateiName2[i]>=97 && DateiName2[i]<=123) DateiName2[i]-=32;

  for (i=0;i<strlen(DateiName2);i++) {
     if (DateiName2[i]==46) {
       if (DateiName2[i+1]==119 || DateiName2[i+1]==87) { format=1; break; }
       if (DateiName2[i+1]==114 || DateiName2[i+1]==82) { format=2; break; }
       if (DateiName2[i+1]==116 || DateiName2[i+1]==84) { format=3; break; }
     }
  }
  /* 1=WAD 2=RAW 3=TEX */

  if (format==0) { ScreenAufbau(); Display64(); Error(3); return; }
  Print(18,10,5,15," 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커 ");
  Print(18,11,5,15," �         Saving Texture Data...         � ");
  Print(18,12,5,15," 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸 ");
  fc=fopen("NWT.TMP","wb+"); if (fc==NULL) { ScreenAufbau(); Display64(); Error(3); return; }
  PEntries=0;
  if (format==2) {
    fa=fopen(DateiName2,"rb+");
    if (fa==NULL) { fa=fopen(DateiName2,"wb"); TDa=0; } else { SchonDa=1; TDa=1; }
    if (fa==NULL) { fclose(fc); ScreenAufbau(); Display64(); Error(3); return; }
    if (TDa) {
      _read(fileno(fa),&n,4); PTEntries=(int)n;
      for (k=0,L=0;L<n;L++) {
	_read(fileno(fa),&ll,4); PTEntry[k].TStart=ll; k++;
      }
      for (k=0,L=0;L<n;L++) {
	fseek(fa,PTEntry[k].TStart,0); _read(fileno(fa),&PTEntry[k].TName,8); PTEntry[k].TName[8]=0; k++;
      }
    }
  }
  if (format==3) {
    fa=fopen(DateiName2,"rb+");
    if (fa==NULL) fa=fopen(DateiName2,"wb+"); else SchonDa=1;
    if (fa==NULL) { fclose(fc); ScreenAufbau(); Display64(); Error(3); return; }
    if (SchonDa) {
      ll=filelength(fileno(fa));
      for (;;) {
	if (ll>=32000) {
	  _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); ll-=32000;
	} else {
	  _read(fileno(fa),Puffer,(int)ll); _write(fileno(fc),Puffer,(int)ll); break;
	}
      }
    }
    fprintf(fc,"%-24s%-8d%d\r\n",DateiName1,NewTx,NewTy);
    for (k=0;k<NTCount;k++) fprintf(fc,"*       %-16s%-8d%d\r\n",PName[NTData[k].EntryNr].Name,NTData[k].x,NTData[k].y);
  }
  if (format==1) {
    fa=fopen(DateiName2,"rb+");
    if (fa==NULL) fa=fopen(DateiName2,"wb+"); else SchonDa=1;
    if (fa==NULL) { fclose(fc); ScreenAufbau(); Display64(); Error(3); return; }
    _write(fileno(fc),&ident,4); _write(fileno(fc),&AnzRes,4);
    _write(fileno(fc),&ResBeg,4); _write(fileno(fa),&ident,4);
    if (SchonDa) {
      _read(fileno(fa),&AnzRes,4); _read(fileno(fa),&ResBeg,4);
      fseek(fa,ResBeg,0); PEntries=(int)AnzRes; PEntry=PE;
      for (i=0;i<PEntries;i++) {
	_read(fileno(fa),&ll,4); PEntry->RStart=ll;
	_read(fileno(fa),&ll,4); PEntry->RLength=ll;
	_read(fileno(fa),PEntry->RName,8); PEntry->RName[8]=0;
	PEntry++;
      }
      PEntry=PE;
		for (i=0;i<PEntries;i++) if (strcmp(TexRName,PEntry->RName)==0) { TDa=1; break; }
      PEntry=PE;
      for (i=0;i<PEntries;i++) {
	if (strcmp(TexRName,PEntry->RName)!=0) {
	  ll=PEntry->RStart; fseek(fa,ll,0); ll=PEntry->RLength;
	  L=ftell(fc); PEntry->RStart=L;
	  for (;;) {
	    if (ll>=32000) {
	      _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); ll-=32000;
	    } else {
	      _read(fileno(fa),Puffer,(int)ll); _write(fileno(fc),Puffer,(int)ll); break;
	    }
	  }
	} else {
	  AnzRes--;
	  l=PEntry->RStart; fseek(fa,l,0); _read(fileno(fa),&n,4); PTEntries=(int)n;
	  for (k=0,L=0;L<n;L++) {
	    _read(fileno(fa),&ll,4); PTEntry[k].TStart=ll+l; k++;
	  }
	  for (k=0,L=0;L<n;L++) {
	    fseek(fa,PTEntry[k].TStart,0); _read(fileno(fa),&PTEntry[k].TName,8); PTEntry[k].TName[8]=0; k++;
	  }
	}
	PEntry++;
      }
    }
  }
  if (format==1 || format==2) {
	 if (SchonDa) {
      if (TDa) {
	TS=ftell(fc); AnzRes++;
	for (i=0;i<PTEntries;i++) if (strcmp(PTEntry[i].TName,DateiName1)==0) { Flag=1; break; }
	_write(fileno(fc),&n,4);
	for (i=0;i<PTEntries;i++) _write(fileno(fc),&n,4);
	if (!Flag) _write(fileno(fc),&n,4); /* falls neuer Textname=einer mehr */
	for (i=0;i<PTEntries;i++) {
	  LPos[i]=ftell(fc); LPos[i]-=TS;
	  _write(fileno(fc),&PTEntry[i].TName,8); ll=0;
	  _write(fileno(fc),&ll,4);
	  if (Flag && strcmp(PTEntry[i].TName,DateiName1)==0) {
	    _write(fileno(fc),&NewTx,2); _write(fileno(fc),&NewTy,2);
	    _write(fileno(fc),&ll,4); _write(fileno(fc),&NTCount,2);
	    for (k=0;k<NTCount;k++) {
	      _write(fileno(fc),&NTData[k].x,2); _write(fileno(fc),&NTData[k].y,2);
	      _write(fileno(fc),&NTData[k].EntryNr,2); dummy=1;
	      _write(fileno(fc),&dummy,2); dummy=0;
	      _write(fileno(fc),&dummy,2);
	    }
	  } else {
	    fseek(fa,PTEntry[i].TStart+12,0);
	    _read(fileno(fa),Puffer,8); _write(fileno(fc),Puffer,8);
	    _read(fileno(fa),&d2,2); _write(fileno(fc),&d2,2);
	    for (k=0;k<d2;k++) { _read(fileno(fa),Puffer,10); _write(fileno(fc),Puffer,10); }
	  }
	}
	if (!Flag) {
	  LPos[PTEntries]=ftell(fc); LPos[PTEntries]-=TS;
	  _write(fileno(fc),&DateiName1,8); ll=0; _write(fileno(fc),&ll,4);
	  _write(fileno(fc),&NewTx,2); _write(fileno(fc),&NewTy,2);
	  _write(fileno(fc),&ll,4); _write(fileno(fc),&NTCount,2);
	  for (k=0;k<NTCount;k++) {
	    _write(fileno(fc),&NTData[k].x,2); _write(fileno(fc),&NTData[k].y,2);
		 _write(fileno(fc),&NTData[k].EntryNr,2); dummy=1;
	    _write(fileno(fc),&dummy,2); dummy=0;
	    _write(fileno(fc),&dummy,2);
	  }
	  PTEntries++; n=(long)PTEntries;
	}
	fseek(fc,TS,0); _write(fileno(fc),&n,4);
	for (i=0;i<PTEntries;i++) _write(fileno(fc),&LPos[i],4);
	fseek(fc,0,2);
      }
    }
  }
  if ( !TDa && (format==1 || format==2) ) {
    TS=ftell(fc); AnzRes++;
    Entry=E; Entry+=(AltPos+AltCPos-2);
    l=Entry->RStart; fseek(f,l,0); _read(fileno(f),&n,4); PTEntries=(int)n;
    for (k=0,L=0;L<n;L++) {
      _read(fileno(f),&ll,4); PTEntry[k].TStart=ll+l; k++;
    }
    for (k=0,L=0;L<n;L++) {
      fseek(f,PTEntry[k].TStart,0); _read(fileno(f),&PTEntry[k].TName,8); PTEntry[k].TName[8]=0; k++;
    }
	 for (i=0;i<PTEntries;i++) if (strcmp(PTEntry[i].TName,DateiName1)==0) { Flag=1; break; }
    _write(fileno(fc),&n,4);
    for (i=0;i<PTEntries;i++) _write(fileno(fc),&n,4);
    if (!Flag) _write(fileno(fc),&n,4); /* falls neuer Textname=einer mehr */
    for (i=0;i<PTEntries;i++) {
      LPos[i]=ftell(fc); LPos[i]-=TS;
      _write(fileno(fc),&PTEntry[i].TName,8); ll=0;
      _write(fileno(fc),&ll,4);
		if (Flag && strcmp(PTEntry[i].TName,DateiName1)==0) {
	_write(fileno(fc),&NewTx,2); _write(fileno(fc),&NewTy,2);
	_write(fileno(fc),&ll,4); _write(fileno(fc),&NTCount,2);
	for (k=0;k<NTCount;k++) {
	  _write(fileno(fc),&NTData[k].x,2); _write(fileno(fc),&NTData[k].y,2);
	  _write(fileno(fc),&NTData[k].EntryNr,2); dummy=1;
	  _write(fileno(fc),&dummy,2); dummy=0;
	  _write(fileno(fc),&dummy,2);
	}
      } else {
	fseek(f,PTEntry[i].TStart+12,0); /* +12 weil Name & 2 ints schon waren */
	_read(fileno(f),Puffer,8); _write(fileno(fc),Puffer,8);
	_read(fileno(f),&d2,2); _write(fileno(fc),&d2,2);
	for (k=0;k<d2;k++) { _read(fileno(f),Puffer,10); _write(fileno(fc),Puffer,10); }
      }
    }
    if (!Flag) {
      LPos[PTEntries]=ftell(fc); LPos[PTEntries]-=TS;
      _write(fileno(fc),&DateiName1,8); ll=0; _write(fileno(fc),&ll,4);
      _write(fileno(fc),&NewTx,2); _write(fileno(fc),&NewTy,2);
      _write(fileno(fc),&ll,4); _write(fileno(fc),&NTCount,2);
      for (k=0;k<NTCount;k++) {
	_write(fileno(fc),&NTData[k].x,2); _write(fileno(fc),&NTData[k].y,2);
	_write(fileno(fc),&NTData[k].EntryNr,2); dummy=1;
	_write(fileno(fc),&dummy,2); dummy=0;
	_write(fileno(fc),&dummy,2);
      }
      PTEntries++; n=(long)PTEntries;
    }
    fseek(fc,TS,0); _write(fileno(fc),&n,4);
    for (i=0;i<PTEntries;i++) _write(fileno(fc),&LPos[i],4);
    fseek(fc,0,2);
  }
  if (format==1) {
    PEntry=PE; L=ftell(fc);
    for (i=0;i<PEntries;i++) {
		if (strcmp(TexRName,PEntry->RName)!=0) {
	ll=PEntry->RStart; _write(fileno(fc),&ll,4);
	ll=PEntry->RLength; _write(fileno(fc),&ll,4);
	_write(fileno(fc),PEntry->RName,8);
		}
		PEntry++;
	 }
	 _write(fileno(fc),&TS,4); ll=L-TS; _write(fileno(fc),&ll,4);
	 _write(fileno(fc),&TexRName,8);
	 fseek(fc,4,0); _write(fileno(fc),&AnzRes,4); _write(fileno(fc),&L,4);
	 fflush(fa); fclose(fa);
  }
  fflush(fc); fclose(fc);
  unlink(DateiName2); rename("NWT.TMP",DateiName2);
  ScreenAufbau(); Display64();
}

int SavePNameRes(void)
{
  /* D2 = Output-File-Name */
/*
  int i,k,dummy,d2;
  char format=0,SchonDa=0;
  char ident[5]="PWAD";
  long L,LPos[500],AnzRes=0,ResBeg,ll,n,l,PS;
*/

  int i;
  char format=0,SchonDa=0;
  char ident[5]="PWAD";
  long L,AnzRes=0,ResBeg,ll,n,PS;

  for (i=0;i<strlen(DateiName2);i++) {
     if (DateiName2[i]==46) {
		 if (DateiName2[i+1]==119 || DateiName2[i+1]==87) { format=1; break; }
		 if (DateiName2[i+1]==114 || DateiName2[i+1]==82) { format=2; break; }
     }
  }
  /* 1=WAD 2=RAW */

  if (format==0) { ScreenAufbau(); DisplayPNames(); Error(3); return -1; }
  Box(1);
  Print(39,11,5,15,"Saving PNames Data...");
  fc=fopen("NWT.TMP","wb+"); if (fc==NULL) { ScreenAufbau(); DisplayPNames(); Error(3); return -1; }
  PEntries=0;
  if (format==1) {
	 fa=fopen(DateiName2,"rb+"); if (fa==NULL) SchonDa=0; else SchonDa=1;
    _write(fileno(fc),&ident,4); _write(fileno(fc),&AnzRes,4);
    _write(fileno(fc),&ResBeg,4);
    if (SchonDa) {
      fseek(fa,4,0); _read(fileno(fa),&AnzRes,4); _read(fileno(fa),&ResBeg,4);
      fseek(fa,ResBeg,0); PEntries=(int)AnzRes; PEntry=PE;
      for (i=0;i<PEntries;i++) {
	_read(fileno(fa),&ll,4); PEntry->RStart=ll;
	_read(fileno(fa),&ll,4); PEntry->RLength=ll;
	_read(fileno(fa),PEntry->RName,8); PEntry->RName[8]=0;
	PEntry++;
		}
      PEntry=PE;
      for (i=0;i<PEntries;i++) {
	if (strcmp("PNAMES",PEntry->RName)!=0) {
	  ll=PEntry->RStart; fseek(fa,ll,0); ll=PEntry->RLength;
	  L=ftell(fc); PEntry->RStart=L;
	  for (;;) {
	    if (ll>=32000) {
	      _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); ll-=32000;
	    } else {
	      _read(fileno(fa),Puffer,(int)ll); _write(fileno(fc),Puffer,(int)ll); break;
	    }
	  }
	} else {
	  AnzRes--;
	}
      }
    }
  }
  AnzRes++;
  PS=ftell(fc); n=(long)PNames; _write(fileno(fc),&n,4);
  for (i=0;i<PNames;i++) _write(fileno(fc),&PName[i].Name,8);

  if (format==1) {
    PEntry=PE; L=ftell(fc);
    for (i=0;i<PEntries;i++) {
		if (strcmp("PNAMES",PEntry->RName)!=0) {
	ll=PEntry->RStart; _write(fileno(fc),&ll,4);
	ll=PEntry->RLength; _write(fileno(fc),&ll,4);
	_write(fileno(fc),PEntry->RName,8);
      }
      PEntry++;
    }
    _write(fileno(fc),&PS,4); ll=L-PS; _write(fileno(fc),&ll,4);
	 for (i=0;i<8;i++) Puffer[i]=0;
	 strcpy(Puffer,"PNAMES"); _write(fileno(fc),Puffer,8);
    fseek(fc,4,0); _write(fileno(fc),&AnzRes,4); _write(fileno(fc),&L,4);
    if (SchonDa) { fflush(fa); fclose(fa); }
  }
  fflush(fc); fclose(fc);
  unlink(DateiName2); rename("NWT.TMP",DateiName2);
  ScreenAufbau(); DisplayPNames(); return 1;
}
