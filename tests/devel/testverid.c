#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "strob.h"
#include "swverid.h"
#include "swmain.h"

int
main(int argc, char ** argv)
{
	int uargc;
	int i;
	char ** uargs;
	char buf[1000];	
	STROB * tmp = strob_open(10);
	SWVERID * pattern;
	SWVERID * candidate_list[20];
	SWVERID ** p_candidate;

	if (argc < 3 || argc > 19) {
		fprintf(stderr, "Usage: testverid  pattern_spec candidate_spec [candidate_spec...]\n");
		exit(1);
	}	
	pattern = swverid_open(NULL, argv[1]);
	swverid_print(pattern, tmp);
	fprintf(stdout,"pattern_spec(1): %s\n", strob_str(tmp));


	/* Test the copy constructor routines */
	pattern = swverid_copy(pattern);	
	swverid_print(pattern, tmp);
	fprintf(stdout,"pattern_spec(2): %s\n", strob_str(tmp));

	uargc = 2;
	i = 0;
	while (uargc < argc) {
		candidate_list[i] = swverid_open(NULL, argv[uargc]);
		swverid_print(candidate_list[i], tmp);
		fprintf(stdout, "candidate_spec: %s\n", strob_str(tmp));
		uargc++; i++;
	}
	candidate_list[i] = NULL;

	
	p_candidate = candidate_list;
	while (*p_candidate != NULL) {
		p_candidate++; 
	}
	
	exit(0);
}
