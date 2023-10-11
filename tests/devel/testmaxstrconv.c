#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "strtoint.h"
#include "inttostr.h"

/* 
18446744073709551615
9223372036854775807
*/


int
main(int argc, char ** argv)
{
	char ** uargv;
	int arg_n;
	char u_buf[UINTMAX_STRSIZE_BOUND];
	char i_buf[UINTMAX_STRSIZE_BOUND];
	unsigned long ul_int;
	long l_int;
	uintmax_t max_uint;
	intmax_t max_int;
	char * p;
	char * su;
	char * si;
	char mode = 0;

	fprintf(stderr, "sizeof intmax_t is %d\n", sizeof(intmax_t));
	fprintf(stderr, "sizeof uintmax_t is %d\n", sizeof(uintmax_t));

	uargv = argv;
	if (argc <= 2) {
		fprintf(stderr, "Usage: %s {u|i} NNN\n", argv[0]);
		exit(1);
	}

	if (*(uargv[1]) == 'u') {
		mode = 'u';	
	} if (*(uargv[1]) == 'i') {
		mode = 'i';	
	}
	argc--;
	uargv++;
	
	arg_n = 1;
	while (argc > 1) {
		p =  uargv[arg_n];

		if (mode == 'u') {
			max_uint = strtoumax(p, (char **)NULL, 10);
			if (max_uint ==  ULLONG_MAX) {
				fprintf(stderr, "conversion error\n");
				exit(1);
			}
			su = umaxtostr(max_uint, u_buf);
			fprintf(stdout, "%s\n", su);
		}	

		if (mode == 'i') {
			max_int = strtoimax(p, (char **)NULL, 10);
			if (max_int == LLONG_MAX) {
				fprintf(stderr, "conversion error\n");
				exit(1);
			}
			si = imaxtostr(max_int, i_buf);
			fprintf(stdout, "%s\n", si);
		}
		arg_n++;
		argc--;
	}
	
	exit(0);
}
