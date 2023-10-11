/* swheader.c  --  Routines for searching the sw_parser output.
 */

/*
   Copyright (C) 1998-2004  James H. Lowe, Jr.  <jhlowe@acm.org>
   All Rights Reserved.
  
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <tar.h>

#include "taru.h"
#include "strob.h"
#include "cplob.h"
#include "uxfio.h"
#include "swlib.h"
#include "uinfile.h"
#include "swheader.h"
#include "swheaderline.h"
#include "uinfile.h"
#include "cpiohdr.h"
#include "swverid.h"
#include "swutilname.h"
#include "swparse.h"

#include "debug_config.h"
#ifdef SWHEADERNEEDDEBUG
#define SWHEADER_E_DEBUG(format) SWBISERROR("SWHEADER DEBUG: ", format)
#define SWHEADER_E_DEBUG2(format, arg) SWBISERROR2("SWHEADER DEBUG: ", format, arg)
#define SWHEADER_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWHEADER DEBUG: ", format, arg, arg1)
#else
#define SWHEADER_E_DEBUG(arg)
#define SWHEADER_E_DEBUG2(arg, arg1)
#define SWHEADER_E_DEBUG3(arg, arg1, arg2)
#endif /* SWHEADERNEEDDEBUG */


#define NEWLINE_LEN 1

static char * swheader_goto_c_next_line (void * vheader, 
				int * output_line_offset, int peek_only);
static void swheader__reset(SWHEADER * swheader);


static char *
swheader_goto_c_next_line (void * vheader, int * output_line_offset,
					int peek_only)
{
	SWHEADER * swheader;
	char * ih;
	char *keyw;
	char *p;
	char * line;
	int n;
	int type;
	
	/* Interface:
	==============
	output_line_offset *output_line_offset	Action
	------------------  ------------------     --------
	NULL		   not applicable	reset	
	not NULL	   INT_MAX		return swheader->image_head_;	
	not NULL		<0	return C address of line at offset:
							-(*output_line_offset)
	*/

	swheader = (SWHEADER*)vheader;
	ih = swheader->image_head_;

	if (!output_line_offset){ 
		swheader__reset(swheader);
		return (char*)(NULL);
	}
	
	if (*output_line_offset == INT_MAX){
		return ih;
	}
        
	if (*output_line_offset < 0) {
		return ih + (-(*output_line_offset));	
	}	
	
	/* Return next line
	 */
	
	
	/* if type 'F' (i.e File name reference), then
	    set value to next line and return.  */

	type = swheaderline_get_type (ih+(*output_line_offset));
	if (type == 'F') {
		p=strchr(ih+(*output_line_offset), '\n')+NEWLINE_LEN;
		if (p) {
			*output_line_offset=p-swheader->image_head_;
		}
		return p;	
	}


	/* Must be a attribute or object keyword. */

	keyw = swheaderline_get_keyword(ih + (*output_line_offset));
	if (keyw == NULL) {
		return NULL;
	}

	n = swheaderline_get_value_length(ih + (*output_line_offset));
	if (n < 0) {
		return NULL;
	}
        if (type == 'A') n++; /* add the space between the value and keyword*/

	/* detecting the last line requires that there be a NULL 
	   after the end of the last line */

	/* assumes newline is 1 char long */

	line = keyw + (strlen(keyw) + n + NEWLINE_LEN);

	/* next char will be a '0' or '-' or NUL.
	   if NUL then we're at the last line.
	  '0' and '-' ar possible first chars of a header line.  */

	if (*line == '\0' && peek_only == 0) {
		*output_line_offset=-1;
	}	
	if (*line == '\0') {
		return (char *) (NULL);
	}	
	if (peek_only == 0) {
		*output_line_offset = (int)(line - swheader->image_head_);
	}
	return line;
}

static void
swheader__reset(SWHEADER * swheader)
{
	swheader->current_offset_=0;
	*(swheader->current_offset_p_)=0;
}

static char *
get_object_by_swverid(SWHEADER * swheader, SWVERID * swverid,
			int (*f_comp)(SWVERID *, SWVERID *), int * nmatches)
{
	char * retval;
	char * next_line;
	SWVERID *t_swverid = NULL;
	int start_offset;
	int object_offset;
	int ret_object_offset;
	Swverid_Cmp_Code comparison_result;


	retval = NULL;
	start_offset = swheader_get_current_offset(swheader);
	ret_object_offset = start_offset;
	if (f_comp == NULL)
		f_comp = swverid_vtagOLD_compare;

	SWHEADER_E_DEBUG("");

	if (nmatches) *nmatches = 0;

	/*
	 * step to first object
 	 */
	next_line=swheader_get_current_line(swheader);
	E_DEBUG2("initial next_line is %p", (void*)next_line);
	if (!next_line)
		return NULL;
	while ( next_line && (swheaderline_get_type(next_line)
						!= SWPARSE_MD_TYPE_OBJ))
		next_line=swheader_f_goto_next(swheader);

	/*
	 * Now compare current object to the object described by 'swverid'
	 */
	do {
		E_DEBUG("");
		object_offset = swheader_get_current_offset(swheader);
		t_swverid = swverid_open(NULL, (char*)(NULL));
		if (swheader_generate_swverid(swheader, t_swverid, next_line)) {
			fprintf(stderr, "swheader_get_object: error returned by get_object.\n");	
			swheader_set_current_offset_p_value(swheader, start_offset);
			return (char *)NULL;
		}
		comparison_result = (*f_comp)(swverid, t_swverid);
		if (comparison_result==SWVERID_CMP_EQ) {
			swverid_close(t_swverid);
			t_swverid = NULL;
			swheader_set_current_offset_p_value(swheader,
								object_offset);
			retval = next_line;
			ret_object_offset = object_offset;
			if (nmatches)
				(*nmatches)++;
			else
				break;
			E_DEBUG("");
		}
		if (t_swverid) swverid_close(t_swverid);
		t_swverid = NULL;
		E_DEBUG("");
	} while ((next_line = swheader_get_next_object(swheader,
					(int)(UCHAR_MAX), (int)UCHAR_MAX)));
	if (t_swverid) swverid_close(t_swverid);
	swheader_set_current_offset_p_value(swheader, ret_object_offset);
	return retval;
}

void
restore_state(SWHEADER * header, SWHEADER_STATE * state)
{
	header->current_offset_p_ = state->save_current_offset_pM;
	header->current_offset_ = state->save_current_offsetM;
	if (header->current_offset_p_)
		*(header->current_offset_p_) = state->save_current_offset_vM;
}

static
char *
get_attribute_in_current_object(SWHEADER * swheader, char *keyword,
				char * object_keyword, int * is_multi_value)
{
	int multi_line;
	struct swsdflt_defaults *swd;
	char * line;
	char * prevline;
	SWHEADER_STATE state;
	
	SWHEADER_E_DEBUG("");
	swheader_store_state(swheader, &state);
	if (object_keyword)
		fprintf(stderr, "warning: get_attribute_in_current_object() is deprecated for this usage\n");

	if (object_keyword) {
		swd=swsdflt_return_entry(object_keyword, keyword);
		if (swd == NULL) {
			multi_line = 
			(enum swsdflt_value_type)(sdf_single_value);
		} else {
			multi_line = swsdflt_get_value_type(swd);
		}
	} else {
		multi_line = (enum swsdflt_value_type)(sdf_single_value);
	}

	if (multi_line == (enum swsdflt_value_type)(sdf_single_value)) {
		/*
		* single value, return the last one.
		*/
		if (is_multi_value) *is_multi_value = 0;
		line = swheader_raw_get_attribute_in_object(swheader, keyword);
		prevline = line;
		while (line) {
			line = swheader_raw_get_attribute_in_object(swheader,
								keyword);
			if (line) prevline = line;
		}
		restore_state(swheader, &state);
		return prevline;
	} else {
		/*
		* multi value attribute.
		*/
		if (is_multi_value) *is_multi_value = 1;
		line = swheader_raw_get_attribute_in_object(swheader,
								keyword);
		restore_state(swheader, &state);
		return line;
	}
}


/* -------------------------------------------------------------------*/
/* ------------------ Public Functions ------------------------------*/

SWHEADER_STATE * 
swheader_state_create(void)
{
	SWHEADER_STATE * x;
	x = malloc (sizeof(SWHEADER_STATE));
  	x->save_current_offset_pM = NULL;
	x->save_current_offsetM = -1; 
	x->save_current_offset_vM = -1; 
	return x;
}

void
swheader_state_delete(SWHEADER_STATE * x)
{
	free(x);
}

void
swheader_state_copy(SWHEADER_STATE * to, SWHEADER_STATE * from)
{
  	to->save_current_offset_pM = from->save_current_offset_pM;
	to->save_current_offsetM = from->save_current_offsetM;
	to->save_current_offset_vM = from->save_current_offset_vM; 
}

void
swheader_store_state(SWHEADER * header, SWHEADER_STATE * fp_state)
{
	SWHEADER_STATE * state;

	if (fp_state)
		state = fp_state;
	else
		state = &(header->saved_);	

	state->save_current_offset_pM = header->current_offset_p_;
	state->save_current_offsetM = header->current_offset_;
	if (header->current_offset_p_) {
		state->save_current_offset_vM = *(header->current_offset_p_);
	} else {
		state->save_current_offset_vM = 0;
	}
}

void
swheader_restore_state(SWHEADER * header, SWHEADER_STATE * state)
{
	if (state)
		restore_state(header, state);
	else
		restore_state(header, &(header->saved_));
}

SWHEADER *
swheader_open(char* (*f_goto_next)(void * , int* , int), void * image_object)
{
	int i;
	SWHEADER *swheader=(SWHEADER *)malloc(sizeof(SWHEADER));
	if (!swheader)
		return swheader;

	for (i=0; i<=SWHEADER_IMAGE_STACK_LEN; i++)
			swheader->image_object_stack_[i]=(void*)(NULL);

	if (image_object){
		swheader_set_image_object(swheader, image_object, 0);
		swheader->image_object_=image_object;
	}else{
		swheader_set_image_object(swheader, (void*)(swheader), 0);
		swheader->image_object_=(void*)swheader;
	}	

	swheader->f_goto_next_=f_goto_next;
	swheader->current_offset_p_=&(swheader->current_offset_);
	swheader->current_offset_=0;
	swheader->image_head_ = NULL;
        if (!swheader->f_goto_next_)
		swheader_set_iter_function(swheader, 
				swheader_goto_c_next_line);
	swheader_store_state(swheader, &(swheader->saved_));
	return swheader;
}

void
swheader_close(SWHEADER * swheader)
{
	if (swheader->image_head_) free(swheader->image_head_);
	if (swheader != NULL) swbis_free(swheader);
}

void
swheader_reset(SWHEADER * swheader)
{
	swheader__reset(swheader);
	if (swheader->f_goto_next_ != swheader_goto_c_next_line){	
		/* If here, then the collection is a C++ collection. */
		
		/* due to bug (probably) in switer.cxx, 
		 * read through the collection.  This
		 * will instantiate the switer objects.
		 */
		while(swheader_f_goto_next(swheader));
	}
	swheader__reset(swheader);
	(*(swheader->f_goto_next_))(swheader->image_object_, NULL, 
							SWHEADER_GET_NEXT);
}

char *
swheader_f_goto_next(SWHEADER * swheader)
{
	char * ret;
	ret =  (*(swheader->f_goto_next_))(swheader->image_object_,
				swheader_get_current_offset_p(swheader),
						SWHEADER_GET_NEXT);
	return ret;
}


/*
 *  swheader_goto_next_line
 *	goto next line relative to the line pointed to by 
 *	*(output_line_offset).  A pointer to the next line is 
 *	returned and output_line_offset is set to this line.
 * 
 *	Other Functionality provided by the interface:
 *		Reset:	
 *			if output_line_offset==NULL 
 *		
 *		Return first line: 
 *			if (*output_line_offset == INT_MAX) 
 * 
 *		 
 *		Convert offset to char pointer without any side effects:
 *			if (*output_line_offset < 0)
 */
o__inline__
char *
swheader_goto_next_line(void * vheader, int * output_line_offset,
						int peek_only)
{
	char * ret;
	ret = (*(((SWHEADER*)(vheader))->f_goto_next_))
				(((SWHEADER*)(vheader))->image_object_,
						output_line_offset, peek_only);
	return ret;
}


char *
swheader_get_current_line(SWHEADER * swheader)
{
		int i;
		char * p;
		
		SWHEADER_E_DEBUG("");
		i = -swheader_get_current_offset(swheader);
		SWHEADER_E_DEBUG2("current offset is %d", i);
		p = (*(swheader->f_goto_next_))
				(swheader->image_object_, &i,
							SWHEADER_PEEK_NEXT);

		if (!p) {
			SWHEADER_E_DEBUG("returning NULL");
			return NULL;
		} else {
			SWHEADER_E_DEBUG3("returning non-NULL p=%p value=[%s]", p, p);
			return p;
		}
}

void
swheader_set_iter_function(SWHEADER * swheader,
				char * (*fc)(void *, int *, int))
{
	SWHEADER_E_DEBUG("");
	swheader->f_goto_next_ = fc;
}

o__inline__
int
swheader_set_image_object(SWHEADER * swheader, void * image, int index)
{
	int i=0;
	SWHEADER_E_DEBUG("");
	if (index < 0){
		while (swheader->image_object_stack_[i]) i++;
		if (i >= SWHEADER_IMAGE_STACK_LEN)
			return -1;
	} else {
		i=index;
	}
	swheader->image_object_stack_[i]=image;
	return i;
}

o__inline__
void
swheader_set_image_object_active(SWHEADER * swheader, int index)
{
	SWHEADER_E_DEBUG("");
	swheader->image_object_=swheader->image_object_stack_[index];
}

o__inline__
void
swheader_set_image_head(SWHEADER * swheader, void * image)
{
	SWHEADER_E_DEBUG("");
	if (swheader->image_head_) free(swheader->image_head_);
	swheader->image_head_=image;
}

o__inline__
void
swheader_set_current_offset(SWHEADER * swheader, int n)
{
	swheader_set_current_offset_p_value(swheader, n);
}

o__inline__
void
swheader_set_current_offset_p_value(SWHEADER * swheader, int n)
{
	*(swheader->current_offset_p_)=n;
}

o__inline__
void
swheader_set_current_offset_p(SWHEADER * swheader, int *n)
{
	if(!n)
		swheader->current_offset_p_=&(swheader->current_offset_);
	else
		swheader->current_offset_p_=n;
}

/* void * */
/* (char * (*)(void *, int*)) */
F_GOTO_NEXT
swheader_get_iter_function(SWHEADER * swheader)
{
	return swheader->f_goto_next_;
}

o__inline__
int *
swheader_get_current_offset_p(SWHEADER * swheader)
{
	return swheader->current_offset_p_;
}

o__inline__
int
swheader_get_current_offset(SWHEADER * swheader)
{
	return *(swheader->current_offset_p_);
}


char *
swheader_get_image_head(SWHEADER * swheader)
{
	int i=INT_MAX;
	return (*(swheader->f_goto_next_))(swheader->image_object_,
						&i, SWHEADER_GET_NEXT);
}

/*
 *  swheader_get_next_object():
 *
 *  Gets next object that meets specified level constraints.
 *  relative_level_* refers to  level numbers as seen in the sw_parse()
 *  output, they are inclusive. They are relative to the level of `image'
 *
 *  To disable level test, use relative_level_min=UCHAR_MAX and
 *  relative_level_max=UCHAR_MAX.
 *
 *  To get next object at same level, use relative_level_min=0 and
 *  relative_level_max=0.
 *
 *  To get all objects in the object `image', use relative_level_min=0 and
 *  relative_level_max=UCHAR_MAX.
 *
*/
char *
swheader_get_next_object(SWHEADER * swheader , int relative_level_min,
						int relative_level_max)
{
	char *next_line;
	int object_level=0;
	int current_relative_level;
        
	SWHEADER_E_DEBUG("");
	next_line=swheader_goto_next_line((void *)swheader,
			swheader_get_current_offset_p(swheader), 
				SWHEADER_GET_NEXT);
	while (next_line) {
		SWHEADER_E_DEBUG("");
		current_relative_level = swheaderline_get_level(next_line) -
								object_level; 
		if (swheaderline_get_type(next_line) == SWPARSE_MD_TYPE_OBJ) {
			if (relative_level_min == UCHAR_MAX && 
					relative_level_max == UCHAR_MAX) {
				SWHEADER_E_DEBUG("");
				return next_line;
			} else if ( 
				current_relative_level < relative_level_min ||
				current_relative_level > relative_level_max) {
				SWHEADER_E_DEBUG("tested NULL");
				return NULL;
			} else if (
				current_relative_level >= relative_level_min &&
				current_relative_level <= relative_level_max) {
				SWHEADER_E_DEBUG("");
				return next_line;
			} 
		}			
		next_line=swheader_goto_next_line((void *)swheader,
				swheader_get_current_offset_p(swheader),
							SWHEADER_GET_NEXT);
	}
	SWHEADER_E_DEBUG("terminating NULL");
	return NULL;
}

int
swheader_generate_swverid(SWHEADER * swheader, SWVERID * swverid, char * object_line)
{
	int len;
	char *next_attr, *keyw, *value, *object_keyword;
	int object_offset=swheader_get_current_offset(swheader);
	
	if (object_line == (char*)(NULL))
		return -1;

	/*
	 * Enforce requirement that we must be at the start of an object
	 */
	if (swheaderline_get_type(object_line) != SWPARSE_MD_TYPE_OBJ)
		return -2;
	
	object_keyword = swheaderline_get_keyword(object_line);

	swverid_set_namespace(swverid, object_keyword);
	while ((next_attr = swheader_get_next_attribute(swheader))) {
		value = swheaderline_get_value(next_attr, &len);
		keyw = swheaderline_get_keyword(next_attr);
		if (!keyw || !value) {
			fprintf(stderr,
				"error in swheader_generate_swverid.\n");	
			swheader_set_current_offset_p_value(swheader,
						object_offset);
			return -3;
		}
		if (swverid_add_attribute(swverid,
				object_keyword, keyw, value) < 0) {
			return -4;
		}
	}

	/*
	 * Restore the position in the header.
	 */
	swheader_set_current_offset_p_value(swheader, object_offset);
	return 0;
}

char *
swheader_get_single_attribute_value(SWHEADER * swheader, char *keyword)
{
        char * line;
        char * value;
	SWHEADER_STATE state;
	swheader_store_state(swheader, &state);
	line = get_attribute_in_current_object(swheader, keyword, NULL, NULL);
	if (line == NULL) {
		swheader_restore_state(swheader, &state);
		return NULL;
	}
        value = swheaderline_get_value(line, NULL);
	swheader_restore_state(swheader, &state);
        return value;
}

char *
swheader_get_attribute(SWHEADER *swheader, char *attribute_keyword, int * is_multi) {
	char **list;
	char **pp;
	char * ret=(char*)(NULL);
	
	SWHEADER_E_DEBUG("");
	list = swheader_get_attribute_list(swheader,
					attribute_keyword, is_multi);
	pp = list;
	if (!list) return (char*)(NULL);
	/* return the last item in the list. */	
	while (*pp) {
		ret=*pp;
		pp++;
	}
	free(list);
	return ret;
}

char *
swheader_get_attribute_in_current_object(SWHEADER * swheader, char *keyword,
				char * object_keyword, int * is_multi_value)
{
	return
		get_attribute_in_current_object(swheader, keyword,
				object_keyword, is_multi_value);
}

char **
swheader_get_attribute_list(SWHEADER *swheader,
			char *attribute_keyword, int *multi_line)
{
	struct swsdflt_defaults *swd;
	char *line, *keyw, *headerline, **ret;
	CPLOB * cplob_obj;
	SWHEADER_STATE state;
	
	swheader_store_state(swheader, &state);
	headerline=swheader_get_current_line(swheader);
	if (swheaderline_get_type(headerline) != SWPARSE_MD_TYPE_OBJ) {
		restore_state(swheader, &state);
		return (char**)NULL;
	}

	/*
	* keyw is the object keyword.
	*/
	keyw=swheaderline_get_keyword(headerline);

	/*
	* Determine if this is a multi line attribute.
	*/
	if (multi_line) {
		swd=swsdflt_return_entry(keyw, attribute_keyword);
		if (swd == NULL) {
			/* *multi_line=(enum swsdflt_value_type)
							(sdf_single_value);
			*/
			*multi_line = 0;
		} else {
			if (swsdflt_get_value_type(swd) == 
				(enum swsdflt_value_type)(sdf_single_value)) {
				*multi_line = 0;
			} else {
				*multi_line = 1;
			}
		}
	}

	cplob_obj=cplob_open(2);
				/* initialize the null terminated array */
	cplob_add_nta(cplob_obj, (char*)(NULL));
	while ((line=swheader_get_next_attribute(swheader))){
		keyw = swheaderline_get_keyword(line);
		if (!strcmp(attribute_keyword, keyw)) {
			 cplob_add_nta(cplob_obj, line);
		}
	}
	ret = cplob_release(cplob_obj);
	restore_state(swheader, &state);
	return ret;
}


/*
* Get next attribute in current object.
*/

char *
swheader_get_next_attribute(SWHEADER * swheader)
{
	static int att_count=0;
	char * currentline;
        char * nextline;
	
	SWHEADER_E_DEBUG("");
        nextline=swheader_goto_next_line((void *)swheader,
		swheader_get_current_offset_p(swheader),
				SWHEADER_PEEK_NEXT);
	currentline=swheader_get_current_line(swheader);
	
	if (!currentline) {
		att_count=0;
		SWHEADER_E_DEBUG("returning NULL loc 0");
		return (char*)(NULL);
	} else if ((swheaderline_get_type(currentline) == 
					SWPARSE_MD_TYPE_OBJ) && 
						att_count) {
		att_count=0;
		SWHEADER_E_DEBUG("returning NULL loc 1");
		return (char*)(NULL);
	} else {
		if (
		!nextline || 
		(swheaderline_get_type(nextline) == SWPARSE_MD_TYPE_OBJ)) {
			att_count=0;
			SWHEADER_E_DEBUG("returning NULL loc 2");
			return (char*)(NULL);
		}
		att_count++;
        	swheader_f_goto_next(swheader); 
		SWHEADER_E_DEBUG2("returning next line [%s]", nextline);
		return nextline;
	}
}

char *
swheader_get_object_by_tag(SWHEADER * swheader, 
					char *object_keyword,
						char *idtag)
{
	SWVERID *swverid;
	char *ret;

	E_DEBUG("");
	swverid = swverid_open(object_keyword, (char*)(NULL));
	E_DEBUG("");
	swverid_set_tag(swverid, SW_A_tag, idtag);
	E_DEBUG("");
	swverid_set_comparison_code(swverid, SWVERID_CMP_EQ);
	E_DEBUG("");
	ret = get_object_by_swverid(swheader, swverid, swverid_vtagOLD_compare, (int*)NULL);
	E_DEBUG("");
	swverid_close(swverid);
	return ret;
}


char *
swverid_get_object_by_swverid(SWHEADER * swheader, SWVERID * swverid,  int * nmatches)
{
	char * ret;
	ret = get_object_by_swverid(swheader, swverid, swverid_vtagOLD_compare, nmatches);
	return ret;
}


int
swheader_get_object_offset_by_control_directory(SWHEADER * swheader,  STRAR * pairs)
{
	char * next_line;
	int object_offset = 0;  /* FIXME */
	char * control_directory;

	/*
	 * first handle a minimal package.
	 * If control_dir is 0 length, then there can only be one 
	 * product and one fileset.
	 */	

	SWHEADER_E_DEBUG("");
	/* step to first object */
	
	next_line=swheader_get_current_line(swheader);
	if (!next_line)
		return -1;

	/*
	 * goto the first object
	 */
	while (
		next_line && 
		(swheaderline_get_type(next_line) != SWPARSE_MD_TYPE_OBJ))
	{
		next_line=swheader_f_goto_next(swheader);
	}

	/*
	 * Loop thru the objects
	 */	
	do {

		control_directory = swheader_get_single_attribute_value(swheader, SW_A_control_directory);

	} while ((next_line = swheader_get_next_object(swheader,
					(int)(UCHAR_MAX), (int)UCHAR_MAX)));

	swheader_set_current_offset_p_value(swheader, object_offset);
	return -1;
}

char *
swheader_raw_get_attribute_in_object(SWHEADER * swheader, char *keyword)
{
	char *line;
	char *keyw;
	
	SWHEADER_E_DEBUG("");
	line = swheader_get_next_attribute(swheader);
	while(line) {
		keyw = swheaderline_get_keyword(line);
		if (strcmp(keyw, keyword) == 0) {
			return line;
		}
		line = swheader_get_next_attribute(swheader);
	}
	return (char*)NULL;
}


int
swheader_fileobject2filehdr(SWHEADER * fileheader, struct new_cpio_header * file_hdr)
{
	char * attr;
	char * keyword;
	char * value;
	int len;
	int ret;
	int info_filetype = 0;
	int tartype = 0;
	mode_t modet;
	uid_t uid;
	gid_t gid;
	char did_uname = 0;
	char did_gname = 0;
	char did_gid = 0;
	char did_uid = 0;
	char did_mtime = 0;
	char did_link_source = 0;
	char did_size = 0;
	char did_major = 0;
	char did_minor = 0;
	char did_type = 0;
	SWHEADER_STATE state;

	taru_init_header(file_hdr);
	taru_init_header_digs(file_hdr);
	
	swheader_store_state(fileheader, &state);

	while((attr=swheader_get_next_attribute(fileheader))) {

		keyword = swheaderline_get_keyword(attr);
		value = swheaderline_get_value(attr, &len);

		if (strcmp(keyword, SW_A_mode) == 0) {
			file_hdr->c_mode = 0;
			taru_otoul(value, &(file_hdr->c_mode));
			file_hdr->c_mode = file_hdr->c_mode & 07777;
			(file_hdr->usage_maskM) |= TARU_UM_MODE;
		} else if (strcmp(keyword, SW_A_size) == 0) {
			file_hdr->c_filesize = (unsigned long)atol(value);		
			if (file_hdr->digsM) {
				swlib_strncpy(file_hdr->digsM->size, value, sizeof(file_hdr->digsM->size));
				file_hdr->digsM->do_size = DIGS_ENABLE_ON;
			}
			did_size = 1;
		} else if (strcmp(keyword, SW_A_path) == 0) {
			ahsStaticSetTarFilename(file_hdr, value);
		} else if (strcmp(keyword, SW_A_uid) == 0) {
			E_DEBUG("uid");
			file_hdr->c_uid = (unsigned long)atol(value);		
			(file_hdr->usage_maskM) |= TARU_UM_UID;
			did_uid = 1;
		} else if (strcmp(keyword, SW_A_gid) == 0) {
			E_DEBUG("gid");
			file_hdr->c_gid = (unsigned long)atol(value);		
			(file_hdr->usage_maskM) |= TARU_UM_GID;
			did_gid = 1;
		} else if (strcmp(keyword, SW_A_link_source) == 0) {
			ahsStaticSetPaxLinkname(file_hdr, value);
			did_link_source = 1;
		} else if (strcmp(keyword, SW_A_owner) == 0) {
			E_DEBUG("owner");
			ahsStaticSetTarUsername(file_hdr, value);
			did_uname = 1;
			(file_hdr->usage_maskM) |= TARU_UM_OWNER;
		} else if (strcmp(keyword, SW_A_group) == 0) {
			E_DEBUG("group");
			ahsStaticSetTarGroupname(file_hdr, value);
			did_gname = 1;
			(file_hdr->usage_maskM) |= TARU_UM_GROUP;
		} else if (strcmp(keyword, SW_A_mtime) == 0) {
			E_DEBUG("mtime");
			file_hdr->c_mtime = strtoul(value, (char**)NULL, 10);
			did_mtime = 1;
			(file_hdr->usage_maskM) |= TARU_UM_MTIME;
		} else if (strcmp(keyword, SW_A_major) == 0) {
			E_DEBUG(SW_A_major);
			file_hdr->c_rdev_maj = swlib_atoi(value, NULL);	
			did_major = 1;
		} else if (strcmp(keyword, SW_A_minor) == 0) {
			E_DEBUG(SW_A_minor);
			file_hdr->c_rdev_min = swlib_atoi(value, NULL);	
			did_minor = 1;
		} else if (strcmp(keyword, "md5sum") == 0) {
			E_DEBUG("md5sum");
			if (file_hdr->digsM) {
				E_DEBUG("md5sum: DIGS_ENABLE_ON");
				swlib_strncpy(file_hdr->digsM->md5, value, sizeof(file_hdr->digsM->md5));
				file_hdr->digsM->do_md5 = DIGS_ENABLE_ON;
			}
		} else if (strcmp(keyword, "sha1sum") == 0) {
			E_DEBUG("sha1sum");
			if (file_hdr->digsM) {
				E_DEBUG("sha1sum: DIGS_ENABLE_ON");
				swlib_strncpy(file_hdr->digsM->sha1, value, sizeof(file_hdr->digsM->sha1));
				file_hdr->digsM->do_sha1 = DIGS_ENABLE_ON;
			}
		} else if (strcmp(keyword, "sha512sum") == 0) {
			E_DEBUG("sha512sum");
			if (file_hdr->digsM) {
				E_DEBUG("sha512sum: DIGS_ENABLE_ON");
				swlib_strncpy(file_hdr->digsM->sha512, value, sizeof(file_hdr->digsM->sha512));
				file_hdr->digsM->do_sha512 = DIGS_ENABLE_ON;
			}
		} else if (strcmp(keyword, SW_A_type) == 0) {
			info_filetype = (int)(*value);	
			did_type = 1;
		} else if (strcmp(keyword, SW_A_is_volatile) == 0) {
			(file_hdr->usage_maskM) |= TARU_UM_IS_VOLATILE;
		}
	} /* while */

	/*
	 * Now add the type bits to the mode field
	 */

	tartype = swheader_getTarTypeFromTypeAttribute(info_filetype);
	if (tartype == LNKTYPE)
		file_hdr->c_is_tar_lnktype = 1;
	else
		file_hdr->c_is_tar_lnktype = 0;
	taru_set_filetype_from_tartype((char)tartype,
			(modet=(mode_t)(file_hdr->c_mode), &modet), "/");
	file_hdr->c_mode = (unsigned long)modet;

	/*
	 * Now set the user/group lookup policy based on what was
	 * present in the INFO file "file" object.
	 */

	if (did_uname && did_uid) {
		file_hdr->c_cu = TARU_C_BY_UNONE;
	} else if (did_uid) {
		ahsStaticSetTarUsername(file_hdr, "");
		file_hdr->c_cu = TARU_C_BY_UNONE;
	}

	if (did_gname && did_gid) {
		file_hdr->c_cg = TARU_C_BY_UNONE;
	} else if (did_gid) {
		ahsStaticSetTarGroupname(file_hdr, "");
		file_hdr->c_cg = TARU_C_BY_UNONE;
	}

	/*
	 * do a sanity check
	 */

	if (
		(!did_gname && !did_gid) ||
		(!did_uname && !did_uid)
	) {
		swheader_restore_state(fileheader, &state);
		return 1;
	}

	if (!did_gid) {
		ret = taru_get_gid_by_name(ahsStaticGetTarGroupname(file_hdr), &gid);
		if (ret == 0) {
			file_hdr->c_gid = (unsigned long)gid;
		}
	}

	if (!did_uid) {
		ret = taru_get_uid_by_name(ahsStaticGetTarUsername(file_hdr), &uid);
		if (ret == 0) {
			file_hdr->c_uid = (unsigned long)uid;
		}
	}

	/*
	 * FIXME, need to impose required attributes per spec.
	 */
	swheader_restore_state(fileheader, &state);
	return 0;
}

char 
swheader_getTarTypeFromTypeAttribute(char ch) {
	switch (ch) {
		case 'f':
			return REGTYPE;
		case 'd':
			return DIRTYPE;
		case 'c':
			return CHRTYPE;
		case 'b':
			return BLKTYPE;
		case 'p':
			return FIFOTYPE;
		case 's':
			return SYMTYPE;
		case 'h':
			return LNKTYPE;
		case SW_ITYPE_y:
			return NOTDUMPEDTYPE;
		default:
			fprintf(stderr, "%s: invalid C701 file type [%c]  not found, ignoring.\n", swlib_utilname_get(), ch);
			return REGTYPE;
	}
}

void
swheader_print_header(SWHEADER * swheader)
{
	char * next_attr;
	char * next_line;
	SWHEADER_STATE state;

	swheader_store_state(swheader, &state);
        swheader_reset(swheader);
        swheader_set_current_offset_p_value(swheader, 0);

	next_line = swheader_get_next_object(swheader, 
				(int)UCHAR_MAX, (int)UCHAR_MAX);
	while (next_line) {
		swheaderline_write_debug(next_line, STDERR_FILENO);

		swheader_goto_next_line((void *)swheader,
			swheader_get_current_offset_p(swheader), SWHEADER_PEEK_NEXT);

		while((next_attr=swheader_get_next_attribute(swheader)))
                            swheaderline_write_debug(next_attr, STDERR_FILENO);
			next_line = swheader_get_next_object(swheader, 
				(int)UCHAR_MAX, (int)UCHAR_MAX);
	}
	swheader_restore_state(swheader, &state);
}
