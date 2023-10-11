/* swexstruct_i.cxx
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr. <jhlowe@acm.org>
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


#include "swexstruct_i.h"

swExStruct * swExStruct_i::lastM = static_cast<swExStruct *>(NULL);
int swExStruct_i::errorCodeM = 0;
int swExStruct_i::outputfdM = STDOUT_FILENO;
int swExStruct_i::previewfdM = STDERR_FILENO;
int swExStruct_i::nullfdM = 0;
swPackageFile * swExStruct_i::archiverM;	
int swExStruct_i::generationFlagsM;
swPackageDir * swExStruct_i::dirM = NULL;
STROB * swExStruct_i::leadingPathM;
time_t swExStruct_i::current_timeM;
time_t swExStruct_i::dir_mtimeM;
char * swExStruct_i::sigFileM;
int swExStruct_i::filesFdM;
int swExStruct_i::verboseM;
int swExStruct_i::store_regtype_onlyM;
int swExStruct_i::allowAbsolutePathsM;
int swExStruct_i::cksumflagsM;
int swExStruct_i::allowMissingSourceFileM;
STRAR * swExStruct_i::errorMessagesM;
struct extendedOptions * swExStruct_i::optaM;
