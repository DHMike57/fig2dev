#if defined(SYSV) || defined(SVR4)
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

/*
 * converters program names
 */
#define FIG2DEV	"fig2dev"
#define PIC2FIG "pic2fig"
#define APG2FIG "apgto f"

/*
 * filename defaults
 */
#define MK "Makefile"
#define TX "transfig.tex"

/* if using LaTeX209, use "documentstyle", if using LaTeX2e use "usepackage" */
#ifdef LATEX2E
#define INCLFIG "usepackage"
#else
#define INCLFIG "documentstyle"
#endif

enum language  {box, epic, eepic, eepicemu, latex,
	pictex, postscript, eps, psfig, pstex, textyl, tpic};
#define MAXLANG tpic

enum input {apg, fig, pic, ps};
#define MAXINPUT xps

typedef struct argument{
	char *name, *interm, *f, *s, *m, *o, *tofig, *topic, *tops;
	enum language tolang;
	enum input type;
	struct argument *next;
} argument ;

extern enum language str2lang();
extern char *lname[];
extern char *iname[];

extern char *sysls(), *mksuff();
extern argument *arglist;
extern char *txfile, *mkfile;

extern char *optarg;
extern int optind;

