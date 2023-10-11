
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>

#include "xstream_config.h"
#include "swstruct.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swdefinitionfile.h"
#include "swparser.h"

#include "swmain.h"

int
main (void)
{
  int len,a;
  swDefinitionFile *psf; 
  swsPSF *swspsf; 

  psf=new swPSF("");
  psf->open_parser(STDIN_FILENO);
  len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  if (psf->generateDefinitions()) exit(2); 
  swspsf = new swsPSF(psf);
  swspsf->generateStructures();
  //len=swspsf->write(STDOUT_FILENO);
  cerr << swspsf->iwrite(STDOUT_FILENO) << endl;
}

