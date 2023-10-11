#include "swuser_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ugetopt.h"
#include "ugetopt_help.h"

void 
ugetopt_print_help(FILE * fp, char * pgm, struct option lop[], struct ugetopt_option_desc desc[])
{
	struct option * ilop = lop;
	struct ugetopt_option_desc * idesc = desc;


	while (ilop && (ilop)->name) {
		if (
			((ilop)->val >= 'A' && (ilop)->val <= 'z') ||
			((ilop)->val == '?')
		) {
			fprintf(fp, "  -%c", (ilop)->val);
		} else {
			fprintf(fp, "     ");
		}
		if (strlen((idesc)->option)) {
			if ((ilop)->val >= 'A' && (ilop)->val <= 'z') {
				fprintf(fp, ",");
			}
			fprintf(fp, " --%s", (ilop)->name);
		}

		if (strlen((idesc)->option) && (ilop)->has_arg) {
			fprintf(fp, "=");
		} else if (strlen((idesc)->option) == 0 && (ilop)->has_arg) {
			fprintf(fp, " ");
		}

		if ((ilop)->has_arg) {
			fprintf(fp, "%s", (idesc)->option_arg);
		} else {
			fprintf(fp, "  ");
		}
		if (desc && idesc->option) {
			fprintf(fp, "  %s", (idesc)->desc);
		}
		fprintf(fp, "\n");
		ilop++; idesc++;
	}
	return;
}
