
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "stream_config.h"
#include "swstruct.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swparser.h"
#include "swdefinitionfile.h"
#include "swstructiter.h"
#include "switer.h"
#include "swspsf.h"
#include "swmain.h"

extern "C" {
#include "swheader.h"
#include "swheaderline.h"
char	 		get_character(char ch);
int 			get_integer(int j);
double 			get_double(double x);
unsigned long 		get_unsigned(unsigned long uu);
signed char 		get_signedchar(int gch);
unsigned long 		get_ulong(unsigned long j);
void 			get_string(char *str);
long 			get_long(long int j);
static char *		uget_gets(char *s);
}

#define LINELEN 1000

int
main (int argc, char ** argv)
{
	swMetaData *swmd; 
 	int ch = 'a'; 
	char line[LINELEN], *t; 
	char * name, * source;
	
	
	while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
	{
		if (strlen(line) >= LINELEN - 2) {
			fprintf (stderr, "line too long : %s\n", line);
		} else {
			fprintf (stderr, ":: %s", line);
			if ((t=strpbrk (line,"\n\r"))) {
				*t = '\0';
			}
		}

		if (!strlen(line)) {
			continue;
		}

		if (strchr(line, ' ')) {
			name = line;
			source = strchr(line, ' ') + 1;
		} else {
			name = line;
			source = line;
		}

		swmd = new swAttribute(name, source);

	}

	sleep(10);
	//while(1) {
	//	fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
	//	fprintf(stdout,"enter tag: <%s> ", value); get_string(value);
	//	swmd = new swAttribute(keyword, value);
	//}
	exit(0);
}


#define UGET_STRLEN 120
#define gets uget_gets

static char *uget_gets(char *);

char
get_character(char ch)
{
	char d[UGET_STRLEN];
	int c = 0;
	if (fflush(stdin))
		printf("flush error\n");
	gets(d);
	while (*(d + c) == 32)
		c++;
	if (*(d + c) == '\0')
		return ch;
	return *(d + c);
}


int
get_integer(int j)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag, i;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}
	}
	while (!(i = atoi(s)));
	return i;
}




double
get_double(double x)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	double retx;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return x;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && (decflag <= 1 && zeroflag))
				return (0);
		}

	}
	while (!(retx = atof(s)));
	return retx;
}


unsigned long
get_unsigned(unsigned long uu)
{
	unsigned long ul;
	//unsigned long get_ulong(unsigned long u);
	while ((ul = get_ulong((unsigned long) uu)) > UINT_MAX);

	return (unsigned long) ul;

}

signed char
get_signedchar(int gch)
{
	int c;


	while (((c = get_integer((signed char) gch)) > SCHAR_MAX) || (c < SCHAR_MIN));
	return (signed char) c;

}


unsigned long
get_ulong(unsigned long j)
{
	char s[UGET_STRLEN], *p1, *endp;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}

	}
	while (!strtoul(s, &endp, 10));
	return strtoul(s, &endp, 10);
}



void
get_string(char *str)
{
	char d[UGET_STRLEN];
	if (fflush(stdin))
		printf("flush error\n");
	gets(d);
	if (d[0] == '\0')
		return;
	strcpy(str, &d[0]);
	return;
}


long
get_long(long int j)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}

	}
	while (!atol(s));
	return atol(s);

}


static char *
uget_gets(char *s)
{
	if (!fgets(s, UGET_STRLEN - 2, stdin)) {
		fprintf
		    (stderr,
		     "usr_get.c: uget_gets(): fgets() failure, returning empty string.\n"
		    );
		s[0] = '\0';
		return s;
	}
	if (strchr(s, (int) '\n'))
		*strchr(s, (int) '\n') = '\0';
	if (strchr(s, (int) '\r'))
		*strchr(s, (int) '\r') = '\0';
	return s;
}

