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
	int ret;
	int ifd;
	int nullfd = open("/dev/null", O_RDWR, 0);
	swPackageFile * package = new swPackageFile();
	

	if (package->xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_FORCEUXFIOFD)) {
		fprintf(stderr, "error opening pacakge\n");
		exit(4);
	}
	if (nullfd < 0) exit(5);
	ifd = package->xFormat_get_ifd();
	package->xFormat_set_ofd(nullfd);


	
	while ((ret = package->swfile_read_archive_header(NULL)) > 0) {
		if (package->xFormat_is_end_of_archive()){
			break;
		}
		package->xFormat_write_header();
		package->xFormat_copy_pass(nullfd, ifd);
		//D package->xFormat_debug_dump(stdout);
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


