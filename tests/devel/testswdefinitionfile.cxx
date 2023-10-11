
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "xstream_config.h"
#include "swpsf.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
	int size, len;
	unsigned long crc=0;
	char md5digest[40];

	swPSF *psf=new swPSF("");
	if (!psf)
  		exit(1);
	psf->open_parser(STDIN_FILENO);
	psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	psf->generateDefinitions(); 

	size=psf->swfile_get_size();
	psf->swfile_get_posix_cksum(&crc);
	assert (psf->swfile_get_ascii_md5(md5digest));
	cerr << "size is :" << size << "\n";
	cerr << "crc is :" << crc << "\n";
	cerr << "md5 is:" << md5digest << "\n";
  
	psf->write_fd(STDOUT_FILENO);
	exit (0);
}

