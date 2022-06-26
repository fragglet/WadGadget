/* SG.C */

#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg.h"
#include "wadview.h"



#define MAXTEST   100       /* Maximum encoded string length    */
#define MAXSTRING 10000     /* Array space for all the encoded strings   */
									 /* BIGGER IS BETTER AS FAR AS MAXSTRING GOES */
#define MAXENTRY  6151      /* Maximum encoded entries                   */
									 /* (a prime number is best for hashing)
										 ((and 6151, which is the smallest prime
										 number greater than (4096*3/2) is dead
										 perfect for our purposes))                */



static U8 *teststring;  /* [MAXTEST]    String to match against         */
static U8 *strings;     /* [MAXSTRING]  Encoded strings go here         */
static S16 *strlocn;    /* [MAXENTRY]   Location of encoded strings     */

static FILE *out;                   /* Destination file for GIF stream  */

static char signature[] = { 0x47, 0x49, 0x46, 0x38, 0x37, 0x61 },
            imagestart[] = { 0x2C },
            terminator[] = { 0x3B };

static int  lentest,            /* Length of current encoder string     */
            lastentry,          /* Last string found in table           */
            numentries,         /* Number of entries in string table,   */
                                /* including "fake" entries when full   */
            numrealentries,     /* Number of real entries only          */
            maxnumrealentries,  /* flush the table if < numrealentries  */
            nextentry;          /* Next available entry in string table */

static S16  entrynum;           /* Hash value of encoded GIF strings    */

static int  clearcode,          /* Code to clear LZW decoder            */
            endcode,            /* LZW end-of-data mark                 */
            gifopen = 0;        /* Flag: is the GIF file open?          */

static U8   block[267];         /* Buffer accumulating output bytes     */

static int  bytecount,          /* Number of bytes and bits in buffer   */
            bitcount;

static int  startbits,          /* Starting code size - 1               */
            gcmapbits,          /* Size in bits of global color map     */
            dcolors,            /* Number of colors in global color map */
            codebits;           /* Current LZW code size                */

static int  log2(register U16);
static void init_bitstream(void);
static void encode_a_value(register U16);
static void init_tables(void);

static U16 masks[] = {
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
};

int get_pixel(int x,int y)
{
  return peekb(B1Seg,B1Off+x+y*320);
}

int create_gif( int swidth,     /* Global screen width                  */
                int sheight,    /* "      "      height                 */
                int gcolors,    /* Global color map size (0 if none)    */
                U8 *palette,    /* "      "     "   entries             */
                int crez,       /* Color resolution (1..8), 0 for auto  */
                char *fname )   /* Filename                             */
{
    int i, c, nbits;
    char openfile[80];
    U8 header[13], *pp;
    U16 bitmap[16], *bp, v;

    if (gifopen) return -1;

    strcpy(openfile, fname);

    if ((out = fopen(openfile,"wb+")) == NULL) return -2;

    memcpy(header, signature, 6);
    PUT16(header[6], swidth);
    PUT16(header[8], sheight);

    if (gcolors > 0) header[10] = 0x80; /* Global color map flag        */
    else header[10] = 0x00;

    gcmapbits = log2(gcolors);
    dcolors = 1 << gcmapbits;
    if (gcolors <= 2) ++gcmapbits;
    header[10] |= (gcmapbits - 1);      /* Color table size             */

    if (crez == 0) {
        memset(bitmap, 0, sizeof(bitmap));
        for (pp = palette, i = 3 * gcolors; i > 0; --i) {
            c = *pp++;
            bitmap[c >> 4] |= masks[c & 0x0F];
        }
        nbits = 0;
        for (bp = bitmap, i = 16; i > 0; --i) {
            v = *bp++;
            do {
                if (v & 0x8000) ++nbits;
                v = (v & 0x7FFF) << 1;
            } while (v);
        }
        crez = log2(nbits);
    }
    header[10] |= ((crez - 1) << 4);

    header[11] = 0;                     /* Background color index       */
    header[12] = 0;                     /* Reserved                     */

    if (fwrite(header, 13, 1, out) != 1) goto write_error;

    if (gcolors > 0) {
        if (fwrite(palette, 3 * dcolors, 1, out) != 1) goto write_error;
    }

    gifopen = 1;
    return 0;                           /* All's well                   */

write_error:
    fflush(out); fclose(out);           /* Close file and return error  */
    return -3;
}

static U16  intstart[] = { 0, 4, 2, 1, 0 },
            intinc[] = { 8, 8, 4, 2, 1 };

int put_image(  int iwidth,     /* Image width                          */
                int iheight,    /* "     height                         */
                int xoff,       /* "     horizontal offset              */
                int yoff,       /* "     vertical offset                */
                int interlace,  /* Interlaced image flag                */
                int lcolors,    /* Local color map size (0 if none)     */
                U8 *lcmap )     /* "     "     "   entries              */
{
    int rval, intpass;
    register int hashentry;
    U16 i, y, ydot, xdot, color, colors;
    U8 header[11], cmask;
    static U16 hashcode;

    if (!gifopen) return -1;

    rval = -2;
    if ((teststring = (U8 *)malloc(MAXTEST)) == NULL) return rval;
    if ((strings = (U8 *)malloc(MAXSTRING)) == NULL) goto memerr3;
	 if ((strlocn = (S16 *)malloc(2 * MAXENTRY)) == NULL) goto memerr2;

    header[0] = *imagestart;
    PUT16(header[1], xoff);
    PUT16(header[3], yoff);
    PUT16(header[5], iwidth);
    PUT16(header[7], iheight);

    if (interlace) {
        header[9] = 0x40;
        intpass = 0;
    } else {
        header[9] = 0x00;   /* We treat non-interlaced images as pass   */
        intpass = 4;        /* 4 to avoid an if() inside the loop.      */
    }

    if (lcolors == 0) {         /* If a local color map is given, use   */
        startbits = gcmapbits;  /* it; otherwise, use the global.       */
        colors = dcolors;
    } else {
        header[9] |= 0x80;
        startbits = log2(lcolors);
        colors = 1 << startbits;
        if (lcolors <= 2) ++startbits;
    }
    header[9] |= (U8)(startbits - 1);
    header[10] = (U8)startbits;         /* &&&DNN this should go after local
                                           color map */

    if (fwrite(header, 11, 1, out) != 1) goto writeerr;

    if (lcolors != 0) {
        if (fwrite(lcmap, 3 * colors, 1, out) != 1) goto writeerr;
    }

    clearcode = 1 << startbits;
    endcode = clearcode + 1;

    codebits = startbits + 1;       /* Set Initial LZW code size    */
    init_bitstream();               /* Initialize encode_a_value()  */
    init_tables();                  /* "          LZW tables        */

    cmask = (U8)(colors - 1);       /* Color value mask             */

    ydot = 0;                       /* Ydot holds actual value, y   */
    for (y = iheight; y > 0; --y) { /* is just a count.             */

        for (xdot = 0; xdot < iwidth; ++xdot) {

            color = get_pixel(xdot, ydot) & cmask;

            teststring[0] = (U8)++lentest;
            teststring[lentest] = (U8)color;

            if (lentest == 1) {
                lastentry = color;
                continue;
            }

            if (lentest == 2) hashcode = 301 * (teststring[1] + 1);
            hashcode *= (color + lentest);
            hashentry = ++hashcode % MAXENTRY;

            for (i = 0; i < MAXENTRY; ++i) {
                if (++hashentry >= MAXENTRY) hashentry = 0;
                if (memcmp(&strings[strlocn[hashentry]+2],
                            teststring, lentest+1) == 0) break;
                if (strlocn[hashentry] == 0) break;
            }

	    if (strlocn[hashentry] != 0 && lentest < MAXTEST-3) {
                memcpy(&entrynum, &strings[strlocn[hashentry]],2);
                lastentry = entrynum;
                continue;
            }
            encode_a_value(lastentry);
            ++numentries;

            if (strlocn[hashentry] == 0) {
                entrynum = numentries + endcode;
                strlocn[hashentry] = nextentry;
                memcpy(&strings[nextentry],   &entrynum,  2);
                memcpy(&strings[nextentry]+2, teststring, lentest+1);
                nextentry += lentest+3;
                ++numrealentries;
            }

            teststring[0] = (U8)(lentest = 1);
            teststring[1] = (U8)(lastentry = color);

            if ((numentries + endcode) == (1 << codebits)) ++codebits;

            if (numentries + endcode > (4096-3) ||
                numrealentries > maxnumrealentries ||
                nextentry > MAXSTRING-MAXTEST-5) {
                encode_a_value(lastentry);
                init_tables();
            }
        }
        if ((ydot += intinc[intpass]) >= iheight) ydot = intstart[++intpass];
    }
    encode_a_value(lastentry);
    encode_a_value(endcode);

    if (fwrite("", 1, 1, out) != 1) rval = -3;  /* Zero-length block    */
    else rval = 0;
    goto nowriteerr;

writeerr:               /* This monstrosity assures that all pointers   */
    rval = -3;          /* allocated at the start of the function are   */
nowriteerr:             /* freed in the proper order, even if an error  */
                        /* occurs at some point.                        */
memerr1:
    free(strlocn);
memerr2:
    free(strings);
memerr3:
    free(teststring);

    return rval;
}

int close_gif(void)
{
    if (!gifopen) return -1;
    fwrite(terminator, 1, 1, out);
    fflush(out); fclose(out);
    return gifopen = 0;
}

static int log2(register U16 val)
{
    if (--val & 0x80) return 8;
    if (val & 0x40) return 7;
    if (val & 0x20) return 6;
    if (val & 0x10) return 5;
    if (val & 0x08) return 4;
    if (val & 0x04) return 3;
    if (val & 0x02) return 2;
    return 1;
}

static void init_bitstream(void)
{
    bytecount = 1;
    bitcount = 0;
    memset(block, 0, sizeof(block));
    return;
}

static void init_tables(void)
{
    encode_a_value(clearcode);
    numentries = numrealentries = lentest = 0;
    maxnumrealentries = (MAXENTRY * 2) / 3;
    nextentry = 1;
    codebits = startbits + 1;

    *strings = '\0';
    memset(strlocn, 0, 2 * MAXENTRY);
    return;
}

static void encode_a_value(register U16 code)
{
    register U16 icode;
    U8 *bp;
    U16 i;

    icode = code << bitcount;
    bp = block + bytecount;

    *bp++ |= (U8)icode;
    *bp++ |= (U8)(icode >> 8);

    icode = (code >> 8) << bitcount;
    *bp++ |= (U8)(icode >> 8);

    bitcount += codebits;
    while (bitcount >= 8) {
        bitcount -= 8;
        ++bytecount;
    }

    if (bytecount > 251 || code == endcode) {
        if (code == endcode) {
            while (bitcount > 0) {
                bitcount -= 8;
                ++bytecount;
            }
        }
        i = bytecount;
        *block = (U8)(i - 1);
        fwrite(block, bytecount, 1, out);

        bytecount = 1;
        memcpy(block+1, block+i, 5);
        memset(block+6, 0, sizeof(block)-6);
    }
    return;
}
