/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2017 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#define		BEGIN_PROLOG1	"\
/$F2psDict 200 dict def\n\
$F2psDict begin\n\
$F2psDict /mtrx matrix put\n\
/col-1 {0 setgray} bind def\n"

#define		BEGIN_PROLOG2	"\
/cp {closepath} bind def\n\
/ef {eofill} bind def\n\
/gr {grestore} bind def\n\
/gs {gsave} bind def\n\
/sa {save} bind def\n\
/rs {restore} bind def\n\
/l {lineto} bind def\n\
/rl {rlineto} bind def\n\
/m {moveto} bind def\n\
/rm {rmoveto} bind def\n\
/n {newpath} bind def\n\
/s {stroke} bind def\n\
/sh {show} bind def\n\
/slc {setlinecap} bind def\n\
/slj {setlinejoin} bind def\n\
/slw {setlinewidth} bind def\n\
/srgb {setrgbcolor} bind def\n\
/rot {rotate} bind def\n\
/sc {scale} bind def\n\
/sd {setdash} bind def\n\
/ff {findfont} bind def\n\
/sf {setfont} bind def\n\
/scf {scalefont} bind def\n\
/sw {stringwidth} bind def\n\
/tr {translate} bind def\n\
/tnt {dup dup currentrgbcolor\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add srgb}\n\
  bind def\n\
/shd {dup dup currentrgbcolor 4 -2 roll mul 4 -2 roll mul\n\
  4 -2 roll mul srgb} bind def\n\
/xfig_image {image Data flushfile} def\n"

#define		FILL_PAT01	"\
% left30\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 48 24]\n\
 /XStep 48\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .7 slw n\n\
  3 4 24 {dup -2 exch m 2 mul -1 l} for\n\
  1 4 22 {dup 2 mul 25 m 50 exch l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P1 exch def\n"
/*
 * % the line above
 *    3 4 24 {dup -2 exch m 2 mul -1 l} for	% is equivalent to ...
 * %	-2  3 m   6 -1 l
 * %	-2  7 m  14 -1 l
 * %	-2 11 m  22 -1 l
 * %		.
 * %		.
 * %	-2 23 m  46 -1 l
 *
 *    1 4 22 {dup 2 mul 25 m 50 exch l} for	% equivalent to ...
 * %	 2 25 m  50  1 l
 * %	10 25 m  50  5 l
 * %	18 25 m  50  9 l
 * %		.
 * %		.
 * %	42 25 m  50 21 l
 */

#define		FILL_PAT02	"\
% right30\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 48 24]\n\
 /XStep 48\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .7 slw n\n\
  -3 -4 -24 {dup -2 exch 24 add m -2 mul 25 l} for\n\
  -1 -4 -22 {dup -2 mul -1 m 50 exch 24 add l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P2 exch def\n"
/*
 * % the line above
 *   -3 -4 -24 {dup -2 exch 24 add m -2 mul 25 l} for	% is equivalent to ...
 * %	-2 21 m   6 25 l
 * %	-2 17 m  14 25 l
 * %	-2 13 m  22 25 l
 * %		.
 * %		.
 * %	-2  1 m  46 25 l
 *
 *   -1 -4 -22 {dup -2 mul -1 m 50 exch 24 add l} for	% equivalent to ...
 * %	 2 -1 m  50 23 l
 * %	10 -1 m  50 19 l
 * %	18 -1 m  50 15 l
 * %		.
 * %		.
 * %	42 -1 m  50  3 l
 */

#define		FILL_PAT03	"\
% crosshatch30\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 48 24]\n\
 /XStep 48\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .7 slw n\n\
  3 4 24 {dup 2 copy -2 exch m 2 mul -1 l -2 exch -1 mul 24 add m 2 mul 25 l} for\n\
  1 4 22 {dup 2 copy 2 mul 25 m 50 exch l 2 mul -1 m 50 exch -1 mul 24 add l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P3 exch def\n"

#define		FILL_PAT04	"\
% left45\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw n\n\
  7 8 24 {dup dup -1 exch m -1 l -2 add dup 25 m 25 exch l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P4 exch def\n"
/*
 * the procedure above was
 *  7 8 24 {dup -1 exch m -1 l} for\n\
 *  5 8 22 {dup 25 m 25 exch l} for\n\
 * equivalent to
 *  -1 7 m 7 -1 l  -1 15 m 15 -1 l  -1 23 m 23 -1 l
 *   5 25 m 25 5 l  13 25 m 25 13 l  21 25 m 25 21 l
 */

#define		FILL_PAT05	"\
% right45\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw n\n\
  1 8 18 {dup dup -1 m 25 exch -1 mul 24 add l 4 add dup -1 exch m\
 -1 mul 24 add 25 l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P5 exch def\n"

#define		FILL_PAT06	"\
% crosshatch45\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24] \n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw n\n\
  1 8 18 {dup 2 copy -1 m 25 exch -1 mul 24 add l 4 add dup -1 exch m\
 -1 mul 24 add 25 l\n\
    dup 6 add dup -1 exch m -1 l 4 add dup 25 m 25 exch l} for\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P6 exch def\n"

#define		FILL_PAT07	"\
% bricks\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 32 32]\n\
 /XStep 32\n\
 /YStep 32\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 4 {dup -1 exch m 34 0 rl 8 add} repeat pop\n\
  2 2 {dup 2 m 0 8 rl 0 8 rm 0 8 rl 16 add} repeat pop\n\
  10 2 {dup -1 m 0 3 rl 0 8 rm 0 8 rl 0 8 rm 0 7 rl 16 add} repeat pop\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P7 exch def\n"

#define		FILL_PAT08	"\
% vertical bricks\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 32 32]\n\
 /XStep 32\n\
 /YStep 32\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 4 {dup -1 m 0 34 rl 8 add} repeat pop\n\
  2 2 {dup 2 exch m 8 0 rl 8 0 rm 8 0 rl 16 add} repeat pop\n\
  10 2 {dup -1 exch m 3 0 rl 8 0 rm 8 0 rl 8 0 rm 7 0 rl 16 add} repeat pop\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P8 exch def\n"

#define		FILL_PAT09	"\
% horizontal lines\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 36 24]\n\
 /XStep 36\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  -2 6 {4 add dup -1 exch m 38 0 rl} repeat pop s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P9 exch def"

#define		FILL_PAT10	"\
% vertical lines\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 36]\n\
 /XStep 24\n\
 /YStep 36\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  -2 6 {4 add dup -1 m 0 38 rl} repeat pop s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P10 exch def\n"

#define		FILL_PAT11	"\
% crosshatch lines\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 36 36]\n\
 /XStep 36\n\
 /YStep 36\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  -2 9 {4 add dup dup -1 exch m 38 0 rl -1 m 0 38 rl} repeat pop s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P11 exch def\n"

#define		FILL_PAT12	"\
% left-shingles\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 3 {dup -1 exch m 26 0 rl 8 add} repeat pop\n\
  2 10 m 6 18 l 10 2 m 14 10 l 18 18 m 22 26 l 20 -2 m 22 2 l\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P12 exch def\n"

#define		FILL_PAT13	"\
% right-shingles\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 3 {dup -1 exch m 26 0 rl 8 add} repeat pop\n\
  2 26 m 6 18 l 14 2 m 10 10 l 18 18 m 22 10 l 2 2 m 4 -2 l\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P13 exch def\n"

#define		FILL_PAT14	"\
% vertical left-shingles\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 3 {dup -1 m 0 26 rl 8 add} repeat pop\n\
  26 2 m 18 6 l 2 14 m 10 10 l 18 18 m 10 22 l 2 2 m -2 4 l\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P14 exch def\n"

#define		FILL_PAT15	"\
% vertical right-shingles\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  1 slw 0 slc n\n\
  2 3 {dup -1 m 0 26 rl 8 add} repeat pop\n\
  10 2 m 18 6 l 2 10 m 10 14 l 18 18 m 26 22 l -2 20 m 2 22 l\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P15 exch def\n"

#define		FILL_PAT16	"\
% fishscales\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 16 8]\n\
 /XStep 16\n\
 /YStep 8\n\
 /PaintProc\n\
 {\n\
  pop\n\
  0.7 slw 0 slc\n\
  8 9 8 17 8 25 0 13 16 13 0 21 16 21 7 {n 11 223 317 arc s} repeat\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P16 exch def\n"

#define		FILL_PAT17	"\
% small fishscales\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 16]\n\
 /XStep 24\n\
 /YStep 16\n\
 /PaintProc\n\
 {\n\
  pop\n\
  0.7 slw 0 slc 2 slj\n\
  n -6 8 27 {2 4 180 360 arc} for s\n\
  n -2 8 23 {6 4 180 360 arc} for s\n\
  n -6 8 27 {10 4 180 360 arc} for s\n\
  n -2 8 23 {14 4 180 360 arc} for s\n\
  n -6 8 27 {18 4 180 360 arc} for s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P17 exch def\n"

#define		FILL_PAT18	"\
% circles\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 16 16]\n\
 /XStep 16\n\
 /YStep 16\n\
 /PaintProc\n\
 {\n\
  pop\n\
  0.7 slw\n\
  n 8 8 8 0 360 arc s\n\
 } bind\n\
>>\n\
\n\
matrix\n\
makepattern\n\
/P18 exch def\n"

#define		FILL_PAT19	"\
% hexagons\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 26 16]\n\
 /XStep 26\n\
 /YStep 16\n\
 /PaintProc\n\
 {\n\
  pop\n\
  0.7 slw 0 slj n\n\
  -1 8 m 2 8 l 6 0 l 15 0 l 19 8 l 15 16 l 6 16 l 2 8 l\n\
  19 8 m 27 8 l s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P19 exch def\n"

#define		FILL_PAT20	"\
% octagons\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 16 16]\n\
 /XStep 16\n\
 /YStep 16\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .8 slw 0 slj n\n\
  5 0 m 11 0 l 16 5 l 16 11 l 11 16 l 5 16 l 0 11 l 0 5 l cp s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P20 exch def\n"

#define		FILL_PAT21	"\
% horizontal sawtooth lines\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .8 slw 0 slj n\n\
  -1 5 m 2 2 l 3 {4 4 rl 4 -4 rl} repeat\n\
  -1 13 m 2 10 l 3 {4 4 rl 4 -4 rl} repeat\n\
  -1 21 m 2 18 l 3 {4 4 rl 4 -4 rl} repeat\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P21 exch def\n"

#define		FILL_PAT22	"\
% vertical sawtooth lines\n\
<<\n\
 /PatternType 1\n\
 /PaintType 2\n\
 /TilingType 1\n\
 /BBox [0 0 24 24]\n\
 /XStep 24\n\
 /YStep 24\n\
 /PaintProc\n\
 {\n\
  pop\n\
  .8 slw 0 slj n\n\
  5 -1 m 2 2 l 3 {4 4 rl -4 4 rl} repeat\n\
  13 -1 m 10 2 l 3 {4 4 rl -4 4 rl} repeat\n\
  21 -1 m 18 2 l 3 {4 4 rl -4 4 rl} repeat\n\
  s\n\
 } bind\n\
>>\n\
matrix\n\
makepattern\n\
/P22 exch def\n"

#define		XFIG_CMAP1	"\
%%BeginResource: CMap (Xfig-UTF8-H)\n\
%%Title: (Xfig-UTF8-H Adobe 0)\n\
%%Version: 1.0\n\
% The PostScript Language Reference, 3rd ed., Section 5.11.4 - CMap Example,\n\
% pp 385-387 and the example for the composefont operator on p 388 was useful.\n\
/CIDInit /ProcSet findresource begin\n\
12 dict begin\n\
begincmap\n\
/CIDSystemInfo [ null ] def\n\
% Xfig Adobe Glyph List\n\
/CMapName /Xfig-AGL-UTF8-H def\n\
/CMapVersion 1.0 def\n\
/CMapType 1 def\n\
/WMode 0 def\n\
% See Adobe Technical Note #5099,\n\
% Building CMap Files for CID-Keyed Fonts (1998), 5099.CMapFiles.pdf, Sec. 3.4:\n\
% \"...the theoretical code space range should be used here, not just the code\n\
%  space range that contains defined characters.\"\n\
%\n\
% Define the UTF-8 code space range\n\
3 begincodespacerange\n\
     <00>     <7F>\n\
   <C280>   <DFBF>\n\
 <E08080> <EFBFBF>\n\
% <F0808080> <F7BFBFBF>\n\
endcodespacerange\n\
0 usefont\n\
42 beginbfrange\n\
% Basic Latin, or ASCII\n\
<20> <7f> [ /space /exclam /quotedbl /numbersign /dollar /percent /ampersand\n\
 /quotesingle /parenleft /parenright /asterisk /plus /comma /hyphen /period\n\
 /slash /zero /one /two /three /four /five /six /seven /eight /nine /colon\n\
 /semicolon /less /equal /greater /question /at /A /B /C /D /E /F /G /H /I /J /K\n\
 /L /M /N /O /P /Q /R /S /T /U /V /W /X /Y /Z /bracketleft /backslash\n\
 /bracketright /asciicircum /underscore /grave /a /b /c /d /e /f /g /h /i /j /k\n\
 /l /m /n /o /p /q /r /s /t /u /v /w /x /y /z /braceleft /bar /braceright\n\
 /asciitilde /controlDEL ]\n\
% Latin-1 Supplement, u+a0--u+bf\n\
<c2a0> <c2bf> [ /nbspace /exclamdown /cent /sterling /currency /yen /brokenbar\n\
 /section /dieresis /copyright /ordfeminine /guillemotleft /logicalnot\n\
 /sfthyphen /registered /macron /degree /plusminus /twosuperior /threesuperior\n\
 /acute /mu /paragraph /periodcentered /cedilla /onesuperior /ordmasculine\n\
 /guillemotright /onequarter /onehalf /threequarters /questiondown ]\n\
% Latin-1 Supplement, u+c0--u+ff\n\
<c380> <c3bf> [ /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE\n\
 /Ccedilla /Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex\n\
 /Idieresis /Eth /Ntilde /Ograve /Oacute /Ocircumflex /Otilde /Odieresis\n\
 /multiply /Oslash /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn\n\
 /germandbls /agrave /aacute /acircumflex /atilde /adieresis /aring /ae\n\
 /ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex\n\
 /idieresis /eth /ntilde /ograve /oacute /ocircumflex /otilde /odieresis /divide\n\
 /oslash /ugrave /uacute /ucircumflex /udieresis /yacute /thorn /ydieresis ]\n\
% Latin Extended-A, u+0100--u+013f\n\
<c480> <c4bf> [ /Amacron /amacron /Abreve /abreve /Aogonek /aogonek /Cacute\n\
 /cacute /Ccircumflex /ccircumflex /Cdotaccent /cdotaccent /Ccaron /ccaron\n\
 /Dcaron /dcaron /Dcroat /dcroat /Emacron /emacron /Ebreve /ebreve /Edotaccent\n\
 /edotaccent /Eogonek /eogonek /Ecaron /ecaron /Gcircumflex /gcircumflex /Gbreve\n\
 /gbreve /Gdotaccent /gdotaccent /Gcedilla /gcedilla /Hcircumflex /hcircumflex\n\
 /Hbar /hbar /Itilde /itilde /Imacron /imacron /Ibreve /ibreve /Iogonek /iogonek\n\
 /Idotaccent /dotlessi /IJ /ij /Jcircumflex /jcircumflex /Kcedilla /kcedilla\n\
 /kgreenlandic /Lacute /lacute /Lcedilla /lcedilla /Lcaron /lcaron /Ldot ]\n\
% Latin Extended-A, u+0140--u+017f\n\
<c580> <c5bf> [ /ldot /Lslash /lslash /Nacute /nacute /Ncedilla /ncedilla\n\
 /Ncaron /ncaron /napostrophe /Eng /eng /Omacron /omacron /Obreve /obreve\n\
 /Ohungarumlaut /ohungarumlaut /OE /oe /Racute /racute /Rcedilla /rcedilla\n\
 /Rcaron /rcaron /Sacute /sacute /Scircumflex /scircumflex /Scedilla /scedilla\n\
 /Scaron /scaron /Tcedilla /tcedilla /Tcaron /tcaron /Tbar /tbar /Utilde /utilde\n\
 /Umacron /umacron /Ubreve /ubreve /Uring /uring /Uhungarumlaut /uhungarumlaut\n\
 /Uogonek /uogonek /Wcircumflex /wcircumflex /Ycircumflex /ycircumflex\n\
 /Ydieresis /Zacute /zacute /Zdotaccent /zdotaccent /Zcaron /zcaron /longs ]\n"

#define		XFIG_CMAP2	"\
% Latin Extended-B (small part), u+1fa--u+1ff\n\
<c7ba> <c7bf> [ /Aringacute /aringacute /AEacute /aeacute /Oslashacute\n\
 /oslashacute ]\n\
<c898> <c89b> [ /Scommaaccent /scommaaccent /uni021A /uni021B ]\n\
% Spacing Modifier Letters (small part), u+2d8--u2dd\n\
<cb98> <cb9d> [ /breve /dotaccent /ring /ogonek /tilde /hungarumlaut ]\n\
% Greek and Coptic, u+384--u+386\n\
<ce84> <ce86> [ /tonos /dieresistonos /Alphatonos ]\n\
<ce88> <ce8a> [ /Epsilontonos /Etatonos /Iotatonos ]\n\
<ce8e> <cea1> [ /Upsilontonos /Omegatonos /iotadieresistonos /Alpha /Beta\n\
  /Gamma /uni0394 /Epsilon /Zeta /Eta /Theta /Iota /Kappa /Lambda /Mu /Nu /Xi\n\
  /Omicron /Pi /Rho ]\n\
<cea3> <cebf> [ /Sigma /Tau /Upsilon /Phi /Chi /Psi /uni03A9 /Iotadieresis\n\
  /Upsilondieresis /alphatonos /epsilontonos /etatonos /iotatonos\n\
  /upsilondieresistonos /alpha /beta /gamma /delta /epsilon /zeta /eta /theta\n\
  /iota /kappa /lambda /uni03BC /nu /xi /omicron ]\n\
<cf82> <cf8e> [ /uni03C2 /sigma /tau /upsilon /phi /chi /psi /omega /iotadieresis\n\
  /upsilondieresis /omicrontonos /upsilontonos /omegatonos ]\n\
% Cyrillic, u+400--u+43f\n\
<d080> <d0bf> [ /uni0400 /afii10023 /afii10051 /afii10052 /afii10053 /afii10054\n\
  /afii10055 /afii10056 /afii10057 /afii10058 /afii10059 /afii10060 /afii10061\n\
  /uni040D /afii10062 /afii10145 /afii10017 /afii10018 /afii10019\n\
  /afii10020 /afii10021 /afii10022 /afii10024 /afii10025 /afii10026 /afii10027\n\
  /afii10028 /afii10029 /afii10030 /afii10031 /afii10032 /afii10033 /afii10034\n\
  /afii10035 /afii10036 /afii10037 /afii10038 /afii10039 /afii10040 /afii10041\n\
  /afii10042 /afii10043 /afii10044 /afii10045 /afii10046 /afii10047 /afii10048\n\
  /afii10049 /afii10065 /afii10066 /afii10067 /afii10068 /afii10069 /afii10070\n\
  /afii10072 /afii10073 /afii10074 /afii10075 /afii10076 /afii10077 /afii10078\n\
  /afii10079 /afii10080 /afii10081 ]\n\
% Cyrillic, u+440--u+463\n\
<d180> <d19f> [ /afii10082 /afii10083 /afii10084 /afii10085 /afii10086\n\
  /afii10087 /afii10088 /afii10089 /afii10090 /afii10091 /afii10092 /afii10093\n\
  /afii10094 /afii10095 /afii10096 /afii10097 /uni0450 /afii10071 /afii10099\n\
  /afii10100 /afii10101 /afii10102 /afii10103 /afii10104 /afii10105 /afii10106\n\
  /afii10107 /afii10108 /afii10109 /uni045D /afii10110 /afii10193 ]\n\
<d1b2> <d1b5> [ /uni0472 /uni0473 /uni0474 /uni0475 ]\n\
<d290> <d293> [ /afii10050 /afii10098 /uni0492 /uni0493 ]\n\
<d296> <d29d> [ /uni0496 /uni0497 /uni0498 /uni0499 /uni049A /uni049B /uni049C\n\
  /uni049D ]\n\
<d2a0> <d2a3> [ /uni04A0 /uni04A1 /uni04A2 /uni04A3 ]\n\
<d2ae> <d2b3> [ /uni04AE /uni04AF /uni04B0 /uni04B1 /uni04B2 /uni04B3 ]\n\
<d2b6> <d2bb> [ /uni04B6 /uni04B7 /uni04B8 /uni04B9 /uni04BA /uni04BB ]\n\
% Latin Extended Additional, u+1e80--u+1e85\n\
<e1ba80> <e1ba85> [ /Wgrave /wgrave /Wacute /wacute /Wdieresis /wdieresis ]\n\
<e28093> <e28095> [ /endash /emdash /afii00208 ]\n\
<e28097> <e2809e> [ /underscoredbl /quoteleft /quoteright /quotesinglbase\n\
 /quotereversed /quotedblleft /quotedblright /quotedblbase ]\n\
<e280a0> <e280a2> [ /dagger /daggerdbl /bullet ]\n\
<e280b9> <e280bc> [ /guilsinglleft /guilsinglright /exclamdbl ]\n\
<e281b4> <e281b9> [ /foursuperior /fivesuperior /sixsuperior /sevensuperior\n\
 /eightsuperior /ninesuperior ]\n\
<e2859b> <e2859e> [ /oneeighth /threeeighths /fiveeighths /seveneighths ]\n\
<e28690> <e28699> [ /arrowleft /arrowup /arrowright /arrowdown /arrowboth\n\
 /arrowupdn /uni2196 /uni2197 /uni2198 /uni2199 ]\n\
<e28790> <e28795> [ /arrowdblleft /arrowdblup /arrowdblright /arrowdbldown\n\
 /arrowdblboth /uni21D5]\n\
<e28885> <e28889> [ /emptyset /Delta /gradient /element /notelement ]\n\
<e2889d> <e288a0> [ /proportional /infinity /orthogonal /angle ]\n\
<e288a7> <e288ab> [ /uni2227 /uni2228 /intersection /union /integral ]\n\
<e28a82> <e28a87> [ /propersubset /propersuperset /notsubset /uni2285\n\
 /reflexsubset /reflexsuperset ]\n\
<e28a95> <e28a99> [ /uni2295 /uni2296 /circlemultiply /uni2298 /uni2299 ]\n\
<e28aa2> <e28aa5> [ /uni22A2 /uni22A3 /uni22A4 /perpendicular ]\n\
<e29590> <e295b0> [ /SF430000 /SF240000 /SF510000 /SF520000 /SF390000 /SF220000\n"

#define		XFIG_CMAP3	"\
 /SF210000 /SF250000 /SF500000 /SF490000 /SF380000 /SF280000 /SF270000 /SF260000\n\
 /SF360000 /SF370000 /SF420000 /SF190000 /SF200000 /SF230000 /SF470000 /SF480000\n\
 /SF410000 /SF450000 /SF460000 /SF400000 /SF540000 /SF530000 /SF440000 /uni256D\n\
 /uni256E /uni256F /uni2570 ]\n\
<e29690> <e29693> [ /rtblock /ltshade /shade /dkshade ]\n\
<e298b9> <e298bc> [ /uni25B9 /smileface /invsmileface /sun ]\n\
<eebf80> <eebfac> [ /uniEFC0 /uniEFC1 /uniEFC2 /uniEFC3 /uniEFC4 /uniEFC5\n\
 /uniEFC6 /uniEFC7 /uniEFC8 /uniEFC9 /uniEFCA /uniEFCB /uniEFCC /uniEFCD\n\
 /uniEFCE /uniEFCF /uniEFD0 /uniEFD1 /uniEFD2 /uniEFD3 /uniEFD4 /uniEFD5\n\
 /uniEFD6 /uniEFD7 /uniEFD8 /uniEFD9 /uniEFDA /uniEFDB /uniEFDC /uniEFDD\n\
 /uniEFDE /uniEFDF /uniEFE0 /uniEFE1 /uniEFE2 /uniEFE3 /uniEFE4 /uniEFE5\n\
 /uniEFE6 /uniEFE7 /uniEFE8 /uniEFE9 /uniEFEA /uniEFEB /uniEFEC ]\n\
<eebfba> <eebfbf> [ /uniEFFA /uniEFFB /uniEFFC /uniEFFD /uniEFFE /uniEFFF ]\n\
<efac80> <efac84> [ /ff /fi /fl /ffi /ffl ]\n\
endbfrange\n\
\n\
100 beginbfchar\n\
   <c692> /florin\n\
   <cb86> /circumflex\n\
   <cb87> /caron\n\
   <cb89> /uni2C9\n\
   <ce87> /uni0387\n\
   <ce8c> /Omicrontonos\n\
   <cf80> /pi\n\
   <cf81> /rho\n\
   <cf91> /theta1\n\
   <c195> /phi1\n\
   <c196> /omega1\n\
   <d1a2> /uni0462\n\
   <d1a3> /uni0463\n\
   <d2aa> /uni04AA\n\
   <d2ab> /uni04AB\n\
   <d380> /uni04C0\n\
   <d38b> /uni04CB\n\
   <d38c> /uni04CC\n\
   <d38f> /uni04CF\n\
   <d398> /uni04D8\n\
   <d399> /afii10846\n\
   <d3a2> /uni04E2\n\
   <d3a3> /uni04E3\n\
   <d3a8> /uni04E8\n\
   <d3a9> /uni04E9\n\
   <d3ae> /uni04EE\n\
   <d3af> /uni04EF\n\
 <e1bbb2> /Ygrave\n\
 <e1bbb3> /ygrave\n\
 <e28082> /uni2002\n\
 <e280a6> /ellipsis\n\
 <e280b0> /perthousand\n\
 <e280b2> /minute\n\
 <e280b3> /second\n\
 <e280be> /uni203E\n\
 <e28184> /fraction\n\
 <e281b0> /zerosuperior\n\
 <e281bf> /nsuperior\n\
 <e282a3> /franc\n\
 <e282a2> /lira	% would be e282a4, but this really is the /cruzeiro\n\
 <e282a7> /peseta\n\
 <e282ac> /Euro\n\
 <e282af> /uni20AF\n\
 <e2839d> /uni20DD\n\
 <e28485> /afii61248\n\
 <e2848f> /uni210F\n\
 <e28491> /Ifraktur\n\
 <e28492> /uni2112\n\
 <e28493> /afii61289\n\
 <e28496> /afii61352\n\
 <e28498> /weierstrass\n\
 <e2849c> /Rfraktur\n\
 <e2849e> /uni211E\n\
 <e284a0> /uni2120\n\
 <e284a2> /trademark\n\
 <e284a6> /uni2126\n\
 <e284a8> /uni2128\n\
 <e284ad> /uni212D\n\
 <e284ae> /estimated\n\
 <e284af> /uni212F\n\
 <e284b5> /aleph\n\
 <e284b6> /uni2136\n\
 <e284b7> /uni2137\n\
 <e286a8> /arrowupdnbse\n\
 <e286b5> /carriagereturn\n\
 <e28784> /uni21C4\n\
 <e28786> /uni21C6\n\
 <e28880> /universal\n\
 <e28882> /partialdiff\n\
 <e28883> /existential\n\
 <e2888b> /suchthat\n\
 <e2888d> /uni220D\n\
 <e2888f> /product\n\
 <e28891> /summation\n\
 <e28892> /minus\n\
 <e28893> /uni2213\n\
 <e28895> /uni2215\n\
 <e28897> /asteriskmath\n\
 <e28899> /uni2219\n\
 <e2889a> /radical\n\
 <e288a3> /uni2223\n\
 <e288a5> /uni2225\n\
 <e288ae> /uni222E\n\
 <e288b4> /therefore\n\
 <e288b5> /uni2235\n\
 <e288b7> /uni2237\n\
 <e28985> /congruent\n\
 <e28988> /approxequal\n\
 <e289a0> /notequal\n\
 <e289a1> /equivalence\n\
 <e289a2> /uni2262\n\
 <e289a4> /lessequal\n\
 <e289a5> /greaterequal\n\
 <e289aa> /uni226A\n\
 <e289ab> /uni226B\n\
 <e28abb> /uni22BB\n\
 <e28c82> /house\n\
 <e28c90> /revlogicalnot\n\
 <e28ca0> /integraltp\n\
 <e28ca1> /integralbt\n\
endbfchar\n\
48 beginbfchar\n\
 <e28ca9> /angleleft\n\
 <e28caa> /angleright\n\
 <e29480> /SF100000\n\
 <e29482> /SF110000\n\
 <e2948c> /SF010000\n\
 <e29490> /SF030000\n\
 <e29494> /SF020000\n\
 <e29498> /SF040000\n\
 <e2949c> /SF080000\n\
 <e294a4> /SF090000\n\
 <e294ac> /SF060000\n\
 <e294b4> /SF070000\n\
 <e294bc> /SF050000\n\
 <e29680> /upblock\n\
 <e29684> /dnblock\n\
 <e29688> /block\n\
 <e2968c> /lfblock\n\
 <e296a0> /filledbox\n\
 <e296a1> /uni25A1\n\
 <e296aa> /H18543\n\
 <e296ab> /H18551\n\
 <e296ac> /filledrect\n\
 <e296b2> /triagup\n\
 <e296b5> /uni25B5\n\
 <e296ba> /triagrt\n\
 <e296bc> /triagdn\n\
 <e296bf> /uni25BF\n\
 <e29783> /uni25C3\n\
 <e29784> /triaglf\n\
 <e29786> /uni25C6\n\
 <e29787> /uni25C7\n\
 <e2978a> /lozenge\n\
 <e2978b> /circle\n\
 <e2978f> /H18533\n\
 <e29798> /invbullet\n\
 <e29799> /invcircle\n\
 <e297a6> /openbullet\n\
 <e29980> /female\n\
 <e29982> /male\n\
 <e299a0> /spade\n\
 <e299a3> /club\n\
 <e299a5> /heart\n\
 <e3809a> /uni301A\n\
 <e3809b> /uni301B\n\
 <e299a6> /diamond\n\
 <e299aa> /musicalnote\n\
 <e299ab> /musicalnotedbl\n\
 <eebebf> /uniEFBF\n\
endbfchar\n\
endcmap\n\
currentdict CMapName exch /CMap defineresource pop\n\
end\n\
end\n\
%%EndResource\n"

#define		ELLIPSE_PS	" \
/DrawEllipse {\n\
	/endangle exch def\n\
	/startangle exch def\n\
	/yrad exch def\n\
	/xrad exch def\n\
	/y exch def\n\
	/x exch def\n\
	/savematrix mtrx currentmatrix def\n\
	x y tr xrad yrad sc 0 0 1 startangle endangle arc\n\
	closepath\n\
	savematrix setmatrix\n\
	} def\n\
"
/* The original PostScript definition for adding a spline section to the
 * current path uses recursive bisection.  The following definition using the
 * curveto operator is more efficient since it executes at compiled rather
 * than interpreted code speed.  The Bezier control points are 2/3 of the way
 * from z1 (and z3) to z2.
 *
 * ---Rene Llames, 21 July 1988.
 */
#define		SPLINE_PS	" \
/DrawSplineSection {\n\
	/y3 exch def\n\
	/x3 exch def\n\
	/y2 exch def\n\
	/x2 exch def\n\
	/y1 exch def\n\
	/x1 exch def\n\
	/xa x1 x2 x1 sub 0.666667 mul add def\n\
	/ya y1 y2 y1 sub 0.666667 mul add def\n\
	/xb x3 x2 x3 sub 0.666667 mul add def\n\
	/yb y3 y2 y3 sub 0.666667 mul add def\n\
	x1 y1 lineto\n\
	xa ya xb yb x3 y3 curveto\n\
	} def\n\
"
#define		END_PROLOG	"\
/$F2psBegin {$F2psDict begin /$F2psEnteredState save def} def\n\
/$F2psEnd {$F2psEnteredState restore end} def\n\
"
