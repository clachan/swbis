#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swlib.h"
#include "swvarfs.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


int 
main (int argc, char ** argv ) {
	char * name, dash[3];
	char * filename;
	char ** uargs = argv;
	struct stat st;
	SWVARFS * swvarfs;
	int fd, count;
	int bufflag;
	int oflags;

	strncpy(dash,"-", 2);
	if (argc == 2) {
		name=dash;	
	} else if (argc == 3) {
		name=argv[2];	
	} else {
		fprintf(stderr,"arg error.\n");
		exit(2);
	}

	if (getenv("XMEM")) {
		bufflag = UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
	} else {
		bufflag = UINFILE_UXFIO_BUFTYPE_FILE;
	}
	oflags = UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCEUXFIOFD;
	oflags |= bufflag;

	swvarfs=swvarfs_open(name, oflags, (mode_t)(NULL));
	if (!swvarfs)
		exit(2);
	filename=*(++uargs);
	
	while (*uargs) {
		if ((fd=swvarfs_u_open(swvarfs, *uargs)) < 0) {
			fprintf(stderr,"%s not found.\n", *uargs);
		} else {
			swvarfs_u_fstat(swvarfs, fd, &st);
			/* swlib_pump_amount(STDOUT_FILENO, fd, st.st_size); */
			swlib_pump_amount(STDOUT_FILENO, fd, -1);
			count = st.st_size;	
			/*	
			while (count) {
				ret = uxfio_read(fd, buf, 512);
				if (ret < 0) {
					fprintf(stderr,"read error\n");
				}
				write(STDOUT_FILENO, buf, ret);	
				count-=ret;
			}
			*/	
			swvarfs_u_close(swvarfs, fd);
		}
		uargs++;
	}
        swvarfs_u_open(swvarfs, " Test.test.test.this_is only a test __124334_3453-will_never_match"); 
	swvarfs_close(swvarfs);
	exit(0);
}



