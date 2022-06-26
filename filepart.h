
#ifndef __FILEPART_H_
#define __FILEPART_H_

#include <dir.h>
#include <string.h>

typedef struct {
  char FileName[13];
  unsigned long Date;
} Datdings;


int SortIt(Datdings *D1,Datdings *D2);

int GetDir(char *Wild);

void ListFiles(void);

void DisplayFiles(void);

void DateiSuchen(void);






#endif