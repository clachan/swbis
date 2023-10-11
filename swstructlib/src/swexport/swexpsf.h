/* swexpsf.h
 */

/*
 * Copyright (C) 2003  James H. Lowe, Jr.  <jhlowe@acm.org>
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef swexpsf_hxx
#define swexpsf_hxx

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swstruct.h"
#include "switer.h"
#include "swspsf.h"
#include "swscollection.h"

class swExPSF
{
	static swsPSF * swspsfM;
	static swIter * switerM;
	public:

	swExPSF(void) { init(NULL); }
	swExPSF(swsPSF * swspsf) { init(swspsf); }

	void swExPSFInitialize(swsPSF * swspsf) {
		swspsfM = swspsf;
		switerM = new swIter(swspsfM->get_swstruct());
	}

	virtual ~swExPSF(void){ delete switerM; }
	swIter * swExPSFGetSwiter(void) { return switerM; }
	swsPSF * swExPSFGetSwspsf(void) { return swspsfM; }

	private:
	void init(swsPSF * swspsf) {
		if(swspsf) swExPSFInitialize(swspsf);
	}	
};
#endif
