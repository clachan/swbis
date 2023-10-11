#ifndef UGETOPT_HELP_H_02
#define UGETOPT_HELP_H_02

#include "usgetopt.h"

struct ugetopt_option_desc {
	char * option;
	char * option_arg;
	char * desc;
};

void ugetopt_print_help(FILE * fp, char * pgm, struct option[], struct ugetopt_option_desc[]);

#endif
