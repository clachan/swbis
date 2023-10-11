/*  swverid.h -- IEEE 1387.2 Version structure.
 */ 

/*
 * Copyright (C) 1998,2004  James H. Lowe, Jr.  <jhlowe@acm.org>
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

#ifndef SWVERID_122998_H
#define SWVERID_122998_H

#include <stdio.h>
#include <stdlib.h>
#include "swsdflt.h"
#include "strob.h"
#include "cplob.h"
#include "swuts.h"

#define SWVERID_OBJECT_NAME_LENGTH 42
#define SWVERID_MAX_TAGS 23

/* 
 * Name space codes
 */
#define SWVERID_NS_IVAL		-2 	/* invalid or error */
#define SWVERID_NS_NA		-1 	/* not applicable */
#define SWVERID_NS_SUPER	0	/* */
#define SWVERID_NS_TOP		1	/* product and bundles  */
#define SWVERID_NS_MID		2	/* subproducts, filesets, controlfiles */
#define SWVERID_NS_LOW		3	/* files */


/* 
 * Swverid_Cmp_Codes: Comparison result codes 		        
 */
#define SWVERID_CMP_NOT_USED	-50 /* not used 	        */
#define SWVERID_CMP_ERROR	-40 /* error 		        */
#define SWVERID_CMP_NEQ		0 /* not equal 	        	*/
#define SWVERID_CMP_LT		1 /* less than 	        	*/
#define SWVERID_CMP_LTE		2 /* less than or equal to    	*/
#define SWVERID_CMP_EQ2		3 /* identical 	        	*/
#define SWVERID_CMP_EQ 		4 /* identical 	        	*/
#define SWVERID_CMP_GTE		5 /* greater than or equal to 	*/
#define SWVERID_CMP_GT		6 /* less than 	        	*/

/*
 * Rel_op strings
 */
#define SWVERID_RELOP_NEQ	"!="  /* 0 */
#define SWVERID_RELOP_LT        "<"   /* 1 */
#define SWVERID_RELOP_LTE	"<="  /* 2 */
#define SWVERID_RELOP_EQ2	"="   /* 3 */
#define SWVERID_RELOP_EQ	"=="  /* 4 */
#define SWVERID_RELOP_GTE	">="  /* 5 */
#define SWVERID_RELOP_GT	">"   /* 6 */

/* 
 * Version Id attributes abbreviations
 */

	/* POSIX version Ids */						
#define SWVERID_VERIDS_POSIX		"ravlq"  /* List of all POSIX Version Ids */
#define SWVERID_QUALIFIER		"bpf"
#define SWVERID_VERIDS_NAME_ERROR	"0"
#define SWVERID_VERIDS_REVISION		"r"
#define SWVERID_VERIDS_ARCHITECTURE	"a"
#define SWVERID_VERIDS_VENDOR_TAG	"v"
#define SWVERID_VERIDS_LOCATION		"l"
#define SWVERID_VERIDS_QUALIFIER	"q"

	/* Implementation Extension Version Ids */
#define SWVERID_VERIDS_SWBIS 		"ig" /* List of all Impl Version Ids */
#define SWVERID_VERIDS_CATALOG_INSTANCE	"i"  /* The installed_software catalog instance_id attribute */
#define SWVERID_VERIDS_RPMFLAG		"g"

#define SWVERID_QUALIFIER_PRODUCT	"p"

#define SWVERID_VERID_TAG		0
#define SWVERID_VERID_NAME_ERROR	(*(SWVERID_VERIDS_NAME_ERROR))
#define SWVERID_VERID_REVISION		(*(SWVERID_VERIDS_REVISION))
#define SWVERID_VERID_ARCHITECTURE	(*(SWVERID_VERIDS_ARCHITECTURE))
#define SWVERID_VERID_VENDOR_TAG	(*(SWVERID_VERIDS_VENDOR_TAG))
#define SWVERID_VERID_LOCATION		(*(SWVERID_VERIDS_LOCATION))
#define SWVERID_VERID_QUALIFIER		(*(SWVERID_VERIDS_QUALIFIER))

#define SWVERID_VERID_CATALOG_INSTANCE	(*(SWVERID_VERIDS_CATALOG_INSTANCE))

typedef int Swverid_Cmp_Code;

#define SWVERID_G_taglistM(X)  (X->taglistM)

struct VER_ID {				/*  */
	char ver_idM[3];		/* version id, always one or two chars */
	char idM[2];			/* main id, always 1 char */
	char vqM[2];			/* version id qualifier, 1 char, bundle product fileset: bpf */
	char rel_opM[3];       		/* Relation operation. */
	int rel_op_codeM;		/* rel_op code (SWVERID_CMP_*) */
	char * valueM;
	struct VER_ID * nextM;		/* Pointer to next in chain. */
};

typedef struct {				/* Object Version Spec */  
	char * object_nameM;			/* object name, i.e object keyword. */ 	
	char * source_copyM;			/* copy of swverid version string */
	char * catalogM;			/* ``catalog'' attribute. */ 	
	int use_path_compareM;			/* if true, use comparison that disregards leading slashes */
	CPLOB * taglistM;			/* list of tags, Note: The first tag in the list is the */
						/* left most tag to which the version spec applies.	*/
	int namespaceM;				/* namespace code */
	Swverid_Cmp_Code comparison_codeM;	/* `this object'  \<comparision_code_\> `other object' */
	struct VER_ID * ver_id_listM;		/* linked list of version id's */
	SWUTS * swutsM;			 	/* POSIX System Id attributes */
	void * altM; /* A swverid object */	/* Next object, to support dependecy alternation */	
	int alter_uuidM;			/* unique to an alternation group */
} SWVERID;

SWVERID * 	swverid_open            	(char * object_keyword, char * swversion_string);
void      	swverid_close           	(SWVERID * swverid);
void 		swverid_set_namespace		(SWVERID * swverid, char * object_keyword);
char * 		swverid_debug_print		(SWVERID * swverid, STROB * buf);
char * 		swverid_print			(SWVERID * swverid, STROB * buf);
int		swverid_add_attribute		(SWVERID * swverid, char * object_keyword, char * keyw, char * value);
int       	swverid_make_ver_id		(SWVERID * swverid, char * attr_name, char * value, char * rel_op);
struct VER_ID*	swverid_get_ver_id		(SWVERID * swverid, char * keyword, struct VER_ID * ver_id);
char * 		swverid_get_object_name		(SWVERID * swverid);
void     	swverid_set_object_name		(SWVERID * swverid, char * name);
void     	swverid_set_tag			(SWVERID * swverid, char * keyw, char * value);
char *   	swverid_get_tag			(SWVERID * swverid, int n);
int		swverid_vtagOLD_compare	(SWVERID * swverid1, SWVERID * swverid2);
int		swverid_compare			(SWVERID * swverid1, SWVERID * swverid2);
void     	swverid_set_comparison_code	(SWVERID * swverid,  Swverid_Cmp_Code code);
int     	swverid_get_ver_id_char		(char * object_keyword, char * attr_name);
Swverid_Cmp_Code  swverid_get_comparison_sense	(SWVERID * swverid1, SWVERID * swverid2);
Swverid_Cmp_Code  swverid_get_comparison_code	(SWVERID * swverid);
void		swverid_replace_verid(SWVERID * swverid, struct VER_ID  * verid);
void		swverid_add_verid(SWVERID * swverid, struct VER_ID  * verid);
CPLOB *		swverid_u_parse_swspec(SWVERID * swverid, char * taglist);
struct VER_ID * swverid_get_verid(SWVERID * swverid, char * verid, int occno);
char *		swverid_get_verid_value(SWVERID * swverid, char * fp_verid, int occno);
char * swverid_show_object_debug(SWVERID * swverid, STROB * buf, char * prefix);
void swverid_add_tag(SWVERID * swverid, char * tag);
void swverid_ver_id_set_object_qualifier(SWVERID * swverid, char * objkeyword);
SWVERID * swverid_copy(SWVERID * src);
struct VER_ID * swverid_create_version_id(char * verid_string);
int swverid_delete_non_fully_qualified_verids(SWVERID * swverid);
int swverid_ver_id_unlink(SWVERID * swverid, struct VER_ID * verid);
SWVERID * swverid_get_alternate(SWVERID * swverid);
char * swverid_print_ver_id(struct VER_ID * next, STROB * buf);
void swverid_disconnect_alternates(SWVERID * swverid);
#endif
