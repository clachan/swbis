#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <typeinfo>

#include "xstream_config.h"

extern "C" {
#include "swlib.h"
}
#include "portablearchive.h"
#include "swmain.h"

#define LINELEN 200

int main (int argc, char ** argv)
{
	portableArchive xfmat(STDIN_FILENO, STDOUT_FILENO);
	XFORMAT * xformat;
	int size;
	int ret;

	ret = xfmat.xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCEUNIXFD);
	if (ret) {
		exit(21);
	}
	xformat = xfmat.xFormat_get_xformat();
	size = swlib_write_OLDcatalog_stream(xformat, STDOUT_FILENO);
	cerr << size;
	exit(size > 0 ? 0 : 1);
}
