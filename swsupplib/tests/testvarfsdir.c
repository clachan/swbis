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
	int fd;
	int oflags;
	int bufferflag = UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"to-stdout", 0, 0, 'O'},
               {"stat-dump", 0, 0, 'd'},
               {"verbose", 0, 0, 'v'},
               {"buffer-type", 1, 0, 'B'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "B:Odv", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'd':
	       		stat_dump = 1;
			break;
	       case 'B':
		 if (strstr(optarg, "mem")) {
			bufferflag = UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		 } else if (strstr(optarg, "file")) {
			bufferflag = UINFILE_UXFIO_BUFTYPE_FILE;
		 }
                 break;
               case 'v':
	       		opt_verbose = 1;
			break;
               case 'O':
               	  	to_stdout=1;        
		 	break;
               default:
		 exit(1);
                 break;
               }
          }

	/* old */ oflags=UINFILE_UXFIO_BUFTYPE_FILE|UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE;
	oflags=UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM|UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE;
	oflags=UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE;
	oflags |= bufferflag;

	devnullfd = open("/dev/null", O_RDWR, 0);
	if (optind < argc) {
		/*
		* open a directory or portable archive file.
		*/
		swvarfs=swvarfs_open(argv[optind], oflags, (mode_t)(NULL));
	} else {
		/*
		* Open an portable archive on stdin.
		*/
		swvarfs=swvarfs_open("-", oflags, (mode_t)(NULL));
	}

	if (!swvarfs) {
		fprintf(stderr, "swvarfs_open() failed.\n");
		exit(2);
	}


	/*
	swvarfs_debug_list_files(swvarfs, 2);
	exit(2);
	*/

	if (opt_verbose) {
		swvarfs->format_descM->verboseM = 1;
	}


	/*swvarfs_uxfio_fcntl(swvarfs, UXFIO_F_ARM_AUTO_DISABLE, 1);
	*/
	if (stat_dump) {
		/*
		fd = swvarfs_fd(swvarfs);
		swlib_pipe_pump(STDOUT_FILENO, fd);
		*/
		unsigned long line_label;
		struct stat st;
		char digest[100];
		char sha1[100];
		path = swvarfs_get_next_dirent(swvarfs, &st);
		while (path) {
			/*D line_label = ahs_debug_dump(swvarfs->ahsM, stdout);	*/
			line_label = 0;
			fd = swvarfs_u_open(swvarfs, path);	
			if (fd < 0) {
				fprintf(stderr, "swvarfs_u_open returned error on %s\n", path);
				exit(3);
			} else {
				if (swvarfs_file_has_data(swvarfs)) {
					swlib_md5(fd, digest, 1); 
					fprintf(stdout, "%05lu file data  md5 = %s\n", line_label, digest);
				}
				swvarfs_u_close(swvarfs, fd);
			}
			path = swvarfs_get_next_dirent(swvarfs, &st);
		}
	} else if (to_stdout) {
		struct stat st;
		path = swvarfs_get_next_dirent(swvarfs, &st);
		while (path) {
			if (opt_verbose) fprintf(stderr, "path=[%s]\n", path);
			fd = swvarfs_u_open(swvarfs, path);	
			if (fd > 0) {
				if (swvarfs_file_has_data(swvarfs)) {
					swlib_pipe_pump(STDOUT_FILENO, fd);
				}
				swvarfs_u_close(swvarfs, fd);
			} else {
				fprintf(stderr, "swvarfs_u_open returned error on %s\n", path);
			}
			path = swvarfs_get_next_dirent(swvarfs, &st);
		}
	} else {
		if (devnullfd < 0) exit(22);
		path = swvarfs_get_next_dirent(swvarfs, &st);
		while (path) {
			fprintf(stdout, "%s\n", path);
			fd = swvarfs_u_open(swvarfs, path);	
			if (fd < 0) {
				fprintf(stderr, "swvarfs_u_open returned error on %s\n", path);
			} else {
				if (swvarfs_file_has_data(swvarfs)) {
					if (swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM) {
						/*
						* sink the data to /dev/null
						*/
						if (swvarfs_file_has_data(swvarfs)) {
							swlib_pipe_pump(-1, fd);
						}
					}
				}
				swvarfs_u_close(swvarfs, fd);
			}
			path = swvarfs_get_next_dirent(swvarfs, &st);
		}
		close(devnullfd);
	}
	if (swvarfs->derrM) fprintf(stderr,"error code is %d\n", swvarfs->derrM);

	swvarfs_close(swvarfs);
	exit(0);
}

