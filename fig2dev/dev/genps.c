/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
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

/*
 *	genps.c: PostScript driver for fig2dev
 *
 *	Modified by Herbert Bauer to support ISO-Characters,
 *	multiple page output, color mode etc.
 *	heb@regent.e-technik.tu-muenchen.de
 *
 *	Modified by Eric Picheral to support the whole set of ISO-Latin-1
 *	Modified by Herve Soulard to allow non-iso coding on special fonts
 *	Herve.Soulard@inria.fr (8 Apr 1993)

*/

#include <sys/param.h>
#if defined(hpux) || defined(SYSV) || defined(BSD4_3) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <stdio.h>
#include <math.h>
#include <pwd.h>
#include <errno.h>
extern char *sys_errlist[];
#include "pi.h"
#include "fig2dev.h"
#include "object.h"
#include "psfonts.h"
#include <string.h>
#include <time.h>

/* for the xpm package */
#ifdef USE_XPM
#include <X11/xpm.h>
int	XpmReadFileToXpmImage();
#endif

/* for the version nubmer */
#include "../../patchlevel.h"

int	ReadFromBitmapFile();

/* ratio of resolution to 80ppi */
extern float	THICK_SCALE;

struct pagedef
{
    char *name;			/* name for page size */
    int width;			/* page width in points */
    int height;			/* page height in points */
};

struct pagedef pagedef[] =
{
    {"A4", 595, 842}, 		/* 21cm x 29.7cm */
    {"B5", 516, 729}, 		/* 18.2cm x 25.7cm */
    {"Letter", 612, 792}, 	/* 8.5" x 11" */
    {"Legal", 612, 1008}, 	/* 8.5" x 14" */
    {"Ledger", 1224, 792}, 	/*  17" x 11" */
    {NULL, 0, 0}
};

#ifdef A4
char *pagesize = "A4";
#else
char *pagesize = "Letter";
#endif

#define		TRUE			1
#define		FALSE			0
#define		POINT_PER_INCH		72
#define		ULIMIT_FONT_SIZE	300

#define		min(a, b)		(((a) < (b)) ? (a) : (b))

int		pagewidth = -1;
int		pageheight = -1;
int		xoff=0;
int		yoff=0;
static int	coord_system;
static int	resolution;
int		show_page = 0;
static double	cur_thickness = 0.0;
static int	cur_joinstyle = 0;
static int	cur_capstyle = 0;
int		pages;
int		no_obj = 0;
int		multi_page = FALSE;

static	arc_tangent();
static	draw_arrow_head();
static	fill_area();
static	iso_text_exist();
static	encode_all_fonts();
static	ellipse_exist();
static	normal_spline_exist();

#define SHADEVAL(F)	1.0*(F)/(NUMSHADES-1)
#define TINTVAL(F)	1.0*(F-NUMSHADES+1)/NUMTINTS

struct	_rgb {
	float r, g, b;
	}
    rgbcols[32] = {
	{0, 0, 0},
	{0, 0, 1},
	{0, 1, 0},
	{0, 1, 1},
	{1, 0, 0},
	{1, 0, 1},
	{1, 1, 0},
	{1, 1, 1},
	{0, 0, .56},
	{0, 0, .69},
	{0, 0, .82},
	{.53, .81, 1},
	{0, .56, 0},
	{0, .69, 0},
	{0, .82, 0},
	{0, .56, .56},
	{0, .69, .69},
	{0, .82, .82},
	{.56, 0, 0},
	{.69, 0, 0},
	{.82, 0, 0},
	{.56, 0, .56},
	{.69, 0, .69},
	{.82, 0, .82},
	{.5, .19, 0},
	{.63, .25, 0},
	{.75, .38, 0},
	{1, .5, .5},
	{1, .63, .63},
	{1, .75, .75},
	{1, .88, .88},
	{1, .84, 0}
    };

#define		BEGIN_PROLOG1	"\
/$F2psDict 200 dict def\n\
$F2psDict begin\n\
$F2psDict /mtrx matrix put\n\
/col-1 {} def\n\
"

#define		BEGIN_PROLOG2	"\
/clp {closepath} bind def\n\
/ef {eofill} bind def\n\
/gr {grestore} bind def\n\
/gs {gsave} bind def\n\
/l {lineto} bind def\n\
/m {moveto} bind def\n\
/n {newpath} bind def\n\
/s {stroke} bind def\n\
/slc {setlinecap} bind def\n\
/slj {setlinejoin} bind def\n\
/slw {setlinewidth} bind def\n\
/srgb {setrgbcolor} bind def\n\
/rot {rotate} bind def\n\
/sc {scale} bind def\n\
/tr {translate} bind def\n\
/tnt {dup dup currentrgbcolor\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add\n\
  4 -2 roll dup 1 exch sub 3 -1 roll mul add srgb}\n\
  bind def\n\
/shd {dup dup currentrgbcolor 4 -2 roll mul 4 -2 roll mul\n\
  4 -2 roll mul srgb} bind def\n\
"


#define		FILL_PROLOG1	"\
% This junk string is used by the show operators\n\
/PATsstr 1 string def\n\
/PATawidthshow { 	% cx cy cchar rx ry string\n\
  % Loop over each character in the string\n\
  {  % cx cy cchar rx ry char\n\
    % Show the character\n\
    dup				% cx cy cchar rx ry char char\n\
    PATsstr dup 0 4 -1 roll put	% cx cy cchar rx ry char (char)\n\
    false charpath		% cx cy cchar rx ry char\n\
    /clip load PATdraw\n\
    % Move past the character (charpath modified the\n\
    % current point)\n\
    currentpoint			% cx cy cchar rx ry char x y\n\
    newpath\n\
    moveto			% cx cy cchar rx ry char\n\
    % Reposition by cx,cy if the character in the string is cchar\n\
    3 index eq {			% cx cy cchar rx ry\n\
      4 index 4 index rmoveto\n\
    } if\n\
    % Reposition all characters by rx ry\n\
    2 copy rmoveto		% cx cy cchar rx ry\n\
  } forall\n\
  pop pop pop pop pop		% -\n\
  currentpoint\n\
  newpath\n\
  moveto\n\
} bind def\n\
"
#define		FILL_PROLOG2	"\
/PATcg {\n\
  7 dict dup begin\n\
    /lw currentlinewidth def\n\
    /lc currentlinecap def\n\
    /lj currentlinejoin def\n\
    /ml currentmiterlimit def\n\
    /ds [ currentdash ] def\n\
    /cc [ currentrgbcolor ] def\n\
    /cm matrix currentmatrix def\n\
  end\n\
} bind def\n\
% PATdraw - calculates the boundaries of the object and\n\
% fills it with the current pattern\n\
/PATdraw {			% proc\n\
  save exch\n\
    PATpcalc			% proc nw nh px py\n\
    5 -1 roll exec		% nw nh px py\n\
    newpath\n\
    PATfill			% -\n\
  restore\n\
} bind def\n\
"
#define		FILL_PROLOG3	"\
% PATfill - performs the tiling for the shape\n\
/PATfill { % nw nh px py PATfill -\n\
  PATDict /CurrentPattern get dup begin\n\
    setfont\n\
    % Set the coordinate system to Pattern Space\n\
    PatternGState PATsg\n\
    % Set the color for uncolored pattezns\n\
    PaintType 2 eq { PATDict /PColor get PATsc } if\n\
    % Create the string for showing\n\
    3 index string		% nw nh px py str\n\
    % Loop for each of the pattern sources\n\
    0 1 Multi 1 sub {		% nw nh px py str source\n\
	% Move to the starting location\n\
	3 index 3 index		% nw nh px py str source px py\n\
	moveto			% nw nh px py str source\n\
	% For multiple sources, set the appropriate color\n\
	Multi 1 ne { dup PC exch get PATsc } if\n\
	% Set the appropriate string for the source\n\
	0 1 7 index 1 sub { 2 index exch 2 index put } for pop\n\
	% Loop over the number of vertical cells\n\
	3 index 		% nw nh px py str nh\n\
	{			% nw nh px py str\n\
	  currentpoint		% nw nh px py str cx cy\n\
	  2 index show		% nw nh px py str cx cy\n\
	  YStep add moveto	% nw nh px py str\n\
	} repeat		% nw nh px py str\n\
    } for\n\
    5 { pop } repeat\n\
  end\n\
} bind def\n\
"
#define		FILL_PROLOG4	"\
% PATkshow - kshow with the current pattezn\n\
/PATkshow {			% proc string\n\
  exch bind			% string proc\n\
  1 index 0 get			% string proc char\n\
  % Loop over all but the last character in the string\n\
  0 1 4 index length 2 sub {\n\
				% string proc char idx\n\
    % Find the n+1th character in the string\n\
    3 index exch 1 add get	% string proe char char+1\n\
    exch 2 copy			% strinq proc char+1 char char+1 char\n\
    % Now show the nth character\n\
    PATsstr dup 0 4 -1 roll put	% string proc chr+1 chr chr+1 (chr)\n\
    false charpath		% string proc char+1 char char+1\n\
    /clip load PATdraw\n\
    % Move past the character (charpath modified the current point)\n\
    currentpoint newpath moveto\n\
    % Execute the user proc (should consume char and char+1)\n\
    mark 3 1 roll		% string proc char+1 mark char char+1\n\
    4 index exec		% string proc char+1 mark...\n\
    cleartomark			% string proc char+1\n\
  } for\n\
  % Now display the last character\n\
  PATsstr dup 0 4 -1 roll put	% string proc (char+1)\n\
  false charpath		% string proc\n\
  /clip load PATdraw\n\
  neewath\n\
  pop pop			% -\n\
} bind def\n\
"
#define		FILL_PROLOG5	"\
% PATmp - the makepattern equivalent\n\
/PATmp {			% patdict patmtx PATmp patinstance\n\
  exch dup length 7 add		% We will add 6 new entries plus 1 FID\n\
  dict copy			% Create a new dictionary\n\
  begin\n\
    % Matrix to install when painting the pattern\n\
    TilingType PATtcalc\n\
    /PatternGState PATcg def\n\
    PatternGState /cm 3 -1 roll put\n\
    % Check for multi pattern sources (Level 1 fast color patterns)\n\
    currentdict /Multi known not { /Multi 1 def } if\n\
    % Font dictionary definitions\n\
    /FontType 3 def\n\
    % Create a dummy encoding vector\n\
    /Encoding 256 array def\n\
    3 string 0 1 255 {\n\
      Encoding exch dup 3 index cvs cvn put } for pop\n\
    /FontMatrix matrix def\n\
    /FontBBox BBox def\n\
    /BuildChar {\n\
	mark 3 1 roll		% mark dict char\n\
	exch begin\n\
	Multi 1 ne {PaintData exch get}{pop} ifelse  % mark [paintdata]\n\
	  PaintType 2 eq Multi 1 ne or\n\
	  { XStep 0 FontBBox aload pop setcachedevice }\n\
	  { XStep 0 setcharwidth } ifelse\n\
	  currentdict		% mark [paintdata] dict\n\
	  /PaintProc load	% mark [paintdata] dict paintproc\n\
	end\n\
	gsave\n\
	  false PATredef exec true PATredef\n\
	grestore\n\
	cleartomark		% -\n\
    } bind def\n\
    currentdict\n\
  end				% newdict\n\
  /foo exch			% /foo newlict\n\
  definefont			% newfont\n\
} bind def\n\
"
#define		FILL_PROLOG6	"\
% PATpcalc - calculates the starting point and width/height\n\
% of the tile fill for the shape\n\
/PATpcalc {	% - PATpcalc nw nh px py\n\
  PATDict /CurrentPattern get begin\n\
    gsave\n\
	% Set up the coordinate system to Pattern Space\n\
	% and lock down pattern\n\
	PatternGState /cm get setmatrix\n\
	BBox aload pop pop pop translate\n\
	% Determine the bounding box of the shape\n\
	pathbbox			% llx lly urx ury\n\
    grestore\n\
    % Determine (nw, nh) the # of cells to paint width and height\n\
    PatHeight div ceiling		% llx lly urx qh\n\
    4 1 roll				% qh llx lly urx\n\
    PatWidth div ceiling		% qh llx lly qw\n\
    4 1 roll				% qw qh llx lly\n\
    PatHeight div floor			% qw qh llx ph\n\
    4 1 roll				% ph qw qh llx\n\
    PatWidth div floor			% ph qw qh pw\n\
    4 1 roll				% pw ph qw qh\n\
    2 index sub cvi abs			% pw ph qs qh-ph\n\
    exch 3 index sub cvi abs exch	% pw ph nw=qw-pw nh=qh-ph\n\
    % Determine the starting point of the pattern fill\n\
    %(px, py)\n\
    4 2 roll				% nw nh pw ph\n\
    PatHeight mul			% nw nh pw py\n\
    exch				% nw nh py pw\n\
    PatWidth mul exch			% nw nh px py\n\
  end\n\
} bind def\n\
"
#define		FILL_PROLOG7	"\
% Save the original routines so that we can use them later on\n\
/oldfill	/fill load def\n\
/oldeofill	/eofill load def\n\
/oldstroke	/stroke load def\n\
/oldshow	/show load def\n\
/oldashow	/ashow load def\n\
/oldwidthshow	/widthshow load def\n\
/oldawidthshow	/awidthshow load def\n\
/oldkshow	/kshow load def\n\
\n\
% These defs are necessary so that subsequent procs don't bind in\n\
% the originals\n\
/fill	   { oldfill } bind def\n\
/eofill	   { oldeofill } bind def\n\
/stroke	   { oldstroke } bind def\n\
/show	   { oldshow } bind def\n\
/ashow	   { oldashow } bind def\n\
/widthshow { oldwidthshow } bind def\n\
/awidthshow { oldawidthshow } bind def\n\
/kshow 	   { oldkshow } bind def\n\
"
#define		FILL_PROLOG8	"\
/PATredef {\n\
  userdict begin\n\
    {\n\
    /fill { /clip load PATdraw newpath } bind def\n\
    /eofill { /eoclip load PATdraw newpath } bind def\n\
    /stroke { PATstroke } bind def\n\
    /show { 0 0 null 0 0 6 -1 roll PATawidthshow } bind def\n\
    /ashow { 0 0 null 6 3 roll PATawidthshow }\n\
    bind def\n\
    /widthshow { 0 0 3 -1 roll PATawidthshow }\n\
    bind def\n\
    /awidthshow { PATawidthshow } bind def\n\
    /kshow { PATkshow } bind def\n\
  } {\n\
    /fill   { oldfill } bind def\n\
    /eofill { oldeofill } bind def\n\
    /stroke { oldstroke } bind def\n\
    /show   { oldshow } bind def\n\
    /ashow  { oldashow } bind def\n\
    /widthshow { oldwidthshow } bind def\n\
    /awidthshow { oldawidthshow } bind def\n\
    /kshow  { oldkshow } bind def\n\
    } ifelse\n\
  end\n\
} bind def\n\
false PATredef\n\
"
#define		FILL_PROLOG9	"\
% Conditionally define setcmykcolor if not available\n\
/setcmykcolor where { pop } {\n\
  /setcmykcolor {\n\
    1 sub 4 1 roll\n\
    3 {\n\
	3 index add neg dup 0 lt { pop 0 } if 3 1 roll\n\
    } repeat\n\
    setrgbcolor - pop\n\
  } bind def\n\
} ifelse\n\
/PATsc {		% colorarray\n\
  aload length		% c1 ... cn length\n\
    dup 1 eq { pop setgray } { 3 eq { setrgbcolor } { setcmykcolor\n\
  } ifelse } ifelse\n\
} bind def\n\
/PATsg {		% dict\n\
  begin\n\
    lw setlinewidth\n\
    lc setlinecap\n\
    lj setlinejoin\n\
    ml setmiterlimit\n\
    ds aload pop setdash\n\
    cc aload pop setrgbcolor\n\
    cm setmatrix\n\
  end\n\
} bind def\n\
"
#define		FILL_PROLOG10	"\
/PATDict 3 dict def\n\
/PATsp {\n\
  true PATredef\n\
  PATDict begin\n\
    /CurrentPattern exch def\n\
    % If it's an uncolored pattern, save the color\n\
    CurrentPattern /PaintType get 2 eq {\n\
      /PColor exch def\n\
    } if\n\
    /CColor [ currentrgbcolor ] def\n\
  end\n\
} bind def\n\
% PATstroke - stroke with the current pattern\n\
/PATstroke {\n\
  countdictstack\n\
  save\n\
  mark\n\
  {\n\
    currentpoint strokepath moveto\n\
    PATpcalc				% proc nw nh px py\n\
    clip newpath PATfill\n\
    } stopped {\n\
	(*** PATstroke Warning: Path is too complex, stroking\n\
	  with gray) =\n\
    cleartomark\n\
    restore\n\
    countdictstack exch sub dup 0 gt\n\
	{ { end } repeat } { pop } ifelse\n\
    gsave 0.5 setgray oldstroke grestore\n\
  } { pop restore pop } ifelse\n\
  newpath\n\
} bind def\n\
"
#define		FILL_PROLOG11	"\
/PATtcalc {		% modmtx tilingtype PATtcalc tilematrix\n\
  % Note: tiling types 2 and 3 are not supported\n\
  gsave\n\
    exch concat					% tilingtype\n\
    matrix currentmatrix exch			% cmtx tilingtype\n\
    % Tiling type 1 and 3: constant spacing\n\
    2 ne {\n\
	% Distort the pattern so that it occupies\n\
	% an integral number of device pixels\n\
	dup 4 get exch dup 5 get exch		% tx ty cmtx\n\
	XStep 0 dtransform\n\
	round exch round exch			% tx ty cmtx dx.x dx.y\n\
	XStep div exch XStep div exch		% tx ty cmtx a b\n\
	0 YStep dtransform\n\
	round exch round exch			% tx ty cmtx a b dy.x dy.y\n\
	YStep div exch YStep div exch		% tx ty cmtx a b c d\n\
	7 -3 roll astore			% { a b c d tx ty }\n\
    } if\n\
  grestore\n\
} bind def\n\
/PATusp {\n\
  false PATredef\n\
  PATDict begin\n\
    CColor PATsc\n\
  end\n\
} bind def\n\
"
#define		FILL_PAT01	"\
% this is the pattern fill program from the Second edition Reference Manual\n\
% with changes to call the above pattern fill\n\
% left30\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 16 true [ 32 0 0 -16 0 16 ]\n\
	{<c000c000300030000c000c000300030000c000c000300030\n\
	000c000c00030003c000c000300030000c000c0003000300\n\
	00c000c000300030000c000c00030003>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P1 exch def\n\
"
#define		FILL_PAT02	"\
% right30\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 16 true [ 32 0 0 -16 0 16 ]\n\
	{<00030003000c000c0030003000c000c0030003000c000c00\n\
	30003000c000c00000030003000c000c0030003000c000c0\n\
	030003000c000c0030003000c000c000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P2 exch def\n\
"
#define		FILL_PAT03	"\
% crosshatch30\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 16 true [ 32 0 0 -16 0 16 ]\n\
	{<033003300c0c0c0c30033003c000c000300330030c0c0c0c\n\
	0330033000c000c0033003300c0c0c0c30033003c000c000\n\
	300330030c0c0c0c0330033000c000c0>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P3 exch def\n\
"
#define		FILL_PAT04	"\
% left45\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 32 true [ 32 0 0 -32 0 32 ]\n\
	{<010101010202020204040404080808081010101020202020\n\
	404040408080808001010101020202020404040408080808\n\
	101010102020202040404040808080800101010102020202\n\
	040404040808080810101010202020204040404080808080\n\
	010101010202020204040404080808081010101020202020\n\
	4040404080808080>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P4 exch def\n\
"
#define		FILL_PAT05	"\
% right45\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 32 true [ 32 0 0 -32 0 32 ]\n\
	{<808080804040404020202020101010100808080804040404\n\
	020202020101010180808080404040402020202010101010\n\
	080808080404040402020202010101018080808040404040\n\
	202020201010101008080808040404040202020201010101\n\
	808080804040404020202020101010100808080804040404\n\
	0202020201010101>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P5 exch def\n\
"
#define		FILL_PAT06	"\
% crosshatch45\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 32 true [ 32 0 0 -32 0 32 ]\n\
	{<828282824444444428282828101010102828282844444444\n\
	828282820101010182828282444444442828282810101010\n\
	282828284444444482828282010101018282828244444444\n\
	282828281010101028282828444444448282828201010101\n\
	828282824444444428282828101010102828282844444444\n\
	8282828201010101>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P6 exch def\n\
"
#define		FILL_PAT07	"\
% bricks\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 16 true [ 16 0 0 -16 0 16 ]\n\
	{<008000800080008000800080\n\
	 0080ffff8000800080008000\n\
	 800080008000ffff>}\n\
        imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P7 exch def\n\
"
#define		FILL_PAT08	"\
% vertical bricks\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 16 true [ 16 0 0 -16 0 16 ]\n\
	{<ff8080808080808080808080\n\
	  8080808080ff808080808080\n\
	  8080808080808080> }\n\
        imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P8 exch def\n\
"
#define		FILL_PAT09	"\
% horizontal lines\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 8 true [ 16 0 0 -8 0 8 ]\n\
	{< ffff000000000000ffff000000000000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P9 exch def\n\
"
#define		FILL_PAT10	"\
% vertical lines\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 8 16 true [ 8 0 0 -16 0 16 ]\n\
	{<11111111111111111111111111111111>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P10 exch def\n\
"
#define		FILL_PAT11	"\
% crosshatch lines\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 16 true [ 16 0 0 -16 0 16 ]\n\
	{<ffff111111111111ffff111111111111ffff111111111111\n\
	ffff111111111111>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P11 exch def\n\
"
#define		FILL_PAT12	"\
% left-shingles\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 48 48 true [ 48 0 0 -48 0 48 ]\n\
	{<000000000001000000000001000000000002000000000002\n\
	000000000004000000000004000000000008000000000008\n\
	000000000010000000000010000000000020000000000020\n\
	000000000040000000000040000000000080ffffffffffff\n\
	000000010000000000010000000000020000000000020000\n\
	000000040000000000040000000000080000000000080000\n\
	000000100000000000100000000000200000000000200000\n\
	000000400000000000400000000000800000ffffffffffff\n\
	000100000000000100000000000200000000000200000000\n\
	000400000000000400000000000800000000000800000000\n\
	001000000000001000000000002000000000002000000000\n\
	004000000000004000000000008000000000ffffffffffff>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P12 exch def\n\
"
#define		FILL_PAT13	"\
% right-shingles\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 48 48 true [ 48 0 0 -48 0 48 ]\n\
	{<000000000080000000000080000000000040000000000040\n\
	000000000020000000000020000000000010000000000010\n\
	000000000008000000000008000000000004000000000004\n\
	000000000002000000000002000000000001ffffffffffff\n\
	008000000000008000000000004000000000004000000000\n\
	002000000000002000000000001000000000001000000000\n\
	000800000000000800000000000400000000000400000000\n\
	000200000000000200000000000100000000ffffffffffff\n\
	000000800000000000800000000000400000000000400000\n\
	000000200000000000200000000000100000000000100000\n\
	000000080000000000080000000000040000000000040000\n\
	000000020000000000020000000000010000ffffffffffff>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P13 exch def\n\
"
#define		FILL_PAT14	"\
% vertical left-shingles\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 48 48 true [ 48 0 0 -48 0 48 ]\n\
	{<000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	000180010001000160010001000118010001000106010001\n\
	000101810001000100610001000100190001000100070001\n\
	000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	000100018001000100016001000100011801000100010601\n\
	000100010181000100010061000100010019000100010007\n\
	000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	800100010001600100010001180100010001060100010001\n\
	018100010001006100010001001900010001000700010001>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P14 exch def\n\
"
#define		FILL_PAT15	"\
% vertical right-shingles\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 48 48 true [ 48 0 0 -48 0 48 ]\n\
	{<000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	000100010007000100010019000100010061000100010181\n\
	000100010601000100011801000100016001000100018001\n\
	000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	000100070001000100190001000100610001000101810001\n\
	000106010001000118010001000160010001000180010001\n\
	000100010001000100010001000100010001000100010001\n\
	000100010001000100010001000100010001000100010001\n\
	000700010001001900010001006100010001018100010001\n\
	060100010001180100010001600100010001800100010001>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P15 exch def\n\
"
#define		FILL_PAT16	"\
% fishscales\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  {  32 16 true [ 32 0 0 -16 0 16 ]\n\
	{<0007e000000c30000018180000700e0001c003800f0000f0\n\
	7800001ec0000003600000063000000c180000180e000070\n\
	038001c000f00f00001e78000003c000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P16 exch def\n\
"
#define		FILL_PAT17	"\
% small fishscales\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 16 true [ 16 0 0 -16 0 16 ]\n\
	{<008000800080014001400220\n\
	0c187007c001800080004001\n\
	40012002180c0770>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P17 exch def\n\
"
#define		FILL_PAT18	"\
% circles\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 48 48 true [ 48 0 0 -48 0 48 ]\n\
	{<000007f000000000780f000000038000e000000c00001800\n\
	001000000400006000000300008000000080010000000040\n\
	020000000020040000000010040000000010080000000008\n\
	100000000004100000000004200000000002200000000002\n\
	200000000002400000000001400000000001400000000001\n\
	400000000001800000000000800000000000800000000000\n\
	800000000000800000000000800000000000800000000000\n\
	400000000001400000000001400000000001400000000001\n\
	200000000002200000000002200000000002100000000004\n\
	100000000004080000000008040000000010040000000010\n\
	020000000020010000000040008000000080006000000300\n\
	001000000400000c0000180000038000e0000000780f0000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P18 exch def\n\
"
#define		FILL_PAT19	"\
% hexagons\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 60 36 true [ 60 0 0 -36 0 36 ]\n\
	{<008000040000000001000002000000000100000200000000\n\
	020000010000000002000001000000000400000080000000\n\
	040000008000000008000000400000000800000040000000\n\
	100000002000000010000000200000002000000010000000\n\
	200000001000000040000000080000004000000008000000\n\
	800000000400000080000000040000000000000003fffff0\n\
	800000000400000080000000040000004000000008000000\n\
	400000000800000020000000100000002000000010000000\n\
	100000002000000010000000200000000800000040000000\n\
	080000004000000004000000800000000400000080000000\n\
	020000010000000002000001000000000100000200000000\n\
	0100000200000000008000040000000000fffffc00000000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P19 exch def\n\
"
#define		FILL_PAT20	"\
% octagons\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 32 true [ 32 0 0 -32 0 32 ]\n\
	{<003fff000040008000800040010000200200001004000008\n\
	080000041000000220000001400000008000000080000000\n\
	800000008000000080000000800000008000000080000000\n\
	800000008000000080000000800000008000000080000000\n\
	400000012000000210000004080000080400001002000020\n\
	0100004000800080>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P20 exch def\n\
"
#define		FILL_PAT21	"\
% horizontal sawtooth lines\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 32 16 true [ 32 0 0 -16 0 16 ]\n\
	{<000000000000000000000000000000000000000000000000\n\
	000000000100010002800280044004400820082010101010\n\
	20082008400440048002800200010001>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P21 exch def\n\
"
#define		FILL_PAT22	"\
% vertical sawtooth lines\n\
11 dict begin\n\
/PaintType 1 def\n\
/PatternType 1 def\n\
/TilingType 1 def\n\
/BBox [0 0 1 1] def\n\
/XStep 1 def\n\
/YStep 1 def\n\
/PatWidth 1 def\n\
/PatHeight 1 def\n\
/Multi 2 def\n\
/PaintData [\n\
  { clippath } bind\n\
  { 16 32 true [ 16 0 0 -32 0 32 ]\n\
	{<400020001000080004000200010000800100020004000800\n\
	100020004000800040002000100008000400020001000080\n\
	01000200040008001000200040008000>}\n\
     imagemask } bind\n\
] def\n\
/PaintProc {\n\
	pop\n\
	exec fill\n\
} def\n\
currentdict\n\
end\n\
/P22 exch def\n\
"

char	*fill_def[NUMPATTERNS] = {
		FILL_PAT01,FILL_PAT02,FILL_PAT03,FILL_PAT04,
		FILL_PAT05,FILL_PAT06,FILL_PAT07,FILL_PAT08,
		FILL_PAT09,FILL_PAT10,FILL_PAT11,FILL_PAT12,
		FILL_PAT13,FILL_PAT14,FILL_PAT15,FILL_PAT16,
		FILL_PAT17,FILL_PAT18,FILL_PAT19,FILL_PAT20,
		FILL_PAT21,FILL_PAT22,
	};

int	patmat[NUMPATTERNS][2] = {
	16,  -8,
	16,  -8,
	16,  -8,
	16, -16,
	16, -16,
	16, -16,
	16,  16,
	16, -16,
	16,  -8,
	 8, -16,
	16, -16,
	24, -24,
	24, -24,
	24, -24,
	24, -24,
	16,  -8,
	 8,  -8,
	16, -16,
	30, -18,
	16, -16,
	16,  -8,
	 8, -16,
	};

#define		SPECIAL_CHAR_1	"\
/reencdict 12 dict def /ReEncode { reencdict begin\n\
/newcodesandnames exch def /newfontname exch def /basefontname exch def\n\
/basefontdict basefontname findfont def /newfont basefontdict maxlength dict def\n\
basefontdict { exch dup /FID ne { dup /Encoding eq\n\
{ exch dup length array copy newfont 3 1 roll put }\n\
{ exch newfont 3 1 roll put } ifelse } { pop pop } ifelse } forall\n\
newfont /FontName newfontname put newcodesandnames aload pop\n\
128 1 255 { newfont /Encoding get exch /.notdef put } for\n\
newcodesandnames length 2 idiv { newfont /Encoding get 3 1 roll put } repeat\n\
newfontname newfont definefont pop end } def\n\
/isovec [\n\
"
#define		SPECIAL_CHAR_2	"\
8#200 /grave 8#201 /acute 8#202 /circumflex 8#203 /tilde\n\
8#204 /macron 8#205 /breve 8#206 /dotaccent 8#207 /dieresis\n\
8#210 /ring 8#211 /cedilla 8#212 /hungarumlaut 8#213 /ogonek 8#214 /caron\n\
8#220 /dotlessi 8#230 /oe 8#231 /OE\n\
8#240 /space 8#241 /exclamdown 8#242 /cent 8#243 /sterling\n\
8#244 /currency 8#245 /yen 8#246 /brokenbar 8#247 /section 8#250 /dieresis\n\
8#251 /copyright 8#252 /ordfeminine 8#253 /guillemotleft 8#254 /logicalnot\n\
8#255 /endash 8#256 /registered 8#257 /macron 8#260 /degree 8#261 /plusminus\n\
8#262 /twosuperior 8#263 /threesuperior 8#264 /acute 8#265 /mu 8#266 /paragraph\n\
8#267 /periodcentered 8#270 /cedilla 8#271 /onesuperior 8#272 /ordmasculine\n\
8#273 /guillemotright 8#274 /onequarter 8#275 /onehalf\n\
8#276 /threequarters 8#277 /questiondown 8#300 /Agrave 8#301 /Aacute\n\
8#302 /Acircumflex 8#303 /Atilde 8#304 /Adieresis 8#305 /Aring\n\
"
#define		SPECIAL_CHAR_3	"\
8#306 /AE 8#307 /Ccedilla 8#310 /Egrave 8#311 /Eacute\n\
8#312 /Ecircumflex 8#313 /Edieresis 8#314 /Igrave 8#315 /Iacute\n\
8#316 /Icircumflex 8#317 /Idieresis 8#320 /Eth 8#321 /Ntilde 8#322 /Ograve\n\
8#323 /Oacute 8#324 /Ocircumflex 8#325 /Otilde 8#326 /Odieresis 8#327 /multiply\n\
8#330 /Oslash 8#331 /Ugrave 8#332 /Uacute 8#333 /Ucircumflex\n\
8#334 /Udieresis 8#335 /Yacute 8#336 /Thorn 8#337 /germandbls 8#340 /agrave\n\
8#341 /aacute 8#342 /acircumflex 8#343 /atilde 8#344 /adieresis 8#345 /aring\n\
8#346 /ae 8#347 /ccedilla 8#350 /egrave 8#351 /eacute\n\
8#352 /ecircumflex 8#353 /edieresis 8#354 /igrave 8#355 /iacute\n\
8#356 /icircumflex 8#357 /idieresis 8#360 /eth 8#361 /ntilde 8#362 /ograve\n\
8#363 /oacute 8#364 /ocircumflex 8#365 /otilde 8#366 /odieresis 8#367 /divide\n\
8#370 /oslash 8#371 /ugrave 8#372 /uacute 8#373 /ucircumflex\n\
8#374 /udieresis 8#375 /yacute 8#376 /thorn 8#377 /ydieresis\
] def\n\
"

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
%%EndProlog\n\
"

static double		scalex, scaley;
static double		origx, origy;

void
genps_option(opt, optarg)
char opt;
char *optarg;
{
	int i;

	switch (opt) {

	case 'f':
		for ( i = 1; i <= MAX_PSFONT + 1; i++ )
			if ( !strcmp(optarg, PSfontnames[i]) ) break;

		if ( i > MAX_PSFONT + 1 )
			fprintf(stderr,
			    "warning: non-standard font name %s\n", optarg);

	    	psfontnames[0] = psfontnames[1] = optarg;
	    	PSfontnames[0] = PSfontnames[1] = optarg;
	    	break;

	case 'c':
	    	center = 1;
		break;

	case 's':
		if (font_size <= 0 || font_size > ULIMIT_FONT_SIZE) {
			fprintf(stderr,
				"warning: font size %d out of bounds\n", font_size);
		}
		break;

	case 'M':
		multi_page = 1;
		break;

	case 'P':
		show_page = 1;
		break;

      	case 'L':
      	case 'm':
		break;

	case 'n':			/* name to put in the "Title:" spec */
		name = optarg;
		break;

      	case 'l':			/* landscape mode */
		landscape = 1;		/* override the figure file setting */
		orientspec = 1;		/* user-specified */
		break;

      	case 'p':			/* portrait mode */
		landscape = 0;		/* override the figure file setting */
		orientspec = 1;		/* user-specified */
		break;

	case 'x':			/* x offset on page */
		xoff = atoi(optarg);
		break;

	case 'y':			/* y offset on page */
		yoff = atoi(optarg);
		break;

	case 'z':			/* pagesize */
		if ((pagesize = (char *) malloc (strlen (optarg) + 1)) == NULL)
		   {
		   (void) fprintf (stderr, "No memory\n");
		   exit (1);
		   }

		(void) strcpy (pagesize, optarg);
		break;

	default:
		put_msg(Err_badarg, opt, "ps");
		exit(1);
		break;
	}
}

void
genps_start(objects)
F_compound	*objects;
{
	char		host[256];
	struct passwd	*who;
	time_t		when;
	int		itmp;
	struct pagedef	*pd;

	resolution = objects->nwcorner.x;
	coord_system = objects->nwcorner.y;
	scalex = scaley = mag * POINT_PER_INCH / (double)resolution;
	/* convert to point unit */
	llx = (int)floor(llx * scalex); lly = (int)floor(lly * scaley);
	urx = (int)ceil(urx * scalex); ury = (int)ceil(ury * scaley);


        for (pd = pagedef; pd -> name != NULL; pd++)
	   if (strcmp (pagesize, pd -> name) == 0)
	      {
	      pagewidth = pd -> width;
	      pageheight = pd -> height;
	      }
	
	if (pagewidth < 0 || pageheight < 0)
	   {
	   (void) fprintf (stderr, "Unknown page size `%s'\n", pagesize);
	   exit (1);
	   }

	if (landscape) {
	   itmp = pageheight; pageheight = pagewidth; pagewidth = itmp;
	   itmp = llx; llx = lly; lly = itmp;
	   itmp = urx; urx = ury; ury = itmp;
	}
	if (show_page) {
	   if (center) {
	      if (landscape) {
		 origx = (pageheight - urx - llx)/2.0;
		 origy = (pagewidth - ury - lly)/2.0;
	      } else {
		 origx = (pagewidth - urx - llx)/2.0;
		 origy = (pageheight + ury + lly)/2.0;
	      }
	   } else {
	      origx = 0.0;
	      origy = landscape ? 0.0 : pageheight;
	   }
	} else {
	   origx = -llx;
	   origy = landscape ? -lly : ury;
	}

	/* finally, adjust by any offset the user wants */
	if (landscape) {
	    origx += yoff;
	    origy += xoff;
	} else {
	    origx += xoff;
	    origy += yoff;
	}

	if (show_page)
	    fprintf(tfp, "%%!PS-Adobe-2.0\n");		/* PostScript magic strings */
	else
	    fprintf(tfp, "%%!PS-Adobe-2.0 EPSF-2.0\n");	/* Encapsulated PostScript */
	who = getpwuid(getuid());
	if (gethostname(host, sizeof(host)) == -1)
	    (void)strcpy(host, "unknown-host!?!?");
	(void) time(&when);
	fprintf(tfp, "%%%%Title: %s\n",
		(name? name: ((from) ? from : "stdin")));
	fprintf(tfp, "%%%%Creator: %s Version %s Patchlevel %s\n",
		prog, VERSION, PATCHLEVEL);
	fprintf(tfp, "%%%%CreationDate: %s", ctime(&when));
	if (who)
	   fprintf(tfp, "%%%%For: %s@%s (%s)\n",
			who->pw_name, host, who->pw_gecos);

	if (!center) {
	   if (landscape)
		pages = (urx/pageheight+1)*(ury/pagewidth+1);
	   else
		pages = (urx/pagewidth+1)*(ury/pageheight+1);
	} else {
	   pages = 1;
	}
	if (landscape) {
	   fprintf(tfp, "%%%%Orientation: Landscape\n");
	   fprintf(tfp, "%%%%BoundingBox: %d %d %d %d\n",
	      (int)origx+llx, (int)origy+lly, (int)origx+urx, (int)origy+ury);
	} else {
	   fprintf(tfp, "%%%%Orientation: Portrait\n");
	   fprintf(tfp, "%%%%BoundingBox: %d %d %d %d\n",
	      (int)origx+llx, (int)origy-ury, (int)origx+urx, (int)origy-lly);
	}
	fprintf(tfp, "%%%%Pages: %d\n", show_page ? pages : 0 );
	fprintf(tfp, "%%%%BeginSetup\n");
	fprintf(tfp, "%%%%IncludeFeature: *PageSize %s\n", pagesize);
	fprintf(tfp, "%%%%EndSetup\n");

	fprintf(tfp, "%%%%EndComments\n");
	fprintf(tfp, "%s", BEGIN_PROLOG1);
	/* define the standard colors */
	genps_std_colors();
	/* define the user colors */
	genps_usr_colors();
	fprintf(tfp, "\nend\n");

	/* must specify translation/rotation before definition of fill patterns */
	fprintf(tfp, "save\n");
	fprintf(tfp, "%.1f %.1f translate\n", origx, origy);
	/* also flip y if necessary */
	if (landscape) {
	    fprintf(tfp, " 90 rotate\n");
	}
	if (coord_system == 2) {
	    fprintf(tfp, "1 -1 scale\n");
	}
	if (pats_used) {
	    int i;
	    fprintf(tfp, ".9 .9 scale %% to make patterns same scale as in xfig\n");
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG1,FILL_PROLOG2,FILL_PROLOG3);
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG4,FILL_PROLOG5,FILL_PROLOG6);
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG7,FILL_PROLOG8,FILL_PROLOG9);
	    fprintf(tfp, "\n%s%s",   FILL_PROLOG10,FILL_PROLOG11);
	    /* only define the patterns that are used */
	    for (i=0; i<NUMPATTERNS; i++)
		if (pattern_used[i])
			fprintf(tfp, "\n%s", fill_def[i]);
	    fprintf(tfp, "1.1111 1.1111 scale %%restore scale\n");
	}
	fprintf(tfp, "\n%s", BEGIN_PROLOG2);
	if (iso_text_exist(objects)) {
	   fprintf(tfp, "%s%s%s", SPECIAL_CHAR_1,SPECIAL_CHAR_2,SPECIAL_CHAR_3);
	   encode_all_fonts(objects);
	}
	if (ellipse_exist(objects))
		fprintf(tfp, "%s\n", ELLIPSE_PS);
	if (normal_spline_exist(objects))
		fprintf(tfp, "%s\n", SPLINE_PS);
	
	fprintf(tfp, "%s\n", END_PROLOG);
	fprintf(tfp, "$F2psBegin\n");
	fprintf(tfp, "10 setmiterlimit\n");	/* make like X server (11 degrees) */

 	if ( pages <= 1 ) {
	    multi_page = FALSE;
	    fprintf(tfp, " %.5f %.5f sc\n", scalex, scaley );
	} else {
	    fprintf(tfp, "initmatrix\n");
	}
}

void
genps_end()
{
    double dx,dy;
    int i, page;
    int h,w;

    if (multi_page) {
       page = 1;
       h = (landscape? pagewidth: pageheight);
       w = (landscape? pageheight: pagewidth);
       for (dy=0; dy < (ury-h*0.1); dy += h*0.9) {
	 for (dx=0; dx < (urx-w*0.1); dx += w*0.9) {
	    fprintf(tfp, "%%%%Page: %d %d\n",page,page);
	    fprintf(tfp,"%.1f %.1f tr",
		-(origx+dx), (origy+(landscape?-dy:dy)));
	    if (landscape) {
	       fprintf(tfp, " 90 rot");
	    }
	    if (coord_system == 2) {
		fprintf(tfp, " 1 -1 sc\n");
	    }
	    fprintf(tfp, " %.3f %.3f sc\n", scalex, scaley);
	    for (i=0; i<no_obj; i++) {
	       fprintf(tfp, "o%d ", i);
	       if (!(i%20)) fprintf(tfp, "\n", i);
	    }
	    fprintf(tfp, "showpage\n");
	    page++;
	 }
       }
    } else {
	if (show_page) {
	    fprintf(tfp, "showpage\n");
	    fprintf(tfp,"%%%%Page: 1 1\n");
	}
    }
    fprintf(tfp, "$F2psEnd\n");
    fprintf(tfp, "restore\n");
}

static
set_style(s, v)
int	s;
double	v;
{
	v /= POINT_PER_INCH / (double)resolution;
	if (s == DASH_LINE) {
	    if (v > 0.0) fprintf(tfp, "\t[%.1f] 0 setdash\n", v);
	    }
	else if (s == DOTTED_LINE) {
	    if (v > 0.0) fprintf(tfp, "\t[1 %.1f] %f setdash\n", v, v);
	    }
	}

static
reset_style(s, v)
int	s;
double	v;
{
	if (s == DASH_LINE) {
	    if (v > 0.0) fprintf(tfp, "\t[] 0 setdash");
	    }
	else if (s == DOTTED_LINE) {
	    if (v > 0.0) fprintf(tfp, "\t[] 0 setdash");
	    }
	fprintf(tfp, "\n");
	}

static
set_linejoin(j)
int	j;
{
	extern int	cur_joinstyle;

	if (j != cur_joinstyle) {
	    cur_joinstyle = j;
	    fprintf(tfp, "%d slj\n", cur_joinstyle);
	    }
	}

static
set_linecap(j)
int	j;
{
	extern int	cur_capstyle;

	if (j != cur_capstyle) {
	    cur_capstyle = j;
	    fprintf(tfp, "%d slj\n", cur_capstyle);
	    }
	}

static
set_linewidth(w)
double	w;
{
	if (w != cur_thickness) {
	    cur_thickness = w;
	    fprintf(tfp, "%.3f slw\n",
		    cur_thickness <= THICK_SCALE ? 	/* make lines a little thinner */
				0.5* cur_thickness :
				cur_thickness - THICK_SCALE);
	    }
	}

FILE	*open_picfile();
void	close_picfile();
int	filtype;

void
genps_line(l)
F_line	*l;
{
	F_point		*p, *q;
	/* JNT */
	int		radius, i = 0;
	FILE		*picf;
	char		buf[512];
	char		*cp;
	int		xmin,xmax,ymin,ymax;
	int		pic_w, pic_h;
	Boolean		namedcol;
	
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	if (l->type != T_PIC_BOX) {  /* pic object has no line thickness */
		set_linejoin(l->join_style);
		set_linecap(l->cap_style);
		set_linewidth((double)l->thickness);
	}
	fprintf(tfp, "%% Polyline\n");
	radius = l->radius;		/* radius of rounded-corner boxes */
	p = l->points;
	q = p->next;
	if (q == NULL) { /* A single point line */
	    fprintf(tfp, "n %d %d m %d %d l gs col%d s gr\n",
			p->x, p->y, p->x, p->y, l->pen_color);
	    if (multi_page)
	       fprintf(tfp, "} bind def\n");
	    return;
	    }
	if (l->type != T_PIC_BOX) {
	    set_style(l->style, l->style_val);
	}

	xmin = xmax = p->x;
	ymin = ymax = p->y;
	while (p->next != NULL) {	/* find lower left and upper right corners */
		p=p->next;
		if (xmin > p->x)
			xmin = p->x;
		else if (xmax < p->x)
			xmax = p->x;
		if (ymin > p->y)
			ymin = p->y;
		else if (ymax < p->y)
			ymax = p->y;
		}

	if (l->type == T_ARC_BOX) {
	    fprintf(tfp, "n %d %d m",xmin+radius, ymin);
	    fprintf(tfp, " %d %d %d %d %d arcto 4 {pop} repeat",
				xmin, ymin, xmin, ymax-radius, radius);
	    fprintf(tfp, " %d %d %d %d %d arcto 4 {pop} repeat", /* arc through bl to br */
				xmin, ymax, xmax-radius, ymax, radius);
	    fprintf(tfp, " %d %d %d %d %d arcto 4 {pop} repeat", /* arc through br to tr */
				xmax, ymax, xmax, ymin+radius, radius);
	    fprintf(tfp, " %d %d %d %d %d arcto 4 {pop} repeat", /* arc through tr to tl */
				xmax, ymin, xmin+radius, ymin, radius);
	}
	else if (l->type == T_PIC_BOX) {  /* postscript (eps), XPM, X bitmap or GIF file */
		int             dx, dy, rotation;
		int		llx, lly, urx, ury;
		double          fllx, flly, furx, fury;
#ifdef USE_XPM
		XpmImage	xpmimage;
#endif

		dx = l->points->next->next->x - l->points->x;
		dy = l->points->next->next->y - l->points->y;
		rotation = 0;
		if (dx < 0 && dy < 0)
			   rotation = 180;
		else if (dx < 0 && dy >= 0)
			   rotation = 90;
		else if (dy < 0 && dx >= 0)
			   rotation = 270;

		fprintf(tfp, "%%\n");

		/* first try for a X Bitmap file format */
		if (ReadFromBitmapFile(l->pic->file, &dx, &dy, &l->pic->bitmap)) {
			fprintf(tfp, "%% Begin Imported X11 Bitmap File: %s\n", l->pic->file);
			fprintf(tfp, "%%\n");
			l->pic->subtype = P_XBM;
			llx = lly = 0;
			urx = dx;		/* size of bitmap from the file */
			ury = dy;

#ifdef USE_XPM
		/* not X11 bitmap, try XPM */
		} else if (XpmReadFileToXpmImage(l->pic->file, &xpmimage, NULL)
					== XpmSuccess) {
			/* yes, say so */
			fprintf(tfp, "%% Begin Imported XPM File: %s\n", l->pic->file);
			/* and set type */
			l->pic->subtype = P_XPM;

			llx = lly = 0;
			urx = xpmimage.width;	/* size of image from the file */
			ury = xpmimage.height;

#endif /* USE_XPM */
		/* not XPM, try GIF */
		} else if (read_gif(l->pic) > 0) {
			/* yes, say so */
			fprintf(tfp, "%% Begin Imported GIF File: %s\n", l->pic->file);

			llx = lly = 0;
			urx = l->pic->bit_size.x;	/* size of image from the file */
			ury = l->pic->bit_size.y;

		/* neither, try EPS */
		} else {
			fprintf(tfp, "%% Begin Imported EPS File: %s\n", l->pic->file);
			fprintf(tfp, "%%\n");

			if ((picf=open_picfile(l->pic->file, &filtype)) == NULL) {
			    fprintf(stderr, "Unable to open EPS file: %s, error: (%d)\n",
					l->pic->file, sys_errlist[errno],errno);
			    return;
			}
			while (fgets(buf, 512, picf) != NULL) {
			    char *c;

			    if (!strncmp(buf, "%%BoundingBox:", 14)) {
				switch (*(c=buf+14)) {
				    case ' ':case '\t':c++;
				}
				if (strncmp(c,"(atend)",7)) {	/* make sure not an (atend) */
				    if (sscanf(c, "%lf %lf %lf %lf",
						&fllx, &flly, &furx, &fury) < 4) {
					fprintf(stderr,"Bad EPS bitmap file: %s\n", l->pic->file);
					close_picfile(picf,filtype);
					return;
				    }
				    l->pic->subtype = P_EPS;
				    llx= floor(fllx);
				    lly= floor(flly);
				    urx= ceil(furx);
				    ury= ceil(fury);
				    break;
				}
			    }
			}
			close_picfile(picf,filtype);
		}

		fprintf(tfp, "n gs\n");
		if (((rotation == 90 || rotation == 270) && !l->pic->flipped) ||
		    (rotation != 90 && rotation != 270 && l->pic->flipped)) {
			pic_h = urx - llx;
			pic_w = ury - lly;
		} else {
			pic_w = urx - llx;
			pic_h = ury - lly;
		}

		/* translate the pic stuff to the right spot on the page */
		fprintf(tfp, "%d %d tr\n", xmin, ymin);

		/* scale the pic stuff to fit into the bounding box */
		/* Note: the origin for fig is in the upper-right corner;
		 *       for postscript its in the lower right hand corner.
		 *       To fix it, we use a "negative"-y scale factor, then
		 *       translate the image up on the page */

		fprintf(tfp, "%f %f sc\n",
			fabs((double)(xmax-xmin)/pic_w), -1.0*(double)(ymax-ymin)/pic_h);

		/* flip the pic stuff */
		/* always translate it back so that the lower-left corner is at the origin */

		/* note: fig measures rotation clockwise; postscript is counter-clockwise */
		/* always translate it back so that the lower-left corner is at the origin */
		switch (rotation) {
		   case 0:
			if (l->pic->flipped) {
				fprintf(tfp, "%d 0 tr\n", pic_w);
				fprintf(tfp, "%d rot\n", 270);
				fprintf(tfp, "1 -1 sc\n");
			} else {
				fprintf(tfp, "0 %d tr\n", -pic_h);
			}
			break;
		   case 90:
			if (l->pic->flipped) {
				fprintf(tfp, "%d %d tr\n", pic_w, -pic_h);
				fprintf(tfp, "-1 1 sc\n");
			} else {
				fprintf(tfp, "%d rot\n", 270);
			}
			break;
		   case 180:
			if (l->pic->flipped) {
				fprintf(tfp, "0 %d tr\n", -pic_h);
				fprintf(tfp, "%d rot\n", 270);
				fprintf(tfp, "-1 1 sc\n");
			} else {
				fprintf(tfp, "%d 0 tr\n", pic_w);
				fprintf(tfp, "%d rot\n", 180);
			}
			break;
		   case 270:
			if (l->pic->flipped) {
				fprintf(tfp, "1 -1 sc\n");
			} else {
				fprintf(tfp, "%d %d tr\n", pic_w, -pic_h);
				fprintf(tfp, "%d rot\n", 90);
			}
			break;
		}

		/* translate the pic stuff so that the lower-left corner is at the origin */
		fprintf(tfp, "%d %d tr\n", -llx, -lly);
		/* save vm so pic file won't change anything */
		fprintf(tfp, "save\n");

		/* XBM file */
		if (l->pic->subtype == P_XBM) {
			int		 i,j;
			unsigned char	*bit;
			int		 cwid;

			fprintf(tfp, "col%d\n ", l->pen_color);
			fprintf(tfp, "%% Bitmap image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			fprintf(tfp, "/pix %d string def\n", (int)((urx+7)/8));
			/* width, height and paint 0 bits */
			fprintf(tfp, "%d %d false\n", urx, ury);
			/* transformation matrix */
			fprintf(tfp, "[%d 0 0 %d 0 %d]\n", urx, -ury, ury);
			/* function for reading bits */
			fprintf(tfp, "{currentfile pix readhexstring pop}\n");
			/* use imagemask to draw in color */
			fprintf(tfp, "imagemask\n");
			bit = l->pic->bitmap;
			cwid = 0;
			for (i=0; i<ury; i++) {			/* for each row */
			    for (j=0; j<(int)((urx+7)/8); j++) {	/* for each byte */
				fprintf(tfp,"%02x", (unsigned char) ~(*bit++));
				cwid+=2;
				if (cwid >= 80) {
				    fprintf(tfp,"\n");
				    cwid=0;
				}
			    }
			    fprintf(tfp,"\n");
			}

#ifdef USE_XPM
		/* XPM file */
		} else if (l->pic->subtype == P_XPM) {
			int	  i, wid, ht;
			XpmColor *coltabl;
			char	 *c, tmpc[8];
			int	  r,g,b;
			unsigned char *cdata, *cp;
			unsigned int  *dp;

			/* start with width and height */
			wid = xpmimage.width;
			ht = xpmimage.height;
			fprintf(tfp, "%% Pixmap image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* modify colortable entries to make consistent */
			coltabl = xpmimage.colorTable;
			namedcol = False;
			/* convert the color defs to a consistent #rrggbb */
			for (i=0; i<xpmimage.ncolors; i++) {
				c = (coltabl + i)->c_color;
				if (c[0] != '#') {		/* named color, set flag */
					namedcol = True;
				/* want to make #RRGGBB from possibly other formats */
				} else if (strlen(c) == 4) {	/* #rgb */
					sprintf(tmpc,"#%.1s%.1s%.1s%.1s%.1s%.1s",
						&c[1],&c[1],&c[2],&c[2],&c[3],&c[3]);
					strcpy((coltabl + i)->c_color,tmpc);
				} else if (strlen(c) == 10) {	/* #rrrgggbbb */
					sprintf(tmpc,"#%.2s%.2s%.2s",&c[1],&c[4],&c[7]);
					strcpy((coltabl + i)->c_color,tmpc);
				} else if (strlen(c) == 13) {	/* #rrrrggggbbbb */
					sprintf(tmpc,"#%.2s%.2s%.2s",&c[1],&c[5],&c[9]);
					strcpy((coltabl + i)->c_color,tmpc);
				}
			}
			/* go lookup the named colors' rgb values */
			if (namedcol)
				convert_names(coltabl,xpmimage.ncolors);
			/* now make separate Red, Green and Blue color arrays */
			for (i=0; i<xpmimage.ncolors; i++) {
			    c = (coltabl + i)->c_color;
			    if (sscanf(c, "#%02x%02x%02x", &r,&g,&b) != 3)
				    fprintf(stderr,"Error parsing color %s\n",c);
			    l->pic->cmap[0][i] = (unsigned char) r;
			    l->pic->cmap[1][i] = (unsigned char) g;
			    l->pic->cmap[2][i] = (unsigned char) b;
			}
			/* and convert the integer data to unsigned char */
			dp = xpmimage.data;
			if ((cdata = (unsigned char *)
			     malloc(wid*ht*sizeof(unsigned char))) == NULL) {
				fprintf(stderr,"can't allocate space for XPM image\n");
				return;
			}
			cp = cdata;
			for (i=0; i<wid*ht; i++)
			    *cp++ = (unsigned char) *dp++;
				
			/* now write out the compressed image data */
			(void) PSencode(tfp, wid, ht, xpmimage.ncolors,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				cdata);
			/* and free up the space */
			free(cdata);
			XpmFree(xpmimage);
#endif /* USE_XPM */

		/* GIF file */
		} else if (l->pic->subtype == P_GIF) {
			int		 wid, ht;

			/* start with width and height */
			wid = l->pic->bit_size.x;
			ht = l->pic->bit_size.y;
			fprintf(tfp, "%% GIF image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* now write out the compressed image data */
			(void) PSencode(tfp, wid, ht, l->pic->numcols,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				l->pic->bitmap);

		/* EPS file */
		} else {
		    fprintf(tfp, "%% EPS file follows:\n");
		    if ((picf=open_picfile(l->pic->file, &filtype)) == NULL) {
			fprintf(stderr, "Unable to open EPS file: %s, error: (%d)\n",
				l->pic->file, sys_errlist[errno],errno);
			fprintf(tfp, "gr\n");
			return;
		    }
		    while (fgets(buf, sizeof(buf), picf) != NULL) {
			if (*buf == '%')		/* skip comment lines */
				continue;
			if ((cp=strstr(buf, "showpage")) != NULL)
				strcpy (cp, cp+8);	/* remove showpage */
			fputs(buf, tfp);
		    }
		    close_picfile(picf,filtype);
		}

		/* restore vm and gsave */
		fprintf(tfp, "restore gr\n");
		fprintf(tfp, "%%\n");
		fprintf(tfp, "%% End Imported PIC File: %s\n", l->pic->file);
		fprintf(tfp, "%%\n");
	} else {
		p = l->points;
		q = p->next;
		fprintf(tfp, "n %d %d m", p->x, p->y);
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		    fprintf(tfp, " %d %d l ", p->x, p->y);
 	    	    if (!((++i)%5))
			fprintf(tfp, "\n");
		}
	}
	if (l->type != T_PIC_BOX) {
		if (l->type == T_POLYLINE)
		    fprintf(tfp, " %d %d l ", q->x, q->y);
		else
		    fprintf(tfp, " clp ");
		if (l->fill_style != UNFILLED)
		    fill_area(l->fill_style, l->pen_color, l->fill_color, xmin, ymin);
		if (l->thickness > 0)
		     fprintf(tfp, " gs col%d s gr ", l->pen_color);

		reset_style(l->style, l->style_val);
		if (l->back_arrow && l->thickness > 0)
		    draw_arrow_head((double)l->points->next->x,
			(double)l->points->next->y,
			(double)l->points->x, (double)l->points->y,
			l->back_arrow, l->pen_color);
		if (l->for_arrow && l->thickness > 0)
		    draw_arrow_head((double)p->x, (double)p->y, (double)q->x,
			(double)q->y, l->for_arrow, l->pen_color);
	}
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
}

void 
genps_spline(s)
F_spline	*s;
{
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	set_linecap(s->cap_style);
	if (int_spline(s))
	    genps_itp_spline(s);
	else
	    genps_ctl_spline(s);
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

genps_itp_spline(s)
F_spline	*s;
{
	F_point		*p, *q;
	F_control	*a, *b, *ar;
	int		 xmin, ymin;

	set_linewidth((double)s->thickness);
	fprintf(tfp, "%% Interp Spline\n");
	a = ar = s->controls;
	p = s->points;
	set_style(s->style, s->style_val);
	fprintf(tfp, "n %d %d m\n", p->x, p->y);
	xmin = 999999;
	ymin = 999999;
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    b = a->next;
	    fprintf(tfp, "\t%.2f %.2f %.2f %.2f %d %d curveto\n",
			a->rx, a->ry, b->lx, b->ly, q->x, q->y);
	    a = b;
	    }
	if (closed_spline(s)) fprintf(tfp, " clp ");
	if (s->fill_style != UNFILLED)
	    fill_area(s->fill_style, s->pen_color, s->fill_color, xmin, ymin);
	if (s->thickness > 0)
	    fprintf(tfp, " gs col%d s gr\n", s->pen_color);
	reset_style(s->style, s->style_val);

	/* draw arrowheads after spline for open arrow (paints over spline end) */
	if (s->back_arrow && s->thickness > 0)
	    draw_arrow_head(ar->rx, ar->ry, (double)s->points->x,
			(double)s->points->y, s->back_arrow, s->pen_color);

	if (s->for_arrow && s->thickness > 0)
	    draw_arrow_head(a->lx, a->ly, (double)p->x,
			(double)p->y, s->for_arrow, s->pen_color);
	}

genps_ctl_spline(s)
F_spline	*s;
{
	double		a, b, c, d, x1, y1, x2, y2, x3, y3;
	double		arx1, ary1, arx2, ary2;
	F_point		*p, *q;
	int		 xmin, ymin;

	p = s->points;
	x1 = arx2 = p->x;
	y1 = ary2 = p->y;
	p = p->next;
	c = arx1 = p->x;
	d = ary1 = p->y;
	set_linewidth((double)s->thickness);
	x3 = a = (x1 + c) / 2;
	y3 = b = (y1 + d) / 2;
	set_style(s->style, s->style_val);
	if (! closed_spline(s)) {
	    fprintf(tfp, "%% Open spline\n");
	    fprintf(tfp, "n %.2f %.2f m %.2f %.2f l\n",
			x1, y1, x3, y3);
	    }
	else {
	    fprintf(tfp, "%% Closed spline\n");
	    fprintf(tfp, "n %.2f %.2f m\n", a, b);
	    }
	xmin = 999999;
	ymin = 999999;
	/* in case there are only two points in this spline */
	x2=x1; y2=y1;
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    x1 = x3; y1 = y3;
	    x2 = c;  y2 = d;
	    c = q->x; d = q->y;
	    x3 = (x2 + c) / 2;
	    y3 = (y2 + d) / 2;
	    fprintf(tfp, "\t%.2f %.2f %.2f %.2f %.2f %.2f DrawSplineSection\n",
			x1, y1, x2, y2, x3, y3);
	    }
	/*
	* At this point, (x2,y2) and (c,d) are the position of the
	* next-to-last and last point respectively, in the point list
	*/
	if (closed_spline(s)) {
	    fprintf(tfp, "\t%.2f %.2f %.2f %.2f %.2f %.2f DrawSplineSection closepath ",
			x3, y3, c, d, a, b);
	    }
	else {
	    fprintf(tfp, "\t%.2f %.2f l ", c, d);
	    }
	if (s->fill_style != UNFILLED)
	    fill_area(s->fill_style, s->pen_color, s->fill_color, xmin, ymin);
	if (s->thickness > 0)
	    fprintf(tfp, " gs col%d s gr\n", s->pen_color);
	reset_style(s->style, s->style_val);
	/* draw arrowheads after spline */
	if (s->back_arrow && s->thickness > 0)
	    draw_arrow_head(arx1, ary1, arx2, ary2, s->back_arrow, s->pen_color);
	if (s->for_arrow && s->thickness > 0)
	    draw_arrow_head(x2, y2, c, d, s->for_arrow, s->pen_color);
	}

void
genps_ellipse(e)
F_ellipse	*e;
{
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	set_linewidth((double)e->thickness);
	set_style(e->style, e->style_val);
	if (e->angle == 0)
	{
	    fprintf(tfp, "%% Ellipse\n");
	    fprintf(tfp, "n %d %d %d %d 0 360 DrawEllipse ",
		  e->center.x, e->center.y, e->radiuses.x, e->radiuses.y);
	}
	else
	{
	    fprintf(tfp, "%% Rotated Ellipse\n");
	    fprintf(tfp, "gs\n");
	    fprintf(tfp, "%d %d tr\n",e->center.x, e->center.y);
	    fprintf(tfp, "%6.3f rot\n",-e->angle*180/M_PI);
	    fprintf(tfp, "n 0 0 %d %d 0 360 DrawEllipse ",
		 e->radiuses.x, e->radiuses.y);
	}
	if (e->fill_style != UNFILLED)
	    fill_area(e->fill_style, e->pen_color, e->fill_color,
			e->center.x - e->radiuses.x, e->center.y - e->radiuses.y);
	if (e->thickness > 0)
	    fprintf(tfp, "gs col%d s gr\n", e->pen_color);
	if (e->angle != 0)
	    fprintf(tfp, "gr\n");
	reset_style(e->style, e->style_val);
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

#define	TEXT_PS		"\
/%s%s findfont %.2f scalefont setfont\n\
"
void
genps_text(t)
F_text	*t;
{
	unsigned char		*cp;

	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	if (PSisomap[t->font+1] == TRUE)
	   fprintf(tfp, TEXT_PS, PSFONT(t), "-iso", PSFONTMAG(t));
	else
	   fprintf(tfp, TEXT_PS, PSFONT(t), "", PSFONTMAG(t));

	fprintf(tfp, "%d %d m\ngs ", t->base_x,  t->base_y);
	if (coord_system == 2)
		fprintf(tfp, "1 -1 sc ");

	if (t->angle != 0)
	   fprintf(tfp, " %.1f rot ", t->angle*180/M_PI);
	/* this loop escapes characters '(', ')', and '\' */
	fputc('(', tfp);
	for(cp = (unsigned char *)t->cstring; *cp; cp++) {
	    if (strchr("()\\", *cp))
		fputc('\\', tfp);
	    if (*cp>=0x80)
		fprintf(tfp,"\\%o", *cp);
	    else
		fputc(*cp, tfp);
		}
	fputc(')', tfp);

	if ((t->type == T_CENTER_JUSTIFIED) || (t->type == T_RIGHT_JUSTIFIED)){

	  	fprintf(tfp, " dup stringwidth pop ");
		if (t->type == T_CENTER_JUSTIFIED) fprintf(tfp, "2 div ");
		fprintf(tfp, "neg 0 rmoveto ");
		}

	else if ((t->type != T_LEFT_JUSTIFIED) && (t->type != DEFAULT))
		fprintf(stderr, "Text incorrectly positioned\n");

	fprintf(tfp, " col%d show gr\n", t->color);

	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

void
genps_arc(a)
F_arc	*a;
{
	double		angle1, angle2, dx, dy, radius, x, y;
	double		cx, cy, sx, sy, ex, ey;
	int		direction;

	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	cx = a->center.x; cy = a->center.y;
	sx = a->point[0].x; sy = a->point[0].y;
	ex = a->point[2].x; ey = a->point[2].y;

	if (coord_system == 2)
	    direction = !a->direction;
	else
	    direction = a->direction;
	set_linewidth((double)a->thickness);
	set_linecap(a->cap_style);
	fprintf(tfp, "%% Arc\n");
	if (a->type != T_PIE_WEDGE_ARC) {
	    if (a->for_arrow && a->thickness > 0) {
		arc_tangent(cx, cy, ex, ey, direction, &x, &y);
		draw_arrow_head(x, y, ex, ey, a->for_arrow, a->pen_color);
		}
	    if (a->back_arrow && a->thickness > 0) {
		arc_tangent(cx, cy, sx, sy, !direction, &x, &y);
		draw_arrow_head(x, y, sx, sy, a->back_arrow, a->pen_color);
		}
	}
	dx = cx - sx;
	dy = cy - sy;
	radius = sqrt(dx*dx+dy*dy);
	angle1 = atan2(sy-cy, sx-cx) * 180 / M_PI;
	angle2 = atan2(ey-cy, ex-cx) * 180 / M_PI;
	/* direction = 1 -> Counterclockwise */
	set_style(a->style, a->style_val);
	fprintf(tfp, "n %.2f %.2f %.2f %.2f %.2f %s\n",
		cx, cy, radius, angle1, angle2,
		((direction == 1) ? "arc" : "arcn"));
	if (a->type == T_PIE_WEDGE_ARC)
		fprintf(tfp,"%.2f %.2f l %.2f %.2f l ",cx,cy,sx,sy);
	if (a->fill_style != UNFILLED)
	    /****** The upper-left values (dx, dy) aren't really correct ******/
	    fill_area(a->fill_style, a->pen_color, a->fill_color, (int)dx, (int)dy);
	if (a->thickness > 0)
	    fprintf(tfp, "gs col%d s gr\n", a->pen_color);
	reset_style(a->style, a->style_val);
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

static
arc_tangent(x1, y1, x2, y2, direction, x, y)
double	x1, y1, x2, y2, *x, *y;
int	direction;
{
	if (direction) { /* counter clockwise  */
	    *x = x2 + (y2 - y1);
	    *y = y2 - (x2 - x1);
	    }
	else {
	    *x = x2 - (y2 - y1);
	    *y = y2 + (x2 - x1);
	    }
	}

/*	draw arrow heading from (x1, y1) to (x2, y2)	*/

static
draw_arrow_head(x1, y1, x2, y2, arrow, col)
double	x1, y1, x2, y2;
F_arrow	*arrow;
int	col;
{
	double	x, y, xb, yb, dx, dy, l, sina, cosa;
	double	xc, yc, xd, yd, xs, ys;
	double	wd, ht;

	wd = arrow->wid;
	ht = arrow->ht;

	dx = x2 - x1;  dy = y1 - y2;
	l = sqrt(dx*dx+dy*dy);
	if (l == 0) {
	     return;
	}
	else {
	     sina = dy / l;  cosa = dx / l;
	}
	xb = x2*cosa - y2*sina;
	yb = x2*sina + y2*cosa;
	/* lengthen the "height" if type 2 */
	if (arrow->type == 2)
	    x = xb - ht * 1.2;
	/* shorten the "height" if type 3 */
	else if (arrow->type == 3)
	    x = xb - ht * 0.8;
	else
	    x = xb - ht;
	y = yb - wd / 2;
	xc = x*cosa + y*sina;
	yc = -x*sina + y*cosa;
	y = yb + wd / 2;
	xd =  x*cosa + y*sina;
	yd = -x*sina + y*cosa;

	/* a point "length" from the end of the shaft */
	xs =  (xb-ht) * cosa + yb * sina + .5;
	ys = -(xb-ht) * sina + yb * cosa + .5;

	set_linecap(0);			/* butt line cap for arrowheads */
	set_linejoin(0);		/* miter join for sharp points */
	set_linewidth(arrow->thickness);
	fprintf(tfp, "n %.2f %.2f m %.2f %.2f l %.2f %.2f l ",
			xc, yc, x2, y2, xd, yd);

	if (arrow->type != 0) {		/* close the path and fill */
	    fprintf(tfp, " %.2f %.2f l %.2f %.2f l clp ", xs, ys, xc, yc);
	    if (arrow->style == 0)		/* hollow, fill with white */
		fill_area(NUMSHADES-1, WHITE_COLOR, WHITE_COLOR, 0, 0);
	    else			/* solid, fill with color  */
		fill_area(NUMSHADES-1, col, col, 0, 0);
	}
	fprintf(tfp, "gs col%d s gr\n",col);
}


/* uses eofill (even/odd rule fill) */
/* ulx and uly define the upper-left corner of the object for pattern alignment */

static
fill_area(fill, pen_color, fill_color, ulx, uly)
int fill, pen_color, fill_color, ulx, uly;
{
   float pen_r, pen_g, pen_b, fill_r, fill_g, fill_b;

   /* get the rgb values for the fill pattern (if necessary) */
   if (fill_color <= NUM_STD_COLS) {
	fill_r=rgbcols[fill_color>0? fill_color: 0].r;
	fill_g=rgbcols[fill_color>0? fill_color: 0].g;
	fill_b=rgbcols[fill_color>0? fill_color: 0].b;
   } else {
	fill_r=user_colors[fill_color-NUM_STD_COLS].r/255.0;
	fill_g=user_colors[fill_color-NUM_STD_COLS].g/255.0;
	fill_b=user_colors[fill_color-NUM_STD_COLS].b/255.0;
   }
   if (pen_color <= NUM_STD_COLS) {
	pen_r=rgbcols[pen_color>0? pen_color: 0].r;
	pen_g=rgbcols[pen_color>0? pen_color: 0].g;
	pen_b=rgbcols[pen_color>0? pen_color: 0].b;
   } else {
	pen_r=user_colors[pen_color-NUM_STD_COLS].r/255.0;
	pen_g=user_colors[pen_color-NUM_STD_COLS].g/255.0;
	pen_b=user_colors[pen_color-NUM_STD_COLS].b/255.0;
   }

   if (fill_color <= 0) {   /* use gray levels for default and black */
	if (fill < NUMSHADES+NUMTINTS)
	    fprintf(tfp, "gs %.2f setgray ef gr ", 1.0 - SHADEVAL(fill));
	/* one of the patterns */
	else {
	    int patnum = fill-NUMSHADES-NUMTINTS+1;
	    fprintf(tfp, "gs /PC [[%.2f %.2f %.2f] [%.2f %.2f %.2f]] def\n",
			fill_r, fill_g, fill_b, pen_r, pen_g, pen_b);
	    fprintf(tfp, "%.2f %.2f sc P%d [%d 0 0 %d %.2f %.2f]  PATmp PATsp ef gr PATusp ",
			THICK_SCALE, THICK_SCALE, patnum,
			patmat[patnum-1][0],patmat[patnum-1][1],
		        (float)ulx/THICK_SCALE, (float)uly/THICK_SCALE);
	}
   /* color other than default(black) */
   } else {
	/* a shade */
	if (fill < NUMSHADES)
	    fprintf(tfp, "gs col%d %.2f shd ef gr ", fill_color, SHADEVAL(fill));
	/* a tint */
	else if (fill < NUMSHADES+NUMTINTS)
	    fprintf(tfp, "gs col%d %.2f tnt ef gr ", fill_color, TINTVAL(fill));
	/* one of the patterns */
	else {
	    int patnum = fill-NUMSHADES-NUMTINTS+1;
	    fprintf(tfp, "gs /PC [[%.2f %.2f %.2f] [%.2f %.2f %.2f]] def\n",
			fill_r, fill_g, fill_b, pen_r, pen_g, pen_b);
	    fprintf(tfp, "%.2f %.2f sc P%d [%d 0 0 %d %.2f %.2f] PATmp PATsp ef gr PATusp ",
			THICK_SCALE, THICK_SCALE, patnum,
			patmat[patnum-1][0],patmat[patnum-1][1],
		        (float)ulx/THICK_SCALE, (float)uly/THICK_SCALE);
	}
   }
}

/* define standard colors as "col##" where ## is the number */
genps_std_colors()
{
    int i;
    for (i=0; i<NUM_STD_COLS; i++) {
	fprintf(tfp, "/col%d {%.3f %.3f %.3f srgb} bind def\n", i,
		rgbcols[i].r, rgbcols[i].g, rgbcols[i].b);
    }
}
	
/* define user colors as "col##" where ## is the number */
genps_usr_colors()
{
    int i;
    for (i=0; i<num_usr_cols; i++) {
	fprintf(tfp, "/col%d {%.3f %.3f %.3f srgb} bind def\n", i+NUM_STD_COLS,
		user_colors[i].r/255.0, user_colors[i].g/255.0, user_colors[i].b/255.0);
    }
}
	
static
iso_text_exist(ob)
F_compound      *ob;
{
   F_compound	*c;
   F_text          *t;
   unsigned char   *s;

   if (ob->texts != NULL)
   {
      for (t = ob->texts; t != NULL; t = t->next)
      {
	 for (s = (unsigned char*)t->cstring; *s != '\0'; s++)
	 {
	    /* look for characters >= 128 */
	    if (*s>127) return(1);
	 }
      }
   }

   for (c = ob->compounds; c != NULL; c = c->next) {
       if (iso_text_exist(c)) return(1);
       }
   return(0);
}

static
encode_all_fonts(ob)
F_compound	*ob;
{
   F_compound *c;
   F_text     *t;

   if (ob->texts != NULL)
   {
	for (t = ob->texts; t != NULL; t = t->next)
	    if (PSisomap[t->font+1] == FALSE)
	    {
		fprintf(tfp, "/%s /%s-iso isovec ReEncode\n", PSFONT(t), PSFONT(t));
		PSisomap[t->font+1] = TRUE;
	    }
   }

   for (c = ob->compounds; c != NULL; c = c->next)
   {
	encode_all_fonts(c);
   }
}

static
ellipse_exist(ob)
F_compound	*ob;
{
	F_compound	*c;

	if (NULL != ob->ellipses) return(1);

	for (c = ob->compounds; c != NULL; c = c->next) {
	    if (ellipse_exist(c)) return(1);
	    }

	return(0);
	}

static
normal_spline_exist(ob)
F_compound	*ob;
{
	F_spline	*s;
	F_compound	*c;

	for (s = ob->splines; s != NULL; s = s->next) {
	    if (normal_spline(s)) return(1);
	    }

	for (c = ob->compounds; c != NULL; c = c->next) {
	    if (normal_spline_exist(c)) return(1);
	    }

	return(0);
	}

struct
driver dev_ps = {
     	genps_option,
	genps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genps_end,
	INCLUDE_TEXT
};
