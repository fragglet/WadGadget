#ifndef __GIFPCX_H_
#define __GIFPCX_H_

int LadeVGAPcx(unsigned int Seg,unsigned int Off);
void SaveGif(int xx,int yy);
void SavePcx(int xx,int yy,unsigned int Seg,unsigned int Off);
int ReadGif(void);
int get_byte(void);
int out_line(unsigned char pixels[],int linelen);
int init_exp(int size);
int get_next_code(void);
int decoder(int linewidth);

#endif