
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <typeinfo>

#include "xstream_config.h"
#include "swpathname.h"
#include "swmain.h"

extern "C" {
#include "swlib.h"
}

int
main (int argc, char ** argv)
{
  swPathName sp; 
  char inbuf[1000], c; 
  char outbuf[1000];
  STROB * buf = strob_open(100);

  for (;;) {
         cin.get(inbuf,800,'\n');
         if (!::strlen(inbuf) || cin.get(c) && c != '\n') {
               if (!::strlen(inbuf)) exit (0);
	       exit (1);
	 }
         if (sp.swp_parse_path(inbuf) < 0) {
	    cerr << "error parsing " << inbuf << endl;
	 }
	 cout << sp.swp_form_path(buf) << endl;
  }

exit (0);

}


