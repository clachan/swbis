
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
main (void)
{
  swPSF *psf=new swPSF("/dev/null");
  if (!psf) exit(1);

  static_cast<swAttribute*>(psf)->debug_writeM = 1;
  psf->open_parser(STDIN_FILENO);
  psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  psf->generateDefinitions(); 
  cerr << psf->write_fd(STDOUT_FILENO) << "\n" ;
  exit (0);
}

