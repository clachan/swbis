static char testpath_cxx[] =
"testswpath1.c,v 1.1.1.1";

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swpath.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

#define LINELEN 900

int main (int argc, char ** argv)
{
  SWPATH * sp=swpath_open(""); 
  char line[LINELEN]; 
  
  
  while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
    {
      line[strlen(line)-1]='\0'; 
      
      if (strlen (line) >= LINELEN - 2)
	{
	  fprintf (stderr, "filesize: line too long : %s\n", line);
	}
      else
	{
         if (swpath_parse_path(sp, line) < 0) {
             fprintf (stderr, "error parsing %s\n", line);
             fprintf (stdout, "=======>>>>> Error parsing %s\n", line);
         }
	 
	 
	 fprintf (stdout, "...\n");
	 fprintf (stdout, "%s\n", line);
	 fprintf (stdout, "   pkgpathname          = \"%s\"\n", swpath_get_pkgpathname(sp));
	 fprintf (stdout, "   prepath              = \"%s\"\n", swpath_get_prepath(sp));
         fprintf (stdout, "   dfiles               = \"%s\"\n", swpath_get_dfiles(sp));
         fprintf (stdout, "   pfiles               = \"%s\"\n", swpath_get_pfiles(sp));
         fprintf (stdout, "   product_control_dir  = \"%s\"\n", swpath_get_product_control_dir(sp));
         fprintf (stdout, "   fileset_dir          = \"%s\"\n", swpath_get_fileset_control_dir(sp));
         fprintf (stdout, "   pathname             = \"%s\"\n", swpath_get_pathname(sp));
         fprintf (stdout, "   filename             = \"%s\"\n", swpath_get_basename(sp));
         fprintf (stdout, "   is catalog           = \"%d\"\n", swpath_get_is_catalog(sp));
         fprintf (stdout, "   control_depth        = \"%d\"\n", sp->control_path_nominal_depth_);
         fprintf (stdout, "   is_minimal_layout    = \"%d\"\n", sp->is_minimal_layoutM);
	/* swpath_debug_dump(sp, stdout);	 */
	
	}
    }
  swpath_close(sp); 
  exit (0);

}
