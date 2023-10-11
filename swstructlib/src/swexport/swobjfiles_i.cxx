/* swobjfiles_i.cxx
 */

/*
 * Copyright (C) 1999  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#include "swobjfiles_i.h"
#include "swexhost.h"
#include "swexdistribution.h"
#include "swexproduct.h"
#include "swexfileset.h"
#include "swexvendor.h"
#include "swexbundle.h"
#include "swexcategory.h"
#include "swexmedia.h"
#include "swexsubproduct.h"
	
int swObjFiles_i::swheader_offsetM;
swDefinitionFile * swObjFiles_i::psfM;
swINDEX * swObjFiles_i::global_indexM = NULL;
SWVARFS * swObjFiles_i::swvarfsM;
STROB * swObjFiles_i::catalog_ownerM;
STROB * swObjFiles_i::catalog_groupM;

STROB * swObjFiles_i::leading_dir_ownerM;
STROB * swObjFiles_i::leading_dir_groupM;
STROB * swObjFiles_i::leading_dir_c_modeM;
STROB * swObjFiles_i::catalog_dir_c_modeM;
STROB * swObjFiles_i::catalog_file_c_modeM;
mode_t swObjFiles_i::current_dir_modeM;

swExStruct * swObjFiles_i::swExFactory(char * object_keyword)
{
	swExStruct* swex = NULL;
	if ( !::strcmp (SW_A_product, object_keyword )) { swex = swExProduct::make_exdist(); }
	else if ( !::strcmp (SW_A_fileset, object_keyword )) { swex = swExFileset::make_exdist(); }
	else if ( !::strcmp (SW_A_host, object_keyword )) { swex = swExHost::make_exdist(); }
	else if ( !::strcmp (SW_A_distribution, object_keyword )) { swex = swExDistribution::make_exdist(); }
	else if ( !::strcmp (SW_A_vendor, object_keyword )) { swex = swExVendor::make_exdist(); }
	else if ( !::strcmp (SW_A_bundle, object_keyword )) { swex = swExBundle::make_exdist(); }
	else if ( !::strcmp (SW_A_category, object_keyword )) { swex = swExCategory::make_exdist(); }
	else if ( !::strcmp (SW_A_media, object_keyword )) { swex = swExMedia::make_exdist(); }
	else if ( !::strcmp (SW_A_subproduct, object_keyword )) { swex = swExSubproduct::make_exdist(); }
	else { return static_cast<swExStruct*>(NULL); }
	return swex;
}
