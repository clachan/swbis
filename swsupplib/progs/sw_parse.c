
#include "swuser_config.h"
#include "swprog_versions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "usgetopt.h"
#include "strob.h"
#include "swlib.h"
#include "swparse.h"
#include "uxfio.h"
#include "swlex_supp.h" 
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int yydebug=0;

extern YYSTYPE yylval;

static int usage (char *);

int 
main (int argc, char *argv[])
{
  int ret=0, fd, c=0;
  char typestring[12];
  char * progname;
  int posix_ignores = 0;
 
  swlex_definition_file = SW_INFO;
  swparse_atlevel=0;
  progname = argv[0]; 

  swparse_form_flag = 0;
  swparse_form_flag |= SWPARSE_FORM_MKUP;

  yylval.strb = strob_open (8);
  if (!yylval.strb) {
      fprintf (stderr," swparse: object allocation error.\n");
      exit (1);
  }
  if (argc < 2) { usage(progname); exit(1); }  
  
         while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
		{"indent", 0, 0, 'b'},
		{"index", 0, 0, 'i'},
		{"info", 0, 0, 'o'},
		{"psf", 0, 0, 'p'},
		{"option", 0, 0, 'd'},
		{"psfi", 0, 0, 'h'},
		{"PSF", 0, 0, 'p'},
		{"INFO", 0, 0, 'o'},
		{"INDEX", 0, 0, 'i'},
		{"debug", 0, 0, 'v'},
		{"level", 1, 0, 'l'},
		{"installed", 0, 0, 200},
		{"INSTALLED", 0, 0, 200},
		{"with-line-lengths", 0, 0, 'n'},
		{"posix-ignores", 0, 0, 'g'},
		{"help", 0, 0, '_'},
		{0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "hdbiopvgl:n_",
                        long_options, &option_index);
             if (c == -1)
            break;

             switch (c)
               {
               case 'b':
                 swparse_form_flag &= ~SWPARSE_FORM_ALL;
                 swparse_form_flag |= SWPARSE_FORM_INDENT;
		 break;
               case 'h':
                 swlex_definition_file = SW_PSF_INCL;
                 strncpy(typestring, "PSFi", 5);  
		 break;
               case 200:
                 swlex_definition_file = SW_INSTALLED;
                 strcpy(typestring, "INSTALLED");  
		 break;
               case 'i':
                 swlex_definition_file = SW_INDEX;
                 strncpy(typestring, "INDEX", 6);  
		 break;
               case 'g':
                 posix_ignores = 1;
                 swparse_form_flag |= SWPARSE_FORM_POLICY_POSIX_IGNORES;
		 break;
               case 'o':
                 swlex_definition_file = SW_INFO;
                 strncpy(typestring, "INFO", 5);  
                 break;
               case 'd':
                 swlex_definition_file = SW_OPTION;
                 strcpy(typestring, "OPTION");  
                 break;
               case 'p':
                 swlex_definition_file = SW_PSF;
                 strncpy (typestring, "PSF", 4);  
                 break;
	       case 'l':
		 if (!optarg) usage(progname);
		 swparse_atlevel=atoi(optarg);
                 break;
	       case 'v':
		 yydebug=1;
                 swlex_debug=1;
                 swparse_debug=1;
                 break;
	       case 'n':
                 swparse_form_flag &= ~SWPARSE_FORM_ALL;
                 swparse_form_flag |= SWPARSE_FORM_MKUP_LEN;
                 break;
               case '_':
                 usage(progname);
                 break;
               case ':':
                 usage(progname);
                 break;
               case '?':
                 usage(progname);
                 break;
               default:
                 usage(progname);
                 break;
               }
           }

  
  if (optind < argc) {
       fd = open(argv[optind], O_RDONLY );
       if (fd <  0) {
      		fprintf(stderr, "%s : %s\n", argv[optind], strerror(errno)); 
		exit(1);
       }
       swlib_strncpy(swlex_filename, argv[optind], sizeof(swlex_filename));
  } else {
       fd = STDIN_FILENO;
       swlib_strncpy(swlex_filename, "stdin", sizeof(swlex_filename));
  }
  
  swlex_inputfd = uxfio_opendup(fd, UXFIO_BUFTYPE_MEM);
  if (swlex_inputfd < 0) { exit (8); }

  /*
   * Turn buffering off.
   */
  if (uxfio_fcntl(swlex_inputfd, UXFIO_F_SET_BUFACTIVE, 0)) {
	fprintf (stderr," error in uxfio_fcntl.\n"); 
  }
 
  swparse_outputfd = uxfio_opendup(STDOUT_FILENO, 0);
  if ( swparse_outputfd < 0 ) {
     fprintf (stderr," error in uxfio_open.\n"); 
     exit (2);
  }
  
  /*
   * Turn buffering off.
   */
  if (uxfio_fcntl (swparse_outputfd, UXFIO_F_SET_BUFACTIVE, 0)) {
     fprintf (stderr," error in uxfio_fcntl.\n"); 
  }
  
  ret=sw_yyparse (swlex_inputfd, swparse_outputfd, typestring, 0, swparse_form_flag); 

  strob_close(yylval.strb);
  /* 
  uxfio_close(swlex_inputfd);
  uxfio_close (swparse_outputfd);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  */
  _exit(ret);
}


static int usage (char * progname) {
    fprintf (stderr, "A POSIX-7.2 Metadata parser (A de-facto Implementation Extension utility).\n");
    fprintf (stderr, "Copyright (C) 2005 James Lowe, Jr.\n");
    fprintf (stderr, "This software is distributed under the GNU GPLv2.\n");
    fprintf (stderr, "Usage: sw_parse {--installed|-i,--index|-o,--info|-p,--psf|-d,--option}\n");
    fprintf (stderr, "       [-level|-l level] [-with-line-lengths|-n] [-b|beautify][file]\n");
    exit (1);
}
