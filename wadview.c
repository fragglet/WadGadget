#include <process.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <dir.h>
#include <conio.h>
#include <stdlib.h>
#include <bios.h>
#include <time.h>
#include <sys\stat.h>


#include "addclean.h"
#include "adlib.h"
#include "dsp_rout.h"
#include "impexp.h"
#include "musplay.h"
#include "patches.h"
#include "pnames.h"
#include "prints.h"
#include "setpal.h"
#include "texture.h"
#include "wadview.h"



extern volatile unsigned char *MUSdata;
extern volatile unsigned long MUStime;
extern unsigned char *score;
extern unsigned char *instruments;

int bad_code_count;


vochead VocKopf;
wavhead WavKopf;

FILE *ff;
int gifx,gify;


_PcxKopf PcxKopf;

pcxheader Pcxkopf;

char DatName1[13];

char id[5]={32,32,32,32,0};
unsigned long Entries;
unsigned long DirPos;
unsigned long PDirPos;
unsigned int PNames=0;



gifkopp gifkopf;
gifkopp2 gifkopf2;


int AltPos,AltCPos,Pos=0,CPos=2,Rx,Ry,Ox,Oy,ViewAble=0,PlayAble=0,MUSFile=0,NTCount,Dateien;
int TPos=0,TCPos=2,Tx,Ty,Tn,TEntries,SPos=0,NewTx,NewTy,PEntries,PTEntries,Marked=0;
long NTKilo;
char SuchStr[9],DateiName1[13],DateiName2[13],TexRName[9],OneName[13];
char TexOutFile[13],PWadName[13],WildCard[13],CurDir[150];
unsigned char Marking=0,MakeTex=0,Texes=0,EdPName=0,GetNewPName=0;
unsigned char MUSisPlaying=0,IsEndoom=0,ShareDoom=0,NWTisda=0;
/* unsigned long uhr1,uhr2; */
char HGR=1;

PNam PName[500];
TEntr TEntry[500];
TEntr PTEntry[500];

unsigned int MarkedEntry[70];
NewTextureData NTData[70];

unsigned char Oldx,Oldy;
struct ftime FDatum;

unsigned char Palette1[48],Palette2[48];
unsigned char DoomPal[768]=
{0,  0,  0,  7,  5,  2,  5,  3,  1, 18, 18, 18, 63, 63, 63,  6, 6,  6,  4,  4,  4,    2,   2,   2,  1,  1,  1, 11, 13,  7,  8, 10,
 3,  5,  7,  1,  3,  5,  0, 19, 14, 10, 17, 12,  8, 15, 10,  6,
63, 45, 45, 61, 42, 42, 60, 40, 40, 58, 37, 37, 57, 35, 35, 55,
33, 33, 54, 30, 30, 52, 28, 28, 50, 26, 26, 49, 24, 24, 47, 22,22, 46, 21, 21, 44, 19, 19, 43, 17, 17, 41, 15, 15, 40, 14, 14,
38, 12, 12, 37, 11, 11, 35, 10, 10, 34,  8,  8, 32,  7,  7, 31, 6,  6, 29,  5,  5, 28,  4,  4, 26,  3,  3, 25,  2,  2, 23,  1,
 1, 22,  1,  1, 20,  1,  1, 19,  0,  0, 17,  0,  0, 16,  0,  0,63, 58, 55, 63, 56, 52, 63, 54, 49, 63, 52, 46, 63, 51, 44, 63,
49, 41, 63, 47, 38, 63, 46, 36, 63, 44, 32, 61, 42, 30, 59, 40,28, 57, 38, 26, 55, 36, 24, 53, 34, 22, 51, 32, 20, 50, 31, 19,
47, 30, 18, 44, 28, 17, 42, 27, 16, 40, 26, 15, 38, 24, 14, 35,23, 13, 33, 21, 12, 31, 20, 11, 29, 19, 10, 26, 17,  9, 23, 16,
 8, 20, 15,  7, 18, 13,  6, 15, 11,  5, 12, 10,  4, 10,  8,  3,59, 59, 59, 57, 57, 57, 55, 55, 55, 54, 54, 54, 52, 52, 52, 50,
50, 50, 49, 49, 49, 47, 47, 47, 45, 45, 45, 44, 44, 44, 42, 42,42, 41, 41, 41, 39, 39, 39, 37, 37, 37, 36, 36, 36, 34, 34, 34,
32, 32, 32, 31, 31, 31, 29, 29, 29, 27, 27, 27, 26, 26, 26, 24,24, 24, 22, 22, 22, 21, 21, 21, 19, 19, 19, 17, 17, 17, 16, 16,
16, 14, 14, 14, 13, 13, 13, 11, 11, 11,  9,  9,  9,  8,  8,  8,29, 63, 27, 27, 59, 25, 25, 55, 23, 23, 51, 21, 22, 47, 19, 20,
43, 17, 18, 39, 15, 16, 36, 13, 15, 32, 11, 13, 28, 10, 11, 24, 8,  9, 20,  6,  7, 16,  5,  5, 12,  3,  4,  8,  2,  2,  5,  1,
47, 41, 35, 45, 39, 33, 43, 37, 31, 41, 35, 29, 39, 33, 27, 38,31, 26, 36, 30, 24, 34, 28, 22, 32, 26, 21, 30, 24, 19, 29, 23,
18, 27, 21, 16, 25, 20, 15, 23, 18, 13, 21, 16, 12, 20, 15, 11,39, 32, 24, 35, 29, 20, 32, 26, 18, 29, 23, 15, 25, 20, 12, 22,
17, 10, 19, 14,  8, 16, 12,  6, 30, 31, 24, 27, 28, 21, 25, 26,19, 22, 24, 17, 20, 21, 14, 17, 19, 12, 15, 17, 10, 13, 15,  9,
63, 63, 28, 58, 54, 21, 53, 46, 16, 48, 38, 11, 43, 30,  7, 38,22,  4, 33, 16,  1, 28, 10,  0, 63, 63, 63, 63, 54, 54, 63, 46,
46, 63, 38, 38, 63, 30, 30, 63, 23, 23, 63, 15, 15, 63,  7,  7,63,  0,  0, 59,  0,  0, 56,  0,  0, 53,  0,  0, 50,  0,  0, 47,
 0,  0, 44,  0,  0, 41,  0,  0, 38,  0,  0, 34,  0,  0, 31,  0, 0, 28,  0,  0, 25,  0,  0, 22,  0,  0, 19,  0,  0, 16,  0,  0,
57, 57, 63, 49, 49, 63, 42, 42, 63, 35, 35, 63, 28, 28, 63, 20,20, 63, 13, 13, 63,  6,  6, 63,  0,  0, 63,  0,  0, 56,  0,  0,
50,  0,  0, 44,  0,  0, 38,  0,  0, 32,  0,  0, 26,  0,  0, 20,63, 63, 63, 63, 58, 54, 63, 53, 46, 63, 49, 38, 63, 44, 30, 63,
40, 22, 63, 35, 14, 63, 31,  6, 60, 28,  5, 58, 27,  3, 55, 25, 3, 53, 23,  2, 50, 21,  1, 48, 19,  0, 45, 17,  0, 43, 16,  0,
63, 63, 63, 63, 63, 53, 63, 63, 44, 63, 63, 35, 63, 63, 26, 63,63, 17, 63, 63,  8, 63, 63,  0, 41, 15,  0, 39, 13,  0, 36, 11,
 0, 33,  8,  0, 19, 14,  9, 16, 11,  6, 13,  8,  4, 11,  6,  2, 0,  0, 20,  0,  0, 17,  0,  0, 14,  0,  0, 11,  0,  0,  8,  0,
 0,  5,  0,  0,  2,    0, 63, 63,    63, 39, 16,  63, 57, 18,  63, 30,63,  63,  0, 63, 51,  0, 51, 39,  0, 38, 27,  0, 26, 41, 26, 26};

unsigned char HereticPal[768]=
{  4,  4,  4,   4,  4,  4,  11, 11, 11,  15, 15, 15,  17, 17, 17,  19, 19, 19,
  21, 21, 21,  22, 22, 22,  24, 24, 24,  26, 26, 26,  28, 28, 28,  30, 30, 30,
  32, 32, 32,  34, 34, 34,  35, 35, 35,  37, 37, 37,  39, 39, 39,  40, 40, 40,
  42, 42, 42,  43, 43, 43,  45, 45, 45,  46, 46, 46,  48, 48, 48,  49, 49, 49,
  50, 50, 50,  52, 52, 52,  53, 53, 53,  54, 54, 54,  56, 56, 56,  56, 56, 56,
  57, 57, 57,  58, 58, 58,  59, 59, 59,  60, 60, 60,  62, 62, 62,  63, 63, 63,
  23, 23, 23,  25, 26, 25,  28, 29, 28,  30, 31, 30,  33, 34, 32,  35, 35, 34,
  37, 38, 36,  39, 40, 38,  41, 42, 40,  42, 44, 42,  44, 46, 43,  46, 48, 45,
  48, 49, 47,  50, 51, 48,  51, 53, 50,  53, 54, 52,  13, 11, 19,  15, 15, 21,
  19, 19, 26,  23, 23, 31,  28, 28, 34,  33, 33, 39,  37, 37, 43,  40, 40, 46,
  45, 45, 50,  48, 48, 52,  51, 52, 54,  53, 54, 56,  56, 56, 57,  58, 58, 58,
  16, 11,  7,  19, 13,  9,  23, 16, 10,  26, 17, 10,  27, 18, 12,  29, 19, 13,
  31, 21, 13,  32, 22, 14,  34, 23, 16,  36, 25, 17,  38, 27, 18,  40, 29, 20,
  42, 31, 22,  44, 33, 23,  46, 35, 25,  47, 37, 27,  50, 39, 29,  51, 41, 31,
  53, 43, 33,  55, 44, 34,  57, 46, 36,  57, 47, 38,  58, 49, 41,  58, 50, 43,
  59, 51, 45,  59, 53, 48,  60, 54, 50,  61, 55, 52,  61, 57, 54,  26, 20,  9,
  29, 23, 11,  32, 25, 14,  34, 27, 17,  36, 29, 19,  37, 31, 22,  40, 34, 24,
  43, 37, 26,  45, 39, 29,  48, 41, 32,  49, 43, 33,  51, 45, 35,  52, 46, 37,
  54, 48, 39,  55, 49, 41,  56, 50, 42,  19, 13,  5,  22, 15,  5,  25, 16,  6,
  28, 18,  5,  30, 19,  4,  32, 20,  2,  35, 22,  4,  38, 24,  2,  41, 26,  6,
  44, 29,  8,  46, 31, 12,  48, 34, 13,  49, 36, 15,  51, 38, 17,  52, 40, 13,
  55, 43, 15,  58, 45, 16,  60, 47, 14,  62, 50, 22,  63, 52, 25,  63, 53, 28,
  63, 55, 31,  63, 56, 35,  63, 57, 37,  63, 58, 40,  63, 59, 43,  47, 23,  5,
  49, 27,  4,  52, 32,  5,  57, 39,  5,  60, 48,  4,  62, 53,  4,  63, 57, 21,
  63, 63,  2,  27,  2,  2,  30,  2,  2,  33,  2,  2,  36,  2,  2,  38,  2,  2,
  41,  2,  2,  43,  2,  2,  46,  2,  2,  49,  2,  2,  51,  2,  2,  53,  2,  2,
  55,  2,  2,  57,  2,  2,  59,  2,  2,  61,  2,  2,  63,  2,  2,  63, 23, 23,
  63, 29, 29,  63, 34, 34,  63, 40, 40,  63, 46, 46,  63, 51, 51,  63, 55, 55,
  63, 57, 57,  26, 10, 33,  31,  7, 37,  36,  7, 41,  41,  2, 44,  46,  2, 50,
  52,  2, 58,  57, 21, 63,  58, 40, 61,  19,  6, 41,  26, 18, 45,  31, 26, 48,
  36, 33, 51,  41, 39, 55,  46, 44, 58,  50, 49, 61,  54, 54, 63,   4,  5, 20,
   4,  6, 23,   6,  7, 25,   4,  6, 27,   4,  6, 30,   2,  5, 33,   2,  5, 34,
   2,  5, 36,   4,  6, 40,   4,  6, 43,   6,  8, 47,  10, 11, 52,  17, 20, 54,
  25, 26, 58,  31, 31, 63,  31, 37, 63,  31, 43, 63,  31, 48, 63,  31, 54, 63,
  29, 58, 63,  31, 60, 63,  32, 63, 63,  46, 63, 63,  52, 63, 62,   9, 14,  7,
  13, 18,  9,  14, 23, 11,  17, 27, 14,  20, 31, 16,  22, 35, 18,  24, 38, 21,
  26, 42, 22,  27, 45, 24,  29, 47, 26,  31, 50, 28,  33, 53, 30,  34, 56, 32,
  36, 58, 34,  38, 61, 36,  39, 63, 38,  14, 17, 14,  16, 18, 16,  17, 21, 17,
  18, 23, 18,  21, 24, 21,  22, 26, 22,  23, 28, 23,  25, 29, 24,  26, 31, 25,
  27, 33, 27,  29, 34, 28,  30, 36, 29,  32, 38, 30,  33, 38, 31,  34, 40, 32,
  36, 42, 34,  63, 58,  2,  63, 53,  2,  63, 47,  2,  63, 41,  2,  63, 34,  2,
  63, 26,  2,  62, 10,  4,  24,  2,  2,  22,  2,  2,  20,  2,  2,  14,  2,  2,
  11, 11, 11,   9,  9,  9,   7,  7,  7,  63, 63, 63
};

unsigned char ScreenPal[768]=
{0,  0,  0,   0,  0, 25,   0, 42,  0,   0, 42, 42,  42,  0,  0,  42,  0, 42,
42, 42,  0,  34, 34, 34,   0,  0, 21,   0,  0, 63,   0, 42, 21,   0, 42, 63,
42,  0, 21,  42,  0, 63,  42, 42, 21,  57, 57, 57,  63, 45, 45,  61, 42, 42,
60, 40, 40,  58, 37, 37,  57, 35, 35,  55, 33, 33,  54, 30, 30,  52, 28, 28,
50, 26, 26,  49, 24, 24,  47, 22, 22,  46, 21, 21,  44, 19, 19,  43, 17, 17,
41, 15, 15,  40, 14, 14,  38, 12, 12,  37, 11, 11,  35, 10, 10,  34,  8,  8,
32,  7,  7,  31,  6,  6,  29,  5,  5,  28,  4,  4,  26,  3,  3,  25,  2,  2,
23,  1,  1,  22,  1,  1,  20,  1,  1,  19,  0,  0,  17,  0,  0,  16,  0,  0,
63, 58, 55,  63, 56, 52,  63, 54, 49,  63, 52, 46,  63, 51, 44,  63, 49, 41,
63, 47, 38,  55, 38, 28,   0,  0, 13,   0,  0, 55,   0, 34, 13,   0, 34, 55,
34,  0, 13,  34,  0, 55,  34, 34, 13,  55, 55, 55,   47, 30, 18,  44, 28, 17,
42, 27, 16,  40, 26, 15,  38, 24, 14,  35, 23, 13,  33, 21, 12,  31, 20, 11,
29, 19, 10,  26, 17,  9,  23, 16,  8,  20, 15,  7,  18, 13,  6,  15, 11,  5,
12, 10,  4,  10,  8,  3,  59, 59, 59,  57, 57, 57,  55, 55, 55,  54, 54, 54,
52, 52, 52,  50, 50, 50,  49, 49, 49,  47, 47, 47,  45, 45, 45,  44, 44, 44,
42, 42, 42,  41, 41, 41,  39, 39, 39,  37, 37, 37,  36, 36, 36,  34, 34, 34,
32, 32, 32,  31, 31, 31,  29, 29, 29,  27, 27, 27,  26, 26, 26,  24, 24, 24,
22, 22, 22,  21, 21, 21,  19, 19, 19,  17, 17, 17,  16, 16, 16,  14, 14, 14,
13, 13, 13,  11, 11, 11,   9,  9,  9,   8,  8,  8,  29, 63, 27,  27, 59, 25,
25, 55, 23,  23, 51, 21,  22, 47, 19,  20, 43, 17,  18, 39, 15,  16, 36, 13,
15, 32, 11,  13, 28, 10,  11, 24,  8,   9, 20,  6,   7, 16,  5,   5, 12,  3,
 4,  8,  2,   2,  5,  1,  47, 41, 35,  45, 39, 33,  43, 37, 31,  41, 35, 29,
39, 33, 27,  38, 31, 26,  36, 30, 24,  34, 28, 22,  32, 26, 21,  30, 24, 19,
29, 23, 18,  27, 21, 16,  25, 20, 15,  23, 18, 13,  21, 16, 12,  20, 15, 11,
39, 32, 24,  35, 29, 20,  32, 26, 18,  29, 23, 15,  25, 20, 12,  22, 17, 10,
19, 14,  8,  16, 12,  6,  30, 31, 24,  27, 28, 21,  25, 26, 19,  22, 24, 17,
20, 21, 14,  17, 19, 12,  15, 17, 10,  13, 15,  9,  63, 63, 28,  58, 54, 21,
53, 46, 16,  48, 38, 11,  43, 30,  7,  38, 22,  4,  33, 16,  1,  28, 10,  0,
63, 63, 63,  63, 54, 54,  63, 46, 46,  63, 38, 38,  63, 30, 30,  63, 23, 23,
63, 15, 15,  63,  7,  7,  63,  0,  0,  59,  0,  0,  56,  0,  0,  53,  0,  0,
50,  0,  0,  47,  0,  0,  44,  0,  0,  41,  0,  0,  38,  0,  0,  34,  0,  0,
31,  0,  0,  28,  0,  0,  25,  0,  0,  22,  0,  0,  19,  0,  0,  16,  0,  0,
57, 57, 63,  49, 49, 63,  42, 42, 63,  35, 35, 63,  28, 28, 63,  20, 20, 63,
13, 13, 63,   6,  6, 63,   0,  0, 63,   0,  0, 56,   0,  0, 50,   0,  0, 44,
 0,  0, 38,   0,  0, 32,   0,  0, 26,   0,  0, 20,  63, 63, 63,  63, 58, 54,
63, 53, 46,  63, 49, 38,  63, 44, 30,  63, 40, 22,  63, 35, 14,  63, 31,  6,
60, 28,  5,  58, 27,  3,  55, 25,  3,  53, 23,  2,  50, 21,  1,  48, 19,  0,
45, 17,  0,  43, 16,  0,  63, 63, 63,  63, 63, 53,  63, 63, 44,  63, 63, 35,
63, 63, 26,  63, 63, 17,  63, 63,  8,  63, 63,  0,  41, 15,  0,  39, 13,  0,
36, 11,  0,  33,  8,  0,  19, 14,  9,  16, 11,  6,  13,  8,  4,  11,  6,  2,
 0,  0, 20,   0,  0, 17,   0,  0, 14,   0,  0, 11,   0,  0,  8,   0,  0,  5,
 0,  0,  2,   0, 63, 63,  63, 39, 16,  63, 57, 18,  63, 30, 63,  63,  0, 63,
51,  0, 51,  39,  0, 38,  27,  0, 26,  41, 26, 26};

unsigned char ClearPal[768];

char Sigs[31][5]={
"DS","DP","POSS","SPOS","CPOS","TROO","SARG","SKUL","HEAD","BOS2","BOSS",
"BSPI","PAIN","SKEL","FATT","VILE","FIRE","SPID","CYBR","SSWV","PLAY",
"WILV","CWIL","PUNG","MIS","SHT","PLS","BFG","PIS","SAW","CHG"};
char SigNames[31][20]={
"","","Zombieman","Shotgun Guy","Heavy Weapon Dude","Imp","Demon","Lost Soul",
"Cacodemon","Hell Knight","Baron Of Hell","Arachnotron","Pain Elemental",
"Revenant","Mancubus","Arch-Vile","Arch-Vile-Attack","Spider Mastermind",
"Cyber Demon","SS Guy","Player","Level Title","Level Title",
"Fist","Rocket-launcher","Shotgun","Plasma Gun","BFG 9000",
"Pistol","Chainsaw","Chaingun"};

char SpSigs[10][9]={
"THINGS","LINEDEFS","SIDEDEFS","VERTEXES","SEGS","SSECTORS","NODES",
"SECTORS","REJECT","BLOCKMAP"};

char Endung[7][5]={".GIF",".PCX",".WAV",".VOC",".WAD",".RAW",".TXT"};

char Setups[10][60]={
" ³ Primary graphic format: [ ] GIF  [ ] PCX           ³ "," ³ Primary sound format:   [ ] WAV  [ ] VOC           ³ "," ³ Primary texture format: [ ] WAD  [ ] RAW  [ ] TXT  ³ ",
" ³ Sound output device:    [ ] SB   [ ] SP   [ ] None ³ "," ³ Window & real palette:  [ ] NO   [ ] YES           ³ "," ³ Put marked to ONE PWad: [ ] NO   [ ] YES           ³ ",
" ³ Directory sort:         [ ] NAME [ ] DATE          ³ "," ³ Marking color:          [ ] BLUE [ ] RED           ³ "," ³ SB Base Adress:         [ ] 220h [ ] 240h          ³ ",
" ³ Palette choice:         [ ] DOOM [ ] HERETIC       ³ "};

char GExport=1,SExport=2,TExport=1,Setuppen=0,SB=0,SP=0,WinPal=1,PutIn1=2;
char Sort=1,SBAdr=0,InstrSindDa=1,WhichPal=0;
char Restore=0,Batch=0,ReadPNames=1,LoadIntern=0,Alternate=0,LevelData=0;

FILE *f,*g,*fa,*fb,*fc;
Entr *E,*Entry,*PEntry,*PE;
Datdings *Datai,*Da;
char *Bild,*Bild2,*OldScr,*OS,*Puffer;
unsigned int B1Seg,B2Seg,B1Off,B2Off;

unsigned int CTRL=4,ALT=8;
/*

#include "sg.c"
#include "patch.c"
#include "gifpcx.c"
#include "dsp_rout.c"
#include "impexp.c"
#include "texture.c"
#include "addclean.c"
#include "pnames.c"
*/

int main(int argc,char *argv[])
{
  int i=0,i2;
  long l;
/*  long l1; */
/*  unsigned char Taste,a,b,c,ro,gr,bl; */
  unsigned int Seg,Off;

  f=fopen(argv[0],"rb"); if (f==0) i=1;
  if (!i) {
    l=filelength(fileno(f)); fclose(f);
	 if (l!=63489) i=1;
  }

  E=(Entr *)calloc(3500,sizeof(Entr));
  if (E==NULL) Ende(3);
  Entry=E;

  PE=(Entr *)calloc(3500,sizeof(Entr));
  if (PE==NULL) { free(E); Ende(3); }
  PEntry=PE;

  OldScr=(char *)malloc(4000);
  if (OldScr==NULL) { free(PE); free(E); Ende(3); }
  OS=OldScr;

  if (!i) {
    printf("ð Ahrrg! EXE changed! Program terminated.\n");
    free(Datai); free(OldScr); free(PE); free(E); Ende(4);
  }

  memset(ClearPal,0,768);
  TexOutFile[0]=0;
  PWadName[0]=0;
  getcurdir(0,CurDir);

  fa=fopen("NWT.CFG","rb");
  if (fa==0) {
    Setuppen=1;
  } else {
    _read(fileno(fa),&GExport,1); _read(fileno(fa),&SExport,1);
    _read(fileno(fa),&TExport,1); _read(fileno(fa),&SB,1);
    _read(fileno(fa),&SP,1); _read(fileno(fa),&WinPal,1);
    _read(fileno(fa),&PutIn1,1); _read(fileno(fa),&Sort,1);
    _read(fileno(fa),&HGR,1); _read(fileno(fa),&SBAdr,1);
    _read(fileno(fa),&WhichPal,1); fclose(fa);
    if (!SBAdr) BASE=0x220; else BASE=0x240;
  }
  if (WinPal==1) {
    DoomPal[741]=0; DoomPal[742]=0; DoomPal[743]=0;
    HereticPal[741]=0; HereticPal[742]=0; HereticPal[743]=0;
  } else {
    DoomPal[741]=0; DoomPal[742]=63; DoomPal[743]=63;
    HereticPal[741]=0; HereticPal[742]=63; HereticPal[743]=63;
  }

  printf("ð NWT v1.3 (c) by TiC 1/95 ð NewWadTool for Doom, Doom II & Heretic ð\n");
  Oldx=wherex(); Oldy=wherey();

  fa = fopen("GENMIDI.OP2", "rb");
  if (fa==NULL) InstrSindDa=0;  else {
    if (readINS(fa)) {
      fclose(fa); InstrSindDa=0;
    }
    fclose(fa);
  }

  if (argc>1) {
    ReadPNames=0; Batch=1;
    for (i=1;i<argc;i++) {
      if (strcmp("-?",argv[i])==0) break;
      if (strcmp("-join",argv[i])==0) {
	ReadPNames=1;
	i2=OpenFile(argv[i+1]);
	if (i2!=0) {
	  OpenWADFile(); PDirPos=DirPos; fclose(f);
	  i2=OpenFile(argv[i+2]);
	  if (i2!=0) {
	    Entry=E; PEntry=PE;
	    for (i2=0;i2<Entries;i2++) {
	      strcpy(PEntry->RName,Entry->RName);
	      l=Entry->RStart; PEntry->RStart=l;
			l=Entry->RLength; PEntry->RLength=l;
	      PEntry->Mark=0;
	      Entry++; PEntry++;
	    }
		 PEntries=Entries;
	  }
	  ReadPNames=0; OpenWADFile(); fa=fopen(argv[i+1],"rb+"); i2=1;
	}
	if (i2==0) { printf("ð Error reading WAD file.\n"); Ende(0); }
	GetSpeicher(); JoinPWads(); fclose(f); fclose(fa); Ende(0);
      }
      if (strcmp("-restore",argv[i])==0) {
	i=OpenFile("DOOM2.WAD");
	if (i==0) {
	  i=OpenFile("DOOM.WAD");
	  if (i==0) {
	    i=OpenFile("HERETIC.WAD");
	    if (i==0) { printf("ð Error reading main WAD file.\n"); Ende(0); }
	  }
	}
	i=RestoreIWad();
	if (i>0) {
	  setftime(fileno(f),&FDatum); fclose(f);
	  printf("ð IWad restoring successful.\n");
	} else {
	  setftime(fileno(f),&FDatum); fclose(f);
	  printf("ð IWad restoring failed.\n");
	}
	Ende(0);
      }
      if (strcmp("-merge",argv[i])==0) {
	i=OpenFile("DOOM2.WAD");
	if (i==0) {
	  i=OpenFile("DOOM.WAD");
	  if (i==0) {
	    i=OpenFile("HERETIC.WAD");
	    if (i==0) { printf("ð Error reading main WAD file.\n"); Ende(0); }
	  }
	}
	if (strlen(argv[i+1])>4) {
	  fa=fopen(argv[i+1],"rb+");
	  if (fa==0) printf("ð Error reading input file.\n"); else {
	    GetSpeicher(); i=MergePWad();
	    if (i>0) {
              setftime(fileno(f),&FDatum); fclose(f);
	      printf("ð PWad merging successful. Use -restore to restore your IWad.\n");
	      printf("ð Don't forget to add your WAD-file when starting the game now!\n");
	    } else {
              setftime(fileno(f),&FDatum); fclose(f);
	      printf("ð PWad merging failed.\n");
	    }
	  }
	} else printf("ð Invalid input file.\n");
	Ende(0);
      }
		if (strcmp("-c",argv[i])==0) { /* CLEAN */
	if (strlen(argv[i+1])>4) {
	  i=OpenFile(argv[i+1]);
	  if (i==0) printf("ð Error reading input file.\n"); else {
	    GetSpeicher(); i=CleanWadFile(); setftime(fileno(f),&FDatum); fclose(f);
	    if (i>0) { unlink(argv[i+1]); rename("NWT.TMP",argv[i+1]); }
	  }
	} else printf("ð Invalid input file.\n");
	Ende(0);
      }
      if (strcmp("-as",argv[i])==0) { /* ADD SPRITES */
	i=OpenFile("DOOM2.WAD");
	if (i==0) {
	  i=OpenFile("DOOM.WAD");
	  if (i==0) {
	    i=OpenFile("HERETIC.WAD");
	    if (i==0) { printf("ð Error reading main WAD file.\n"); Ende(0); }
	  }
	}
	if (strlen(argv[i+1])>4) {
	  fa=fopen(argv[i+1],"rb"); if (fa==0) { printf("ð Error reading input file.\n"); Ende(0); }
	  GetSpeicher(); AddSprites(); unlink(argv[i+1]); rename("NWT.TMP",argv[i+1]);
	} else printf("ð Invalid in/output file.\n");
        Ende(0);
      }
		if (strcmp("-af",argv[i])==0) { /* ADD FLATS */
	i=OpenFile("DOOM2.WAD");
	if (i==0) {
	  i=OpenFile("DOOM.WAD");
	  if (i==0) {
	    i=OpenFile("HERETIC.WAD");
	    if (i==0) { printf("ð Error reading main WAD file.\n"); Ende(0); }
	  }
	}
	if (strlen(argv[i+1])>4) {
	  fa=fopen(argv[i+1],"rb"); if (fa==0) { printf("ð Error reading input file.\n"); Ende(0); }
	  GetSpeicher(); AddFlats(); unlink(argv[i+1]); rename("NWT.TMP",argv[i+1]);
	} else printf("ð Invalid in/output file.\n");
        Ende(0);
      }
		if (strcmp("-file",argv[i])==0) { /* LOAD ALTERNATE WAD */
	if (strlen(argv[i+1])>4) {
	  i=OpenFile(argv[i+1]); if (i==0) { printf("ð Error reading input file.\n"); Ende(0); }
	  if (strnicmp(argv[i+1],"DOOM1",5)==0) ShareDoom=1;
	  if (strnicmp(argv[i+1],"HERETIC1",8)==0) ShareDoom=1;
	  Alternate=1; ReadPNames=1; OpenWADFile(); Batch=0;
	  break;
	} else printf("ð Invalid input file.\n");
	Ende(0);
      }
    }
  }
  if (Batch==1) {
    printf("\nValid commands are:\n");
    printf(" NWT -file wadfile.wad    Use alternate WAD file.\n NWT -join 1.wad 2.wad    Combines two PWads.\n NWT -c wadfile.wad       WAD cleaner.\n");
    printf(" NWT -as wadfile.wad      Add sprites to PWAD.\n NWT -af wadfile.wad      Add flats to PWAD.\n");
    printf(" NWT -merge wadfile.wad   Merge a PWAD into your IWAD file. Will only combine\n                          both directories.\n                          Note: You must add this PWAD when starting!\n");
    printf(" NWT -restore             Restores your IWAD after merging.\n");
    printf(" NWT -?                   Show this help.\n\nOr just copy NWT in your Doom directory and start without parameters.\n");
    printf(" e-mail: denis@doomsday.shnet.org\n");
    Ende(0);
  }

  Restore=1; memcpy(OldScr,MK_FP(0xb800,0),4000);

  if (!Alternate) {
    i=OpenFile("DOOM2.WAD");
    if (i==0) {
      i=OpenFile("DOOM.WAD");
      if (i==0) {
	i=OpenFile("HERETIC.WAD");
	if (i==0) Ende(2);
      }
    }
    OpenWADFile();    /* Einlesen des Resource-Directory */
  }

  for (i=0;i<4000;i++) pokeb(0xb800,i,0);

  Seg=FP_SEG(ScreenPal);
  Off=FP_OFF(ScreenPal);
  SetPal(Seg,Off);
  ScreenAufbau(); Display();
  if (Setuppen) Setup();
  MainSchleife();
  Ende(0);

  return 0;
}



int OpenFile(char *WFile)
{
  int i=0;

  i=chmod(WFile,S_IREAD | S_IWRITE);
  if (i) return 0;
  f=fopen(WFile,"rb+");
  if (f==0) return 0; else return 1;
}

void GetSpeicher(void)
{
  Datai=(Datdings *)calloc(1500,sizeof(Datdings));
  if (Datai==NULL) { free(OldScr); free(PE); free(E); Ende(3); }
  Da=Datai;

  Bild=(char *)malloc(64000);
  if (Bild==NULL) { free(OldScr); free(Datai); free(PE); free(E); Ende(3); }

  Bild2=(char *)malloc(64000);
  if (Bild2==NULL) { free(OldScr); free(Datai); free(PE); free(E); free(Bild); Ende(3); }

  Puffer=(char *)malloc(32000);
  if (Puffer==NULL) { free(Datai); free(OldScr); free(Bild2); free(PE); free(E); free(Bild); Ende(3); }

  B1Seg=FP_SEG(Bild); B2Seg=FP_SEG(Bild2);
  B1Off=FP_OFF(Bild); B2Off=FP_OFF(Bild2);
}

void MainSchleife(void)
{
  int i;
  unsigned char Taste=0,Ente=0,Bios;

  if (!EdPName) GetSpeicher();
  for(;;) {
    if (Entries==0) break;
    if (MUSisPlaying) {
      Print(77,1,1,15+128,"\x0d");
      if (!MUSdata) {
	ShutdownTimer();
	DeinitAdlib();
	MUSisPlaying=0; Print(77,1,1,15," ");
	free(score); /* free(instruments); */
      }
    }
    Taste=0; Bios=0;
    Taste=getch(); Bios=bioskey(2);
/*    gotoxy(1,1); printf("Taste:%d Bios:%3d ",Taste,Bios); */
    if (Taste==27) break;
    if (Taste==73) { Pos-=21; if (Pos<0) { Pos=0; CPos=2; } Display(); Taste=0; }
    if (Taste==81) {
      Pos+=21; if (Pos>Entries) Pos-=21;
      if (Pos>Entries-22) { Pos=Entries-22; CPos=23; }
      if (Pos<0) { Pos=0; CPos=2; }
      Display(); Taste=0;
    }
    if (Taste==72 && CPos==2 && Pos>0) { Pos--; Display(); Taste=0; }
    if (Taste==80 && CPos==23) { Pos++; if (Pos>Entries-22) Pos=Entries-22; Display(); Taste=0; }
    if (Taste==72 && CPos==2 && Pos==0) Taste=0;
    if (Taste==80 && Pos+CPos<Entries+1) { CPos++; Display(); Taste=0; }
    if (Taste==72) { CPos--; Display(); Taste=0; }
    if (Taste==79) { Pos=Entries-22; CPos=23; if (Pos<0) { Pos=0; CPos=Entries+1; } Display(); }
    if (Taste==71) { Pos=0; CPos=2; Display(); }
    if (Taste==13) {
      if (!EdPName) {
	if (strnicmp(Entry->RName,"TEXT",4)==0) {
	  Ente=1; strcpy(TexRName,Entry->RName);
	  AltPos=Pos; AltCPos=CPos; SuchStr[0]=0; SPos=0; Patches(); Pos=AltPos; CPos=AltCPos;
	}
	if (strnicmp(Entry->RName,"PNAMES",6)==0) {
	  Ente=1; AltPos=Pos; AltCPos=CPos; SuchStr[0]=0; SPos=0; EditPNames(); Pos=AltPos; CPos=AltCPos;
	}
      }
      if (!Ente) {
	if (!ViewAble && !PlayAble && !MUSFile && !IsEndoom) Error(1);
	if (ViewAble) GetPic();
	if (PlayAble && (SB || SP)) PlayResource();
	if (MUSFile) PlayMUS();
	if (IsEndoom) DisplayEndoom();
      }
      Display(); Ente=0;
    }
  if (!EdPName) {    /* Wird nur abgefragt, wenn EdPName=0 ist */
	 if (Taste==45 && !(Bios&ALT) ) {
      i=Entries;
      if (Marked==0) Entry->Mark=1;
      DeleteResource(); Taste=0; if (Entries==0) break;
      i-=Entries;
      if (Pos+CPos-2>=Entries) CPos-=i;
      if (CPos<2) { CPos=2; Pos-=i; }
      if (Pos<0) Pos=0;
      ScreenAufbau(); Display();
    }
	 if ( (Taste==18 || Taste==69 || Taste==101) && Bios&ALT) { EditResource(); Taste=0; }
	 if ( (Taste==19 || Taste==82 || Taste==114) && Bios&ALT) { RenameResource(); Taste=0; }
    if (Taste==82) { InsertResource(); Taste=0; }
	 if ( (Taste==46 || Taste==67 || Taste==99) && Bios&ALT) { DosShell(); Taste=0; }
	 if ( (Taste==31 || Taste==115 || Taste==83) && Bios&ALT) Taste=133;  /* Alt-S - Setup */
    if (Taste==133) Setup();
	 if ( (Taste==38 || Taste==108 || Taste==76) && Bios&ALT) {  /* Alt-L - Load PWAD as IWAD */
      Taste=0;
      for (;;) {
	Box(1);
	Print(29,10,5,15," Load new PWAD (IWAD) ");
	Print(32,11,5,15,"Infile:");
	strcpy(DateiName1,"*.WAD"); Print(39,11,5,0,DateiName1);
	if (Eingabe1(39,11,5,0)<0) break; else CWeg();
	if (!strchr(DateiName1,'.')) strcat(DateiName1,".WAD");
	fa=fopen(DateiName1,"rb");
	if (fa==0) Error(2); else {
	  fclose(fa); setftime(fileno(f),&FDatum); fclose(f); i=OpenFile(DateiName1);
	  ReadPNames=1; LoadIntern=1; OpenWADFile(); break;
	}
      }
      Pos=0; CPos=2; ScreenAufbau(); Display();
      if (ShareDoom) break;
    }
	 if ( (Taste==20 || Taste==116 || Taste==84) && Bios&ALT) {  /* Alt-T - Textures */
      AltPos=Pos; AltCPos=CPos; Entry=E;
      for (Ente=0,i=0;i<Entries;i++) {
	if (strnicmp(Entry->RName,"TEXT",4)==0) { Ente=1; break; }
	Entry++;
      }
      if (!Ente) {
	Pos=AltPos; CPos=AltCPos; Display(); Error(8);
      } else {
	Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	Display(); Ente=0;
      }
      Taste=0;
    }
	 if ( (Taste==25 || Taste==112 || Taste==80) && Bios&ALT) {  /* Alt-P - PNames */
      AltPos=Pos; AltCPos=CPos; Entry=E;
      for (Ente=0,i=0;i<Entries;i++) {
	if (strnicmp(Entry->RName,"PNAMES",6)==0) { Ente=1; break; }
	Entry++;
      }
      if (!Ente) {
	Pos=AltPos; CPos=AltCPos; Display(); Error(13);
      } else {
	Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	Display(); Ente=0;
      }
      Taste=0;
    }
    if (Taste==59) HexDump();      /* F1 - Hex Dump */
    if (Taste==60) {               /* F2 - Export 2 Raw */
      if (Marked>0) {
	Entry=E; AltPos=Pos; AltCPos=CPos;
	for (i=0;i<Entries;i++) {
	  if (Entry->Mark==1) {
	    Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	    if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	    Display(); SaveRaw();
	  }
	  Entry++;
	}
	Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2); Display();
      } else SaveRaw();
    }
    if (Taste==61) {               /* F3 - Export 2 Gif/Wav */
      if (Marked>0) {
	Entry=E; AltPos=Pos; AltCPos=CPos;
	for (i=0;i<Entries;i++) {
	  if (Entry->Mark==1) {
	    Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	    if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	    Display();
	    if (ViewAble) SavePic();
	    if (PlayAble) SaveVoc();
	    if (!ViewAble && !PlayAble) Error(4);
	  }
	  Entry++;
	}
	Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2); Display();
      } else {
	if (ViewAble) SavePic();
	if (PlayAble) SaveVoc();
        if (!ViewAble && !PlayAble) Error(4);
      }
    }
    if (Taste==62) {               /* F4 - Export 2 Pwad */
      AltPos=Pos; AltCPos=CPos;
      if (Marked>0) {
	i=1;
	if (PutIn1==2) i=GetOneName();
	if (i>0) {
	  Entry=E;
	  for (i=0;i<Entries;i++) {
	    if (Entry->Mark==1) {
	      Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	      if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	      Display(); Export2Pwad();
	    }
	    Entry++;
	  }
	} else {
	  ScreenAufbau(); Display();
	}
      } else Export2Pwad();
      Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2);
      ScreenAufbau(); Display();
    }
    if (Taste==63) {               /* F5 - Import Raw 2 Iwad */
      if (Marked>0) {
	Entry=E; AltPos=Pos; AltCPos=CPos;
	for (i=0;i<Entries;i++) {
	  if (Entry->Mark==1) {
	    Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	    if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	    Display(); ImportRaw2Iwad();
	  }
	  Entry++;
	}
	Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2); Display();
      } else ImportRaw2Iwad();
    }
    if (Taste==64) {               /* F6 - Import Raw 2 Pwad */
      if (Marked>0) {
	i=1;
	if (PutIn1==2) i=GetOneName();
	if (i>0) {
	  Entry=E; AltPos=Pos; AltCPos=CPos;
	  for (i=0;i<Entries;i++) {
	    if (Entry->Mark==1) {
	      Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	      if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	      Display(); ImportRaw2Pwad();
	    }
	    Entry++;
	  }
	  Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2); Display();
	} else {
	  ScreenAufbau(); Display();
	}
      } else ImportRaw2Pwad();
    }
    if (Taste==65) DoItNow(Taste);  /* F7 - Import Gif/Wav 2 Iwad */
    if (Taste==66) DoItNow(Taste);  /* F8 - Import Gif/Wav 2 Pwad */
    if (Taste==67) DoItNow(Taste);  /* F9 - Import Gif/Wav 2 Raw */
  } /* Obiges wird nur abgefragt, wenn EdPName=0 ist */

    if ( ((Taste>94 && Taste<123) || (Taste>32 && Taste<58)) && SPos<8) {
      SuchStr[SPos]=Taste; SuchStr[SPos+1]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      SPos++; if (SPos>0) Suchen();
    }
    if (Taste==8 && SPos>0) {
      SPos--; SuchStr[SPos]=0; Print(27,22,0,15,"           "); Print(27,22,0,15,SuchStr);
      if (SPos>0) Suchen();
    }
    if (Taste==32) {
      Ente=0;
      if (Entry->Mark==1) { Ente=1; Entry->Mark=0; Marked--; }
      if (!Ente && Entry->Mark==0) { Entry->Mark=1; Marked++; }
      if (Pos+CPos-1<=Entries && CPos<24) CPos++;
      if (CPos==24) { Pos++; CPos=23; } if (Pos>Entries-22) Pos=Entries-22;
      Display();
    }
    if (Taste==68) {
      DeleteMarked(); Display();
    }
  }
}

void EditResource(void)
{
  int i;
  int NewX=0,NewY=0,NOx,NOy;
  long l;

  if (NWTisda) { Error(18); return; } /* Do nothing if IWad is merged! */
  if (ViewAble!=1) { Error(21); i=-1; goto AusAus; }

  Box(2);
  Print(29,10,5,15," Editing resource ");
  Print(32,11,5,15,"Size X:               Size Y:");
  Print(30,12,5,15,"X-Offset:             Y-Offset:");

  l=Entry->RStart; fseek(f,l,0);
  _read(fileno(f),&Rx,2); _read(fileno(f),&Ry,2); /* Breite, Laenge */
  _read(fileno(f),&Ox,2); _read(fileno(f),&Oy,2); /* Offset x,y */

  NewX=Rx; NewY=Ry; NOx=Ox; NOy=Oy;

  PrintInt3(39,11,5,0,Rx); PrintInt3(39,12,5,0,Ox);
  PrintInt3(61,11,5,0,Ry); PrintInt3(61,12,5,0,Oy);
  itoa(NewX,DateiName1,10); i=Eingabe1(39,11,5,0); if (i<0) goto AusAus;
  NewX=atoi(DateiName1); if (NewX<=0) goto AusAus;
  itoa(NewY,DateiName1,10); i=Eingabe1(61,11,5,0); if (i<0) goto AusAus;
  NewY=atoi(DateiName1); if (NewY<=0) goto AusAus;
  if (Ox>0 && Oy>0) {
    NOx=NewX/2-1; NOy=NewY-5; PrintInt3(39,12,5,0,NOx); PrintInt3(61,12,5,0,NOy);
  }
  itoa(NOx,DateiName1,10); i=Eingabe1(39,12,5,0); if (i<0) goto AusAus;
  NOx=atoi(DateiName1);
  itoa(NOy,DateiName1,10); i=Eingabe1(61,12,5,0); if (i<0) goto AusAus;
  NOy=atoi(DateiName1);
  l=Entry->RStart; fseek(f,l,0);
  _write(fileno(f),&NewX,2); _write(fileno(f),&NewY,2); /* Breite, Laenge */
  _write(fileno(f),&NOx,2); _write(fileno(f),&NOy,2); /* Offset x,y */
AusAus:
  CWeg();
  if ( (NewX<=0 || NewY<=0) && i>=0 ) Error(20);
  ScreenAufbau(); Display();
}

void InsertResource(void)
{
  long ll,l2;
  int i,k;

  if (NWTisda) { Error(18); return; } /* Do nothing if IWad is merged! */

  Box(1);
  Print(29,10,5,15," Insert Resource - Enter a name: ");
  Print(34,11,5,15,"Name:");
  strcpy(DateiName1,"");
  i=Eingabe1(39,11,5,0); CWeg(); if (i<0) { ScreenAufbau(); Display(); return; }
  Entry=E;
  for (i=0;i<Entries;i++) {
	 if (stricmp(DateiName1,Entry->RName)==0) { i=9999; break; }
    Entry++;
  }
  if (i==9999) Error(17);             /* Warning - name already exists */
  DateiName1[8]=0; strupr(DateiName1);
  k=Entries-(Pos+CPos-2);
  Entry=E+Entries;
  for (;k>=0;k--) {
    strcpy(Entry->RName,(Entry-1)->RName);
    ll=(Entry-1)->RStart; Entry->RStart=ll;
	 ll=(Entry-1)->RLength; Entry->RLength=ll;
    Entry--;
  }
  Entry=E; Entry+=(Pos+CPos-2); strcpy(Entry->RName,DateiName1);
  Entry->RStart=0; Entry->RLength=0;
  fseek(f,DirPos,0); l2=DirPos;
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (Entry->RStart>DirPos) { l2=filelength(fileno(f)); break; }
    Entry++;
  }
  if (l2!=DirPos) {
    Box(2);
    Print(35,11,5,15,"Directory is not the last entry.");
    Print(38,12,0,15,"Correcting, please wait...");
    DirPos=l2; fseek(f,8,0); _write(fileno(f),&DirPos,4);
  }
  Entries++;
  fseek(f,4,0); _write(fileno(f),&Entries,4); fseek(f,DirPos,0);
  Entry=E;
  for (i=0;i<Entries;i++) {
    ll=Entry->RStart; _write(fileno(f),&ll,4);
    ll=Entry->RLength; _write(fileno(f),&ll,4);
    _write(fileno(f),Entry->RName,8);
    Entry++;
  }
  fflush(f);
  ScreenAufbau(); Display();
}

void RenameResource(void)
{
  long ll;
  int i;

  if (NWTisda) { Error(18); return; } /* Do nothing if IWad is merged! */

  Box(1);
  Print(29,10,5,15," Rename Resource - Enter new name: ");
  Print(34,11,5,15,"Name:");
  strcpy(DateiName1,"");
  i=Eingabe1(39,11,5,0); CWeg(); if (i<0) { ScreenAufbau(); Display(); return; }
  Entry=E;
  for (i=0;i<Entries;i++) {
	 if (stricmp(DateiName1,Entry->RName)==0) { i=9999; break; }
    Entry++;
  }
  if (i==9999) Error(17);
  DateiName1[8]=0; strupr(DateiName1);
  Entry=E; Entry+=(Pos+CPos-2); strcpy(Entry->RName,DateiName1);
  fseek(f,DirPos,0);
  Entry=E;
  for (i=0;i<Entries;i++) {
    ll=Entry->RStart; _write(fileno(f),&ll,4);
    ll=Entry->RLength; _write(fileno(f),&ll,4);
    _write(fileno(f),Entry->RName,8);
    Entry++;
  }
  fflush(f);
  ScreenAufbau(); Display();
}

void CheckForShare(void)
{
  int i=9999;
  long l;
  l=filelength(fileno(f));
  if ( (l>=4207819 && l<=4274218) || (l>=5100000 && l<=5200000) ) {
	 Entry=E;
	 for (i=0;i<Entries;i++) {
		if (strnicmp("E2M1",Entry->RName,4)==0) { i=9999; break; }
		Entry++;
	 }
  }
/*  if (i!=9999) ShareDoom=1; */
}

void OpenWADFile(void)
{
  int i,k;
  long l,ll;

  getftime(fileno(f),&FDatum);
  _read(fileno(f),&id,4);
  _read(fileno(f),&Entries,4);
  _read(fileno(f),&DirPos,4);

  if (!LoadIntern) printf("ð Reading %ld entries...\n",Entries);

  fseek(f,DirPos,0); Entry=E; Marked=0; NWTisda=0;
  for (l=0;l<Entries;l++) {
    _read(fileno(f),&ll,4); Entry->RStart=ll;
    _read(fileno(f),&ll,4); Entry->RLength=ll;
    _read(fileno(f),Entry->RName,8);
    Entry->RName[8]=0; Entry->Mark=0;
    Entry++;
  }
  if (ReadPNames) {
    Entry=E;
    for (i=0,l=0;l<Entries;l++) {
      if (strnicmp("PNAMES",Entry->RName,6)==0) { i=1; break; }
      Entry++;
    }
  }
  if (!i) ReadPNames=0;
  if (ReadPNames) {
    l=Entry->RStart; fseek(f,l,0);
    _read(fileno(f),&k,2); PNames=k; _read(fileno(f),&i,2);
    if (!LoadIntern) printf("ð Reading %d pnames...\n",PNames);
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
  Entry=E+Entries-1;
  for (l=Entries-1;l>=0;l--) {
    if (strnicmp(Entry->RName,"NWT",3)==0) { NWTisda=1; break; }
    Entry--;
  }
  CheckForShare();
  if (ShareDoom && !LoadIntern) {
    printf("\x07ð Don't try to use NWT with Shareware versions of Doom or Heretic.\nð Buy it first.\x07\n");
    Restore=0; Ende(0);
  }
}

void DosShell(void)
{
/*  unsigned int Seg,Off;*/

  free(Datai); free(Puffer); free(Bild); free(Bild2);
  Mode3(); Print(0,0,0,15,"ð NWT v1.3 DOS Shell, type EXIT to return..."); printf("\n\n");
  system("");
  GetSpeicher(); SwitchMode(0); ScreenAufbau(); Display();
}

void PlayMUS(void)
{
  int i;
  long l,l2,l3;

  if (MUSisPlaying) {
    ShutdownTimer();
    DeinitAdlib();
    MUSisPlaying=0;
    free(score); /* free(instruments); */
  }

  l=Entry->RStart; l2=Entry->RLength;
  fa=fopen("NWT.TMP","wb+"); if (fa==NULL) { Error(3); return; }
  fseek(f,l,0);
  for (l3=l2;;) {
    if (l3>=32000) {
      _read(fileno(f),Puffer,32000); _write(fileno(fa),Puffer,32000); l3-=32000;
    } else {
      _read(fileno(f),Puffer,(int)l3); _write(fileno(fa),Puffer,(int)l3); break;
    }
  }
  fflush(fa); fclose(fa);
  Box(2);
  Print(36,11,5,15,"Now playing MUS-sound-resource.");
  Print(30,12,5,15,"SPACE = background playing       ESC = stop");
  i=PlayMUSFile();
  if (i==4) { Error(14); return; }
  if (i==5 || i==6) { Error(15); return; }
  if (i==7 || i==8) { Error(16); return; }
  ScreenAufbau(); Display();
}

void DeleteResource(void)
{
  int i,k,i2=0;
  long ll;

  if (NWTisda) { Error(18); return; }
  Box(1);
  Print(40,11,5,15,"Deleting resource(s)...");
  fseek(f,DirPos,0);
  Entry=E; k=(int)Entries;
  for (i=0;i<k;i++) {
    if (Entry->Mark==0) {
      ll=Entry->RStart; _write(fileno(f),&ll,4);
      ll=Entry->RLength; _write(fileno(f),&ll,4);
      _write(fileno(f),Entry->RName,8);
    } else {
      Entries--; i2++;
    }
    Entry++;
  }
  if (i2==0) return;
  fflush(f); fseek(f,DirPos,0); Entry=E; Marked=0;
  for (i=0;i<Entries;i++) {
    _read(fileno(f),&ll,4); Entry->RStart=ll;
    _read(fileno(f),&ll,4); Entry->RLength=ll;
    _read(fileno(f),Entry->RName,8);
    Entry->RName[8]=0; Entry->Mark=0;
    Entry++;
  }
  fseek(f,4,0); _write(fileno(f),&Entries,4); fflush(f);
}

void DoItNow(unsigned char Taste)
{
  int i=1;
  if (Marked==0 && Entry->RStart==0 && Taste==65) {
    Box(1);
    Print(30,11,5,15,"Resource type unknown (G)raphic or (S)ound?");
    Taste=getch(); ViewAble=0; PlayAble=0;
    if (Taste==71 || Taste==103) ViewAble=1;
    if (Taste==83 || Taste==115) PlayAble=1;
    Taste=65;
    if (!ViewAble && !PlayAble) { ScreenAufbau(); Display(); return; }
  }
  if (Marked>0) {
    if (Taste==66 && PutIn1==2) i=GetOneName();
    if (i>0) {
      Entry=E; AltPos=Pos; AltCPos=CPos;
      for (i=0;i<Entries;i++) {
	if (Entry->Mark==1) {
	Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
	  if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
	  Display();
	  DoItReal(Taste);
	}
	Entry++;
      }
      Pos=AltPos; CPos=AltCPos; Entry=E; Entry+=(Pos+CPos-2); Display();
    } else {
      ScreenAufbau(); Display();
    }
  } else {
    DoItReal(Taste);
  }
}

void DoItReal(unsigned char Taste)
{
  if (!ViewAble && !PlayAble && Entry->RStart==0) { Error(19); return; }
  if (ViewAble)
	 switch (Taste) {
		case 65: ImportGif2Iwad(); break;
		case 66: ImportGif2Pwad(); break;
		case 67: ImportGif2Raw(); break;
	 }
  if (PlayAble)
	 switch (Taste) {
		case 65: ImportVoc2Iwad(); break;
		case 66: ImportVoc2Pwad(); break;
		case 67: ImportVoc2Raw(); break;
	 }
  if (!ViewAble && !PlayAble && Entry->RStart!=0) Error(7);
}

int GetOneName(void)
{
  int i;
  Box(1);
  Print(29,10,5,15," Saving into ONE PWad ");
  Print(31,11,5,15,"Outfile:");
  strcpy(DateiName1,PWadName); CheckForVile(1); Print(39,11,5,0,PWadName);
  i=Eingabe1(39,11,5,0); CWeg(); if (i<0) return -1;
  if (!strchr(DateiName1,'.')) strcat(DateiName1,".WAD");
  strcpy(OneName,DateiName1); strcpy(PWadName,DateiName1);
  return 1;
}

void Setup(void)
{
  int i=1,k=1;
  unsigned char Taste=0;
  Print(13, 8,5,15," Ú Program Setup ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ ");
  Print(13, 9,5,15," ³                                                    ³ ");
  for (i=0;i<9;i++) Print(13,10+i,5,15,Setups[i]);
  i=1;
  Print(13,20,5,15," ³                                                    ³ ");
  Print(13,21,5,15," ³ ESC-Ok             ARROWS-Move            RET-Save ³ ");
  Print(13,22,5,15," ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ");
  DisplaySetup(1);
  for (;;) {
	 Taste=getch();
	 if (Taste==27) break;
	 if (Taste==75 || Taste==77) {
		if (i==1) if (GExport==1) GExport=2; else GExport=1;
		if (i==2) if (SExport==1) SExport=2; else SExport=1;
		if (i==5) if (WinPal==1) WinPal=2; else WinPal=1;
		if (i==6) if (PutIn1==1) PutIn1=2; else PutIn1=1;
		if (i==7) if (Sort==1) Sort=2; else Sort=1;
		if (i==8) if (HGR==1) HGR=4; else HGR=1;
      if (i==9) if (SBAdr==0) SBAdr=1; else SBAdr=0;
      if (i==10) if (WhichPal==0) WhichPal=1; else WhichPal=0;
    }
    if (Taste==75 && i>=3 && i<=4 && k>1) k--;
    if (Taste==77 && i>=3 && i<=4 && k<3) k++;
    if ( (Taste==75 || Taste==77) && i==4) {
      SB=0; SP=0;
      if (k==1) SB=1;
      if (k==2) SP=1;
    }
    if ( (Taste==75 || Taste==77) && i==3) TExport=k;
    if (Taste==72 && i>1) {
      i--; k=0;
      if (i==3) k=TExport;
      if (i==4 && SB==1) k=1;
      if (i==4 && SP==1) k=2;
      if (i==4 && !SB && !SP) k=3;
    }
    if (Taste==80 && i<10) {
      i++; k=0;
      if (i==3) k=TExport;
      if (i==4 && SB==1) k=1;
      if (i==4 && SP==1) k=2;
      if (i==4 && !SB && !SP) k=3;
    }
    if (Taste==13) {
      fa=fopen("NWT.CFG","wb+");
      if (fa==0) {
	Error(3);
      } else {
	_write(fileno(fa),&GExport,1); _write(fileno(fa),&SExport,1);
	_write(fileno(fa),&TExport,1); _write(fileno(fa),&SB,1);
	_write(fileno(fa),&SP,1); _write(fileno(fa),&WinPal,1);
	_write(fileno(fa),&PutIn1,1); _write(fileno(fa),&Sort,1);
	_write(fileno(fa),&HGR,1); _write(fileno(fa),&SBAdr,1);
	_write(fileno(fa),&WhichPal,1); fclose(fa);
      }
      break;
    }
    DisplaySetup(i);
  }
  ScreenAufbau(); Display();
}

void DisplaySetup(int i)
{
  int k;
  for (k=1;k<11;k++) Print(13,9+k,5,15,Setups[k-1]);
  if (GExport==1) Print(41,10,5,0,"X"); else Print(50,10,5,0,"X");
  if (SExport==1) Print(41,11,5,0,"X"); else Print(50,11,5,0,"X");
  if (WinPal==1) Print(41,14,5,0,"X"); else Print(50,14,5,0,"X");
  if (PutIn1==1) Print(41,15,5,0,"X"); else Print(50,15,5,0,"X");
  if (Sort==1) Print(41,16,5,0,"X"); else Print(50,16,5,0,"X");
  if (HGR==1) Print(41,17,5,0,"X"); else Print(50,17,5,0,"X");
  if (WhichPal==0) Print(41,19,5,0,"X"); else Print(50,19,5,0,"X");
  if (TExport==1) Print(41,12,5,0,"X");
  if (TExport==2) Print(50,12,5,0,"X");
  if (TExport==3) Print(59,12,5,0,"X");
  if (SB) Print(41,13,5,0,"X");
  if (SP) Print(50,13,5,0,"X");
  if (!SB && !SP) Print(59,13,5,0,"X");
  if (!SBAdr) Print(41,18,5,0,"X");
  if (SBAdr) Print(50,18,5,0,"X");
  if (i==1) Print(16,10,5,0,"Primary graphic format:");
  if (i==2) Print(16,11,5,0,"Primary sound format:");
  if (i==3) Print(16,12,5,0,"Primary texture format:");
  if (i==4) Print(16,13,5,0,"Sound output device:");
  if (i==5) Print(16,14,5,0,"Window & real palette:");
  if (i==6) Print(16,15,5,0,"Put marked to ONE PWad:");
  if (i==7) Print(16,16,5,0,"Directory sort:");
  if (i==8) Print(16,17,5,0,"Marking color:");
  if (i==9) Print(16,18,5,0,"SB Base Adress:");
  if (i==10) Print(16,19,5,0,"Palette choice:");
  if (!SBAdr) BASE=0x220; else BASE=0x240;
  if (WinPal==1) {
    DoomPal[741]=0; DoomPal[742]=0; DoomPal[743]=0;
    HereticPal[741]=0; HereticPal[742]=0; HereticPal[743]=0;
  } else {
	 DoomPal[741]=0; DoomPal[742]=63; DoomPal[743]=63;
	 HereticPal[741]=0; HereticPal[742]=63; HereticPal[743]=63;
  }
}

void PlayResource(void)
{
  int i,freq;
/*  unsigned int samples,k,sam; */
  unsigned int samples,k;
  long l,l3;

  Box(2);
  Print(39,11,5,15,"Playing sound resource...");
  Print(41,12,5,0,"Hit a key to abort...");
  l=Entry->RStart;
  fseek(f,l,0); _read(fileno(f),&i,2);
  _read(fileno(f),&freq,2); _read(fileno(f),&samples,2);
  _read(fileno(f),&i,2); freq=(int)(200-1000000/freq);
  if (SB) {
    for (l3=0,k=0;k<samples;l3++,k++) { i=fgetc(f); pokeb(B1Seg,B1Off+k,i); }
    for (i=0;i<100;i++,k++) pokeb(B1Seg,B1Off+k,127);
    i=direkt_soundausgabe(MK_FP(B1Seg,B1Off),samples,freq);
    if (i==1) Error(5);
    if (i==2) Error(6);
  }
  ScreenAufbau(); Display();
}

void Choose64(void)
{
  int i,k,x,y,xoff,yoff,vx=0,vy=0;
  unsigned char Taste;
  char Puffer1[15],Puffer2[15];
  unsigned int Off,PNummer;
  long lll;
  Marked=0; Marking=1; SuchStr[0]=0; SPos=0; Texes=1; NTCount=0;
  Box(2);
  Print(30,11,5,15,"Tex-Name:              Outfile:");
  Print(32,12,5,15,"Size X:               Size Y:");
  for (Off=0;Off<64000;Off++) {
    pokeb(B1Seg,B1Off+Off,0); pokeb(B2Seg,B2Off+Off,0);
  }
  if (strlen(TexOutFile)==0) {
	 strcpy(DateiName2,TEntry[TPos+TCPos-2].TName); strcat(DateiName2,Endung[TExport+3]);
  } else {
	 strcpy(DateiName2,TexOutFile);
  }
  strcpy(DateiName1,TEntry[TPos+TCPos-2].TName);
  PrintInt3(39,12,5,0,Tx); Print(39,11,5,0,DateiName1);
  PrintInt3(61,12,5,0,Ty); Print(61,11,5,0,DateiName2);
  i=Eingabe1(39,11,5,0); if (i<0) goto Aus13;
  i=Eingabe2(61,11,5,0); if (i<0) goto Aus13;
  strcpy(TexOutFile,DateiName2);
  strcpy(Puffer1,DateiName1); strcpy(Puffer2,DateiName2);
  itoa(Tx,DateiName1,10); itoa(Ty,DateiName2,10);
  i=Eingabe1(39,12,5,0); if (i<0) goto Aus13;
  i=Eingabe2(61,12,5,0); if (i<0) goto Aus13;
  CWeg();
  NewTx=atoi(DateiName1); NewTy=atoi(DateiName2);
  if (NewTx>256 || NewTx<0) NewTx=256; if (NewTy>128|| NewTy<0) NewTy=128;
  PrintInt3(39,12,5,0,NewTx); PrintInt3(61,12,5,0,NewTy);
  strcpy(DateiName1,Puffer1); strcpy(DateiName2,Puffer2);
  DeleteMarked();
  ScreenAufbau(); Pos=0; CPos=2; DisplayPNames();
  Box(2);
  Print(32,11,5,15,"Now mark all resources (pnames) you want");
  Print(34,12,5,15,"for your new texture-patch. (max 64)");
  getch();
  ScreenAufbau(); DisplayPNames(); ListPNames();
  if (Marked!=0) {
    Entry=E;
    for (k=0,i=0;i<Entries;i++) {
      if (Entry->Mark==1) { MarkedEntry[k]=i; k++; }
      Entry++;
    }
    x=(320-NewTx)/2; y=(200-NewTy)/2; Window(B1Seg,B1Off,x,y,NewTx,NewTy);
    Pos=0; CPos=2; Marking=0; MakeTex=1; NTCount=0; NTKilo=0;
    ScreenAufbau(); Display64();
    for(;;) {
      Taste=getch();
      if (Taste==27) break;
      if (Taste==73) { Pos-=21; if (Pos<0) { Pos=0; CPos=2; } Display64(); Taste=0; }
      if (Taste==81) { Pos+=21; if (Pos>Marked-22) { Pos=Marked-22; CPos=23; if (Pos<0) { Pos=0; CPos=Marked+1; } } Display64(); Taste=0; }
      if (Taste==72 && CPos==2 && Pos>0) { Pos--; Display64(); Taste=0; }
      if (Taste==80 && CPos==23) { Pos++; if (Pos>Marked-22) Pos=Marked-22; Display64(); Taste=0; }
      if (Taste==72 && CPos==2 && Pos==0) Taste=0;
      if (Taste==80 && CPos<Marked+1) { CPos++; Display64(); Taste=0; }
      if (Taste==72) { CPos--; Display64(); Taste=0; }
      if (Taste==79) { Pos=Marked-22; CPos=23; if (Pos<0) { Pos=0; CPos=Marked+1; } Display64(); }
      if (Taste==71) { Pos=0; CPos=2; Display64(); }
      if (Taste==68) {
	if (NTCount==0) {
	  Error(10); Display64();
	} else {
	  SaveTexture(DateiName1,DateiName2);
	  break;
	}
      }
      if (Taste==60) {      /* View Texture */
	SwitchMode(1);
	Move(B1Seg,B1Off+9600,0xa000,9600,32768-9600);
	DoomPalette(); getch(); Taste=0;
	SwitchMode(0); ScreenAufbau(); Display64();
      }
      if (Taste==59) {      /* List Patches */
	Print(25,9,1,15,"ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´");
	for (i=10;i<23;i++) {
	  Print(25,i,1,15,"³ "); Print(27,i,0,15,"                                                 ");
	  Print(76,i,1,15," ³");
	}
	Print(25,23,1,15,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
	for (vx=0,vy=0,i=0;i<NTCount;i++,vy++) {
	  if (vy>12) { vy=0; x+=10; }
	  Print(27+vx,10+vy,0,15,PName[NTData[i].EntryNr].Name);
	}
	getch(); ScreenAufbau(); Display64();
      }
      if (Taste==61) {      /* Clear Texture */
	Box(1);
	Print(39,11,5,15,"Texture data cleared.");
	for (Off=0;Off<64000;Off++) pokeb(B1Seg,B1Off+Off,0);
	Window(B1Seg,B1Off,x,y,NewTx,NewTy);
	NTCount=0; NTKilo=0;
	getch(); ScreenAufbau(); Display64();
      }
      if (Taste==13) {
	i=Pos; k=CPos; Pos=MarkedEntry[Pos+CPos-2]-CPos+2;
	GetPic(); Pos=i; CPos=k; Display64();
      }
      if (Taste==32 && NTCount+1>64) {
	Error(11); Display64(); Taste=0;
      }
      if (Taste==32) {
	if (NTKilo+Entry->RLength<=64000) {
	  for (i=0;i<PNames;i++) if (strcmp(Entry->RName,PName[i].Name)==0) { PNummer=i; break; }
	  lll=Entry->RStart; xoff=0; yoff=0;
	  for (Off=0;Off<64000;Off++) pokeb(B2Seg,B2Off+Off,247);
	  SwitchMode(1); DoomPalette();
	  TakePic(lll,0,0,0,0,320,200,B2Seg,B2Off);
	  Move(B1Seg,B1Off+9600,0xa000,9600,32768-9600);
	  PushPic(0xa000,0,x,y,xoff,yoff,Rx,Ry,x+NewTx-1,y+NewTy-1);
	  TextAusgabe(6,1,4,"ARROWS-Move  RET-Ok  ESC-Abort");
	  for (;;) {
	    Taste=0;
	    if (kbhit()) Taste=getch();
	    if (Taste==71) { xoff=0; yoff=0; }
	    if (Taste==73) { xoff=NewTx-Rx; yoff=0; }
	    if (Taste==79) { xoff=0; yoff=NewTy-Ry; }
	    if (Taste==81) { xoff=NewTx-Rx; yoff=NewTy-Ry; }
	    if (Taste==72) yoff--;
	    if (Taste==80) yoff++;
	    if (Taste==75) xoff--;
	    if (Taste==77) xoff++;
	    if (Taste==27) break;
	    if (Taste==13) {
	      PushPic(B1Seg,B1Off,x,y,xoff,yoff,Rx,Ry,x+NewTx-1,y+NewTy-1);
	      SwitchMode(0); ScreenAufbau(); Display64();
	      NTData[NTCount].EntryNr=PNummer;
	      NTData[NTCount].x=xoff; NTData[NTCount].y=yoff;
	      NTCount++; NTKilo+=Entry->RLength; Taste=0; break;
	    }
	    if (Taste!=0) {
	      Move(B1Seg,B1Off+9600,0xa000,9600,32768-9600);
	      PushPic(0xa000,0,x,y,xoff,yoff,Rx,Ry,x+NewTx-1,y+NewTy-1);
	      itoa(xoff,Puffer1,10); itoa(yoff,Puffer2,10);
	      TextAusgabe(14,24,4,"X,Y [    ,    ] ");
	      TextAusgabe(19,24,4,Puffer1); TextAusgabe(24,24,4,Puffer2);
	    }
	  }
	  SwitchMode(0); ScreenAufbau(); Display64();
	} else {
	  Error(12); Display64();
	}
      }
    }
  }
  MakeTex=0; Marking=0; Marked=0; Texes=0;
  DeleteMarked();
Aus13:
  ScreenAufbau(); DisplayPatch();
}

void DisplayInfo(void)
{
  int i;
  long l,HSek;
  char a,b,c,d;
  double long dl,GameTics;
  unsigned int min,sec,hsec;

  l=Entry->RStart; ViewAble=0; PlayAble=0; LevelData=0; MUSFile=0; IsEndoom=0;
  fseek(f,l,0);
  _read(fileno(f),&Rx,2); _read(fileno(f),&Ry,2); /* Breite, Laenge */
  _read(fileno(f),&Ox,2); _read(fileno(f),&Oy,2); /* Offset x,y */
  Print(27,8,0,15,"                                                 ");
  Print(27,7,0,15,"Type:"); Print(55,7,0,15,"Usage:");
  for (i=0;i<10;i++) {
	 if (strcmp(SpSigs[i],Entry->RName)==0) {
		LevelData=1; Print(27,8,0,7,"Level Data"); return;
	 }
  }
  l=Entry->RLength; if (l==4160) l=4096;
  if ( (Rx>0 && Ry>0 && Rx<321 && Ry<201 && l!=0) || l==4096) {
    if (l==4096 && ((Rx>320 || Rx<1) || (Ry>200 || Ry<1)) ) { Rx=64; Ry=64; }
    Print(27,8,0,7,"Graphic Resource [");
    PrintInt2(45,8,0,7,Rx); Print(48,8,0,7,",");
    PrintInt2(49,8,0,7,Ry); Print(52,8,0,7,"]");
    if (l==4096 && Rx==64 && Ry==64) {
		Print(55,8,0,7,"Floor/Ceiling"); ViewAble=2; return;
    }
    ViewAble=1;
    for (i=2;i<31;i++) {
      if (strnicmp(Sigs[i],Entry->RName,strlen(Sigs[i]))==0) {
        if (i==10 && l>60000) break;
	Print(55,8,0,7,SigNames[i]);
	break;
      }
    }
	 return;
  }
  for (i=0;i<2;i++) {
	 if (strnicmp(Sigs[i],Entry->RName,2)==0 && Rx==0 && Ry==l-4) {
      Print(27,8,0,7,"Speaker Sound Resource         ");
		return;
    }
	 if (strnicmp(Sigs[i],Entry->RName,2)==0) {
		Print(27,8,0,7,"Sound Resource                 ");
		PlayAble=1; return;
    }
  }
  if (Rx==3 && Ry==11025) {
    Print(27,8,0,7,"Sound Resource                 ");
	 PlayAble=1; return;
  }
  if (strnicmp("D_",Entry->RName,2)==0 ||
      strnicmp("MUS",Entry->RName,3)==0) {
    Print(27,8,0,7,"Music Resource (MUS)           ");
	 MUSFile=1; return;
  }
  if (strnicmp("DEMO",Entry->RName,4)==0) {
    l=Entry->RStart; fseek(f,l,0);
	 _read(fileno(f),&a,1); a=(unsigned char)a-100; if (a<0) a=0;  /* version */
    if (a>10) { a=4; l=ftell(f); l--; fseek(f,l,0); }
    _read(fileno(f),&b,1); b++;                                   /* skill */
    _read(fileno(f),&c,1);                                        /* Episode */
    _read(fileno(f),&d,1);                                        /* Map */
    l=Entry->RLength; GameTics=(double long)((l-14)/4); dl=(double long)GameTics*100/35; HSek=(long)dl;
    min=(unsigned int)(HSek/6000); HSek-=min*6000;
    sec=(unsigned int)HSek/100;
    hsec=(unsigned int)HSek-sec*100;
    Print(27,8,0,7,"Demo LMP v1.");
    gotoxy(40,9); printf("%d Skill %d, E%dM%d, Time: %d:%d,%dm",(int)a,(int)b,(int)c,(int)d,(int)min,(int)sec,(int)hsec);
	 gotoxy(2,2); return;
  }
  if (strnicmp("END",Entry->RName,3)==0 ||
      strnicmp("LOADING",Entry->RName,6)==0) {
    Print(27,8,0,7,"Text mode screen");
	 IsEndoom=1; return;
  }
  Print(27,8,0,7,"Unknown                        ");

}

void Suchen(void)
{
  int i;
  Entry=E;
  for (i=0;i<Entries;i++) {
    if (strnicmp(SuchStr,Entry->RName,SPos)==0) {
      Pos=i-10; CPos=12; if (Pos<0 || Entries<22) { CPos=i+2; Pos=0; break; }
		if (Pos>Entries-22) { Pos=Entries-22; CPos=Entries-i; CPos=24-CPos; }
      break;
    }
    Entry++;
  }
  Display();
}

void PNameSuchen(void)
{
  int i;
  for (i=0;i<PNames;i++) {
	 if (strnicmp(SuchStr,PName[i].Name,SPos)==0) {
		Pos=i-10; CPos=12; if (Pos<0) { CPos=i+2; Pos=0; }
		if (Pos>PNames-22) { Pos=PNames-22; CPos=24-(PNames-i); }
		break;
	 }
  }
  DisplayPNames();
}

void Ende(int code)
{
/*  int i; */
/*  unsigned char a,b,c; */

  if (Restore) {
	 Mode3(); memcpy(MK_FP(0xb800,0),OldScr,4000); gotoxy(Oldx,Oldy);
  }
  if (code!=3 && code!=4) { free(Puffer); free(Datai); free(OldScr); free(E); free(PE); free(Bild); free(Bild2); }
  if (code!=0 && code!=4) printf("ð Abnormal termination. Error #%d!\n",code);
  chdir(CurDir); setftime(fileno(f),&FDatum); fclose(f);
  if (MUSisPlaying) {
    ShutdownTimer();
    DeinitAdlib();
    MUSisPlaying=0;
    free(score);
  }
  if (InstrSindDa) free(instruments);
  if (LoadIntern && ShareDoom) printf("\x07ð Don't try to use NWT with Shareware versions of Doom or Heretic.\nð Buy it first.\x07\n");
  exit(0);
}
