#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "usgetopt.h"
#include "portablearchive.h"
#include "swmain.h"

extern "C" {
#include "swlib.h"
}

static char *
getnextpath(FILE * fp)
{
	char * ret;
	static char buf[1024];
	char * s;

	ret = fgets(buf, sizeof(buf), fp);
	if (!ret) return ret;
	buf[sizeof(buf) - 1] = '\0';
	if ((s=strrchr(buf, '\n'))) *s = '\0';
	if ((s=strrchr(buf, '\r'))) *s = '\0';
	return ret;
}


int 
main (int argc, char ** argv) {
	int c;
	struct stat st;
	portableArchive xfmat;
	char * path;
	int to_stdout = 0;
	int data_dump = 0;
	int stat_dump = 0;
	int devnullfd;
	int fd;
	int opt_verbose = 0;
	int opt_testarchive = 0;
	char * filelistname = NULL;
	int ret;
	FILE * flp = NULL;

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"to-stdout", 0, 0, 'O'},
               {"stat-dump", 0, 0, 'd'},
               {"data-dump", 0, 0, 'l'},
               {"verbose", 0, 0, 'v'},
               {"file-list", 1, 0, 'f'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "Odlvf:x", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'l':
	       		data_dump = 1;
			break;
               case 'd':
	       		stat_dump = 1;
			break;
               case 'v':
	       		opt_verbose = 1;
			break;
               case 'x':
	       		opt_testarchive = 1;
			break;
               case 'O':
               	  	to_stdout=1;        
		 	break;
               case 'f':
			filelistname = strdup(optarg);
			flp = fopen(filelistname, "r");
			if (!flp) {
				fprintf(stderr, "file %s not found\n", filelistname);
				exit(3);
			}
		 	break;
               default:
		 exit(1);
                 break;
               }
	}


	if (!flp && !opt_testarchive) {
		flp = fopen("/dev/tty", "r");
		if (!flp) {
			fprintf(stderr, "/dev/tty open error\n");
			exit(3);
		}
	}
	devnullfd = open("/dev/null", O_RDWR, 0);

	if (optind < argc) {
		ret = xfmat.xFormat_open_archive(argv[optind], UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCE_SEEK, (mode_t)(NULL));
		//ret = xfmat.xFormat_open_archive(argv[optind], UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, (mode_t)(NULL));
	} else {
		ret = xfmat.xFormat_open_archive("-", UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCEUXFIOFD, (mode_t)(NULL));
		//ret = xfmat.xFormat_open_archive("-", UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM, (mode_t)(NULL));
	}

	if (ret) {
		fprintf(stderr, "error opening archive\n");
		exit(2);
	}

	if (flp)
		path = getnextpath(flp);
	else
		path = xfmat.xFormat_get_next_dirent(&st);

	while (path) {
		if (data_dump) {	
			fd = xfmat.xFormat_u_open_file(path);
			swlib_pipe_pump(STDOUT_FILENO, fd);
			xfmat.xFormat_u_close_file(fd);
		} else {
			if (xfmat.xFormat_u_lstat(path, &st))
				fprintf(stderr, "_lstat failed on %s\n", path);
			else
				swlib_write_stats(NULL, NULL, &st, 0, "+", STDOUT_FILENO, NULL);	
		}
		if (flp)
			path = getnextpath(flp);
		else
			path = xfmat.xFormat_get_next_dirent(&st);
	}
	if (flp) fclose(flp);
	exit(0);
}
