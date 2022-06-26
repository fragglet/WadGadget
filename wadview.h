#ifndef __WADVIEW_H_
#define __WADVIEW_H_

#include <stdio.h>


#define OUT_OF_MEMORY -10
#define BAD_CODE_SIZE -20
#define READ_ERROR -1
#define WRITE_ERROR -2
#define OPEN_ERROR -3
#define CREATE_ERROR -4
#define NULL   0L
#define MAX_CODES   4095


typedef struct {
  unsigned long RStart;
  unsigned long RLength;
  char RName[9];
  unsigned char Mark;
} Entr;


typedef struct {
  char Riff[4];
  unsigned long Laenge;
  char Wave[8];
  unsigned long Laenge2;
  unsigned int format,kanals;
} wavhead;

typedef struct {
  char Titel[20];
  int DataStart,Version,Id;
} vochead;

typedef struct {
  char FileName[13];
  unsigned long Date;
} Datdings;


typedef struct {
  char TName[9];
  unsigned long TStart;
  unsigned int UsedPName;
} TEntr;

typedef struct {
  char Name[9];
  unsigned int Num;
  unsigned int UsedInTex;
} PNam;

typedef struct {
  unsigned int EntryNr;
  int x,y;
} NewTextureData;

typedef struct _PcxKopf {
  char h,v,c,n;
  int l,t,r,b,x,y;
  char p[48],d,a;
  int e;
};

typedef struct {
  char id[6];
  unsigned int Gx,Gy;
  unsigned char Flag,Bg,par;
  char pal[768];
  unsigned char byte;
} gifkopp;

typedef struct {
  int xoffset,yoffset,gx,gy;
  unsigned char byte2;
} gifkopp2;


typedef struct {
  unsigned char Kennung,Version,Kodierung,BpP;
  unsigned int x1,y1,x2,y2,SizeX,SizeY;
  unsigned char Pal[48];
  unsigned char d,a;
  unsigned int BpZ;
  char Rest[60];
} pcxheader;


extern vochead VocKopf;
extern wavhead WavKopf;

extern char Endung[7][5];

extern char GExport,SExport, TExport,Setuppen,SB,SP,WinPal,PutIn1;
extern char Sort,SBAdr,InstrSindDa,WhichPal;
extern char Restore,Batch,ReadPNames,LoadIntern,Alternate,LevelData;

extern int TPos,TCPos,Tx,Ty,Tn,TEntries,SPos,NewTx,NewTy,PEntries,PTEntries,Marked;
extern long NTKilo;
extern char SuchStr[9],DateiName1[13],DateiName2[13],TexRName[9],OneName[13];
extern char TexOutFile[13],PWadName[13],WildCard[13],CurDir[150];
extern unsigned char Marking,MakeTex,Texes,EdPName,GetNewPName;
extern unsigned char MUSisPlaying,IsEndoom,ShareDoom,NWTisda;


extern char HGR;

extern int PEntries;

extern Entr *E,*Entry,*PEntry,*PE;

extern unsigned long Entries;
extern unsigned long PDirPos;

extern FILE *f,*fa,*fb, *fc, *ff;

extern char *Puffer;
extern unsigned long DirPos;


extern char DatName1[13];

extern _PcxKopf PcxKopf;


extern pcxheader Pcxkopf;



extern gifkopp gifkopf;
extern gifkopp2 gifkopf2;



extern int AltPos,AltCPos,Pos,CPos,Rx,Ry,Ox,Oy,ViewAble,PlayAble,MUSFile,NTCount,Dateien;

extern unsigned int PNames;

extern PNam PName[500];

extern TEntr TEntry[500];

extern TEntr PTEntry[500];

extern NewTextureData NTData[70];

extern unsigned int MarkedEntry[70];



extern char SuchStr[9],DateiName1[13],DateiName2[13],TexRName[9],OneName[13];


extern char SpSigs[10][9];

extern unsigned char ClearPal[768];

extern unsigned char DoomPal[768];


extern unsigned int B1Seg,B2Seg,B1Off,B2Off;

extern int gifx,gify;


extern int bad_code_count;


extern unsigned int B1Seg,B2Seg,B1Off,B2Off;



int OpenFile(char *WFile);
void GetSpeicher(void);
void MainSchleife(void);
void EditResource(void);
void InsertResource(void);
void CheckForShare(void);
void OpenWADFile(void);
void DosShell(void);
void PlayMUS(void);
void DeleteResource(void);
void DoItNow(unsigned char Taste);
void DoItReal(unsigned char Taste);
int GetOneName(void);
void Setup(void);
void DisplaySetup(int i);
void PlayResource(void);
void Choose64(void);
void DisplayInfo(void);
void Suchen(void);
void PNameSuchen(void);
void Ende(int code);
void RenameResource(void);


#endif