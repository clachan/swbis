#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "usgetopt.h"
#include <typeinfo>
#include "stream_config.h"
#include "swparser.h"
#include "swpackagefile.h"
#include "swcatalogfile.h"
#include "swmain.h"

extern "C" {
#include "swlib.h"
}


void usage(char * argv0)
{
	fprintf(stderr, "Usage %s dirname filename\n", argv0);
	exit(2);
}


int 
main (int argc, char ** argv) {
	char * dirname, *filename;
	int fd;
	int fd2;
	swPackageFile * swcatfile;
	swPackageFile * swcatfile2;


	if (argc < 3) usage(argv[0]);

	dirname = argv[1];
	filename = argv[2];


	swcatfile = new swCatalogFile();
	swcatfile2 = new swCatalogFile();

	swcatfile->xFormat_open_archive(dirname);
	fd = swcatfile->swfile_open_public_image_fd(filename);
	fd2 = swcatfile2->swfile_open_public_image_fd();

	if (fd < 0) exit(22);
	if (fd2 < 0) exit(23);

	swlib_pipe_pump(fd2, fd);

	// fd2 = swcatfile2->swfile_open_public_image_fd();
		// or
	uxfio_lseek(fd2, (off_t)(0), SEEK_SET);

	swlib_pipe_pump(STDOUT_FILENO, fd2);

	exit(0);
}
