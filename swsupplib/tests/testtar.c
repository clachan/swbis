#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "usgetopt.h"
#include "xformat.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
void
usage(char * name)
{
	fprintf(stderr, "Usage: %s [options] directory\n", name);
	fprintf(stderr, "-D,--debug-dump\n");
	fprintf(stderr, "-x,--numeric-owner\n");
	fprintf(stderr, "-a,--be-like-pax\n");
	fprintf(stderr, "-O,--to-stdout\n");
	fprintf(stderr, "--help\n");

}


int 
main (int argc, char ** argv ) {
	int c;
	int ret = 0;
	struct stat st;
	SWVARFS * swvarfs;
	int arf_format = arf_ustar;
	char * path;
	char * t;
	char * opt_format;
	int to_stdout = 0;
	XFORMAT * xformat = xformat_open(-1, STDOUT_FILENO, arf_ustar);

	if (taru_set_tar_header_policy(xformat->taruM, "ustar0", &arf_format)) exit(1);

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"debug-dump", 0, 0, 'D'},
               {"numeric-owner", 0, 0, 'x'},
               {"format", 1, 0, 'H'},
               {"be-like-pax", 0, 0, 'a'},
               {"to-stdout", 0, 0, 'O'},
               {"help", 0, 0, '\005'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "axOD", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
	       case '\005':
			usage(argv[0]);
			exit(2);
		       	break;
               case 'D':
			/*D fprintf(stderr, "%s", xformat_dump_string_s(xformat, ""));  */
		 	break;
               case 'H':
			if (taru_set_tar_header_policy(xformat->taruM, optarg, &arf_format))
				exit(1);
                        break;
               case 'a':
			xformat_set_tarheader_flag(xformat, TARU_TAR_BE_LIKE_PAX, 1);
			break;
               case 'x':
			xformat_set_numeric_uids(xformat, 1);
			break;
               case 'O':
			to_stdout=1;        
			break;
               default:
			exit(1);
			break;
               }
	}

	if (optind < argc) {
		swvarfs=swvarfs_open(argv[optind], UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE, (mode_t)(NULL));
	} else {
		fprintf(stderr,"Usage: testtar dirname\n");
		exit(2);
	}
	
	xformat_set_output_format(xformat, arf_format);

	if (!swvarfs) {
		fprintf(stderr,"swvarfs null\n");
		exit(2);
	}
	path = swvarfs_get_next_dirent(swvarfs, &st);
	while (ret >= 0 && path) {
		if ( (t=strpbrk (path,"\n\r"))) {
			*t = '\0';
		}
		if ( strlen(path)) {
			ret = xformat_write_by_name(xformat, path, &st);
		}
		path = swvarfs_get_next_dirent(swvarfs, &st);
	}
	if (ret >= 0) 
		xformat_write_trailer(xformat);
	xformat_close(xformat);
	swvarfs_close(swvarfs);
	exit(0);
}

