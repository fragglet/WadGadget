#ifndef __IMPEXP_H_
#define __IMPEXP_H_

void Write2Iwad(void);
void Write2Pwad(void);
unsigned int ReadVoc(void);
unsigned int ReadWav(void);
void ImportVoc2Raw(void);
void ImportVoc2Pwad(void);
void ImportVoc2Iwad(void);
void ImportGif2Pwad(void);
void ImportRaw2Pwad(void);
void Export2Pwad(void);
void SaveVoc(void);
void SaveRaw(void);
void SavePic(void);
void ImportRaw2Iwad(void);
void ImportGif2Iwad(void);
void ImportGif2Raw(void);
void SaveVocData(unsigned int Laenge);
void SaveData(int x,int y,int xoff,int yoff);

#endif