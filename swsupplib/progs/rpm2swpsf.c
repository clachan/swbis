#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "um_rpmlib.h"
#include "um_header.h"
#include "usgetopt.h"
#include "ugetopt_help.h"

#include "uxfio.h"
#include "rpmpsf.h"
#include "swlib.h"
#include "shcmd.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

static TOPSF * gl_topsf;
static void sig_usr (int signo);
static void usage (char * progname);


extern YYSTYPE yylval;
int yydebug=0;
/* int swlex_definition_file = SW_PSF; */


int 
main(int argc, char **argv)
{
	int  nfd;
	TOPSF *topsf;
	int ret;
  	int c=0;
	int with_archive=0;
	char  *progname;
	progname = argv[0];

	yylval.strb = strob_open (8);
	strcpy(swlex_filename,"none");

	/* if (argc < 2) { usage(progname); exit(1); }   */
  
         while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"with-archive", 0, 0, 'a'},
               {"help", 0, 0, 132},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "a", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'a':
                 with_archive=1;        
		 break;
               case 132:
                 usage(progname);
		 fprintf(stderr,
 "%s reads a RPM and writes a PSF file (defined in Posix 7.2) to stdout.\n"
 "The -a option causes the PSF and the distribution files referenced by the\n"
 "PSF to be written as a cpio archive to stdout.\nend of help.\n", progname);
		 exit(0);
		 break;
               default:
                 usage(progname);
		 exit(1);
                 break;
               }
          }

	if (optind < argc) {
		topsf = topsf_open(argv[optind], UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, NULL);
	} else {
		topsf = topsf_open("-", UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, NULL);	/* stdin */
	}
	if (!topsf) {
		exit(1);
	}
	gl_topsf=topsf;
	signal (SIGPIPE, sig_usr); 
	/*
	topsf_set_cwd_prefix(topsf, rpmpsf_make_package_prefix(topsf, getcwd(pwdir, 500)));
	*/
	topsf_set_cwd_prefix(topsf, rpmpsf_make_package_prefix(topsf, ""));

	/* rpmpsf_write_psf must be run here to cause the file list to be generated
	   ahead of calling topsf_copypass_swacfl_list(topsf) */
	
	if (with_archive){
		nfd=open("/dev/null", O_RDWR);
		rpmpsf_write_psf(topsf, nfd);
	} else {
		/*rpmpsf_write_psf(topsf, STDOUT_FILENO); */
		rpmpsf_write_beautify_psf(topsf, STDOUT_FILENO);
		topsf_close(topsf);	
		exit(0);
	}
	
	/* now the file link lists are generated.  */
	
	ret=topsf_copypass_swacfl_list(topsf, STDOUT_FILENO);
	
	signal (SIGPIPE, SIG_IGN); 
	topsf_close(topsf);	
	exit(ret);
}

static void
sig_usr (int signo)
{

	if (signo == SIGPIPE){
		/* fprintf (stderr,"in sig han\n"); */
		topsf_close(gl_topsf);	
		exit(0);	
	}
	exit(1);	
}


static 
void  usage (char * progname) {
    fprintf (stderr, "Usage: %s [-a, --with-archive] [--help] [file]\n", progname);

}



	

