
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>

#include "xstream_config.h"
#include "swdeffile.h"
#include "swparser.h"
#include "swpathname.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
  swPathName sp; 
  char inbuf[1000], c; 

  //sp.set_prepath (argv[1]);

  for (;;) {
         cin.get(inbuf,800,'\n');
         if (!::strlen(inbuf) || cin.get(c) && c != '\n') {
               if (!::strlen(inbuf)) exit (0);
	       exit (1);
	 }
         if (sp.swp_parse_path(inbuf) < 0) {
	    cerr << "error parsing " << inbuf << endl;
	 }
	 
	 cout << "----:" << inbuf << endl;
	 cout << "prepath:" << sp.swp_get_prepath() << endl;
         cout << "dfiles:" << sp.swp_get_dfiles() << endl;
         cout << "pfiles:" << sp.swp_get_pfiles() << endl;
         cout << "prod control dir:" << sp.swp_get_product_control_dir() << endl;
         cout << "prod fileset dir:" << sp.swp_get_fileset_control_dir() << endl;
         cout << "filename:" << sp.swp_get_basename() << endl;
         cout << "pathname:" << sp.swp_get_pathname() << endl;
         cout << "is catalog:" << sp.swp_get_is_catalog() << endl;
         
	 cout << endl;
  }

exit (0);

}


