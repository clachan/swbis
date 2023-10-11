/* swsdflt.h: software structure defaults of the IEEE 1387.2 Std.
 */

/* 
 *  Copyright (C) 1997, 1998, 1999  James H. Lowe, Jr.
 *  This file may be copied under the terms of the GNU GPL.
 */

#ifndef swsdflt_19981213jhl_h
#define swsdflt_19981213jhl_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

   enum obj_type 	   	{sdf_unknown, sdf_object_kw, sdf_attribute_kw, sdf_extended_kw }; 
   enum swsdflt_sanction   	{sdf_POSIX_7_2, sdf_swbis, sdf_this_implementation, sdf_dpkg, sdf_rpm, XDSA_C701, sdf_POSIX_7_2_annex};
   enum swsdflt_group      	{sdf_not_found, sdf_id, sdf_attr, sdf_objs, CONTROLFILE_EXT, FILE_EXT, SWDEFFILE};
   enum swsdflt_value_type 	{sdf_single_value, sdf_list};
   enum swsdflt_has_default_value   {sdf_no=0, sdf_yes=1, sdf_IMDEF=2 /*ImplementationDefined*/ };
   enum swsdflt_length              {sdf_undefined};
   enum swsdflt_object_keyword 	{sdf_host, sdf_distribution, sdf_product, sdf_subproduct, sdf_fileset,
				sdf_bundle, sdf_vendor, sdf_category, sdf_media, sdf_file, sdf_control_file,
				sdf_PSF, sdf_PSFi, sdf_INFO, sdf_INDEX, sdf_END, sdf_OPTION,
				sdf_installed_software,  sdf_INSTALLED, sdf_leftmost, sdf_all, sdf_default, sdf_NULL}; 

/*
* char  PortableCharacterString[] = "portable character string";
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

   enum swsdflt_permitted_value {
		sdf_PortableCharacterString,
		sdf_FilenameCharacterString,
		sdf_ListofDependency_specs,
		sdf_PathnameCharacterString,
		sdf_SoftwarePatternMatchingString,
		sdf_ListofSoftware_specs,
		sdf_IntegerCharacterString,
		sdf_StateList,
		sdf_OctalCharacterString,
		sdf_ControlScript,
		sdf_not_set};

#define RPMTAG_ 0
   
   struct swsdflt_defaults
          { 
	    unsigned char object_keywordM;
	    unsigned char sanction;
	    unsigned char group;
	    char * keyword;
	    short int length;
	    unsigned char value_type;
	    unsigned char permitted_valueM; /* not used */
	    unsigned char has_default_value;
            char * value;
          };

   struct swsdflt_defaults*  swsdflt_defaults_array(void);
   int                swsdflt_is_sw_keyword    (char* object_keyword, char* keyword);
   int                swsdflt_get_attr_group   (char * object_keyword, char * keyword);
   struct swsdflt_defaults*  swsdflt_return_entry     (char * object, char * keyword);
   int                swsdflt_return_entry_group   (int index);
   char *             swsdflt_return_entry_keyword (char * buf, int entry_index);
   int                swsdflt_return_entry_index (char * object,  char * keyword);
   char *	swsdflt_get_object_keyword 	(struct swsdflt_defaults * swd);
   int		swsdflt_get_sanction		(struct swsdflt_defaults * swd);
   int		swsdflt_get_group		(struct swsdflt_defaults * swd);
   char *	swsdflt_get_keyword		(struct swsdflt_defaults * swd);
   int		swsdflt_get_value_type 		(struct swsdflt_defaults * swd);
   /* char *	swsdflt_get_permitted_value 	(struct swsdflt_defaults * swd); */
   int 		swsdflt_get_has_default_value 	(struct swsdflt_defaults * swd);
   char *	swsdflt_get_default_value_by_ent  (struct swsdflt_defaults * swd);
   char *       swsdflt_get_default_value        (char * object, char * keyword);
#endif
