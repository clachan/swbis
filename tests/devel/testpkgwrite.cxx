#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typeinfo>
#include "stream_config.h"
#include "swstruct.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swparser.h"
#include "swdefinitionfile.h"
#include "swindex.h"
#include "uxformat.h"

#include "swmain.h"

int main (int argc, char ** argv) {
	int format_code = arf_ustar;
	swDefinitionFile *swindex;
	swindex=new swINDEX();
	if (!swindex) exit(1);
	swindex->open_parser(STDIN_FILENO);
	swindex->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (swindex->generateDefinitions()) exit(2); 

	if (argc > 1) {
		if (!strcmp(argv[1], "ustar")) {
			format_code = arf_ustar;
		} else if (!strcmp(argv[1], "newc")) {
			format_code = arf_newascii;
		} else if (!strcmp(argv[1], "crc")) {
			format_code = arf_crcascii;
		} else if (!strcmp(argv[1], "odc")) {
			format_code = arf_oldascii;
		} else {
			format_code = arf_ustar;
		}
	}
	
	swindex->xFormat_set_format(format_code);
	swindex->xFormat_set_ofd(STDOUT_FILENO);
	swindex->swfile_write_pkg();
  	swindex->xFormat_write_trailer(); 
	exit (0);
}




