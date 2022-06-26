
#ifndef __SETPAL_H_
#define __SETPAL_H_


void CWeg(void);
void SetPal(unsigned int _Seg,unsigned int _Off);
void TextAusgabe(int x,int y,unsigned char _f,char *str);
void Mode3(void);
void SwitchMode(int i);
void DoomPalette(void);
void Move(unsigned int _Seg1,unsigned int _Off1,unsigned int _Seg2,unsigned int _Off2,unsigned int Laenge);
int Eingabe1(int x,int y,int h,int v);
int Eingabe2(int x,int y,int h,int v);
void Window(unsigned int Seg,unsigned int Offset,int xo,int yo,int x,int y);

#endif