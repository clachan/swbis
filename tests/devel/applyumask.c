


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "swmain.h"


int
main(int argc, char ** argv)
{
	int aa[4];
	mode_t mode;
	mode_t umask;


/*
	umask = 0;
	sscanf(argv[1], "%o", &umask);
	sscanf(argv[2], "%o", &mode);


	fprintf(stdout, "\numask = %04o\n", umask);
	fprintf(stdout, "mode = %04o\n", mode);
	fprintf(stdout, "mode = %04o\n", 0752);
	fprintf(stdout, "new mode = %04o\n",  0777 & ~umask);
*/

	mode = (0000 | S_IRWXU | (S_IRWXG & (S_IXGRP|S_IRGRP)) | (S_IRWXO & (S_IXOTH|S_IROTH)));
	fprintf(stdout, "mode = %o\n", mode);


}


