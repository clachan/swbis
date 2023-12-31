/** \ingroup rpmts
 * \file lib/rpmvercmp.c
 */

/*
    2005-12-3 Jim Lowe  <jhlowe@acm.org>
    This file was taken from the rpm-4.2 package and is
    used with no substantive modifications.
    The copying license is GNU GPL
*/

/*
#include "system.h"
#include <rpmlib.h>
#include "debug.h"
*/

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define xisalnum isalnum
#define xisdigit isdigit
#define xisalpha isalpha

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
int swlib_rpmvercmp(const char * a, const char * b)
{
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;

    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    str1 = alloca(strlen(a) + 1);
    str2 = alloca(strlen(b) + 1);

    strcpy(str1, a);
    strcpy(str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    /*@-branchstate@*/
/*@-boundsread@*/
    while (*one && *two) {
	while (*one && !xisalnum(*one)) one++;
	while (*two && !xisalnum(*two)) two++;

	str1 = one;
	str2 = two;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (xisdigit(*str1)) {
	    while (*str1 && xisdigit(*str1)) str1++;
	    while (*str2 && xisdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && xisalpha(*str1)) str1++;
	    while (*str2 && xisalpha(*str2)) str2++;
	    isnum = 0;
	}

	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
/*@-boundswrite@*/
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';
/*@=boundswrite@*/

	/* take care of the case where the two version segments are */
	/* different types: one numeric, the other alpha (i.e. empty) */
	if (one == str1) return -1;	/* arbitrary */
	/* XXX See patch #60884 (and details) from bugzilla #50977. */
	if (two == str2) return (isnum ? 1 : -1);

	if (isnum) {
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */

	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    if (strlen(one) > strlen(two)) return 1;
	    if (strlen(two) > strlen(one)) return -1;
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) return rc;

	/* restore character that was replaced by null above */
/*@-boundswrite@*/
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
/*@=boundswrite@*/
    }
    /*@=branchstate@*/
/*@=boundsread@*/

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
/*@-boundsread@*/
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
/*@=boundsread@*/
}

/*
int
main(int argc, char **argv)
{
	int ret = swlib_rpmvercmp(argv[1], argv[2]);

	if (ret > 0) {
		fprintf(stdout, "%s newer than %s\n", argv[1], argv[2]);
	} else if (ret == 0) {
		fprintf(stdout, "%s equals %s\n", argv[1], argv[2]);
	} else {
		fprintf(stdout, "%s older than %s\n", argv[1], argv[2]);
	}
	exit(ret);
}
*/
