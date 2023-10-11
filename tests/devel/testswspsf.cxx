
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>

#include "xstream_config.h"
#include "swparser.h"
#include "swstruct.h"
#include "swspsf.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swscollection.h"
#include "swdefinitionfile.h"
#include "swpsf.h"
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

  //X fprintf(stderr, "JL len=%d\n", len);
  //X psf->write_fd(STDERR_FILENO);

  swspsf=new swsPSF(psf);
  swspsf->generateStructuresFromParser();
  len=swspsf->iwrite(STDOUT_FILENO);
  cerr << len << endl;

}

