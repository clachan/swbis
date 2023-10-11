#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "xformat.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

#define LINELEN 3000

int
main (int argc, char *argv[])
{

  char line[LINELEN], *t; 
  int c;
  int debugmode = 0;
  struct stat st;
  int format=arf_ustar;
  XFORMAT * xxformat;
  char * name, * source;
  char * statname;
  char * s;

         while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "DH:",
                        long_options, &option_index);
             if (c == -1) break;

             switch (c)
               {
		case 'D':
			debugmode = 1;
			break;
		case 'H':
			if (!strcmp(optarg,"ustar")) {
  				format=arf_ustar;
			} else if (!strcmp(optarg,"newc")) {
  				format=arf_newascii;
			} else if (!strcmp(optarg,"crc")) {
  				format=arf_crcascii;
			} else if (!strcmp(optarg,"odc")) {
  				format=arf_oldascii;
			} else {
				fprintf (stderr,"unrecognized format: %s\n", optarg);
				exit(2);
			}
		break;
               }
           }
 
 xxformat=xformat_open(-1, STDOUT_FILENO, format);

 /*D if (debugmode) fprintf(stderr, "%s", xformat_dump_string_s(xxformat, ""));  */

 while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
    {
      if (strlen (line) >= LINELEN - 2)
	{
	  fprintf (stderr, "filesize: line too long : %s\n", line);
	}
      else
	{
	  
	  if ( (t=strpbrk (line,"\n\r"))) {
             *t = '\0';
	  }
	  
	  if (strlen(line)) {
		if ((s = strchr(line, ' '))) {
			*s = '\0';
			source = ++s;
	   		name = line;
			statname = source;
		} else {
	   		name = line;
			source = NULL; 
			statname = line;
		}
		if(lstat (statname, &st) == 0 ) {
			xformat_write_file(xxformat, &st, name, source);
		} else {
			fprintf (stderr,"list2tar: lstat %s %s\n", name, strerror(errno));
		}	
	} 
 	/*D if (debugmode) fprintf(stderr, "%s", xformat_dump_string_s(xxformat, ""));  */
     }
}
 xformat_write_trailer(xxformat);
 /*D if (debugmode) fprintf(stderr, "%s", xformat_dump_string_s(xxformat, ""));  */
 xformat_close(xxformat);
  exit (0);
}

