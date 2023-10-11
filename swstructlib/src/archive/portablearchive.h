#ifndef portablearchive_19980703_h
#define portablearchive_19980703_h

// 
//  Copyright (C) 1999  James H. Lowe, Jr.  <jhlowe@acm.org>
//  This file may be copied under the terms of the GNU GPL.
//

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <tar.h>
#include <sys/types.h>
#include <fcntl.h>
#include <typeinfo>
#include "uxformat.h"

class portableArchive: public uxFormat
{
  public:

	portableArchive(int ifd, int ofd):  uxFormat(ifd, ofd) {
		return;
	}

	portableArchive(int ofd): uxFormat(STDIN_FILENO, ofd) {
		return;
	}

	portableArchive(char * archivename, int flags): uxFormat() {
		xFormat_open_archive(archivename, flags, (mode_t)0);
	}

	portableArchive(void): uxFormat() { }
	virtual ~portableArchive(void) { }
};
#endif
