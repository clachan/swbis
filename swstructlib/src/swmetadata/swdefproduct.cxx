//  swdefproduct.cxx
//  Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
//
//  This file may be copied under the terms of the GNU GPL.

#include "swuser_config.h"
#include "swdefproduct.h"

swDefProduct::swDefProduct(void): swDefinition() { }
swDefProduct::swDefProduct(int level): swDefinition(SW_A_product, level) { }

swDefProduct::~swDefProduct(void){ }

