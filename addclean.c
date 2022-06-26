#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "addclean.h"
#include "wadview.h"


/* Add Sprites & Flats, Cleaner fÅr WADVIEW - NWT */

void JoinPWads(void)
{
  int i,k,i2=0,ii,m,Dinge=0;
  long ll,l;
  char over=0,Taste;

  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    Entry=E;
    for (k=0;k<Entries;k++) {
      if (strcmp(Entry->RName,PEntry->RName)==0) {
	for (ii=0,m=0;ii<10;ii++) if (strcmp(SpSigs[ii],Entry->RName)==0) { m=1; break; }
	if (!m) {
	  i2=1; Entry->Mark=1; PEntry->Mark=1;
	}
	break;
      }
      Entry++;
    }
    PEntry++;
  }
  if (i2) {
    printf(" There are resources with the same name. Overwrite? (y/n)");
	 Taste=getch(); printf("\n");
    if (Taste==89 || Taste==121) over=1;
  }
  PEntry=PE; l=PDirPos;
  for (i=0;i<PEntries;i++) {
    if (PEntry->RStart>PDirPos) { l=filelength(fileno(fa)); printf(" Directory is not the last entry. Correcting...\n"); break; }
    PEntry++;
  }
  Entry=E; fseek(fa,l,0);
  for (i=0;i<Entries;i++) {
    if (!Entry->Mark || (Entry->Mark && over) ) {
      if (Entry->RLength>0) {
	l=Entry->RStart; fseek(f,l,0); l=Entry->RLength; ll=ftell(fa);
	for (;;) {
	  if (l>=32000) {
	    _read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); l-=32000;
	  } else {
	    _read(fileno(f),Puffer,(int)l); _write(fileno(fa),Puffer,(int)l); break;
	  }
	}
	Entry->RStart=ll;
      }
      if (!Entry->Mark) Dinge++;
    }
    Entry++;
  }
  printf(" %d resources added.\n",Dinge);
  PDirPos=ftell(fa); PEntry=PE; Dinge=0;
  for (i=0;i<PEntries;i++) {
    if (!PEntry->Mark || (PEntry->Mark && !over) ) {
      l=PEntry->RStart; _write(fileno(fa),&l,4);
      l=PEntry->RLength; _write(fileno(fa),&l,4);
      _write(fileno(fa),PEntry->RName,8);
      Dinge++;
    }
    PEntry++;
  }
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (!Entry->Mark || (Entry->Mark && over) ) {
      l=Entry->RStart; _write(fileno(fa),&l,4);
      l=Entry->RLength; _write(fileno(fa),&l,4);
      _write(fileno(fa),Entry->RName,8);
      Dinge++;
    }
    Entry++;
  }
  printf(" Done. %d resources processed.\n",Dinge);
  l=(long)Dinge; fseek(fa,4,0); _write(fileno(fa),&l,4); _write(fileno(fa),&PDirPos,4);
}

int RestoreIWad(void)
{
  int i;
/*  int i,i2; */
  long l,ll,LL;
  char NeuSchreiben=0,Letzte=0;

  OpenWADFile();
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (strnicmp(Entry->RName,"NWT",3)==0) {
      i=9999; break;
    }
    Entry++;
  }
  if (i==9999) {
    printf(" Found original IWAD directory. Restoring...\n");
    PDirPos=Entry->RStart; fseek(f,PDirPos,0); l=Entry->RLength;
    if (l>4000) l/=16; PEntries=(int)l;
    PEntry=PE;
    for (LL=0;LL<PEntries;LL++) {
      _read(fileno(f),&ll,4); PEntry->RStart=ll;
      _read(fileno(f),&ll,4); PEntry->RLength=ll;
		_read(fileno(f),PEntry->RName,8);
      PEntry->RName[8]=0;
      PEntry++;
    }
    PEntry=PE; l=ftell(f);
    if (DirPos!=l) NeuSchreiben=1;
    l=(long)Entries*16; l+=DirPos; ll=filelength(fileno(f));
    if (l!=ll) NeuSchreiben=1; else Letzte=1;

    Entry=E; PEntry=PE;
    for (i=0;i<PEntries;i++) {
      if (strcmp(Entry->RName,PEntry->RName)==0) {
	if (Entry->RStart!=PEntry->RStart || Entry->RLength!=PEntry->RLength) {
	  PEntry->RStart=Entry->RStart; PEntry->RLength=Entry->RLength; NeuSchreiben=1;
	}
      }
      Entry++; PEntry++;
    }

    if (NeuSchreiben) {
      printf(" IWad changed... rewriting directory...\n");
      if (!Letzte) fseek(f,ll,0); else fseek(f,DirPos,0);
      PDirPos=ftell(f); PEntry=PE;
      for (i=0;i<PEntries;i++) {
	l=PEntry->RStart; _write(fileno(f),&l,4);
	l=PEntry->RLength; _write(fileno(f),&l,4);
	_write(fileno(f),PEntry->RName,8);
        PEntry++;
      }
      DirPos=ftell(f);
    }
    l=4; fseek(f,l,0);
    l=PEntries; _write(fileno(f),&l,4);  /* No of entries */
    l=PDirPos; _write(fileno(f),&l,4);   /* DirPos */
    fseek(f,DirPos,0); _write(fileno(f),&l,0); /* alte DirPos = End of file */
	 fflush(f); return 1;
  } else {
    printf(" Original IWAD directory NOT found.\n");
    return 0;
  }
}

int MergePWad(void)
{
  int i,SBeg=0,SEnd=0,k,m;
  long l,ll;
  char SpBeg[10],SpEnd[10];

  strcpy(SpBeg,"S_START"); strcpy(SpEnd,"S_END");
  OpenWADFile();

  _read(fileno(fa),Puffer,4);
  _read(fileno(fa),&l,4); PEntries=(int)l;
  _read(fileno(fa),&l,4); PDirPos=l;

  printf(" Reading %d entries...\n",PEntries);

  i=fseek(fa,l,0); if (i!=0) { printf(" Error reading input file.\n"); return 0; }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    _read(fileno(fa),&ll,4); PEntry->RStart=ll;
    _read(fileno(fa),&ll,4); PEntry->RLength=ll;
    _read(fileno(fa),PEntry->RName,8);
    PEntry->RName[8]=0; PEntry->Mark=0;
    PEntry++;
  }
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (strncmp(Entry->RName,"S_START",7)==0) { SBeg=i; break; }
    Entry++;
  }
  if (SBeg>0) {
    for (i=SBeg;i<Entries;i++) {
      if (strncmp(Entry->RName,"F_END",5)==0) { SEnd=i; break; }
      Entry++;
    }
  }
  if (SBeg==0 || SEnd==0) { printf(" Can't find start/end of Sprites/Flats.\n"); return 0; }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (strnicmp(PEntry->RName,"S_",2)==0 || strnicmp(PEntry->RName,"F_",2)==0) {
      strcpy(PEntry->RName,"");
    }
    Entry=E+SBeg;
    for (k=SBeg;k<SEnd;k++) {
      if (strcmp(Entry->RName,PEntry->RName)==0) {
	Entry->Mark=1; strcpy(Entry->RName,"");
	break;
      }
      Entry++;
    }
    PEntry++;
  }
  fseek(f,0,2); ll=ftell(f); /* New Begin Main Directory */
  Entry=E; m=0;
  for (i=0;i<Entries;i++) {
    l=Entry->RStart; _write(fileno(f),&l,4);
    l=Entry->RLength; _write(fileno(f),&l,4);
    _write(fileno(f),Entry->RName,8);
    Entry++;
  }
  Entry=E; strcpy(Entry->RName,"NWT"); Entry->RStart=DirPos;
  Entry->RLength=(long)Entries*16;
  l=Entry->RStart; _write(fileno(f),&l,4);
  l=Entry->RLength; _write(fileno(f),&l,4);
  _write(fileno(f),Entry->RName,8);

  m=Entries+1;

  l=4; fseek(f,l,0);
  l=(long)m; _write(fileno(f),&l,4); _write(fileno(f),&ll,4);
  fflush(f);

  PEntry=PE; m=0; fseek(fa,PDirPos,0);
  for (i=0;i<PEntries;i++) {
    if (strlen(PEntry->RName)>0) {
      l=PEntry->RStart; _write(fileno(fa),&l,4);
      l=PEntry->RLength; _write(fileno(fa),&l,4);
      _write(fileno(fa),PEntry->RName,8);
      m++;
    }
    PEntry++;
  }
  PEntry=E; strcpy(PEntry->RName,"S_END"); PEntry->RStart=0;
  PEntry->RLength=0;
  l=PEntry->RStart; _write(fileno(fa),&l,4);
  l=PEntry->RLength; _write(fileno(fa),&l,4);
  _write(fileno(fa),PEntry->RName,8);
  m++;
  PEntry=E; strcpy(PEntry->RName,"F_END"); PEntry->RStart=0;
  PEntry->RLength=0;
  l=PEntry->RStart; _write(fileno(fa),&l,4);
  l=PEntry->RLength; _write(fileno(fa),&l,4);
  _write(fileno(fa),PEntry->RName,8);
  m++;
  l=4; fseek(fa,l,0);
  l=(long)m; _write(fileno(fa),&l,4);
  fflush(fa); fclose(fa);
  return 1;
}

void AddSprites(void)
{
  int i,SBeg=0,SEnd=0,k,m,PSBeg=9999,PSEnd=9999;
  long l,ll;
  char SpBeg[10],SpEnd[10],a=0,ident[5]="PWAD";

  strcpy(SpBeg,"S_START"); strcpy(SpEnd,"S_END");
  OpenWADFile();

  _read(fileno(fa),Puffer,4);
  _read(fileno(fa),&l,4); PEntries=(int)l;
  _read(fileno(fa),&l,4);

  printf(" Reading %d entries...\n",PEntries);

  i=fseek(fa,l,0); if (i!=0) { printf(" Error reading input file.\n"); Ende(0); }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    _read(fileno(fa),&ll,4); PEntry->RStart=ll;
    _read(fileno(fa),&ll,4); PEntry->RLength=ll;
    _read(fileno(fa),PEntry->RName,8);
    PEntry->RName[8]=0; PEntry->Mark=0;
    PEntry++;
  }
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (strncmp(Entry->RName,"S_START",7)==0) { SBeg=i; break; }
    Entry++;
  }
  if (SBeg>0) {
    for (i=SBeg;i<Entries;i++) {
      if (strncmp(Entry->RName,"S_END",5)==0) { SEnd=i; break; }
      Entry++;
    }
  }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (strncmp(PEntry->RName,"S_START",7)==0) { PSBeg=i; break; }
    PEntry++;
  }
  if (PSBeg<9999) {
    for (i=PSBeg;i<PEntries;i++) {
      if (strncmp(PEntry->RName,"S_END",5)==0) { PSEnd=i; break; }
      PEntry++;
    }
  }
  if (PSEnd==9999) PSEnd=PEntries;
  if (SBeg==0 || SEnd==0) { printf(" Can't find start/end of Sprites in main IWad.\n"); Ende(0); }
  if (PSBeg==9999 || PSEnd==9999) { printf(" Can't find start/end of Sprites in PWad.\n Detecting Sprites... (Warning: errors possible!)\n"); PSBeg=9999; PSEnd=9999; }
  fc=fopen("NWT.TMP","wb"); if (fc==0) { printf(" Can't open output file.\n"); Ende(0); }
  _write(fileno(fc),&ident,4); _write(fileno(fc),Puffer,8);
  printf(" Writing non-sprite resources...\n");
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    a=0;
    if (i>PSBeg && i<PSEnd) {
      PEntry->Mark=1; a=1;
      Entry=E; Entry+=SBeg;
      for (k=SBeg;k<SEnd;k++) {
	if (strcmp(Entry->RName,PEntry->RName)==0) { Entry->Mark=1; PEntry->Mark=1; a=1; break; }
	Entry++;
      }
    }
    if (i==PSBeg || i==PSEnd) { PEntry->Mark=1; a=1; }
    if (PSBeg==9999 && PSEnd==9999) {
      if (!a) {
        Entry=E; Entry+=SBeg;
	for (k=SBeg;k<SEnd;k++) {
	  if (strcmp(Entry->RName,PEntry->RName)==0) { Entry->Mark=1; PEntry->Mark=1; a=1; break; }
	  Entry++;
	}
      }
    }
    if (!a) {
      l=PEntry->RStart; fseek(fa,l,0); l=PEntry->RLength; ll=ftell(fc);
      for (;;) {
	if (l>=32000) {
	  _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	} else {
	  _read(fileno(fa),Puffer,(int)l); _write(fileno(fc),Puffer,(int)l); break;
	}
      }
      PEntry->RStart=ll;
    }
    PEntry++;
  }
  Entry=E; Entry+=SBeg;
  printf(" Writing old sprite resources...\n");
  for (i=SBeg;i<SEnd;i++) {
    if (!Entry->Mark) {
      l=Entry->RStart; fseek(f,l,0); l=Entry->RLength; ll=ftell(fc);
	for (;;) {
	  if (l>=32000) {
	    _read(fileno(f),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	  } else {
	    _read(fileno(f),Puffer,l); _write(fileno(fc),Puffer,l); break;
	  }
	}
      Entry->RStart=ll;
    }
    Entry++;
  }
  PEntry=PE;
  printf(" Writing new sprite resources...\n");
  for (m=0,i=0;i<PEntries;i++,m++) {
    if (PEntry->Mark && PEntry->RLength>0) {
      l=PEntry->RStart; fseek(fa,l,0); l=PEntry->RLength; ll=ftell(fc);
	for (;;) {
	  if (l>=32000) {
	    _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	  } else {
	    if (l>0) { _read(fileno(fa),Puffer,l); _write(fileno(fc),Puffer,l); }
	    break;
	  }
	}
      PEntry->RStart=ll;
    }
    PEntry++;
  }
  m=0; ll=ftell(fc); /* Begin Resource Dir */
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (!PEntry->Mark) {
      l=PEntry->RStart; _write(fileno(fc),&l,4);
      l=PEntry->RLength; _write(fileno(fc),&l,4);
      _write(fileno(fc),PEntry->RName,8);
      m++;
    }
    PEntry++;
  }
  Entry=E; Entry+=SBeg;
  for (i=SBeg;i<SEnd;i++) {
    if (!Entry->Mark) {
      l=Entry->RStart; _write(fileno(fc),&l,4);
      l=Entry->RLength; _write(fileno(fc),&l,4);
      _write(fileno(fc),Entry->RName,8);
      m++;
    }
    Entry++;
  }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (PEntry->Mark && i!=PSBeg && i!=PSEnd) {
      l=PEntry->RStart; _write(fileno(fc),&l,4);
      l=PEntry->RLength; _write(fileno(fc),&l,4);
      _write(fileno(fc),PEntry->RName,8);
      m++;
    }
    PEntry++;
  }
  Entry=E; Entry+=SEnd;
  l=Entry->RStart; _write(fileno(fc),&l,4);
  l=Entry->RLength; _write(fileno(fc),&l,4);
  _write(fileno(fc),Entry->RName,8);
  m++;
  l=4; fseek(fc,l,0);
  l=(long)m; _write(fileno(fc),&l,4); _write(fileno(fc),&ll,4);
  fflush(fc); fclose(fc); fclose(fa); fclose(f);
  printf(" Done. %d resources processed.\n",m);
}

void AddFlats(void)
{
  int i,SBeg=0,SEnd=0,k,m,PSBeg=9999,PSEnd=9999;
  long l,ll;
  char SpBeg[10],SpEnd[10],a=0,ident[5]="PWAD";

  strcpy(SpBeg,"F_START"); strcpy(SpEnd,"F_END");
  OpenWADFile();

  _read(fileno(fa),Puffer,4);
  _read(fileno(fa),&l,4); PEntries=(int)l;
  _read(fileno(fa),&l,4);

  printf(" Reading %d entries...\n",PEntries);

  i=fseek(fa,l,0); if (i!=0) { printf(" Error reading input file.\n"); Ende(0); }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    _read(fileno(fa),&ll,4); PEntry->RStart=ll;
    _read(fileno(fa),&ll,4); PEntry->RLength=ll;
    _read(fileno(fa),PEntry->RName,8);
    PEntry->RName[8]=0; PEntry->Mark=0;
    PEntry++;
  }
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (strncmp(Entry->RName,"F_START",7)==0) { SBeg=i; break; }
    Entry++;
  }
  if (SBeg>0) {
    for (i=SBeg;i<Entries;i++) {
      if (strncmp(Entry->RName,"F_END",5)==0) { SEnd=i; break; }
      Entry++;
    }
  }
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (strncmp(PEntry->RName,"F_START",7)==0) { PSBeg=i; break; }
    PEntry++;
  }
  if (PSBeg<9999) {
    for (i=PSBeg;i<PEntries;i++) {
      if (strncmp(PEntry->RName,"F_END",5)==0) { PSEnd=i; break; }
      PEntry++;
    }
  }
  if (PSEnd==9999) PSEnd=PEntries;
  if (SBeg==0 || SEnd==0) { printf(" Can't find start/end of Flats in main IWad.\n"); Ende(0); }
  if (PSBeg==9999 || PSEnd==9999) { printf(" Can't find start/end of Flats in PWad.\n Detecting Flats... (Warning: errors possible!)\n"); PSBeg=9999; PSEnd=9999; }
  fc=fopen("NWT.TMP","wb"); if (fc==0) { printf(" Can't open output file.\n"); Ende(0); }
  _write(fileno(fc),&ident,4); _write(fileno(fc),Puffer,8);
  printf(" Writing non-flat resources...\n");
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    a=0;
    if (i>PSBeg && i<PSEnd) {
      PEntry->Mark=1; a=1;
      Entry=E; Entry+=SBeg;
      for (k=SBeg;k<SEnd;k++) {
	if (strcmp(Entry->RName,PEntry->RName)==0) { Entry->Mark=1; PEntry->Mark=1; a=1; break; }
	Entry++;
      }
    }
    if (i==PSBeg || i==PSEnd) { PEntry->Mark=1; a=1; }
    if (PSBeg==9999 && PSEnd==9999 && !a) {
      Entry=E; Entry+=SBeg;
      for (k=SBeg;k<SEnd;k++) {
	if (strcmp(Entry->RName,PEntry->RName)==0) { Entry->Mark=1; PEntry->Mark=1; a=1; break; }
	Entry++;
      }
    }
    if (!a) {
      l=PEntry->RStart; fseek(fa,l,0); l=PEntry->RLength; ll=ftell(fc);
	for (;;) {
	  if (l>=32000) {
	    _read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	  } else {
	    if (l>0) { _read(fileno(fa),Puffer,l); _write(fileno(fc),Puffer,l); }
	    break;
	  }
	}
      PEntry->RStart=ll;
    }
    PEntry++;
  }
  Entry=E; Entry+=SBeg;
  printf(" Writing flat resources...\n");
  for (i=SBeg;i<SEnd;i++) {
    if (!Entry->Mark) {
      l=Entry->RStart; fseek(f,l,0); l=Entry->RLength; ll=ftell(fc);
	for (;;) {
	  if (l>=32000) {
	    _read(fileno(f),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	  } else {
	    if (l>0) { _read(fileno(f),Puffer,l); _write(fileno(fc),Puffer,l); }
	    break;
	  }
	}
      Entry->RStart=ll;
    } else {
      PEntry=PE;
      for (k=0;k<PEntries;k++) {
	if (strcmp(Entry->RName,PEntry->RName)==0) {
	  l=PEntry->RStart; fseek(fa,l,0); l=PEntry->RLength; ll=ftell(fc);
	    for (;;) {
	      if (l>=32000) {
		_read(fileno(fa),Puffer,32000); _write(fileno(fc),Puffer,32000); l-=32000;
	      } else {
		if (l>0) { _read(fileno(fa),Puffer,l); _write(fileno(fc),Puffer,l); }
		break;
	      }
	    }
	  PEntry->RStart=ll;
	}
	PEntry++;
      }
    }
    Entry++;
  }
  m=0; ll=ftell(fc); /* Begin Resource Dir */
  PEntry=PE;
  for (i=0;i<PEntries;i++) {
    if (!PEntry->Mark) {
      l=PEntry->RStart; _write(fileno(fc),&l,4);
      l=PEntry->RLength; _write(fileno(fc),&l,4);
      _write(fileno(fc),PEntry->RName,8);
      m++;
    }
    PEntry++;
  }
  Entry=E; Entry+=SBeg;
  for (i=SBeg;i<=SEnd;i++) {
    if (!Entry->Mark) {
      l=Entry->RStart; _write(fileno(fc),&l,4);
      l=Entry->RLength; _write(fileno(fc),&l,4);
      _write(fileno(fc),Entry->RName,8);
      m++;
    } else {
      PEntry=PE;
      for (k=0;k<PEntries;k++) {
	if (strcmp(Entry->RName,PEntry->RName)==0) {
	  l=PEntry->RStart; _write(fileno(fc),&l,4);
	  l=PEntry->RLength; _write(fileno(fc),&l,4);
	  _write(fileno(fc),PEntry->RName,8);
	  m++;
	}
	PEntry++;
      }
    }
    Entry++;
  }
  l=4; fseek(fc,l,0);
  l=(long)m; _write(fileno(fc),&l,4); _write(fileno(fc),&ll,4);
  fflush(fc); fclose(fc); fclose(fa); fclose(f);
  printf(" Done. %d resources processed.\n",m);
}

int CleanWadFile(void)
{
  int i,y,k;
  long DateiLaenge,Summe=0,Diff,l,Pos,ll,NewStart,DatZeiger=0;
  char Taste;
  unsigned char ident[5]="PWAD";

  OpenWADFile(); DateiLaenge=filelength(fileno(f));
  Entry=E;
  for (i=0;i<Entries;i++) {
    l=Entry->RLength; Summe+=l;
    Entry++;
  }
  Summe+=12; Pos=Summe; l=(long)Entries*16; Summe+=l; Diff=DateiLaenge-Summe;
  if (Diff>0) printf(" There are %ld bytes unused stuff. Continue? (y/n)",Diff); else { printf(" There are 0 bytes unused stuff. Aborting.\n"); return -1; }
  Taste=getch(); printf("\n"); if (Taste!=89 && Taste!=121) return -1;
  fa=fopen("NWT.TMP","wb+");
  fseek(f,0,0); _read(fileno(f),&ident,4);
  _write(fileno(fa),&ident,4); _write(fileno(fa),&Entries,4);
  _write(fileno(fa),&Pos,4);
  printf(" Working on resource"); y=wherey();
  Entry=E; NewStart=ftell(fa); DatZeiger=ftell(f);
  for (i=0,ll=0;i<Entries;i++,ll++) {
    l=(long)(ll*100)/Entries; k=(int)l; if (i==Entries-1) k=100;
    gotoxy(23,y); printf("%4d (%3d%%)",i+1,k);
    l=Entry->RStart; if (l!=DatZeiger) { DatZeiger=l; fseek(f,l,0); }
    l=Entry->RLength; Entry->RStart=NewStart; NewStart+=l; DatZeiger+=l;
      for (;;) {
	if (l>=32000) {
	  _read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); l-=32000;
	} else {
	  _read(fileno(f),Puffer,l); _write(fileno(fa),Puffer,l); break;
	}
      }
    Entry++;
  }
  Entry=E;
  for (i=0;i<Entries;i++) {
    l=Entry->RStart; _write(fileno(fa),&l,4);
    l=Entry->RLength; _write(fileno(fa),&l,4);
    _write(fileno(fa),Entry->RName,8);
    Entry++;
  }
  l=filelength(fileno(fa));
  printf("\n Done. Size before:%ld  New size:%ld\n",DateiLaenge,l);
  fclose(fa);
  return 1;
}

