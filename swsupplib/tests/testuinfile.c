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
	int fd;

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'O':
               	  	to_stdout=1;        
		 	break;
               default:
		 exit(1);
                 break;
               }
          }

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

	/*
	* Since only swvarfs_u_open_current(swvarfs) is being called,
	* seeking ( potentially on a pipe) is not needed hence we turn all
	* buffering off in the next line.
	*/

	/* swvarfs_set_uxfio_buffer_type(swvarfs, UXFIO_BUFTYPE_NOBUF);
	*/

	fd = swvarfs_fd(swvarfs);

	swlib_pipe_pump(STDOUT_FILENO, fd);
	
	swvarfs_close(swvarfs);
	exit(0);
}

