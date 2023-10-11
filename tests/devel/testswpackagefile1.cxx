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
	swPackageFile * package = new swPackageFile();
	swPtrList<swPackageFile> * archive_members = new swPtrList<swPackageFile>();

	if (package->xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE))
		exit(4);

	ifd = package->xFormat_get_ifd();
	while ((ret = package->swfile_read_archive_header(NULL)) > 0) {
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

	delete archive_members;
	delete package;
	exit(0);
}


