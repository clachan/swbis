#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swptrlist.h"
#include "swpackagefile.h"
#include "swattributefile.h"
#include "swmain.h"

#define LINELEN 1000
int
main (int argc, char ** argv)
{
	int index;
	int fd;
  	char line[LINELEN], *t; 
	int format_code = arf_ustar;
	int datafd;
	swAttributeFile * member;
	swPackageFile * package = new swPackageFile();
	swPtrList<swAttributeFile> * archiveMemberList = new swPtrList<swAttributeFile>();

	member = new swAttributeFile("");
	member->xFormat_set_format(format_code);
	member->xFormat_set_ofd(STDOUT_FILENO);

	while (fgets(line, LINELEN - 1, stdin) != (char *)(NULL)) {
		if (strlen(line) >= LINELEN - 2) {
			fprintf (stderr, "line too long : %s\n", line);
			continue;
		}
		if ( (t=strpbrk(line,"\n\r"))) {
			*t = '\0';
		}
		if (strlen(line) == 0) continue;

		member = new swAttributeFile(line);
		if (member->xFormat_set_from_statbuf(line)) {
			fprintf(stderr, "error : %s\n", line);
			exit(2);
		}
		if (member->xFormat_get_tar_typeflag() == REGTYPE) {
			datafd = member->swfile_open_public_image_fd();
			if (datafd < 0) {
				fprintf(stderr, "internal error\n");	
				exit(3);
			}
			fd = open(line, O_RDONLY, 0);
			if (fd < 0) {
				fprintf(stderr, "internal error : %s\n", strerror(errno));	
				exit(3);
			}
			swlib_pipe_pump(datafd, fd);
			close(fd);	
		} else {
			fprintf(stderr, "This test can not handle directories.\n");
			exit(3);
			datafd = -1;
		}
		member->swfile_set_package_filename(line);
		member->swfile_re_close_public_image_fd();
		archiveMemberList->list_add(member);

		//fprintf(stderr, "%s", swpackagefile_dump_string_s(member, ""));
		
	}

	index = 0;
	member = archiveMemberList->get_pointer_from_index(index++);
	while(member) {
		member->swfile_re_open_public_image_fd();
		member->xFormat_write_file();
		member->swfile_close_public_image_fd();
		member = archiveMemberList->get_pointer_from_index(index++);
	}

	member->xFormat_write_trailer();
	delete member;
	delete package;
	exit(0);
}










