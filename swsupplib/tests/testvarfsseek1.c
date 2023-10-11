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
	unsigned long line_label;
	char digest[100];
	char * filelistname = NULL;
	FILE * flp = NULL;

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"to-stdout", 0, 0, 'O'},
               {"stat-dump", 0, 0, 'd'},
               {"filelist", 1, 0, 'f'},
               {"verbose", 0, 0, 'v'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "Odvf:", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'd':
	       		stat_dump = 1;
			break;
               case 'v':
	       		opt_verbose = 1;
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

	devnullfd = open("/dev/null", O_RDWR, 0);
	if (optind < argc) {
		/*
		* open a directory or portable archive file.
		*/
		swvarfs=swvarfs_open(argv[optind], UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE, (mode_t)(NULL));
	} else {
		/*
		* Open an portable archive on stdin.
		*/
		swvarfs=swvarfs_open("-", UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE, (mode_t)(NULL));
	}

	if (!swvarfs) {
		fprintf(stderr, "swvarfs_open() failed.\n");
		exit(2);
	}

	if (opt_verbose) {
		swvarfs->format_descM->verboseM = 1;
	}


	if (flp)
		path = getnextpath(flp);
	else
		path = swvarfs_get_next_dirent(swvarfs, &st);

	while (path) {
		/*
		* line_label = ahs_debug_dump(swvarfs->ahsM, stdout);	
		*/	
		/* fprintf(stderr, "DEBUG path=%s\n", path); */
		fd = swvarfs_u_open(swvarfs, path);
		if (fd < 0) {
			fprintf(stderr, "swvarfs_u_open failed on %s\n", path);
		} else {
			if (swvarfs_file_has_data(swvarfs)) {
				swlib_pipe_pump(-1, fd);
			}
		
			/* ahs_debug_dump(swvarfs->ahsM, stdout);	
			*/
			swvarfs_u_usr_stat(swvarfs, fd, STDOUT_FILENO);		
			/* fprintf(stderr, "DEBUG LOOP before u_close offset=%d\n",  (int)uxfio_lseek(swvarfs->fdM, 0 , SEEK_CUR));
			*/
			swvarfs_u_close(swvarfs, fd);
		}

		/* fprintf(stderr, "DEBUG LOOP after u_close offset=%d\n",  (int)uxfio_lseek(swvarfs->fdM, 0 , SEEK_CUR));
		*/
		if (flp)
			path = getnextpath(flp);
		else
			path = swvarfs_get_next_dirent(swvarfs, &st);
	}

	if (flp) fclose(flp);
	swvarfs_close(swvarfs);
	exit(0);
}

