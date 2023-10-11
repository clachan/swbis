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
	time_t tm;
	struct utimbuf ub;
	unsigned long create_time;
	int ret;

	if (argc < 3) {
		fprintf(stderr, "Usage : mtimetouch calendartime path\n");
		exit(1);
	}

	sscanf(argv[1], "%lu", &create_time);
	ub.actime = (time_t)create_time;
	ub.modtime = (time_t)create_time;
	ret = utime(argv[2], &ub);
	if (ret == 0) exit(0);
	exit(1);
}
