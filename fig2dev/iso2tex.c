/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1992 Herbert Bauer and  B. Raichle
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */


/* map ISO-Font Symbols to appropriate sequences in TeX */
/* Herbert Bauer 22.11.1991 */

/* B.Raichle 12.10.92, changed some of the definitions */


/* B.Raichle 12.10.92, changed some of the definitions */


char *ISOtoTeX[] =   /* starts at octal 240 */
{
  "{}",
  "{!`}",	/* inverse ! */
  "{}",		/* cent sign (?) */
  "\\pounds{}",
  "{}",		/* circle with x mark */
  "{}",		/* Yen */
  "{}",		/* some sort of space - doen't work under mwm */
  "\\S{}",	/* paragraph sign */
  "\\\"{}",		/* diaresis points */
  "\\copyright{}",
  "\\b{a}",
  "\\mbox{$\\ll$}",		/* << */
  "{--}", 	/* longer dash - doesn't work with mwm */
  "{-}",		/* short dash */
  "{}",		/* trademark */
  "{}",		/* overscore */
/* 0xb0 */
  "{\\lower.2ex\\hbox{\\char\\'27}}",		/* degree */
  "\\mbox{$\\pm$}",	/* plus minus - math mode */
  "\\mbox{$\\mathsurround 0pt{}^2$}",		/* squared  - math mode */
  "\\mbox{$\\mathsurround 0pt{}^3$}",		/* cubed  - math mode */
  "\\'{}",		/* accent egue */
  "\\mbox{$\\mu$}",	/* greek letter mu - math mode */
  "\\P{}",	/* paragraph */
  "\\mbox{$\\cdot$}",	/* centered dot  - math mode */
  "",
  "\\mbox{$\\mathsurround 0pt{}^1$}",		/* superscript 1  - math mode */
  "\\b{o}",
  "\\mbox{$\\gg$}",		/* >> */
  "\\mbox{$1\\over 4$}",	/* 1/4 - math mode */
  "\\mbox{$1\\over 2$}",	/* 1/2 - math mode */
  "\\mbox{$3\\over 4$}",	/* 3/4 - math mode */
  "{?`}",		/* inverse ? */
/* 0xc0 */
  "\\`A",
  "\\'A",
  "\\^A",
  "\\~A",
  "\\\"A",
  "\\AA{}",
  "\\AE{}",
  "\\c C",
  "\\`E",
  "\\'E",
  "\\^E",
  "\\\"E",
  "\\`I",
  "\\'I",
  "\\^I",
  "\\\"I",
/* 0xd0 */
  "{\\rlap{\\raise.3ex\\hbox{--}}D}", /* Eth */
  "\\~N",
  "\\`O",
  "\\'O",
  "\\^O",
  "\\~O",
  "\\\"O",
  "\\mbox{$\\times$}",	/* math mode */
  "\\O{}",
  "\\`U",
  "\\'U",
  "\\^U",
  "\\\"U",
  "\\'Y",
  "{}",		/* letter P wide-spaced */
  "\\ss{}",
/* 0xe0 */
  "\\`a",
  "\\'a",
  "\\^a",
  "\\~a",
  "\\\"a",
  "\\aa{}",
  "\\ae{}",
  "\\c c",
  "\\`e",
  "\\'e",
  "\\^e",
  "\\\"e",
  "\\`\\i{}",
  "\\'\\i{}",
  "\\^\\i{}",
  "\\\"\\i{}",
/* 0xf0 */
  "\\mbox{$\\partial$}",	/* correct?  - math mode */
  "\\~n",
  "\\`o",
  "\\'o",
  "\\^o",
  "\\~o",
  "\\\"o",
  "\\mbox{$\\div$}",	/* math mode */
  "\\o{}",
  "\\`u",
  "\\'u",
  "\\^u",
  "\\\"u",
  "\\'y",
  "{}",		/* letter p wide-spaced */
  "\\\"y"
};

