/* testheader.cxx
*/

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
#include "swstructiter.h"
#include "switer.h"
#include "swmain.h"

extern "C" {
#include "swheader.h"
}

int
main (void)
{
  int len;
  int offset=0;
  char * line;
  swIter * switer;
  swDefinitionFile *psf; 
  swsPSF *swspsf; 
  SWHEADER * swheader;
  int devnull=open("/dev/null", O_RDWR);

  psf=new swPSF("");
  if (psf->open_parser(STDIN_FILENO)){
	cerr << "Error opening  parser.\n";
  }
  
  
  if ( (len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN)) < 0){
	cerr << "Error in parser.\n";
  	exit(1);
  }

  swspsf=new swsPSF(psf);
  swspsf->generateStructuresFromParser();
  //swspsf->write(STDOUT_FILENO);
  //cout <<"-----------------------------------------------------\n"; 
  //swspsf->write(STDOUT_FILENO);
  //cout <<"-----------------------------------------------------\n"; 
  
  switer=new swIter(swspsf->get_swstruct()); 
 
  swheader=swheader_open(swIter::switer_get_nextline, static_cast<void*>(switer));
  swheader_set_current_offset_p(swheader, &offset);
  
  offset=0; 
  line=swheader_f_goto_next(swheader); 
  while (line){
  	line=swheader_f_goto_next(swheader); 
  }
  
  
  switer->reset(); 
  offset=0; 
  line=swheader_f_goto_next(swheader); 
  while (line){
  	::swheaderline_write(line, devnull);
  	line=swheader_f_goto_next(swheader); 
  }
  
  switer->reset(); 
  offset=0; 
  line=swheader_f_goto_next(swheader); 
  while (line){
  	::swheaderline_write(line, devnull);
  	line=swheader_f_goto_next(swheader); 
  }
  
  switer->reset(); 
  offset=0; 
  line=swheader_f_goto_next(swheader); 
  while (line){
	fflush(stdout);
  	::swheaderline_write(line, STDOUT_FILENO);
	fflush(stdout);
  	line=swheader_f_goto_next(swheader); 
  }
  
  
  swheader_close(swheader);
  delete psf;
  delete switer;
  delete swspsf;

}




