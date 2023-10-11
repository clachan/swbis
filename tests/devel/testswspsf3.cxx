// testswspsf3.cxx

#include "swuser_config.h"
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
#include "swstructiter.h"
#include "switer.h"
#include "swspsf.h"

#include "swmain.h"

int
main (int argc, char ** argv)
{
  int inode = 0;
  int len=0;
  swDefinition *swdef; 
  swMetaData *swmd; 
  swDefinitionFile *psf; 
  swsPSF *swspsf; 
  SWHEADER        * swheader;
  swStruct * sws; 
  swIter * switer, *switer1; 
  char * first_line, *line;
  int ifd;
  

  ifd = STDIN_FILENO;
  psf=new swPSF("");
  psf->open_parser(ifd);
  psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  if (psf->generateDefinitions()) exit(2); 
 
  swspsf = new swsPSF(psf);
  swspsf->generateStructures();
  
  switer  = new swIter(swspsf->get_swstruct()); 
  switer1  = new swIter(swspsf->get_swstruct()); 
  //cerr << swspsf->write(STDOUT_FILENO) << endl;
  //exit(0); 
  
  swheader = swheader_open(swIter::switer_get_nextline, static_cast<void*>(switer)); 
  swheader_set_current_offset_p(swheader, &inode);

  //
  // Initialize the SWHEADER object.
  //
  //swheader_reset(swheader);
  //swheader_set_current_offset_p_value(swheader, 0);


  inode = INT_MAX;
  first_line = swheader_goto_next_line(swheader, &inode, SWHEADER_PEEK_NEXT); 
  inode = 0;
  line = swheader_f_goto_next(swheader);
  while (line){
	swmd=swIter::switer_get_attribute(static_cast<void*>(switer1), inode);
	assert(swmd);
	sws = swIter::switer_get_swstruct(static_cast<void*>(switer1), inode);
	if (swmd->get_type() == swstructdef::sdf_object_kw) {
		assert(sws);
		swdef=dynamic_cast<swDefinition*>(swmd);
		assert(swdef);
		len += swdef->write_fd(STDOUT_FILENO);
 	} else {
		assert(!sws);
	}
	line=swheader_f_goto_next(swheader);
  }
  cerr << len << endl;
}
