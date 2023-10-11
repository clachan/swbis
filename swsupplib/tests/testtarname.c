static char testpath_cxx[] =
"testtarname.c,v 1.1.1.1";

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swparser_global.h"
#include "taru.h"
#include "swpath.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
#include "swevents_array.h"
#include "swcommon_options.h"

#define LINELEN 900

int main (int argc, char ** argv)
{
  int ret;
  int do_long_link;
  int is_dir = 0;
  char line[LINELEN]; 
  char * nl;

  if (argc > 1) is_dir = 1;
  
  while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
    {

      nl = strpbrk(line, "\n\r");
      if (nl) *nl = '\0';
      
      if (strlen (line) >= LINELEN - 2)
	{
	  fprintf (stderr, "filesize: line too long : %s\n", line);
	}
      else
	{
		ret = taru_is_tar_filename_too_long(line, TARU_TAR_GNU_LONG_LINKS /* tarheaderflags */, &do_long_link, is_dir);
	  	fprintf(stdout, "%d %s\n", ret, line);
	}
    }
  exit (0);
}



