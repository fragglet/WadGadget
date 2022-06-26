#ifndef __GLOBAL_H_
#define __GLOBAL_H_


#ifdef __386__
 typedef unsigned long DWORD;
#else
 typedef unsigned char BYTE;
 typedef unsigned int  WORD;
 typedef unsigned long DWORD;
#endif

/* used for interrupt functions */
#ifdef __cplusplus
	 #define __CPPARGS ...
#else
	 #define __CPPARGS void
#endif


#endif