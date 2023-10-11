
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "xstream_config.h"
#include "swparser.h"
#include "swpsf.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
	swPSF *psf;

	if (argc < 2)	
		psf=new swPSF("/dev/null");
	else 
		psf=new swPSF(argv[1]);

	if (!psf) exit(1);

	psf->open_parser(STDIN_FILENO);

	if (psf->run_parser(0, SWPARSE_FORM_MKUP_LEN) < 0 ) exit(1);
  	
	if (psf->generateDefinitions()) {
		fprintf(stderr, "error from generateDefinitions");	
		exit(2); 
	}
	cerr << psf->write_fd(STDOUT_FILENO) << "\n" ;

	close(1);
	close(2);
	exit (0);
}
