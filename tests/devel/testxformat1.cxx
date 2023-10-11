// testxformat1.cxx

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "portablearchive.h"
#include "swmain.h"

#define LINELEN 200

int main (int argc, char ** argv)
{
	int fd;
	STROB * namebuf = strob_open(100);
	char * name;
	char * archive_name;
	portableArchive xfmat;

	fprintf(stderr, "this test is obsolete  %s \n", argv[0]);
	exit(2);

	if (argc > 1) {
		archive_name = argv[1];
	} else {
		archive_name = "-";
	}
	if (xfmat.xFormat_open_archive(archive_name) < 0) exit(2);
	do {
		//fd = xfmat.xFormat_u_open_file();
		if (fd > 0) {
			fprintf(stdout,"%s\n", xfmat.xFormat_get_name(namebuf));	
			name = strob_str(namebuf);
			if (!strcmp(xfmat.xFormat_get_name(namebuf),"/usr/lib/libguile.so")) {
				fprintf(stdout,"AAAAAAAAAA\n");	
			}
			xfmat.xFormat_u_close_file(fd);
		}
		if (fd < 0) {
			fprintf(stderr,"error from xFormat_open_file(): %s\n", xfmat.xFormat_get_name(namebuf));
			exit(2);
		}
	} while (fd > 0);
	exit (0);
}
