#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

int
main (int argc, char *argv[])
{
	fprintf(stdout, "%lu\n", (unsigned long)time(NULL));
	exit(0);
}
