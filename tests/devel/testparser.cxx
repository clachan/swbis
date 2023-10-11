#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ugetopt.h"
#include "swinfo.h"
#include "swindex.h"
#include "swpsf.h"
//#include "swstruct.h"
//#include "swstruct_i.h"
//#include "swdefinition.h"
//#include "swspsf.h"
//#include "swparser.h"
//#include "swdefinitionfile.h"
//#include "switer.h"
//#include "swstructiter.h"
//#include "swspsf.h"
//#include "swparser_global.h"


extern "C" {
#include "swparse.h"
}
#include "swmain.h"

extern int swparse_atlevel;
extern char  swlex_filename[512];

int main (int argc, char *argv[])
{
	int fd, len;
	swDefinitionFile * swdef=NULL;
	int oForm = SWPARSE_FORM_MKUP;
	swparse_atlevel=0;

	if (argc <= 0) {
		exit (2);
	}
	swdef=new swPSF("noname");        
	if (!swdef) {
               exit(2);
	}

	if (atoi(argv[1]) == 1) {
		oForm=SWPARSE_FORM_MKUP_LEN;
		fd = STDIN_FILENO;
		strcpy(swlex_filename, "stdin" );
		swdef->open_parser(fd, STDOUT_FILENO);
		len=swdef->run_parser(swparse_atlevel, (int)oForm);
	} else if (atoi(argv[1]) == 2) {
		int ofd = uxfio_open("/dev/null", O_RDWR, 0);
		int ifd = uxfio_open("/dev/null", O_RDWR, 0);
		uxfio_fcntl(ofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
		uxfio_fcntl(ofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
		
		uxfio_fcntl(ifd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
		uxfio_fcntl(ifd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

		fd = STDIN_FILENO;
		swlib_pump_amount(ifd, fd, -1);
		uxfio_lseek(ifd, SEEK_SET, 0);
		
		//swlib_pump_amount(ifd, STDOUT_FILENO, -1);
		//exit (0);	
		
		oForm = SWPARSE_FORM_INDENT;
		strcpy(swlex_filename, "stdin" );
		swdef->open_parser(ifd, ofd);
		len=swdef->run_parser(swparse_atlevel, (int)oForm);
		
		uxfio_lseek(ofd, SEEK_SET, 0);
		//swlib_pump_amount(ofd, STDOUT_FILENO, -1);
		//exit (0);	

		swdef->close_parser();
		
		oForm= SWPARSE_FORM_MKUP_LEN;
		swdef->open_parser(ofd, STDOUT_FILENO);
		len=swdef->run_parser(swparse_atlevel, (int)oForm);
	} else {
		exit(3);
	}
	delete swdef;
	exit (0);
}
