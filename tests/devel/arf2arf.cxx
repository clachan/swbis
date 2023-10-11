static char Id_list2tar_c[] =
"arf2arf.cxx 1.1.1.1";

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
#include "swpackagefile.h"
#include "swmain.h"

#define LINELEN 3000


int
usage(char * name)
{

	fprintf(stderr, "Usage %s [-H format] [-D]\n", name);
	fprintf(stderr, "             format=ustar,newc,crc,odc\n");
	return 0;
}


int
main (int argc, char *argv[])
{
	int c;
	int debugmode = 0;
	int format=arf_ustar;
	XFORMAT * xxformat;
	char * name, * source;
	char * s;
	int ret;
	int ifd;
	int flags;
	swPackageFile * package = new swPackageFile();

         while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"format", 1, 0, 'H'},
               {"numeric-owner", 0, 0, 'x'},
               {"debug-mode", 0, 0, 'D'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "xDH:",
                        long_options, &option_index);
             if (c == -1) break;

             switch (c)
               {
		case 'x':
			package->xFormat_set_numeric_uids(1);
			break;
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
		default:
			usage(argv[0]);
			exit (2);
		break;
               }
	}



	flags = UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_MEM;
	if (package->xFormat_open_archive(STDIN_FILENO, flags))
		exit(4);

	package->xFormat_set_output_format((int)format);
	package->xFormat_set_false_inodes(XFORMAT_OFF);

	ifd = package->xFormat_get_ifd();
	while ((ret = package->swfile_read_archive_header(NULL)) > 0) {
		//D if (debugmode) fprintf(stderr, "%s", swpackagefile_dump_string_s(package, ""));
		if (package->xFormat_is_end_of_archive()){
			break;
		}
		package->xFormat_write_header();
		package->xFormat_copy_pass(STDOUT_FILENO, ifd);
	}
	
	if (ret >= 0) {
		package->xFormat_write_trailer();
		if (package->xFormat_get_pass_fd()) {
			package->xFormat_clear_pass_buffer();
		}
	}

	delete package;
	exit(0);
}


