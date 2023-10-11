#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "swptrlist.h"
#include "swpackagefile.h"
#include "swmain.h"

int
main (int argc, char ** argv)
{
	int mfd;
	int ret;
	int ifd;
	int devnull_fd = open("/dev/null", O_WRONLY, 0);
	char * name;
	swPackageFile * package = new swPackageFile();
	unsigned long line_label;
	unsigned char digest[40];

	if (argc < 2) {
		//
		// Open portable archive on stdin.
		//
		if (package->xFormat_open_archive(STDIN_FILENO, 
			UINFILE_UXFIO_BUFTYPE_MEM|UINFILE_DETECT_FORCE_SEEK|UINFILE_DETECT_NATIVE
							)) exit(4);
	} else {
		//
		// Open directory.
		//
		fprintf(stderr, "this won't work with the usage in this test program, see usage of *get_next_dirent\n");
		exit(2);
		if (package->xFormat_open_archive(argv[1]))
			exit(5);
	}

	ifd = package->xFormat_get_ifd();

	while ((ret = package->swfile_read_archive_header(NULL)) > 0) {

		if (package->xFormat_is_end_of_archive()){
			break;
		}

		// line_label = ahs_debug_dump(package->xFormat_hdr(), stdout);
		line_label = 0;
		if (package->xFormat_file_has_data()) {
			//
			// seek backwards because thats what is
			// required by swfile_open_public_image_fd.
			//
			package->xFormat_unread_header();     

			mfd = package->swfile_open_public_image_fd();
			//uxfio_debug_dump(mfd);
			if (mfd < 0) {
				fprintf(stderr, "error opening %s\n", name);
			} else {
				uxfio_fcntl(mfd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
				uxfio_fcntl(mfd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
				::swlib_pump_amount(devnull_fd, mfd, package->xFormat_get_filesize());
				/*
				if (uxfio_lseek(mfd, 0, UXFIO_SEEK_VSET) < 0) {
					fprintf(stderr, "testswpackagefile3: error in uxfio_lseek\n");
				}
				*/
				swlib_md5(mfd, digest, 1 /* do_ascii */);
				fprintf(stdout, "%05lu file data md5 = %s\n", line_label, digest);
				package->swfile_close_public_image_fd();
			}
		}	
	}

	close(devnull_fd);
	package->xFormat_close_archive();
	delete package;
	exit(0);
}
