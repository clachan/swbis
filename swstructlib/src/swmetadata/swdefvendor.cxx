/* swdefvendor.cxx
 *
 *  Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *  This file may be copied under the terms of the GNU GPL.
 */

#include "swuser_config.h"
#include "swdefvendor.h"


swDefVendor::swDefVendor(void): swDefinition() { }
swDefVendor::swDefVendor(int level): swDefinition("vendor", level) { }

swDefVendor::~swDefVendor(void) { }
