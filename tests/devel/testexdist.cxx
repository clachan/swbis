#ifdef DOTO_ID
static char testswdefinitionwrite_cxx[] =
"testexdist.cxx,v 1.1.1.1";
#endif


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "stream_config.h"
#include "swparser.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swdefinitionfile.h"
#include "swexdistribution.h"

#include "swmain.h"

int
main (void)
{
  int len;
  swDefinitionFile 	*psf; 
  swsPSF 		*swspsf; 
  //swExDistribution 	*swexdist;

  psf=new swPSF("");
  assert(psf);
  psf->open_parser(STDIN_FILENO);
  len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  if (len < 0) exit(2);

  swspsf=new swsPSF(psf);
  swspsf->generateStructuresFromParser();
  assert(swspsf);
  
  //swexdist = new swExDistribution();
  //swexdist->initialize(swspsf);
  //assert(swexdist);

  //len = swexdist->test_write(); 
  //cerr << len << endl;

  delete psf; 
  delete swspsf; 
  //delete swexdist; 
  
  exit(0);
}
