/*
 * TransFig: Facility for Translating Fig code
 * This routine is from PSencode.c, in the xwpick package by:
 *      E.Chernyaev (IHEP/Protvino)
 * Parts Copyright (c) 1994 Brian V. Smith
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include <stdio.h>
#include "fig2dev.h"

#define MAXWIDTH       4096

#define PageSideMargin 12
#define PageTopMargin  12
#define PageWidth      612
#define PageHeight     792

#define put_string nc=strlen(s); for(i=0;i<nc;i++) (putc((s[i]),File)); Nbyte += nc

typedef unsigned char byte;

/***********************************************************************
 *                                                                     *
 * Name: PSencode                                    Date:    13.01.93 *
 *                                                                     *
 * Function: Output image in PostScript format (runlength encoding)    *
 *                                                                     *
 * Input: File       - stream for output                               *
 *        Width      - image width                                     *
 *        Height     - image height                                    *
 *        Ncol       - number of colors                                *
 *        R[]        - red components                                  *
 *        G[]        - green components                                *
 *        B[]        - blue components                                 *
 *        data[]     - array for image data (byte per pixel)           *
 *                                                                     *
 * Return: size of PS                                                  *
 *                                                                     *
 ***********************************************************************/
long
PSencode(File, Width, Height, Ncol, R, G, B, data)
	 FILE *File;
         int  Width, Height, Ncol;
         byte R[], G[], B[];
	 unsigned char *data;
{

  long    Nbyte;
  char    s[80], **q;
  byte    *ptr, *end;
  int     i, nc, k, current, previous, run, y;

  static char h[] = "0123456789abcdef";

  static char *PostScript[] = {
    "%***********************************************************************",
    "%*                                                                     *",
    "%* Object: Image decoding PS-routine                    Date: 01.02.93 *",
    "%* Author: Evgeni CHERNYAEV (chernaev@vxcern.cern.ch)                  *",
    "%*                                                                     *",
    "%* Function: Display a run-length encoded color image.                 *",
    "%*           The image is displayed in color on viewers and printers   *",
    "%*           that support color Postscript, otherwise it is displayed  *",
    "%*           as grayscale.                                             *",
    "%*                                                                     *",
    "%***********************************************************************",
    "/byte 1 string def",
    "/color 3 string def",
    "systemdict /colorimage known { /cnt 3 def } { /cnt 1 def } ifelse",
    "/String 256 cnt mul string def",
    "%***********************************************************************",
    "/DecodePacket            % Decode color packet                         *",
    "%***********************************************************************",
    "{",
    "  currentfile byte readhexstring pop 0 get",
    "  /Nbyte exch 1 add cnt mul def",
    "  /color ColorMap currentfile byte readhexstring pop 0 get get def",
    "  String dup",
    "  0 cnt Nbyte 1 sub { color putinterval dup } for",
    "  pop 0 Nbyte getinterval",
    "} bind def",
    "%***********************************************************************",
    "/DisplayImage            % Display run-length encoded color image      *",
    "%***********************************************************************",
    "{",
    "  gsave",
    "  currentfile String readline pop",
    "  token { /columns exch def } { } ifelse",
    "  token { /rows exch def pop } { } ifelse",
    "  currentfile String readline pop",
    "  token { /Ncol exch def pop } { } ifelse",
    "  /ColorMap Ncol array def",
    "  systemdict /colorimage known {",
    "    0 1 Ncol 1 sub {",
    "      ColorMap exch",
    "      currentfile 3 string readhexstring pop put",
    "    } for",
    "    columns rows 8",
    "    [ columns 0 0 rows neg 0 rows ]",
    "    { DecodePacket } false 3 colorimage",
    "  }{",
    "    0 1 Ncol 1 sub {",
    "      ColorMap exch",
    "      1 string dup 0",
    "      currentfile color readhexstring pop pop",
    "      color 0 get 0.299 mul",
    "      color 1 get 0.587 mul add",
    "      color 2 get 0.114 mul add",
    "      cvi put put",
    "    } for",
    "    columns rows 8",
    "    [ columns 0 0 rows neg 0 rows ]",
    "    { DecodePacket } image",
    "  } ifelse",
    "  grestore",
    "} bind def",
    "%***********************************************************************",
    "%*                          Image decoding                             *",
    "%***********************************************************************",
    "DisplayImage",
    NULL
  };

  /*   C H E C K   P A R A M E T E R S   */
  
  if (Width <= 0 || Width > MAXWIDTH || Height <= 0 || Height > MAXWIDTH) {
    fprintf(stderr,
            "\nIncorrect image size: %d x %d\n", Width, Height);
    return 0;
  }

  if (Ncol <= 0 || Ncol > 256) {
    fprintf(stderr,"\nWrong number of colors: %d\n", Ncol);
    return 0;
  }

  /*   O U T P U T   H E A D E R   */

  Nbyte = 0;

  /*   O U T P U T   P O S T S C R I P T   P R O G R A M   */

  for (q=PostScript; *q; q++) {
    sprintf(s,"%s\n",*q);                          put_string;
  }

  /*   O U T P U T   I M A G E   D A T A   */

  sprintf(s,"%d %d\n", Width, Height);             put_string;
  sprintf(s,"%d\n",Ncol);                          put_string;

  for (k=0; k<Ncol; k++) {
    sprintf(s,"%02x%02x%02x", R[k], G[k], B[k]);   put_string; 
    if (k % 10 == 9 || k == Ncol-1) { 
      sprintf(s,"\n");                             put_string;
    }else{
      sprintf(s," ");                              put_string;
    }
  }

  /*   R U N - L E N G T H    C O M P R E S S I O N   */

  run   = 0;
  nc    = 0;
  s[72] = '\n';
  s[73] = '\0';
  for(y=0; y<Height; y++) {
    ptr = (data+y*Width);
    end = ptr + Width;
    if (y == 0) previous = *ptr++;
    while (ptr < end) {
      current = *ptr++;
      if (current == previous && run < 255) {
        run++;
        continue;
      }
      if (nc == 72) {
        put_string; 
        nc = 0;
      }
      s[nc++] = h[run / 16];
      s[nc++] = h[run % 16];
      s[nc++] = h[previous / 16];
      s[nc++] = h[previous % 16];
      previous = current;
      run = 0;
    }
  }

  if (nc == 72) {
    put_string; 
    nc = 0;
  }
  s[nc++] = h[run / 16];
  s[nc++] = h[run % 16];
  s[nc++] = h[previous / 16];
  s[nc++] = h[previous % 16];
  s[nc++] = '\n';
  s[nc]   = '\0';
  put_string; 
  return Nbyte;
}
