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
	char * name;
	char * path;
	swPackageFile * package = new swPackageFile();
	swPtrList<swPackageFile> * archive_members = new swPtrList<swPackageFile>();

	if (package->xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_FORCE_SEEK|UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE))
		exit(4);

	ifd = package->xFormat_get_ifd();
	while ((ret = package->swfile_read_archive_header(NULL)) > 0) {
		if (package->xFormat_is_end_of_archive()){
			break;
		}
		if (package->xFormat_file_has_data()) {
			package->xFormat_unread_header();     
			mfd = package->swfile_open_public_image_fd();
			if (mfd < 0) {
				fprintf(stderr, "error opening %s\n", name);
			} else {
				::swlib_pump_amount(STDOUT_FILENO, mfd, package->xFormat_get_filesize());
				package->swfile_close_public_image_fd();
			}
		}	
	}
	
	delete archive_members;
	delete package;
	exit(0);
}

