// uxformatcpio.h
// 
//  Copyright (C) 2000 James H. Lowe, Jr.  <jhlowe@acm.org>
//  This file may be copied under the terms of the GNU GPL.

#ifndef uxformatcpio_200007_h
#define uxformatcpio_200007_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "xformat.h"
#include "uxformat.h"
class uxFormatCpio: public uxFormat
{
  public:
	uxFormatCpio(int ofd): uxFormat(STDIN_FILENO, ofd, CPIO_POSIX_FILEFORMAT){}
	uxFormatCpio(int ifd, int ofd): uxFormat(ifd, ofd, CPIO_POSIX_FILEFORMAT){}
	~uxFormatCpio(void){}
};
#endif
