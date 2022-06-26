#ifndef __SG_H_
#define __SG_H_

/* GIF.H
**
** Header file for public domain GIF encoding and decoding routines
** by Bert Tyler and Lee Daniel Crocker.
*/

/* The following typedefs must be changed to reflect the unsigned and
** signed 8, 16, and 32-bit integer types of whichever compiler you are
** using.  Those below will work on most 8086 compilers.
*/

typedef unsigned char   U8;
typedef signed char     S8;
typedef unsigned short  U16;
typedef signed short    S16;
typedef unsigned long   U32;
typedef signed long     S32;

/**************************************
** The definitions below must be changed to reflect the byte ordering of
** target processor.  8086, 6502, VAX, and similar machines will use the
** first set; 6800, 68000, and others use the second set.
*/

#ifdef M_I86    /* Microsoft C for 8086 defines this */
#   define LOBYTE(w)    (*((U8*)&(w)))
#   define HIBYTE(w)    (*((U8*)&(w)+1))
#   define LOWORD(d)    (*((U16*)&(d)))
#   define HIWORD(d)    (*((U16*)&(d)+1))
#   define PUT16(c,w)   (*((U16*)&(c))=(w))
#   define GET16(c,w)   ((w)=*((U16*)&(c)))
#else   /* For Macintosh, Amiga, ATARI, and others */
#   define LOBYTE(w)    (*((U8*)&(w)+1))
#   define HIBYTE(w)    (*((U8*)&(w)))
#   define LOWORD(d)    (*((U16*)&(d)+1))
#   define HIWORD(d)    (*((U16*)&(d)))
#   define PUT16(c,w)   (c)=LOBYTE(w),*((char*)(&(c)+1))=HIBYTE(w)
#   define GET16(c,w)   LOBYTE(w)=(c),HIBYTE(w)=*((char*)(&(c)+1))
#endif

/**************************************
** NOTE: get_pixel() can be defined as a macro for extra speed if needed,
** or declared external here and defined in some module of the program.
** This function will be called to retrieve values of points in GIF storage
** order; i.e., in strict left-to-right, top-down order for non-interlaced
** images and left-to-right, interlaced for interlaced ones.
*/
/*
#if 1

#define get_pixel(x,y) getpixel(x,y)

#else

int get_pixel(int, int);

#endif
*/
/**************************************
** Fix MSDOS Text mode file open "feature".
*/

#ifdef MSDOS
#   define READMODE "rb"
#   define WRITEMODE "wb"
#else
#   define READMODE "r"
#   define WRITEMODE "w"
#endif

/**************************************
** GIF encoder interface routines:
*/

int create_gif(int, int, int, U8 *, int, char *);
int put_image(int, int, int, int, int, int, U8 *);
int close_gif(void);

#endif
