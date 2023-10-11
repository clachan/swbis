/*  swverid.c: POSIX-7.2 Object Identification Routines.
 */

/*
   Copyright (C) 1998,2004,2005,2006,2007  James H. Lowe, Jr.
   All rights reserved.
  
   COPYING TERMS AND CONDITIONS
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include "strob.h"
#include "swlib.h"
#include "swutilname.h"
#include "misc-fnmatch.h"
#include "swverid.h"

static int sg_swverid_uuid;   /* provides a uuid number for alternation groups of swverid objects */

static char * rel_op_array[] = {
		SWVERID_RELOP_NEQ,
		SWVERID_RELOP_LT ,
		SWVERID_RELOP_LTE,
		SWVERID_RELOP_EQ2,
		SWVERID_RELOP_EQ,
		SWVERID_RELOP_GTE,
		SWVERID_RELOP_GT,
		(char *)NULL
};

static
int
is_fully_qualified(SWVERID * spec)
{
	return 0;
}

static
char
get_ver_id_char_from_this(struct VER_ID * this)
{
	char * verid = this->ver_idM;
	if (strlen(verid) > 1) {
		/*
		 * Step over the object modifier letter.
		 * e.g.   'f' == fileset
		 *        'p' == product
		 *        'b' == bundle
		 *  where for example "pr" refers to the product revision
		 */
		verid++;
	}
	return *verid;
}

static
int
determine_rel_op_code(char * rel_op)
{
	char * s;
	int index;

	index = 0;
	s = rel_op_array[index];
	while(s) {	
		if (strcmp(s, rel_op) == 0) return index;
		index++;
		s = rel_op_array[index];
	}
	return -1;
}

static
int
compare_relop(int rel, int req)
{
	if (req == SWVERID_CMP_NOT_USED) {
		return rel;
	}

	if (req == SWVERID_CMP_NEQ) {

		if (rel != SWVERID_CMP_EQ) {
			return 0;
		} else {
			return 1;
		}

	} else if (req == SWVERID_CMP_EQ || req == SWVERID_CMP_EQ2) {
		if (rel != SWVERID_CMP_EQ && rel != SWVERID_CMP_EQ2) {
			return 1;
		} else {
			return 0;
		}
	} else if (req == SWVERID_CMP_LTE) {
		if (	
			rel == SWVERID_CMP_EQ ||
			rel == SWVERID_CMP_LTE ||
			rel == SWVERID_CMP_LT ||
			0
			) {
			return 0;
		} else {
			return 1;
		}
	} else if (req == SWVERID_CMP_LT) {
		if (	
			rel == SWVERID_CMP_LTE ||
			0
			) 
		{
			/*
			 * This is an error
			 */
			/* FIXME: do something? */
			return -1;
		}

		if (	
			rel == SWVERID_CMP_LT ||
			0
			) 
		{
			return 0;
		} else {
			return 1;
		}
	} else if (req == SWVERID_CMP_GT) {
		if (	
			rel == SWVERID_CMP_GTE ||
			0
			) 
		{
			/*
			 * This is an error
			 */
			/* FIXME: do something? */
			return -1;
		}

		if (	
			rel == SWVERID_CMP_GT ||
			0
			) 
		{
			return 0;
		} else {
			return 1;
		}
	} else if (req == SWVERID_CMP_GTE) {
		if (	
			rel == SWVERID_CMP_EQ ||
			rel == SWVERID_CMP_GTE ||
			rel == SWVERID_CMP_GT ||
			0
			) 
		{
			return 0;
		} else {
			return 1;
		}
	} else {
		return -1;
	}
}

static
int
swverid_i_rpmvercmp(char * target_list, char * candidate_list)
{
	int ret;
	char * target;
	char * candidate;
	
	target = target_list;
	candidate = candidate_list;
	/*
	 * FIXME, match multiple version ids.
	 */
	if (!target || !candidate) {
		return 0; /* match */
	}
	ret = swlib_rpmvercmp(target, candidate);
	return ret;
}

char *
swverid_i_print(SWVERID * swverid, STROB * buf)
{
	char * s;
	int i = 0;
	char * ret;

	STROB * version = strob_open(32);
	struct  VER_ID  * next;

	while ((s = cplob_val(swverid->taglistM, i++))) {
		if (i > 1) strob_strcat(buf, ".");
		strob_strcat(buf, s);
	}
	next = swverid->ver_id_listM;
	swverid_print_ver_id(next, version);
	if (strob_strlen(version) > 0) {
		strob_strcat(buf, ",");
		strob_strcat(buf, strob_str(version));
	}
	ret = strob_str(buf);
	strob_close(version);
	return ret;
}

static
int
swverid_i_fnmatch(char * candidate, char * target, int flags)
{
	int ret;

	if (!target || !candidate) {
		return 0; /* match */
	}
	ret = fnmatch (/*pattern*/ candidate, /*string*/ target, flags);
	return ret;
} 

static 
void
delete_version_id(struct VER_ID * verid)
{
	/* if (verid->valueM) free(verid->valueM); */
	free(verid);
}

static 
struct VER_ID *
create_version_id(char * version_id_string)
{
	struct VER_ID *ver_id=(struct VER_ID*)malloc(sizeof(struct VER_ID));
	STROB * tmp;
	char *s = version_id_string;
	char *olds;
	int count;
	int ret;

	memset(ver_id->ver_idM, '\0', sizeof(ver_id->ver_idM));
	memset(ver_id->idM, '\0', sizeof(ver_id->idM));
	memset(ver_id->vqM, '\0', sizeof(ver_id->vqM));
	memset(ver_id->rel_opM, '\0', sizeof(ver_id->rel_opM));
	ver_id->rel_op_codeM = 0;
	ver_id->valueM = NULL;
	ver_id->nextM = NULL;

	tmp = strob_open(10);
	olds = s;
	while (isalpha(*s)) s++;  /* Step over the 'r', 'a', etc */

	if ((olds - s) == 0 || (s - olds) > 2) {
		/*
		 * Error, enforce that a verid is one or two chars only
		 */
		swbis_free(ver_id);
		strob_close(tmp);
		return NULL;
	}

	/*
	 * Store the version id
	 * If two chars long, the first char is a qualifier
	 */
	if ((s - olds) == 1) {
		strncpy(ver_id->idM, olds, 1);
		if (strpbrk(ver_id->idM, SWVERID_VERIDS_POSIX SWVERID_VERIDS_SWBIS) == NULL) {
			return NULL;
		}
	} else if ((s - olds) == 2) {
		strncpy(ver_id->vqM, olds, 1);
		if (strpbrk(ver_id->vqM, SWVERID_QUALIFIER) == NULL) {
			return NULL;
		}
		strncpy(ver_id->idM, olds+1, 1);
		if (strpbrk(ver_id->idM, SWVERID_VERIDS_POSIX SWVERID_VERIDS_SWBIS) == NULL) {
			return NULL;
		}
	} else {
		fprintf(stderr, "swverid fatal error\n");
		exit(77);
	}

	strncpy(ver_id->ver_idM, olds, (int)(s - olds));

	/*
	 * Now read the rel_op
	 */
	count=0;
	olds = s;
	while (	count < 3 && (*s == '=' ||
		*s == '|' ||
		*s == '<' ||
		*s == '>')
	) {
		s++;
		count++;
	}

	/*
	 * sanity check
	 */
	if (count > 2) {
		strob_close(tmp);
		return NULL;
	}

	/*
	 * The Rel_op can only be 1 or 2 bytes long
	 */
	if ((olds - s) == 0 || (s - olds) > 2) {
		swbis_free(ver_id);
		strob_close(tmp);
		return NULL;
	}

	/*
	 * Store the rel_op
	 */
	strncpy(ver_id->rel_opM, olds, (int)(s - olds));
	ver_id->rel_opM[(int)(s - olds)] = '\0';


	/*
	 * Now check if it is a valid rel_op string
	 */
	
	ret = determine_rel_op_code(ver_id->rel_opM);
	if (ret < 0) {
		strob_close(tmp);
		return NULL;
	}
	ver_id->rel_op_codeM = ret;	

	/*
	 * Enforce the POSIX requirement that only the revision can use rel_ops
	 */
	if (*(ver_id->idM) == SWVERID_VERID_REVISION) {
		/*
		 * all rel_ops supported except '='
		 */
		if (ver_id->rel_op_codeM == SWVERID_CMP_EQ2) {
			fprintf(stderr, "%s: error: invalid use of '=' for revision specs\n",
					swlib_utilname_get());
			return NULL;
		}
	} else {
		/*
		 * the rel_op must be '='
		 */
		if (ver_id->rel_op_codeM != SWVERID_CMP_EQ2) {
			return NULL;
		}
	}

	ver_id->valueM = NULL;

	if (strlen(s)) {
		if (strcmp(ver_id->idM, SWVERID_VERIDS_LOCATION) == 0) {
			/* Alter the value for a the LOCATION id, imposing
			   that it must be an absolute path */
			swlib_squash_trailing_slash(s);
			if (*s != '/') {
				strob_strcpy(tmp, "/");
				strob_strcat(tmp, s);
				s = strdup(strob_str(tmp));
			} else {
				s = strdup(s);
			}
		} else {
			s = strdup(s);
		}
	} else {
		/* support something like  *,r==,v=tag  
		   which becomes *,r==*,v=tag
		   This covers a bug in rpmpsf.c */
		
		/* No, it is appropropriate to leave it "" */
		s = strdup(""); /* 2006-12-20  s = strdup("*"); */
	}
	SWLIB_ASSERT(s != NULL);

	ver_id->valueM = s;

	strob_close(tmp);
	return ver_id;
}

static 
struct VER_ID *
copy_construct_version_id(struct VER_ID * ver_id) {
	STROB * buf;
	struct VER_ID * new_ver_id;
	char * verid;
	char * val;

	if (ver_id == NULL) return NULL;

	buf = strob_open(12);
	strob_strcpy(buf, ver_id->vqM);
	strob_strcat(buf, ver_id->idM);
	strob_strcat(buf, ver_id->rel_opM);
	if ((val=ver_id->valueM) != NULL) {
		strob_strcat(buf, val);
	} else {
		;
	}
	
	verid = strob_str(buf);
	new_ver_id = create_version_id(verid);
	strob_close(buf);
	return new_ver_id;
}
	
static
void
ver_ids_copy(SWVERID * dst, SWVERID * src)
{
	struct VER_ID * vid;
	struct VER_ID * ver_id;
	struct VER_ID * last;
	struct VER_ID * copy_of_last;

	/* Delete the old verids in dst */
	ver_id = dst->ver_id_listM;
	while(ver_id){
		vid=ver_id->nextM;
		swbis_free(ver_id);
		ver_id=vid;
	}
	dst->ver_id_listM = NULL;

	/* Create and add new ver_ids */
	last = src->ver_id_listM;
	/*
	copy_of_last = copy_construct_version_id(last);
	dst->ver_id_listM = NULL;
	swverid_add_verid(dst, copy_of_last);
	*/
	while (last) {
		copy_of_last = copy_construct_version_id(last);
		swverid_add_verid(dst, copy_of_last);
		last = last->nextM;
	}
}

static
int
compare_version_id(struct VER_ID * id_target, struct VER_ID * id_candidate, int req)
{
	int ret = -1;
	int retval = -1;
	int flags;
	char verid_letter;

	if (!id_target || !id_candidate) {
		if(id_candidate)
			id_candidate->rel_op_codeM = SWVERID_CMP_NOT_USED;
		return 0;
	}

	verid_letter = get_ver_id_char_from_this(id_candidate);
	if (verid_letter == SWVERID_VERID_REVISION) {
		/*
		 * Its a revision, compare as dotted string.
		 */

		ret = swverid_i_rpmvercmp(id_target->valueM, id_candidate->valueM);
		/* return 1: a is newer than b */
		/*        0: a and b are the same version */
		/*       -1: b is newer than a */

		if (ret == 1) {
			id_candidate->rel_op_codeM = SWVERID_CMP_LT;	
		} else if (ret == -1) {
			id_candidate->rel_op_codeM = SWVERID_CMP_GT;	
			retval = 1;
		} else if (ret == 0) {
			id_candidate->rel_op_codeM = SWVERID_CMP_EQ;	
			retval = 0;
		} else {
			return -1;
		}
	} else {
		/*
		 * Its not a revision, compare as a shell pattern
		 */
		flags =  0;
		
		ret = swverid_i_fnmatch (/*pattern*/ id_candidate->valueM, /*string*/ id_target->valueM, flags);
		if (ret == 0) {
			id_candidate->rel_op_codeM = SWVERID_CMP_EQ;	
			retval = 0;
		} else {
			id_candidate->rel_op_codeM = SWVERID_CMP_NEQ;	
			retval = 1;
		}
	}
	retval = compare_relop(id_candidate->rel_op_codeM, req);
	return retval;
}

static
int
compare_taglist(SWVERID * pattern, SWVERID * fq_candidate)
{




	return -1;
}

static
int
compare_all_version_ids(SWVERID * target, SWVERID * candidate)
{
	struct VER_ID * id_target; 
	struct VER_ID * id_candidate; 

	/*
	 * target is a fully qualified software spec
	 * candidtate is a possible match to the target
	 */

	if (is_fully_qualified(target) == 0) {
		/*
		 * User error
		 */
		return -1;
	}

	/*
	 * Compare the revision
	 */
	id_target = swverid_get_verid(target, SWVERID_VERIDS_REVISION, 1);
	id_candidate = swverid_get_verid(candidate, SWVERID_VERIDS_REVISION, 1);
	compare_version_id(id_target, id_candidate, SWVERID_CMP_NOT_USED);

	/*
	 * Compare the architecture
	 */
	id_target = swverid_get_verid(target, SWVERID_VERIDS_ARCHITECTURE, 1);
	id_candidate = swverid_get_verid(candidate, SWVERID_VERIDS_ARCHITECTURE, 1);
	compare_version_id(id_target, id_candidate, SWVERID_CMP_NOT_USED);

	/*
	 * Compare the vendor_tag
	 */
	id_target = swverid_get_verid(target, SWVERID_VERIDS_VENDOR_TAG, 1);
	id_candidate = swverid_get_verid(candidate, SWVERID_VERIDS_VENDOR_TAG, 1);
	compare_version_id(id_target, id_candidate, SWVERID_CMP_NOT_USED);

	/*
	 * Compare the location
	 */
	id_target = swverid_get_verid(target, SWVERID_VERIDS_LOCATION, 1);
	id_candidate = swverid_get_verid(candidate, SWVERID_VERIDS_LOCATION, 1);
	compare_version_id(id_target, id_candidate, SWVERID_CMP_NOT_USED);

	/*
	 * Compare the qualifier
	 */
	id_target = swverid_get_verid(target, SWVERID_VERIDS_QUALIFIER, 1);
	id_candidate = swverid_get_verid(candidate, SWVERID_VERIDS_QUALIFIER, 1);
	compare_version_id(id_target, id_candidate, SWVERID_CMP_NOT_USED);


	return 0;
}

static 
int
parse_version_ids_string(SWVERID * swverid, char * verid_string)
{
	struct VER_ID *verid;
	STROB * buf = strob_open(10);
	char  *s;

	s = strob_strtok(buf, verid_string, ",");
	while (s) {
		verid = create_version_id(s);	
		if (verid == (struct VER_ID *)(NULL)) return -1;
		swverid_add_verid(swverid, verid);
		s = strob_strtok(buf, NULL, ",");
	}
	strob_close(buf);
	return 0;
}

static
int
parse_swspec_string (SWVERID * swverid)
{
	char * source = swverid->source_copyM;
	char * tag;
	char * tags;
	char * verids;
	char * verid;
	int ret;
	STROB * tmp = strob_open(20);
	STROB * tag_tmp2 = strob_open(20);
	STROB * ver_tmp2 = strob_open(20);

	if (!source || strlen(source)  == 0) return -2;
	cplob_shallow_reset(swverid->taglistM);

	/*
	 * parse a software spec
	 *    bash.bash-doc,r==3.3
	 */


	/*
	 * parse the list of tags
	 * which are dot separated tags
	 */
	tags = strob_strtok(tmp, source, ",");

	if (tags == NULL)
		return -1;

	tag = strob_strtok(tag_tmp2, tags, ".");

	if (tag == NULL)
		return -1;

	if (strpbrk(tag, SWVERID_RELOP_NEQ SWVERID_RELOP_LT
			SWVERID_RELOP_LTE SWVERID_RELOP_EQ2
			SWVERID_RELOP_EQ SWVERID_RELOP_GTE
				SWVERID_RELOP_GT)) {
		/* it looks like a verid, with no tags given,
		   that is tag cannot have =><! chars,
		   set 'tag' to a "*" */
		strob_strcpy(tag_tmp2, "*");
		strob_strcat(tag_tmp2, ",");
		strob_strcat(tag_tmp2, source);
		tags = strob_strtok(tmp, strob_str(tag_tmp2), ",");
		tag = strob_strtok(tag_tmp2, tags, ".");
	}

	while (tag) {
		cplob_add_nta(swverid->taglistM, strdup(tag));
		tag = strob_strtok(tag_tmp2, NULL, ".");
	}

	/*
	 * parse the version ids which are comma
	 * separated strings.
	 */

	/* FIXME??  what about escaped ,'s */
	verids = strob_strtok(tmp, NULL, ",");
	if (verids) {
		verid = verids;
		while (verid) {
			if (strcmp(verid, "*") == 0) {
				/* special case to handle ``foo,*'' */
				ret = 0;
			} else {
				ret = parse_version_ids_string(swverid, verid);
			}
			if (ret) {
				fprintf(stderr, "%s: error parsing version id [%s]\n",
					swlib_utilname_get(),  verid);
				return -1;
			}
			verid = strob_strtok(tmp, NULL, ",");
		}
	}
	strob_close(tmp);
	strob_close(tag_tmp2);
	strob_close(ver_tmp2);
	return 0;
}

static 
int
classify_namespace (char * object_kw)
{
	if (!object_kw)  return SWVERID_NS_NA;
	if (!strncmp("fileset", object_kw, 7)) {
		return SWVERID_NS_MID;
	} else if (!strncmp(SW_A_control_file, object_kw, 11)) {
		return SWVERID_NS_MID;
	} else if (!strncmp(SW_A_subproduct, object_kw, 10)) {
		return SWVERID_NS_MID;
	} else if (!strncmp(SW_A_product, object_kw, 7)) {
		return SWVERID_NS_TOP;
	} else if (!strncmp(SW_A_bundle, object_kw, 6)) {
		return SWVERID_NS_TOP;
	} else if (!strncmp(SW_A_file, object_kw, 4)) {
		return SWVERID_NS_LOW;
	} else {
		return SWVERID_NS_NA;
	}
}

void
i_replace_verid(SWVERID * swverid, struct VER_ID  * verid, int do_replace) {
	char new_id;
	struct  VER_ID  * prev;
	struct  VER_ID  * last;
	struct  VER_ID  * newverid;

	last = swverid->ver_id_listM;
	verid->nextM = NULL;
	prev = NULL;
 	new_id = get_ver_id_char_from_this(verid);
	if (!last) {
		newverid = copy_construct_version_id(verid);
		swverid_add_verid(swverid, newverid);
		return;
	}	
	while (last) {
		if (
			get_ver_id_char_from_this(last) == new_id &&
			do_replace
		) {
			newverid = copy_construct_version_id(verid);
			if (prev) {
				prev->nextM = newverid;
			}
			newverid->nextM = last->nextM;
			return;
		} else if (
			get_ver_id_char_from_this(last) == new_id &&
			do_replace == 0
		) {
			return;
		}
		prev = last;
		last = last->nextM;
	}
	newverid = copy_construct_version_id(verid);
	swverid_add_verid(swverid, newverid);
}

/* ------------------- Public Functions ---------------------------------*/

SWVERID *
swverid_copy(SWVERID * src)
{
	SWVERID * dst;
	char * tag;
	int n;

	dst = swverid_open(NULL, NULL);

	if (src->object_nameM) {
		if (dst->object_nameM) free(dst->object_nameM);
		dst->object_nameM = strdup(src->object_nameM);
	} else {
		dst->object_nameM = NULL;
	}

	if (src->source_copyM) {
		if (dst->source_copyM) free(dst->source_copyM);
		dst->source_copyM = strdup(src->source_copyM);
	} else {
		dst->source_copyM = NULL;
	}

	if (src->catalogM) {
		if (dst->catalogM) free(dst->catalogM);
		dst->catalogM = strdup(src->catalogM);
	} else {
		dst->catalogM = NULL;
	}

	dst->use_path_compareM = src->use_path_compareM;
	dst->namespaceM = src->namespaceM;
	dst->comparison_codeM = src->comparison_codeM;

	cplob_close(dst->taglistM);
	dst->taglistM = cplob_open(1);
	cplob_add_nta(dst->taglistM, (char*)(NULL));

	n = 0;
	while((tag=swverid_get_tag(src, n++)) != NULL) {
		cplob_add_nta(dst->taglistM, strdup(tag));
	}

	ver_ids_copy(dst, src);
	return dst;
}

void
swverid_add_tag(SWVERID * swverid, char * tag)
{
	cplob_add_nta(swverid->taglistM, strdup(tag));
}

void
swverid_ver_id_set_object_qualifier(SWVERID * swverid, char * objkeyword)
{
	struct  VER_ID  *last;
	last = swverid->ver_id_listM;
	while (last) {
		last->vqM[0] = *objkeyword;
		last->vqM[1] = '\0';
		last = last->nextM;
	}
}

int
swverid_ver_id_unlink(SWVERID * swverid, struct VER_ID * verid)
{
	struct  VER_ID  *prev;
	struct  VER_ID  *last;
	prev = NULL;
	last = swverid->ver_id_listM;
	while (last) {
		if (last == verid) {
			if (prev) {
				prev->nextM = last->nextM;
			} else {
				swverid->ver_id_listM = swverid->ver_id_listM->nextM;
			}
			delete_version_id(verid);
			return 0;
		}
		prev = last;
		last = last->nextM;
	}
	return -1;
}

static
SWVERID *
swverid_i_open(char * object_keyword, char *swversion_string)
{
	int ret;
	SWVERID * swverid;

	swverid = (SWVERID*)malloc(sizeof(SWVERID));
	if (!swverid) return NULL;
	
	swverid->object_nameM=swlib_strdup("");
	swverid->taglistM = cplob_open(3);
	cplob_add_nta(swverid->taglistM, (char*)(NULL));
	swverid->catalogM=swlib_strdup("");
	swverid->comparison_codeM=SWVERID_CMP_EQ;
	swverid_set_namespace(swverid, object_keyword);
	swverid_set_object_name(swverid, object_keyword);
	swverid->ver_id_listM=(struct VER_ID*)(NULL);
	swverid->source_copyM = (char*)(NULL);
	swverid->use_path_compareM = 0;
	swverid->swutsM = swuts_create();
	swverid->altM = NULL;
	swverid->alter_uuidM = 0; /* unset */
	/* swverid->dependency_statusM = SWVERID_DEPSTATUS_UNSET; */
	if (swversion_string != (char*)(NULL)) {
		swverid->source_copyM = swlib_strdup(swversion_string);
		ret = parse_swspec_string(swverid);
		if (ret < 0) {
			return NULL;
		}
	}
	return swverid;
}

SWVERID *
swverid_open(char * object_keyword, char *swversion_string)
{
	SWVERID * swverid;
	SWVERID * parent;
	SWVERID * last;
	STROB * tmp;
	int i;
	char * s;

	if (swversion_string != (char*)(NULL)) {
		i = 0;
		last = NULL;
		parent = NULL;
		tmp = strob_open(32);
		s = strob_strtok(tmp, swversion_string, "|\n\r");
		while(s) {	
			swverid = swverid_i_open(object_keyword, s);
			if (swverid == NULL) return NULL;
			if (last)
				last->altM = swverid;	
			last = swverid;
			if (i == 0)
				parent = swverid;
			s = strob_strtok(tmp, NULL, "|\n\r");
			i++;
		}
		if (parent == NULL) return NULL;
		last = parent;
		if (sg_swverid_uuid == 0)
			sg_swverid_uuid++;
		parent->alter_uuidM = sg_swverid_uuid++;
		while((last=swverid_get_alternate(last))) {
			last->alter_uuidM = parent->alter_uuidM;
		}
		strob_close(tmp);
	} else {
		parent = swverid_i_open(object_keyword, NULL);
		if (parent) parent->alter_uuidM = sg_swverid_uuid++;
	}
	return parent;
}

void
swverid_set_namespace(SWVERID * swverid, char * object_keyword)
{
	swverid->namespaceM = classify_namespace(object_keyword);
	swverid_set_object_name(swverid, object_keyword);
}

void
swverid_close(SWVERID * swverid)
{
	struct VER_ID *vid, *ver_id=swverid->ver_id_listM;
	while(ver_id){
		vid=ver_id->nextM;
		swbis_free(ver_id);
		ver_id=vid;			
	}
	if (swverid->object_nameM) {
		swbis_free(swverid->object_nameM);
		swverid->object_nameM = NULL;
	}
	if (swverid->source_copyM) {
		swbis_free(swverid->source_copyM);
		swverid->source_copyM = NULL;
	}
	/* if (swverid->altM) swverid_close(swverid->altM); */

	/* FIXME this causes a core dump: cplob_close(swverid->taglistM); */
	swuts_delete(swverid->swutsM);
	swbis_free(swverid);
}

o__inline__
void
swverid_set_object_name(SWVERID * swverid, char *name)
{
	if (swverid->object_nameM) swbis_free(swverid->object_nameM);
	if (name)
		swverid->object_nameM=swlib_strdup(name);
	else
		swverid->object_nameM=swlib_strdup("");
}

o__inline__
char *
swverid_get_object_name(SWVERID * swverid)
{
	return swverid->object_nameM;
}

int
swverid_verid_compare(SWVERID * swverid1_target, SWVERID * swverid2_candidate)
{
	int ret;
	ret = compare_all_version_ids(swverid1_target, swverid2_candidate);
	return ret;
}


/*
 * Legacy (possibly depreicated) function.
 */
int
swverid_vtagOLD_compare(SWVERID * swverid1_target, SWVERID * swverid2_candidate)
{
	char * tag1;	

	if (
		strcmp(swverid1_target->object_nameM,
			swverid2_candidate->object_nameM)
	) {
		return SWVERID_CMP_NEQ;
	}

	if (
		swverid1_target->namespaceM != SWVERID_NS_NA &&
		swverid1_target->namespaceM != swverid2_candidate->namespaceM
	) {
		return SWVERID_CMP_NEQ;
	}

	tag1 = cplob_val(swverid1_target->taglistM, 0);
	if (!tag1 || strlen(tag1) == 0) {
		 return SWVERID_CMP_EQ;
	}
	if (strcmp(tag1, "*") == 0) {
		 return SWVERID_CMP_EQ;
	}

	if (swverid1_target->use_path_compareM || swverid2_candidate->use_path_compareM) {
		if (!swlib_compare_8859(cplob_val(swverid1_target->taglistM,0),
				cplob_val(swverid2_candidate->taglistM,0))
		) {
			return SWVERID_CMP_EQ;
		} else {
			return SWVERID_CMP_NEQ;
		}
	} else { 	
		if (!strcmp(cplob_val(swverid1_target->taglistM,0), cplob_val(swverid2_candidate->taglistM,0))) {
			return SWVERID_CMP_EQ;
		} else {
			return SWVERID_CMP_NEQ;
		}
	}
}

int
swverid_compare(SWVERID * swverid1_target, SWVERID * swverid2_candidate)
{
	int ret;
	char * tag1;	

	/*
	 * Make sure we are not comparing different objects
	 */
	if (
		strcmp(swverid1_target->object_nameM,
			swverid2_candidate->object_nameM)
	) {
		/*
		 * Comparison failed
		 */
		return SWVERID_CMP_NEQ;
	}

	/*
	 * Make sure we are not comparing objects from different namespaces
	 */
	if (
		swverid1_target->namespaceM != SWVERID_NS_NA &&
		swverid1_target->namespaceM != swverid2_candidate->namespaceM
	) {
		/*
		 * Comparison failed
		 */
		return SWVERID_CMP_NEQ;
	}

	tag1 = cplob_val(swverid1_target->taglistM, 0);
	if (!tag1 || strlen(tag1) == 0) {
		/*
		 * FIXME ??
		 *  Null target returns Equal
		 */
		 return SWVERID_CMP_EQ;
	}

	if (swverid1_target->use_path_compareM && swverid2_candidate->use_path_compareM) {
		/*
		 * Here, just compare the path which is stored in taglist
		 * FIXME : this functionality probably does not belong in this
		 * object 
		 */
		if (!swlib_compare_8859(cplob_val(swverid1_target->taglistM,0),
				cplob_val(swverid2_candidate->taglistM,0))
		) {
			return SWVERID_CMP_EQ;
		} else {
			return SWVERID_CMP_NEQ;
		}
	} else if (swverid1_target->use_path_compareM == 0 && swverid2_candidate->use_path_compareM == 0) {
		/*
		if (!strcmp(cplob_val(swverid1_target->taglistM,0), cplob_val(swverid2_candidate->taglistM,0))) {
		*/

		/*
		 * Compare the tags
		 */
		compare_taglist(swverid1_target, swverid2_candidate);

		/*
		 * Compare the version ids
		 */
		ret = compare_all_version_ids(swverid1_target, swverid2_candidate);
		return ret;
	} else {
		/*
		 * Invalid usage
		 */
		return -1;
	}
}

int
swverid_add_attribute(SWVERID * swverid, char * object_keyword, char * keyword, char * value) {
	char ver_id[3];
	struct VER_ID *version_id;
	STROB * verid_string;
	int c;

	if (strcmp(object_keyword, "file") == 0 || strcmp(object_keyword, "control_file") == 0) {
		swverid->use_path_compareM = 1;
	}
	
	c = swverid_get_ver_id_char(object_keyword, keyword);
	if (c < 0) {
		/*
		 * Ok, attribute is not a POSIX versioning attribute.
		 * It might be a UTS attribute or it might not be.
		 */
		swuts_add_attribute(swverid->swutsM, keyword, value);
		return 0;
	}
	ver_id[0] = ver_id[1] = ver_id[2] = '\0';
	if (c == 0) {
		/*
		 * the object is a 'file' object, hence in this
		 * usage the pathnames are compared.
		 * FIXME: maybe a 'swverid' object should not be used for this
		 * purpose.
		 */
		if (swverid->source_copyM) {
			swbis_free(swverid->source_copyM);
			swverid->source_copyM = NULL;
		}
		swverid->source_copyM = swlib_strdup(value);
		cplob_additem(swverid->taglistM, 0, swverid->source_copyM);
		return 1;
	}

	if (0 && object_keyword) {
		/* FIXME:  Disabled as of 2005-12-03 pending further development.
		 * store the form of a version Id that specifies the
		 * object to which it belongs
		 */
		ver_id[0] = *object_keyword;
		ver_id[1] = (char)(c);
	} else {
		ver_id[0] = (char)(c);
	}

	verid_string = strob_open(24);
	strob_strcpy(verid_string, ver_id);
	if (ver_id[0] == 'r' || ver_id[1] == 'r') {
		strob_strcat(verid_string,"==");
	} else {
		strob_strcat(verid_string,"=");
	}
	strob_strcat(verid_string, value);

	/*
	 * Now parse the constructed version id
	 */
	version_id = create_version_id(strob_str(verid_string));
	if (version_id == NULL) {
		return -1;
	}

	/*
	 * Add the version id
	 */
	swverid_add_verid(swverid, version_id);

	strob_close(verid_string);
	return 1;
}

int
swverid_get_comparison_sense(SWVERID * swverid1, SWVERID * swverid2)
{
	return 0;
}

Swverid_Cmp_Code
swverid_get_comparison_code(SWVERID * swverid)
{
	return swverid->comparison_codeM;
}

void
swverid_set_comparison_code(SWVERID * swverid, Swverid_Cmp_Code code)
{
	swverid->comparison_codeM=code;
}

char *
swverid_get_tag(SWVERID * swverid, int n)
{
	return cplob_val(swverid->taglistM, n);
}

void
swverid_set_tag(SWVERID * swverid, char * key, char *value)
{
	if (value == NULL) return;
	if (!strcmp(key, "catalog")) {
		if (swverid->catalogM) { 
			swbis_free(swverid->catalogM);
			swverid->catalogM = NULL;
		}
		swverid->catalogM=swlib_strdup(value);
	} else {
		if (cplob_val(swverid->taglistM,0)) { 
			swbis_free(cplob_val(swverid->taglistM,0));
		}
		cplob_additem(swverid->taglistM, 0, swlib_strdup(value));
	}
}

int 
swverid_get_ver_id_char(char * object, char * attr_name)
{
	if (!strcmp(attr_name, SW_A_revision)) {
		return SWVERID_VERID_REVISION;
	}
	else if (!strcmp(attr_name, SW_A_architecture)) {
		return SWVERID_VERID_ARCHITECTURE;
	}
	else if (!strcmp(attr_name, SW_A_vendor_tag)) {
		return SWVERID_VERID_VENDOR_TAG;
	}
	else if (!strcmp(attr_name, SW_A_location)) {
		return SWVERID_VERID_LOCATION;
	}
	else if (!strcmp(attr_name, SW_A_qualifier)) {
		return SWVERID_VERID_QUALIFIER;
	}
	else if (!strcmp(attr_name, SW_A_tag)) {
		return 0;
	}
	else if (!strcmp(attr_name, SW_A_path)) {
		if (!strcmp(object, "file") || !strcmp(object, SW_A_distribution))
			return 0;
		else 
			return -1;
	}
	else {
		return -1;
	}
}

struct VER_ID *
swverid_get_verid(SWVERID * swverid, char * fp_verid, int occno) {
	char * verid;
	char * qualifier;
	int occ_count;
	struct  VER_ID  *last;

	verid = fp_verid;
	occ_count = 0;
	/*
	 * Step over the object modifier letter
	 * e.g. 'br' 'pr' etc which is an suggested
	 * extension 
	 */
	if (strlen(verid) > 2) {
		fprintf(stderr, "invalid version id: %s\n", fp_verid);
		return NULL;
	}

	if (strlen(verid) > 1)
		verid++;
	
	if (strlen(verid) > 1)
		qualifier = verid;
	else
		qualifier = "";
 
	last = swverid->ver_id_listM;
	while (last) {
		if ( *(last->idM) == *verid) {
			if (strlen(qualifier) > 0) {
				if (strlen(last->vqM) > 0) {
					if (*(last->vqM) == *qualifier) {
						if (occ_count >= occno - 1) {
							return last;
						}
						occ_count++;
					}
				} else {
					/* FIXME, what if *(last->vqM) exists? */
					if (occ_count >= occno - 1) {
						return last;
					}
					occ_count++;
				}
			} else {
				/*
				 * Found it
				 */
				if (occ_count >= occno - 1) {
					return last;
				}
				occ_count++;
			}
		}
		last = last->nextM;
	}
	return NULL;
}

char *
swverid_print_ver_id(struct VER_ID * next, STROB * buf)
{
	int vi;
	char * val;
	int n = 0;

	strob_strcpy(buf, "");
	while (next) {
		if (n++) strob_strcat(buf, ",");
		strob_strcat(buf, next->vqM);
		strob_strcat(buf, next->idM);
		strob_strcat(buf, next->rel_opM);
		vi = 0;
		if ((val=next->valueM) != NULL) {
			strob_strcat(buf, val);
		} else {
			;
		}
		next = next->nextM; 
	}
	return strob_str(buf);
}

void
swverid_add_verid(SWVERID * swverid, struct VER_ID  * verid) {
	struct  VER_ID  * prev;
	struct  VER_ID  * last;

	last = swverid->ver_id_listM;
	if (verid)
		verid->nextM = NULL;

	if (!last) {
		swverid->ver_id_listM = verid;
		return;
	}	

	while (last) {
		prev = last;
		last = last->nextM;
	}
	prev->nextM = verid;
}

void
swverid_add_verid_if(SWVERID * swverid, struct VER_ID  * verid)
{
	/* add if not already present */
	i_replace_verid(swverid, verid, 0 /* do_replace */);
}

void
swverid_replace_verid(SWVERID * swverid, struct VER_ID  * verid) {
	/* add or replace */
	i_replace_verid(swverid, verid, 1 /* do_replace */);
}

SWVERID *
swverid_get_alternate(SWVERID * swverid) {
	return swverid->altM;
}

void
swverid_disconnect_alternates(SWVERID * swverid) {
	SWVERID * oldparent;
	SWVERID * parent;
	parent = swverid;
	while (parent) {
		oldparent = parent;
		parent = parent->altM;
		oldparent->altM = NULL;	
	}
}

char *
swverid_debug_print(SWVERID * swverid, STROB * buf)
{
	STROB  * tmp;

	tmp = strob_open(32);
	swverid_print(swverid, tmp);
	strob_strcpy(buf, "");
	strob_sprintf(buf, 0, "%d: %s", swverid->alter_uuidM, strob_str(tmp));
	strob_close(tmp);
	return strob_str(buf);
}

char *
swverid_print(SWVERID * swverid, STROB * fp_buf)
{
	char * ret;
	STROB * buf;
	static STROB * nb;
	SWVERID * next;

	if (fp_buf) {
		buf = fp_buf;
	} else {
		if (nb == NULL) {
			nb = strob_open(32);
		}
		buf = nb;		
	}

	strob_strcpy(buf, "");
	swverid_i_print(swverid, buf);
	next = swverid;	
	while((next=swverid_get_alternate(next))) {
		strob_strcat(buf, "|");
		swverid_i_print(next, buf);
	}
	ret = strob_str(buf);
	return ret;
}

char *
swverid_show_object_debug(SWVERID * swverid, STROB * fp_buf, char * prefix)
{
	SWVERID * xx;
	static STROB * buf;
	STROB * buf1;
	char * tag;
	int in;

	if (fp_buf == NULL) {
		if (buf == NULL)
			buf = strob_open(32);
	} else {
		buf = fp_buf;
	}

	xx = swverid;
	buf1 = strob_open(10);

	strob_strcpy(buf, "");
	swverid_print(swverid, buf1);

	strob_sprintf(buf, 1, "%s%p (SWVERID*)\n", prefix,  (void*)xx);
	strob_sprintf(buf, 1, "%s%p swverid_print() = [%s]\n", prefix, (void*)xx, swverid_print(swverid, buf1));
	strob_sprintf(buf, 1, "%s%p->object_nameM = [%s]\n", prefix, (void*)xx,  xx->object_nameM);
	strob_sprintf(buf, 1, "%s%p->source_copyM = [%s]\n", prefix, (void*)xx,  xx->source_copyM);
	strob_sprintf(buf, 1, "%s%p->catalogM = [%s]\n", prefix, (void*)xx,  xx->catalogM);
	strob_sprintf(buf, 1, "%s%p->use_path_compareM = [%d]\n", prefix, (void*)xx,  xx->use_path_compareM);
	strob_sprintf(buf, 1, "%s%p->comparison_codeM = [%d]\n", prefix, (void*)xx,  xx->comparison_codeM);
	strob_sprintf(buf, 1, "%s%p->ver_id_listM = (struct VER_ID*)%p\n", prefix, (void*)xx,  (void*)(xx->ver_id_listM));
	strob_sprintf(buf, 1, "%s%p->ver_id_listM = [%s]\n", prefix, (void*)xx,  swverid_print_ver_id(xx->ver_id_listM, buf1));

	in = 0;
	strob_strcpy(buf1, "");
	while((tag=cplob_val(swverid->taglistM, in++)) != NULL) {
		if (in>1)
			strob_sprintf(buf1, 1, ".", tag);
		strob_sprintf(buf1, 1, "%s", tag);
	}
	strob_sprintf(buf, 1, "%s%p->taglistM = (CPLOB*)(%p)\n", prefix, (void*)xx,  (void*)(xx->taglistM));
	strob_sprintf(buf, 1, "%s%p->taglistM = [%s]\n", prefix, (void*)xx,  strob_str(buf1));
	strob_sprintf(buf, 1, "%s%p->namespaceM = [%d]\n", prefix, (void*)xx, xx->namespaceM);

	strob_sprintf(buf, STROB_DO_APPEND,
			"os_name=%s\n"
			"os_version=%s\n"
			"os_release=%s\n"
			"machine_type=%s\n",
			swverid->swutsM->sysnameM,
			swverid->swutsM->versionM,
			swverid->swutsM->releaseM,
			swverid->swutsM->machineM);

	return strob_str(buf);
}

CPLOB *
swverid_u_parse_swspec(SWVERID * swverid, char * swspec)
{
	CPLOB * list;
	CPLOB * savecplob;
	char * savesource;

	savecplob = swverid->taglistM;
	list = cplob_open(3);
	swverid->taglistM = list;
	savesource = swverid->source_copyM;
	swverid->source_copyM = strdup(swspec);
		
	parse_swspec_string(swverid);

	free(swverid->source_copyM);		
	swverid->taglistM = savecplob;
	swverid->source_copyM = savesource;
	return list;
}

struct VER_ID *
swverid_create_version_id(char * verid_string) {
	struct VER_ID * ver_id;
	ver_id = create_version_id(verid_string);
	return ver_id;
}

char *
swverid_get_verid_value(SWVERID * swverid, char * fp_verid, int occno) {
	char * ret;
	struct VER_ID * verid; 
	verid = swverid_get_verid(swverid, fp_verid, occno);
	if (!verid) return (char*)NULL; /* ""; */
	ret = verid->valueM;
	return ret;
}

int
swverid_delete_non_fully_qualified_verids(SWVERID * swverid)
{
	int retval;
	struct  VER_ID  *tmp;
	struct  VER_ID  *last;

	retval = 0;
	last = swverid->ver_id_listM;
	while (last) {
		if (swlib_is_sh_tainted_string(last->valueM)) {
			tmp = last;
			last = last->nextM;
			swverid_ver_id_unlink(swverid, tmp);
			retval++;
		} else {
			last = last->nextM;
		}
	}
	return retval;
}
