/* glbindex.h -- 
 */

#ifndef GLBINDEX_20050214_H
#define GLBINDEX_20050214_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "strob.h"
#include "uxfio.h"
#include "swverid.h"
#include "strar.h"
#include "cpiohdr.h"
#include "swheader.h"
#include "swpath.h"


int glbindex_find_by_swpath_ex(SWHEADER * global_index, SWPATH_EX * swpath_ex);


#endif
