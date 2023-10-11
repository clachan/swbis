#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swpackagefile.h"
#include "swattributefile.h"
#include "swmain.h"
int
main (int argc, char ** argv)
{
	int datafd;
	swAttributeFile * member;
	swPackageFile * package = new swPackageFile();

	if (argc < 2) {
		fprintf(stderr, "usage %s pathname\n", argv[0]);
		exit(1);
	}

	member = new swAttributeFile(argv[1]);

	member->xFormat_set_ofd(STDOUT_FILENO);

	datafd = member->swfile_open_public_image_fd();

	if (datafd < 0) {
		fprintf(stderr, "internal error\n");	
		exit(3);
	}

	swlib_pipe_pump(datafd, STDIN_FILENO);

	member->xFormat_write_file();
	member->xFormat_write_trailer();

	member->swfile_close_public_image_fd();
	delete member;
	delete package;
	exit(0);
}
