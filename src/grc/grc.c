
/*
    GEOS resource compiler
    
    by Maciej 'YTM/Alliance' Witkowiak

    Error function by Uz

    see GEOSLib documentation for license info

*/

/* 
 - make it work, then do it better
 - more or less comments? it was hard to code, should be even harder to
   understand =D
 - add loadable icons feature (binary - 63 bytes)
*/

/* - err, maybe free allocated memory, huh? (who cares, it's just a little prog...)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "grc.h"


void Error (const char* Format, ...)
/* borrowed from cl65/error.c	  */
/* Print an error message and die */
{
    va_list ap;
    va_start (ap, Format);
    fprintf (stderr, "%s: ", progName);
    vfprintf (stderr, Format, ap);
    va_end (ap);
    exit (EXIT_FAILURE);
}

void printCHeader (void) {

    fprintf(outputCFile, "\n/*\n\tThis file was generated by GEOS Resource Compiler\n"
	    "\n\tDO NOT EDIT! Any changes will be lost!\n"
	    "\n\tEdit proper resource file instead\n"
	    "\n*/\n\n");
}

void printSHeader (void) {

    fprintf(outputSFile, "\n;\n;\tThis file was generated by GEOS Resource Compiler\n;"
	    "\n;\tDO NOT EDIT! Any changes will be lost!\n;"
	    "\n;\tEdit proper resource file instead\n;"
	    "\n;\n\n");
}

void openCFile (void) {
    if ((CFnum==0) && (forceFlag==0)) {
	/* test if file exists already and no forcing*/
	if ((outputCFile = fopen (outputCName,"r"))!=0)
	    Error("file %s already exists, no forcing, aborting\n", outputCName);
	}
    if ((outputCFile = fopen (outputCName,outputCMode))==0) 
	Error("can't open file %s for writting: %s\n",outputCName,strerror (errno));
    if (CFnum==0) { outputCMode[0]='a'; printCHeader(); CFnum++; }
}

void openSFile (void) {
    if ((SFnum==0) && (forceFlag==0)) {
	/* test if file exists already and no forcing*/
	if ((outputSFile = fopen (outputSName,"r"))!=0)
	    Error("file %s already exists, no forcing, aborting\n", outputSName);
	}
    if ((outputSFile = fopen (outputSName,outputSMode))==0) 
	Error("can't open file %s for writting: %s\n",outputSName,strerror (errno));
    if (SFnum==0) { outputSMode[0]='a'; printSHeader(); SFnum++; }
}

void printUsage (void) {
    fprintf(stderr, "Usage: %s [options] file\n"
	    "Options:\n"
	    "\t-h, -?\t\tthis help\n"
	    "\t-f\t\tforce writting files\n"
	    "\t-o name\t\tname C output file\n"
	    "\t-s name\t\tname asm output file\n",
	    progName);

}

int findToken (const char **tokenTbl, const char *token) {
/* takes as input table of tokens and token, returns position in table or -1 if not found */
int a=0;

    while (strlen(tokenTbl[a])!=0) {
	if (strcmp(tokenTbl[a],token)==0) break;
	a++;
    }
    if (strlen(tokenTbl[a])==0) a=-1;
    return a;
}

char *nextPhrase() {
    return strtok(NULL, "\"");
    }

char *nextWord() {
    return strtok(NULL, " ");
    }

void setLen (char *name, int len) {
    if (strlen(name)>len)
	name[len]='\0';
}

void fillOut (char *name, int len, char *filler) {
int a;
    setLen (name, len);
    fprintf(outputSFile, ".byte \"%s\"\n\t\t", name);
    a = len - strlen(name);
    if (a!=0) {
	fprintf(outputSFile, ".byte %s", filler);
        while (--a!=0) fprintf(outputSFile, ", %s", filler);
	fprintf(outputSFile, "\n\t\t");
	}
}

char *bintos(unsigned char a, char *out) {
int i=0;
    for (;i<8;i++) {
    out[7-i] = ((a & 1)==0) ? '0' : '1';
    a = a >> 1; };
    out[i]='\0';
return out;
}

int getNameSize (const char *word) {
/* count length of a word using BSW 9 font table */
int a=0, i=0;

    while (word[i]!='\0') {
	a+=(BSWTab[word[i]-31] - BSWTab[word[i]-32]); i++; }

    return a;
}

void DoMenu (void) {

int a, size, tmpsize, item=0;
char *token;
char namebuff[255]="";
struct menu myMenu;
struct menuitem *curItem, *newItem;

    openCFile();

    myMenu.name=nextWord();
    myMenu.left=atoi(nextWord());
    myMenu.top=atoi(nextWord());
    myMenu.type=nextWord();

    if (strcmp(nextWord(),"{")!=0) { 
	Error ("menu %s description has no opening bracket!\n", myMenu.name);
	};
    curItem=malloc(sizeof(struct menuitem));
    myMenu.item=curItem;
    do {
	token = nextWord();
	if (strcmp(token,"}")==0) break;
	if (token[strlen(token)-1]!='"') {
	    strcpy (namebuff, token);
	    do {
		token = nextWord();
		strcat (namebuff, " ");
		strcat (namebuff, token);
		} while (token[strlen(token)-1]!='"');
	    token = malloc(strlen(namebuff));
	    strcpy (token, namebuff);
	}
	curItem->name=token;
	curItem->type=nextWord();
	curItem->target=nextWord();
	newItem=malloc(sizeof(struct menuitem));
	curItem->next=newItem;
	curItem=newItem;
	item++;
	} while (strcmp(token,"}")!=0);
    if (item==0) Error ("menu %s has 0 items!\n", myMenu.name);
    if (item>31) Error ("menu %s has too many items!\n", myMenu.name);

    curItem->next=NULL;

    /* Count menu sizes */

    size=0;
    curItem=myMenu.item;
    if (strstr(myMenu.type,"HORIZONTAL")!=NULL) {
	/* menu is HORIZONTAL, ysize=15, sum xsize of all items +~8?*/
	    myMenu.bot=myMenu.top+15;
	    for (a=0;a!=item;a++) {
		size+=getNameSize(curItem->name);
		curItem=curItem->next;
		};
	} else {
	/* menu is VERTICAL, ysize=item*15, count largest xsize of all items +~8? */
	    myMenu.bot=myMenu.top+(14*item)-1;
	    for (a=0;a!=item;a++) {
		tmpsize=getNameSize(curItem->name);
		size = (size > tmpsize) ? size : tmpsize;
		curItem=curItem->next;
		};
	};
    myMenu.right=myMenu.left+size-1;

    curItem=myMenu.item;
    for (a=0;a!=item;a++) {
	/* print prototype only if MENU_ACTION or DYN_SUB_MENU are present in type */
	if ((strstr(curItem->type, "MENU_ACTION")!=NULL) || (strstr(curItem->type, "DYN_SUB_MENU")!=NULL))
	    fprintf(outputCFile, "void %s (void);\n", curItem->target);
	curItem=curItem->next;
	}

    fprintf(outputCFile, "\nconst void %s = {\n\t(char)%i, (char)%i,\n\t(int)%i, (int)%i,\n\t"
	    "(char)(%i | %s),\n", myMenu.name, myMenu.top, myMenu.bot, myMenu.left,
	    myMenu.right, item, myMenu.type);

    curItem=myMenu.item;
    for (a=0;a!=item;a++) {
	fprintf(outputCFile, "\t%s, (char)%s, (int)", curItem->name, curItem->type);
	if ((strstr(curItem->type, "SUB_MENU")!=NULL) && (strstr(curItem->type, "DYN_SUB_MENU")==NULL))
	    fprintf(outputCFile, "&");
	fprintf(outputCFile, "%s,\n", curItem->target);
	curItem=curItem->next;
	}

    fprintf(outputCFile, "\t};\n\n");

    if (fclose (outputCFile)!=0) 
	Error("error closing %s: %s\n",outputCName,strerror (errno));
}

void DoHeader (void) {

time_t t;
struct tm *my_tm;

struct appheader myHead;
char *token;
char i1[9], i2[9], i3[9];
int a, b;

    openSFile();

    token = nextWord();
    
    a = findToken (hdrFTypes, token);

    if (a>1)
	Error("filetype %s isn't supported yet\n", token);

    switch (a) {
	case 0: myHead.geostype = 6; break;
	case 1: myHead.geostype = 14; break;
	}

    myHead.dosname = nextPhrase();
    nextPhrase();
    myHead.classname = nextPhrase();
    nextPhrase();
    myHead.version = nextPhrase();

    /* put default values into myHead here */
    myHead.author = "cc65";
    myHead.info = "Program compiled with cc65 and GEOSLib.";
    myHead.dostype = 128+3;
    myHead.mode = 0;

    t = time(NULL);
    my_tm = localtime (&t);

    myHead.year = my_tm->tm_year;
    myHead.month = my_tm->tm_mon+1;
    myHead.day = my_tm->tm_mday;
    myHead.hour = my_tm->tm_hour;
    myHead.min = my_tm->tm_min;

    if (strcmp(nextWord(),"{")!=0) { 
	Error ("header %s has no opening bracket!\n", myHead.dosname);
	};

    do {
	token=nextWord();
	if (strcmp(token, "}")==0) break;
        switch (a = findToken (hdrFields, token)) {
		case -1:
		    Error ("unknown field %s in header %s\n", token, myHead.dosname);
		    break;
		case 0: /* author */
		    myHead.author = nextPhrase(); break;
		case 1: /* info */
		    myHead.info = nextPhrase(); break;
		case 2:	/* date */
		    myHead.year = atoi(nextWord());
		    myHead.month = atoi(nextWord());
		    myHead.day = atoi(nextWord());
		    myHead.hour = atoi(nextWord());
		    myHead.min = atoi(nextWord());
		    break;
		case 3:	/* dostype */
		    switch (b = findToken (hdrDOSTp, nextWord())) {
			case -1:
			    Error ("unknown dostype in header %s\n", myHead.dosname);
			    break;
			default:
			    myHead.dostype = b/2 + 128 + 1;
			    break;
		    }
		    break;
		case 4:	/* mode */
		    switch (b = findToken (hdrModes, nextWord())) {
			case -1:
			    Error ("unknown mode in header %s\n", myHead.dosname);
			case 0:
			    myHead.mode = 0x40; break;
			case 1:
			    myHead.mode = 0x00; break;
			case 2:
			    myHead.mode = 0xc0; break;
			case 3:
			    myHead.mode = 0x80; break;
		    }
		    break;
        }
    
    } while (strcmp(token, "}")!=0);

    /* OK, all information is gathered, do flushout */

    fprintf(outputSFile,
	 "\t\t\t.segment \"HEADER\"\n\n\t\t.byte %i\n\t\t.word 0\n\t\t", myHead.dostype);

    fillOut(myHead.dosname,16,"$a0");

    fprintf(outputSFile,
	".word 0\n\t\t.byte 0\n\t\t.byte %i\n\t\t.byte %i, %i, %i, %i, %i\n\n\t\t"
	".word 0\n\t\t.byte \"PRG formatted GEOS file V1.0\"\n\n\t\t.res $c4\n\n\t\t"
	".byte 3, 21, 63 | $80\n\t\t",
	myHead.geostype, myHead.year, myHead.month, myHead.day, myHead.hour,
	myHead.min);

    for (a=0;a!=63;a=a+3) {
	fprintf(outputSFile,
	     ".byte %%%s, %%%s, %%%s\n\t\t",
	     bintos(icon1[a], i1), bintos(icon1[a+1], i2), bintos(icon1[a+2], i3)); };

    fprintf(outputSFile,
	    "\n\t\t.byte %i, %i, 0\n\t\t.word $0400, $0400-1, $0400\n\n\t\t",
	    myHead.dostype, myHead.geostype);

    fillOut(myHead.classname,12,"$20");

    fillOut(myHead.version,4,"0");
    
    fprintf(outputSFile,
	    ".byte 0, 0, 0\n\t\t.byte %i\n\n\t\t", myHead.mode);

    setLen(myHead.author,62);
    fprintf(outputSFile,
	    ".byte \"%s\"\n\t\t.byte 0\n\t\t.res (63-%i)\n\n\t\t",
	    myHead.author, strlen(myHead.author)+1);

    setLen(myHead.info, 95);
    fprintf(outputSFile,
	    ".byte \"%s\"\n\t\t.byte 0\n\t\t.res (96-%i)\n\n",
	    myHead.info, strlen(myHead.info)+1);

    if (fclose (outputSFile)!=0) 
	Error("error closing %s: %s\n",outputSName,strerror (errno));


}

char *filterInput (FILE *F, char *tbl) {
/* loads file into buffer filtering it out */
int a, prevchar=-1, i=0, bracket=0, quote=1;

    while (1) {
	a = getc(F);
	if ((a=='\n')||(a=='\015')) a = ' ';
	if (a==',') a = ' ';
	if (a=='\042') quote=!quote;
	if (quote) {
	    if ((a=='{')||(a=='(')) bracket++;
	    if ((a=='}')||(a==')')) bracket--;
	}
	if (a==EOF) { tbl[i]='\0'; realloc(tbl, i+1); break; };
	if (isspace(a)) {
	    if ((prevchar!=' ') && (prevchar!=-1)) { tbl[i++]=' '; prevchar=' '; }
	} else {
	    if (a==';' && quote) { do { a = getc (F); } while (a!='\n'); fseek(F,-1,SEEK_CUR); }
		else {
		    tbl[i++]=a; prevchar=a; }
	}
    }

    if (bracket!=0) Error("there are unclosed brackets!\n");

    return tbl;
}

void processFile (const char *filename) {

FILE *F;

char *str;
char *token;

int head=0;

    if ((F = fopen (filename,"r"))==0) 
	Error("can't open file %s for reading: %s\n",filename,strerror (errno));

    str=filterInput(F, malloc(BLOODY_BIG_BUFFER));

    token = strtok (str," ");

    do {
        if (str!=NULL) {
	    switch (findToken (mainToken, token)) {
	    
	    case 0: DoMenu(); break;
	    case 1: 
		if (++head!=1) {
			Error ("more than one HEADER section, aborting.\n");
		    } else {
			DoHeader();
		    } 
		break;
	    case 2: break;
	    case 3: break;
	    default: Error ("unknown section %s.\n",token); break;
	    }
	}
	token = nextWord();
    } while (token!=NULL);
}

int main(int argc, char *argv[]) {

int ffile=0, i=1;
char *p, *tmp;

    progName = argv[0];
    while (i < argc) {
	const char *arg = argv[i];
	if (arg[0] == '-') {
	    switch (arg[1]) {
		case 'f':
		    forceFlag=1;
		    break;
		case 'o':
		    outputCName=argv[++i];
		    break;
		case 's':
		    outputSName=argv[++i];
		    break;
		case 'h':
		case '?':
		    printUsage();
		    exit (EXIT_SUCCESS);
		    break;
		default: Error("unknown option %s\n",arg);
	    }
	} else {
	    ffile++;

	    tmp = malloc(strlen(arg)+4);
	    strcpy (tmp, arg);
	    if ((p = strrchr (tmp, '.')))
		*p = '\0';


	    if (outputCName==NULL) {
		outputCName = malloc(strlen(arg));
		strcpy (outputCName, tmp);
		strcat (outputCName, ".h");
	    }


	    if (outputSName==NULL) {
		outputSName = malloc(strlen(arg));
		strcpy (outputSName, tmp);
		strcat (outputSName, ".s");
	    }


	    processFile(arg);

	    }
	i++;
	}
    if (ffile==0) Error("no input file\n");

    return EXIT_SUCCESS;
}
