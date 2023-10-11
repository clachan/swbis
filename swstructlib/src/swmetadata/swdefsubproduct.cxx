/* swdefsubproduct.cxx
 *
 *  Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *  This file may be copied under the terms of the GNU GPL.
 */

#include "swuser_config.h"
#include "swdefsubproduct.h"

swDefsubProduct::swDefsubProduct(void): swDefinition() { }
swDefsubProduct::swDefsubProduct(int level): swDefinition("subproduct", level) { }

swDefsubProduct::~swDefsubProduct(void){ }

