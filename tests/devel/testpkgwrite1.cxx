
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "stream_config.h"
#include "swparser.h"
#include "swpsf.h"

#include "swmain.h"

int
main (void)
{
  swPSF *psf=new swPSF("/usr/tmp/aa.1");
  if (!psf) exit(1);
  psf->write_fd(STDOUT_FILENO);
  exit (0);
}

