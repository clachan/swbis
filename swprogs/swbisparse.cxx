// swbisparse.cxx - A stand-alone parser for INFO, INDEX and PSF files.

/*
 Copyright (C) 2001 Jim Lowe

*/
/*
 COPYING TERMS AND CONDITIONS:

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <typeinfo>
#include "stream_config.h"

extern "C" {
#include "swparse.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swlib.h"
#include "swfork.h"
#include "swevents.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swcommon.h"
#include "swgp.h"
#include "fmgetpass.h"
#include "swutilname.h"
#include "swfdio.h"
#include "swextopt.h"
}

#include "swmain.h"  /* this file should be included by the main program file */
#include "swparser.h"
#include "swdefinitionfile.h"
#include "swpsf.h"
#include "swpackagefile.h"
#include "swexstruct.h"
#include "swexdistribution.h"

#include "swexfileset.h"
#include "swexproduct.h"
#include "swexpsf.h"
#include "swexdistribution.h"
#include "swscollection.h"
#include "swinstalled.h"
#include "swinfo.h"
#include "swoption.h"

extern int swparse_atlevel;
extern char  swlex_filename[512];

static int usage (char * progname, struct option  lop[])
{
    fprintf (stderr, "A POSIX-7.2 Metadata parser and testing and development.\n");
    fprintf (stderr, "utility of the swbis project.\n");
    fprintf (stderr, "Copyright (C) 2003 Jim Lowe\n");
    fprintf (stderr, "This software may only be redistributed under terms of the GNU GPL.\n");
    fprintf (stderr, "\nUsage:   swbisparse [options] {-i|-o|-p} [psf_file]\n");
    fprintf (stderr, "Write annotated, single line output to stdout.\n");
    fprintf (stderr, "\n");

       static struct ugetopt_option_desc help_desc[] =
             {
               {"indent", "", "Beautify only, output is semantically unchanged."},
               {"index", "", "parse according to INDEX file syntax."},
               {"installed", "", "parse according to INSTALLED file syntax."},
               {"info", "", "parse according to INFO file syntax."},
               {"psf", "", "parse according to PSF file syntax."},
               {"psfi", "", "parse an include file (attribute value)\n"
	       "             Internal use only."},
               {"option", "", "parse according to extended option file syntax."},
               {"PSF", "", "same as lower case equivalent."},
               {"INFO", "", "same as lower case equivalent."},
               {"INDEX", "", "same as lower case equivalent."},
               {"INSTALLED", "", "same as lower case equivalent."},
    	       {"debug", "", "Be verbose when parsing."},
               {"level", "LEVEL", "Set level to LEVEL."},
               {"with-line-lengths", "", "Include attribute value lengths in output"},
               {"posix-ignores", "", "Exclude certain attributes from output according to the Std."},
               {"help", "", "Show this help"},
               {0, 0, 0}
             };

	ugetopt_print_help(stderr, progname, lop, help_desc);

    exit (1);
}

int main (int argc, char *argv[])
{
  int fd, c=0, ret;
  char  *progname;
  swDefinitionFile * swdef = NULL;
  int oForm = SWPARSE_FORM_MKUP;
  int do_debug = 0;
 
  swparse_atlevel=0;
 
 static struct option long_options[] =
             {
               {"indent", 0, 0, 'b'},
               {"index", 0, 0, 'i'},
               {"installed", 0, 0, 200},
               {"info", 0, 0, 'o'},
               {"psf", 0, 0, 'p'},
               {"psfi", 0, 0, 'h'},
               {"option", 0, 0, 'd'},
               {"PSF", 0, 0, 'p'},
               {"INFO", 0, 0, 'o'},
               {"INDEX", 0, 0, 'i'},
               {"INSTALLED", 0, 0, 200},
               {"debug", 0, 0, 'v'},
               {"level", 1, 0, 'l'},
               {"with-line-lengths", 0, 0, 'n'},
		{"posix-ignores", 0, 0, 'g'},
               {"help", 0, 0, '\007'},
               {0, 0, 0, 0}
 };

  progname = argv[0];
  swlib_utilname_set(progname);
  
  if (argc < 2) { usage(progname, long_options); exit(1); }  
  
         while (1)
           {
             int option_index = 0;

             c = ugetopt_long (argc, argv, "vdbiopgl:n_",
                        long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'b':
                 oForm &= ~SWPARSE_FORM_ALL;
		 oForm |= SWPARSE_FORM_INDENT;
		 break;
               case 'd':
                 swdef=new swOPTION("");
		 break;
               case 'h':
                 swdef=new swPSFi("");
		 break;
               case 'i':
                 swdef=new swINDEX("");
		 break;
               case 200:
                 swdef=new swINSTALLED("");
		 break;
	       case 'g':
                 oForm |= SWPARSE_FORM_POLICY_POSIX_IGNORES;
		 break;

               case 'o':
                 swdef=new swINFO("");
                 break;

               case 'p':
                 //swlex_definition_file = SW_PSF;
                 swdef=new swPSF("aaaa");        
                 break;
               
	       case 'v':
		 do_debug = 1;
		/*
                 swlex_debug = 1;
                 swparse_debug = 1;
		*/
                 break;
               case '?':
                 usage(progname, long_options);
                 break;
	       case 'l':
                 if (!optarg) usage(progname, long_options);
		 swparse_atlevel=atoi(optarg);
                 break;
	       case 'n':
                 oForm &= ~SWPARSE_FORM_ALL;
                 oForm |= SWPARSE_FORM_MKUP_LEN;
                 break;
	       case '_':
                 usage(progname, long_options);
                 break;
	       case ':':
                 usage(progname, long_options);
                 break;
               default:
                 usage(progname, long_options);
                 break;
               }
          }
	  if (!swdef) {
                 usage(progname, long_options);
	  }

          if (optind < argc) {
             fd = open ( argv[optind], O_RDONLY );
             if (fd <  0) exit (4);
             swlib_strncpy(swlex_filename, argv[optind], sizeof(swlex_filename));
          } else {
             fd = STDIN_FILENO;
             strcpy(swlex_filename, "stdin" );
          }

          swdef->open_parser(fd, STDOUT_FILENO);

	  if (do_debug) {
                 swlex_debug = 1;
                 swparse_debug = 1;
	  }

          ret=swdef->run_parser(swparse_atlevel, oForm);

  delete swdef;
 // close(1);
  exit (ret == 0 ? 0 : 1);
}

