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
#include "swindex.h"

#include "swmain.h"

int main (int argc, char ** argv) {
	swDefinitionFile *swindex;
	swindex=new swINDEX();
	if (!swindex) exit(1);
	swindex->open_parser(STDIN_FILENO);
	swindex->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (swindex->generateDefinitions()) exit(2); 

	
	swindex->xFormat_set_ofd(STDOUT_FILENO);
	swindex->swfile_write_pkg();
  	swindex->xFormat_write_trailer(); 
	exit (0);
}




