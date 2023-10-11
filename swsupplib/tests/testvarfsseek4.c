#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swlib.h"
#include "swvarfs.h"
#include "usgetopt.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int 
main (int argc, char ** argv ) {
	int c;
	struct stat st;
	SWVARFS * swvarfs;
	char * path;
	int to_stdout = 0;
	int stat_dump = 0;
	int devnullfd;
	int opt_verbose = 0;
	int cfd;
	int fd;
	int flags;

	if (argc < 2) exit(2);

	if (atoi(argv[1])) {
		cfd = swlib_open_memfd();
		swlib_pipe_pump(cfd, STDIN_FILENO);
		uxfio_fcntl(cfd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
		uxfio_lseek(cfd, (off_t)0, SEEK_SET);
		flags = UINFILE_DETECT_NATIVE;
		flags = UINFILE_DETECT_FORCEUXFIOFD |
			UINFILE_DETECT_NATIVE |
			UINFILE_DETECT_OTARALLOW |
			UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		swvarfs = swvarfs_opendup(cfd, flags, (mode_t)0);
	} else {
		flags = UINFILE_DETECT_FORCEUXFIOFD |
			UINFILE_DETECT_NATIVE |
			UINFILE_DETECT_OTARALLOW |
			UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		swvarfs = swvarfs_opendup(STDIN_FILENO, flags, (mode_t)0);
	}

	if (!swvarfs) {
		fprintf(stderr, "swvarfs_open() failed.\n");
		exit(2);
	}

	path = swvarfs_get_next_dirent(swvarfs, &st);
	while (path && strlen(path)) {
		fd = swvarfs_u_open(swvarfs, path);
		if (fd < 0) {
			fprintf(stderr, "swvarfs_u_open failed on %s\n", path);
		} else {
			if (swvarfs_file_has_data(swvarfs)) {
				swlib_pipe_pump(-1, fd);
			}
			swvarfs_u_usr_stat(swvarfs, fd, STDOUT_FILENO);		
			swvarfs_u_close(swvarfs, fd);
		}

		path = swvarfs_get_next_dirent(swvarfs, &st);
	}
	swvarfs_close(swvarfs);
	exit(0);
}

