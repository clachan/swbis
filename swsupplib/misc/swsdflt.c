/* swsdflt.c: Software Structure Defaults of IEEE 1387.2 Std.
 */

/*
   Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
   All rights reserved.
  
   COPYING TERMS AND CONDITIONS
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



#include "swuser_config.h"
#include "swsdflt.h"

/*
* char  sdf_PortableCharacterString[] = "portable character string";
* char  FilenameCharacterString[] = "filename character string";
* char  ListofDependency_specs[] = "list of dependency_specs";
* char  PathnameCharacterString[] = "pathname character string";
* char  SoftwarePatternMatchingString[] = "software pattern matching string";
* char  ListofSoftware_specs[] = "list of software_specs";
* char  IntegerCharacterString[] = "integer character string";
* char  StateList[] = "configured,installed,corrupt,removed,available,transient";
* char  OctalCharacterString[] = "octal character string";
* char  ControlScript[] = "program text";
*/
   
static char * swsdflts_objects[] = {"host", "distribution", "product", "subproduct", "fileset",
				"bundle", "vendor", "category", "media", "file", "control_file",
				"PSF", "PSFi", "INFO", "INDEX", "_END", "OPTION",
				"installed_software", "INSTALLED", "_leftmost", "_all", "_default", (char*)NULL};
  
  static struct swsdflt_defaults  swsdflt_arr[]= {

/* // OBJECT KEYWORD	Sanction    Attr.Group   Keyword		Length		Value-Type	Permitted Values           Has_a_default    Default Value */
/* //-------                ------      -------     ------      	 	------         ------------	-----------                   ------          ---------- */

{sdf_default,	sdf_swbis,		sdf_id, 	"*", 			sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_objs,	"leftmost", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_id,		"revision", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_id,		"architecture", 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_id,		"vendor_tag", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_id,		"location", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_leftmost,	sdf_this_implementation,sdf_id,		"qualifier", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_PSF,		sdf_swbis,		SWDEFFILE,	"PSF",		 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_PSFi,	sdf_swbis,		SWDEFFILE,	"PSFi",	 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_INFO,	sdf_swbis,		SWDEFFILE,	"INFO",		 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_INDEX,	sdf_swbis,		SWDEFFILE,	"INDEX",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_INSTALLED,	sdf_swbis,		SWDEFFILE,	"INSTALLED",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_OPTION,	sdf_swbis,		SWDEFFILE,	"OPTION",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2,		sdf_objs, 	"host", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_id,		"hostname", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_attr, 	"os_name", 		32,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_attr, 	"os_release", 		32,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_attr, 	"os_version", 		32,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_attr, 	"machine_type", 	32,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_objs, 	"distribution", 	sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes, 	""},
{sdf_host,	sdf_POSIX_7_2_annex,	sdf_objs, 	"installed_software", 	sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs, 	sdf_yes, 	""},

{sdf_distribution,sdf_POSIX_7_2, 	sdf_objs, 	"distribution",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2, 	sdf_id, 	"path", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2, 	sdf_attr, 	"dfiles", 		64,		sdf_single_value,	sdf_FilenameCharacterString, 	sdf_yes, 	"dfiles"},
{sdf_distribution,sdf_POSIX_7_2, 	sdf_attr, 	"layout_version", 	64,		sdf_single_value,	sdf_not_set, 			sdf_yes, 	"1.0"},
{sdf_distribution,sdf_POSIX_7_2, 	sdf_attr, 	"pfiles", 		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes, 	"pfiles"},
{sdf_distribution,sdf_POSIX_7_2, 	sdf_attr, 	"uuid", 		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"tag",	 		64,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"title",	 	256,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"description",	 	64,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"revision",	 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"media_type",	 	64,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"copyright",	 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"create_time",	 	16,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"number",	 	64,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,sdf_POSIX_7_2_annex,	sdf_attr, 	"architecture",	 	64,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"owner", 		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"group", 		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"control_directory", 	sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"release",	 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"url",	 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"chksum_md5",	 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"signature",	 	sdf_undefined,	sdf_list,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,	sdf_swbis,	sdf_attr,	"rpm_default_prefix",	sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"/"},
{sdf_distribution,	sdf_swbis,	sdf_attr,	"rpm_obsoletes",	sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_distribution,        sdf_swbis,	sdf_attr, 	"rpm_buildarchs", 	sdf_undefined,	sdf_list,		sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_distribution,	sdf_swbis,	sdf_attr, 	"rpmversion", 		32,		sdf_single_value,	sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_distribution,	sdf_POSIX_7_2, 	sdf_objs, 	"media", 		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes, 	""},
{sdf_distribution,	sdf_POSIX_7_2, 	sdf_objs, 	"bundle", 		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes, 	""},
{sdf_distribution,	sdf_POSIX_7_2, 	sdf_objs, 	"product", 		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes, 	""},
{sdf_distribution,	sdf_POSIX_7_2,  SWDEFFILE,	"INDEX", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},
{sdf_distribution,	sdf_POSIX_7_2,  SWDEFFILE,	"INFO", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},

{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_objs, 	"installed_software", 	sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_id, 	"path", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_id, 	"catalog", 		sdf_undefined, 	sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_attr, 	"dfiles", 		64,		sdf_single_value,	sdf_FilenameCharacterString, 	sdf_yes, 	"dfiles"},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_attr, 	"layout_version", 	64,		sdf_single_value,	sdf_not_set, 			sdf_yes, 	"1.0"},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_attr, 	"pfiles", 		64,		sdf_single_value, 	sdf_FilenameCharacterString,	sdf_yes, 	"pfiles"},
{sdf_installed_software,	sdf_swbis,	sdf_attr, 	"install_time", 	32, 		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_installed_software,	sdf_POSIX_7_2, 	sdf_objs, 	"bundle", 		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes, 	""},
{sdf_installed_software,	sdf_POSIX_7_2,  SWDEFFILE,	"INDEX", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},
{sdf_installed_software,	sdf_POSIX_7_2,  SWDEFFILE,	"INFO", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},

{sdf_media,		sdf_POSIX_7_2,	sdf_objs,	"media",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	"1"},
{sdf_media,		sdf_POSIX_7_2,	sdf_id,		"sequence_number",	64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	"1"},

{sdf_vendor,		sdf_POSIX_7_2,	sdf_objs,	"vendor",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_vendor,		sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_vendor,		sdf_POSIX_7_2,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_vendor,		sdf_POSIX_7_2,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_vendor,		sdf_swbis,	sdf_attr,	"qualifier",		sdf_undefined,	sdf_single_value,	sdf_not_set,			sdf_no,""},

{sdf_category,		XDSA_C701,	sdf_objs,	"category",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_category,		XDSA_C701,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_no,	""},
{sdf_category,		XDSA_C701,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_category,		XDSA_C701,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_category,		XDSA_C701,	sdf_attr,	"revision",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},

{sdf_product,		sdf_POSIX_7_2,	sdf_objs,	"product",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"vendor_tag",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"architecture",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"location",		sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,"<product.directory>"},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"qualifier",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_id,		"revision",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"all_filesets",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"control_directory",	sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"<product.tag>"},
{sdf_product,		sdf_swbis,	sdf_attr,	"copyrighters",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_swbis,	sdf_attr,	"build_root",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"/"},
{sdf_product,		sdf_swbis,	sdf_attr,	"build_host",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"copyright",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"create_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"directory",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"/"},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"instance_id",		16,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"1"},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"is_locatable",		8,		sdf_single_value,	sdf_not_set,		sdf_yes,	"true"},
{sdf_product,		sdf_POSIX_7_2, 	sdf_attr, 	"layout_version", 	64,		sdf_single_value,	sdf_not_set, 			sdf_yes, 	"1.0"},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"postkernel",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_IMDEF,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"machine_type",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"mod_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,		sdf_no,	NULL},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"number",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"os_name",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"os_release",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"os_version",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"size",			32,		sdf_single_value,	sdf_IntegerCharacterString,		sdf_no,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_swbis,	sdf_attr,	"source_package",	sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_swbis,	sdf_attr,	"sourcerpm",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,		sdf_swbis,	sdf_attr,	"all_patches",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_product,    		sdf_swbis,	sdf_attr, 	"url",		 	sdf_undefined,	sdf_list,		sdf_PortableCharacterString, 	sdf_yes, 	""},
{sdf_product,		sdf_swbis,	sdf_attr,	"rpm_provides",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_no,	""},
{sdf_product,		sdf_swbis,	sdf_attr, 	"change_log",	 	sdf_undefined,	sdf_list,		sdf_PortableCharacterString, 	sdf_no, 	NULL},
{sdf_product,		XDSA_C701,	sdf_attr,	"is_patch",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"false"},
{sdf_product,		XDSA_C701,	sdf_attr,	"category_tag",		sdf_undefined,	sdf_list,		sdf_not_set,		sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_objs,	"control_file",		sdf_undefined,	sdf_list,		sdf_not_set,		sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_objs,	"subproduct",		sdf_undefined,	sdf_list,		sdf_not_set,		sdf_yes,	""},
{sdf_product,		sdf_POSIX_7_2,	sdf_objs,	"fileset",		sdf_undefined,	sdf_list,		sdf_not_set,		sdf_yes,	""},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"checkinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"preinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"postinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"verify",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"fix",			sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"checkremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"preremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"postremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"configure",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"unconfigure",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"request",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"unpreinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"unpostinstall",	sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"space",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		CONTROLFILE_EXT,"control_file",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,	sdf_no,	NULL},
{sdf_product,	sdf_POSIX_7_2,		 SWDEFFILE,	"INFO", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},

{sdf_bundle,	sdf_POSIX_7_2,	sdf_objs,	"bundle",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"vendor_tag",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"architecture",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"location",		sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"<bundle.directory>"},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"qualifier",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_id,		"revision",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"contents",		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"copyright",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"create_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,		NULL},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"directory",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"instance_id",		16,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"1"},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"is_locatable",		8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"true"},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"machine_type",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"mod_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"number",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"os_name",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"os_release",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"os_version",		64,		sdf_single_value,	sdf_SoftwarePatternMatchingString,	sdf_yes,	""},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"size",			32,		sdf_single_value,	sdf_IntegerCharacterString,		sdf_no,	NULL},
{sdf_bundle,	sdf_POSIX_7_2,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_bundle,	XDSA_C701,	sdf_attr,	"is_patch",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"false"},
{sdf_bundle,	XDSA_C701,	sdf_attr,	"category_tag",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},

{sdf_fileset,	sdf_POSIX_7_2,	sdf_objs,	"fileset",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"vendor_tag",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"create_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"mod_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"size",			32,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"control_directory",	sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"<fileset.tag>"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"corequisites",		sdf_undefined,	sdf_list,		sdf_ListofDependency_specs,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"exrequisites",		sdf_undefined,	sdf_list,		sdf_ListofDependency_specs,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"is_kernel",		8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"false"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"is_locatable",		8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"true"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"is_reboot",		8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"false"},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"is_sparse",		sdf_undefined,	sdf_single_value,	sdf_not_set,			sdf_yes,	"false"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"location",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"<product.directory>"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"media_sequence_number",sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	"1"},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"prerequisites",	sdf_undefined,	sdf_list,		sdf_ListofDependency_specs,	sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"revision",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_attr,	"state",		16,		sdf_single_value,	sdf_StateList,			sdf_no,	NULL},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"rpm_flag",		sdf_undefined,	sdf_list,		sdf_IntegerCharacterString,	sdf_yes,	"0"},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"exclusive_arch",	sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"exclude_arch",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"exclusive_os",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_swbis,	sdf_attr,	"exclude_os",		sdf_undefined,	sdf_list,		sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_objs,	"control_file",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},
{sdf_fileset,	sdf_POSIX_7_2,	sdf_objs,	"file",			sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"checkinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"preinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"postinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"verify",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"fix",			sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"checkremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"preremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"postremove",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"configure",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"unconfigure",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"request",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"unpreinstall",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"unpostinstall",	sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"space",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	CONTROLFILE_EXT,"control_file",		sdf_undefined,	sdf_single_value,	sdf_ControlScript,		sdf_no,	NULL},
{sdf_fileset,sdf_POSIX_7_2,	 SWDEFFILE,	"INFO", 		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no, 	NULL},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"is_patch",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"false"},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"category_tag",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"ancestor",		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes,	"<product.tag>"},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"applied_patches",	sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_yes,	""},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"patch_state",		16,		sdf_list,		sdf_not_set,			sdf_no,	""},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"saved_files_directory",sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	""},
{sdf_fileset,	XDSA_C701,	sdf_attr,	"supersedes",		sdf_undefined,	sdf_list,		sdf_ListofSoftware_specs,	sdf_no,	""},

{sdf_subproduct,	sdf_POSIX_7_2,	sdf_objs,	"subproduct",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"create_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"description",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"mod_time",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"size",			32,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"title",		256,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_subproduct,	sdf_POSIX_7_2,	sdf_attr,	"contents",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},
{sdf_subproduct,	XDSA_C701,	sdf_attr,	"is_patch",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	"false"},
{sdf_subproduct,	XDSA_C701,	sdf_attr,	"category_tag",		sdf_undefined,	sdf_list,		sdf_not_set,			sdf_yes,	""},

{sdf_file,	sdf_POSIX_7_2,	sdf_objs,	"file",			sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_id,		"path",			sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"chksum",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_swbis,	sdf_attr,	"chksum_md5",		32,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"compressed_chksum",	16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"compressed_size",	16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"compression_state",	16,		sdf_single_value,	sdf_not_set,			sdf_yes,	"uncompressed"},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"compression_type",	64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"revision",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"size",			16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"source",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"gid",			16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	"sdf_undefined"},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"group",		sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"is_volatile",		8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"false"},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"link_source",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_swbis,	sdf_attr,	"rdev",			16,		sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"major",		16,		sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"minor",		16,		sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"mode",			16,		sdf_single_value,	sdf_OctalCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"mtime",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"owner",		sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"type",			8,		sdf_single_value,	sdf_not_set,			sdf_yes,	"f"},
{sdf_file,	sdf_POSIX_7_2,	sdf_attr,	"uid",			16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	sdf_swbis,	sdf_attr,	"rpm_fileflags",	16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_file,	XDSA_C701,	sdf_attr,	"archive_path",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_yes,	""},

{sdf_control_file,sdf_POSIX_7_2,	sdf_objs,	"control_file",		64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_control_file,sdf_POSIX_7_2,	sdf_id,		"tag",			64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"chksum",		16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_swbis,	sdf_attr,	"chksum_md5",		32,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"compressed_chksum",	16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"compressed_size",	16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"compression_state",	16,		sdf_single_value,	sdf_not_set,			sdf_yes,	"uncompressed"},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"compression_type",	64,		sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	""},
{sdf_control_file,sdf_swbis,	sdf_attr,	"contents",		sdf_undefined,	sdf_single_value,	sdf_PortableCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"revision",		64,		sdf_single_value,	sdf_PortableCharacterString,	sdf_yes,	""},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"size",			16,		sdf_single_value,	sdf_IntegerCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"source",		sdf_undefined,	sdf_single_value,	sdf_PathnameCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"interpreter",		sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_yes,	"sh"},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"path",			sdf_undefined,	sdf_single_value,	sdf_FilenameCharacterString,	sdf_no,	NULL},
{sdf_control_file,sdf_POSIX_7_2,	sdf_attr,	"result",		sdf_undefined,	sdf_single_value,	sdf_not_set,			sdf_yes,	"none"},
{sdf_END, sdf_this_implementation,sdf_attr,	"_END",			sdf_undefined,	sdf_single_value,  	sdf_PathnameCharacterString,	sdf_no,   NULL},
{sdf_NULL,sdf_this_implementation,	sdf_attr,	(char*)(NULL),sdf_undefined,	sdf_single_value,  	sdf_PathnameCharacterString,	sdf_no,    ""}
     };

struct swsdflt_defaults* swsdflt_defaults_array(void) {
  return swsdflt_arr;
}


int swsdflt_get_attr_group (char * object_keyword, char * keyword) {
   struct swsdflt_defaults * list;
   if ((list=swsdflt_return_entry((char*)(object_keyword), keyword)) != NULL) {
      return (int)(list->group);
   } else { 
      return sdf_not_found; /* // unrecognized attribute */
   }
}

int swsdflt_is_sw_keyword(char * object, char * keyword ) {
   if ( swsdflt_return_entry(object, keyword) != NULL) {
      return 1;
   } else {
      return 0;
   }
}

int swsdflt_return_entry_group(int entry_index) {
   int last=swsdflt_return_entry_index ("_END", "_END");
   struct swsdflt_defaults * list = swsdflt_defaults_array() + entry_index;
   if (last <= entry_index) {
     return -1;
   }
   return (int) list->group; 
}

struct swsdflt_defaults* swsdflt_return_entry (char * object, char * keyword ) {
   int r1, r2;
   struct swsdflt_defaults * list = swsdflt_defaults_array();
   while (list->object_keywordM != sdf_NULL) {
	r1 = (object) ?  strcmp(object, swsdflts_objects[list->object_keywordM]) : 0;
	r2 = (keyword) ? strcmp(keyword, list->keyword) : 0;
	if (!r1 && !r2) {
		/* fprintf(stderr,"%s = %s  %s = %s\n", object, swsdflts_objects[list->object_keywordM], keyword, list->keyword); */ 
		return list;
	}
	list++;
   }
   return NULL;
}

int swsdflt_return_entry_index (char * object, char * keyword) {
   struct swsdflt_defaults* list = swsdflt_defaults_array(), *ent;
   ent = swsdflt_return_entry(object, keyword); 
   if (!ent) ent = list;
   return ent - list;
}

char * swsdflt_return_entry_keyword (char * buf, int entry_index) {
   int last=swsdflt_return_entry_index ("_END", "_END");
   struct swsdflt_defaults * list = swsdflt_defaults_array() + entry_index;
   
   if (last <= entry_index) {
     return (char*)(NULL);
   }
   if (buf) { 
      strcpy (buf, list->keyword);
      return buf;
   } else {
      return list->keyword; 
   }
}

/* 
 * char * 
 * swsdflt_get_object_keyword(struct swsdflt_defaults * swd) {
 * 	return swd->object_keyword;
 * }

 * o__inline__
 * int 
 * swsdflt_get_sanction(struct swsdflt_defaults * swd) {
 * 	return swd->sanction;
 * }

 * o__inline__
 * int 
 * swsdflt_get_group(struct swsdflt_defaults * swd) {
 * 	return swd->group;
 * }
*/

o__inline__
char * 
swsdflt_get_keyword(struct swsdflt_defaults * swd) {
	if (!swd) return (char*)NULL;
	return swd->keyword;
}

o__inline__
int 
swsdflt_get_value_type(struct swsdflt_defaults * swd) {
	if (!swd) return -1;
	return (int)(swd->value_type);
}

o__inline__
int 
swsdflt_get_has_default_value(struct swsdflt_defaults * swd){
	if (!swd) return -1;
	return swd->has_default_value;
}

o__inline__
char * 
swsdflt_get_default_value_by_ent(struct swsdflt_defaults * swd){
	if (!swd) return (char*)NULL;
	return swd->value;
}

char *
swsdflt_get_default_value(char * object, char * keyword)
{
   struct swsdflt_defaults *ent;
   char * s;
   if ((s=strchr(object, '.')) != NULL) {
      *s = '\0';
      ent = swsdflt_return_entry(object, s+1); 
      *s = '.';
   } else {
      ent = swsdflt_return_entry(object, keyword); 
   }
   if (!ent) return NULL;
   return ent->value;	
}
