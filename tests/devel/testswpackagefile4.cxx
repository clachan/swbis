#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "usgetopt.h"

#include "swptrlist.h"
#include "swpackagefile.h"
#include "uxformat.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
	int mfd;
	int debugmode = 0;
	int devnull_fd = open("/dev/null", O_WRONLY, 0);
	char * path;
	swPackageFile * package;
	unsigned long line_label;
	unsigned char digest[40];
	struct stat st;
	int c;

	package = new swPackageFile();

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"debug-mode", 0, 0, 'D'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "D", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'D':
	       		debugmode = 1;
			break;
               default:
		 exit(1);
                 break;
               }
	}

	if (optind >= argc) {
		//
		// Open portable archive on stdin.
		//
		if (package->xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_FORCE_SEEK|UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE))
			exit(4);
	} else {
		//
		// Open directory in file system.
		//
		if (package->xFormat_open_archive(argv[optind]))
			exit(5);
	}

	// ifd = package->xFormat_get_ifd();

	path = package->xFormat_get_next_dirent(&st);
	while(path) {
		//D if (debugmode) {
		//D 	fprintf(stderr, "%s", swpackagefile_dump_string_s(package, ""));
		//D }
		mfd = package->swfile_open_public_image_fd(path);
		
		//D line_label = ahs_debug_dump(package->xFormat_hdr(), stdout);
		line_label = 0;
		if (mfd < 0) {
			fprintf(stderr, "error opening %s\n", path);
		} else {
			if (package->xFormat_file_has_data()) {
				if (mfd < 0) {
					fprintf(stderr, "error opening %s\n", path);
				} else {
					swlib_md5(mfd, digest, 1);
					fprintf(stdout, "%05lu file data md5 = %s\n", line_label, digest);
				}
			}
		}
		fprintf(stdout, "\n");
		/* fprintf(stdout, "%s\n", path); */
		package->swfile_close_public_image_fd();
		path = package->xFormat_get_next_dirent(&st);
	}

	close(devnull_fd);
	delete package;
	exit(0);
}
