
#ifndef __PRINTS_H_
#define __PRINTS_H_


void Display64(void);
void CheckForVile(unsigned char i);
void Box(int h);
void DisplayEndoom(void);
void HexDump(void);
void DumpIt(void);
void Print(int x,int y,char h,char v,char Text[]);
void PrintZahl(int x,int y,char h,char v,long Zahl);
void PrintInt(int x,int y,char h,char v,int Zahl);
void PrintInt2(int x,int y,char h,char v,int Zahl);
void PrintInt3(int x,int y,char h,char v,int Zahl);
void PrintHex(int x,int y,char h,char v,int Zahl);
void Error(int i);
void TakePic(long lll,int xo,int yo,int wxo,int wyo,int mx,int my,unsigned int Seg,unsigned int Offset);
void PushPic(unsigned int Seg,unsigned int Off,int xo,int yo,int wxo,int wyo,int x,int y,int mx,int my);
void GetPic(void);
void ScreenAufbau(void);
void Display(void);

#endif