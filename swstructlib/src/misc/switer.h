/* switer.h - a non-intrusive Iterator for a swStruct Collection.
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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



#ifndef switer_h_1999041
#define switer_h_1999041

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// swIter 
//    This class provides the f_goto_next() function
//    in swsupplib/misc/swheader.c for the case of parsed
//    metadata referenced by a swsPSF or similarly derived
//    object.
//
//
//    the function <f_goto_next_> must be a class static function 
//    and has the prototype:
//           char* (*f_goto_next)(void * swNextLine_object_addr, char * current_line)
//
//    and returns a pointer to the line after `current_line'


#include "swstruct.h"
#include "swmetadata.h"
#include "limits.h"
#include "swstructiter.h"


extern "C" {
#include "cplob.h"
#include "debug_config.h"
#ifdef SWITERNEEDDEBUG
#define SWITER_E_DEBUG(format) SWBISERROR("SWITER DEBUG: ", format)
#define SWITER_E_DEBUG2(format, arg) SWBISERROR2("SWITER DEBUG: ", format, arg)
#define SWITER_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWITER DEBUG: ", format, arg, arg1)
#else
#define SWITER_E_DEBUG(arg)
#define SWITER_E_DEBUG2(arg, arg1)
#define SWITER_E_DEBUG3(arg, arg1, arg2)
#endif /* SWITERNEEDDEBUG */
}

class swMetaDataVB;
class swIter {
	int stack_index_;
	int last_popped_index_;
	int last_popped_level_;
	int ptos_;
	int vtos_;
	CPLOB * iteratorsM;  // List of swStructIter objects
        swStruct * rootM;
      public:
	
	struct save_set {
		int sv_stack_index_;
		int sv_last_popped_index_;
		int sv_last_popped_level_;
		int sv_vtos_;
		int sv_ptos_;
	};
	struct save_set sv_;

	void		show_debug(FILE * fp);
	static void 	switer_x_write_debug(swStructIter * c, FILE * fp);
	
	void set_last_popped_index(int i){last_popped_index_=i;}
	void undo_mark(void){undo_mark(&(sv_));}
	void undo_back(void){undo_back(&(sv_));}
	
	swIter(swStruct * root)
	{
		init_();
		add(root);
		stack_index_=vtos_;
		rootM = root;
	}
	
	virtual ~swIter(void)
	{
		int i=0;
		void * c=(void*)cplob_val(iteratorsM, i++);
		while (c) {
			delete static_cast<swStructIter*>(c);
			c=(void*)cplob_val(iteratorsM, i++);
		}
		cplob_set_nused(iteratorsM, 0);
		cplob_close(iteratorsM);
	}
	
	void undo_back(struct save_set * sv)
	{
		int i=0;
		swStructIter * swit;
		stack_index_ = sv->sv_stack_index_;
		last_popped_index_ = sv->sv_last_popped_index_;
		last_popped_level_ = sv->sv_last_popped_level_;
		ptos_ = sv->sv_ptos_;
		vtos_ = sv->sv_vtos_;
		while ((swit=static_cast<swStructIter*>((void*)cplob_val(iteratorsM, i++))) && (swit->undo_back(), 1)) { ; }
	}
	
	void undo_mark(struct save_set * sv)
	{
		int i=0;
		swStructIter * swit;
		sv->sv_stack_index_ = stack_index_;
		sv->sv_last_popped_index_ = last_popped_index_;
		sv->sv_last_popped_level_ = last_popped_level_;
		sv->sv_ptos_=ptos_;
		sv->sv_vtos_=vtos_;
		while ((swit=static_cast<swStructIter*>((void*)cplob_val(iteratorsM, i++))) && (swit->undo_mark(), 1)) { ; }
	}
	
	
	void reset(void)
	{
		swStructIter * swit;
		int i = 0;
		vtos_ = 1;
		stack_index_ = 1;

		while ((swit=static_cast<swStructIter*>((void*)cplob_val(iteratorsM, i++))) && (swit->reset(), 1)) { ; }
		last_popped_level_ = -1;
		last_popped_index_ = -1;
	}
	
	
	static swStruct * 
	switer_get_swstruct_by_p_offset(void * this_obj, int offset)
	{
		return	(
			static_cast<swStructIter*>
			((void*)cplob_val((static_cast<swIter*>(this_obj))->iteratorsM, 0))
			) ->find_swstruct_by_p_offset(static_cast<swIter*>(this_obj), offset);
	}
	
	static swStruct * 
	switer_get_swstruct(void * this_obj, int ino)
	{
		return	(
			static_cast<swStructIter*>
			((void*)cplob_val((static_cast<swIter*>(this_obj))->iteratorsM, 0))
			) ->find_swstruct_by_ino(static_cast<swIter*>(this_obj), ino);
	
	}
	
	static swMetaData * 
	switer_get_attribute_by_p_offset(void * this_obj, int offset)
	{
		swMetaData * d;
		swStructIter *it;
		int i=0;
		
		while ((it=static_cast<swStructIter*>((void*)cplob_val((static_cast<swIter*>(this_obj))->iteratorsM, i++)))) {
			d = it->find_by_p_offset(static_cast<swIter*>(this_obj), offset);
			if (d) {
				return d;
			}
		}
		return static_cast<swMetaData*>(NULL);
	}
	
	
	static swMetaData * 
	switer_get_attribute(void * this_obj, int ino)
	{
		swMetaData * d;
		swStructIter *it;
		int i=0;
	
		while ((it=static_cast<swStructIter*>((void*)cplob_val((static_cast<swIter*>(this_obj))->iteratorsM, i++)))) {
			d = it->find_by_ino(static_cast<swIter*>(this_obj), ino);
			if (d) {
				return d;
			}
		}
		return static_cast<swMetaData*>(NULL);
	}
	
	static char * 
	switer_get_nextline(void * this_obj, int * current_offset, int peek_only)
	{
		return switer_I_get_nextline(this_obj, current_offset, peek_only);
	}
	
	static char * 
	switer_peek_at_nextline(void * this_obj, int * current_offset)
	{
		return switer_I_get_nextline(this_obj, current_offset, /* peek */ 1);
	}
	
	swStructIter * get_current_swstructiter(void)
	{
		return static_cast<swStructIter*>((void*)cplob_val(iteratorsM, vtos_-1));
	}
	
	swStruct * get_current_swstruct(void)
	{
		return get_current_swstructiter()->get_swstruct();
	}
	
	int get_current_swstruct_level(void)
	{
		return (get_current_swstruct()->get_swdefinition())->get_level();
	}
	
	void add(swStructIter * swit)
	{
		cplob_add_nta(iteratorsM, (char*)(static_cast<void*>(swit)));
		last_popped_index_=-1;
		vtos_++; 
		ptos_++;
	}
	
	void add(swStruct * sw)
	{
		add(new swStructIter(sw));
	}
	
	swStructIter * popif(swStruct * sw)
	{
		int g;
		swStructIter * swit;
		
		if (stack_index_) g = stack_index_-1;
		else g = stack_index_;
		
		swit=popif_i(sw, g);	
		
		if (swit) {
			return swit;
		}
		swit=popif_i(sw, 0);	
		if (swit) {
			return swit;
		}
		return pushif_i(sw);
	}
	
	swStructIter * pop(swStructIter * lswit)
	{
	    int level;
	    int current_level=get_current_swstruct_level();
	    swStructIter * swit, *oswit=static_cast<swStructIter *>(NULL);
	
	    if  (
	    	lswit &&
	        vtos_ == stack_index_ && 
		last_popped_index_ >= 0 && 
		current_level == last_popped_level_
		) 
		{
			// Optimization Path.
			oswit=peek(last_popped_index_);
			if (oswit && lswit->get_swstruct()->get_swdefinition()->get_level() -
		     	              oswit->get_swstruct()->get_swdefinition()->get_level() == 1){
				last_popped_level_=current_level; 
	    			stack_index_=last_popped_index_; 
				return oswit;
	   		}
	    }
	    do { 
	        if (stack_index_==0) return static_cast<swStructIter *>(NULL);
		swit=static_cast<swStructIter*>((void*)cplob_val(iteratorsM, --stack_index_));
	        if (!swit) return static_cast<swStructIter *>(NULL);
		level=swit->get_swstruct()->get_swdefinition()->get_level(); 
	    } while(stack_index_ >= 0 && level == current_level);
	    
	    last_popped_level_=current_level; 
	    last_popped_index_=stack_index_; 
	    return swit;
	}
	
	void push(swStructIter * swit)
	{
		add(swit),  stack_index_=vtos_;
	}
	
	void set_stack_index(int i)
	{
		stack_index_=i;
	}
	
	int get_stack_index(void)
	{
		return stack_index_;
	}
	
	swStructIter * peek(int i)
	{
		return static_cast<swStructIter*>((void*)cplob_val(iteratorsM, i));
	}
	
	swStructIter * peek(void)
	{
		if (!stack_index_) return static_cast<swStructIter*>(NULL);
		return static_cast<swStructIter*>((void*)cplob_val(iteratorsM, stack_index_-1));
	}
	
    private: // ------------ Private ---------------------------------
	
	static char * 
	switer_I_get_nextline(void * this_obj, int * current_offset, int peek)
	{
		swIter * switer=static_cast<swIter*>(this_obj);
		swMetaData * swm;
		swStructIter * swit;
		struct swIter::save_set sv;
		char * ret;

		SWITER_E_DEBUG3("ENTER this=%p current_p=%p", this_obj, (void*)current_offset);
		SWITER_E_DEBUG3("ENTER current_offset=%d  peek=%d", current_offset ? (*current_offset):-999999, peek);

	
		/* Interface:
		current_offset		*current_offset		Action
		------------------      ------------------     --------
		NULL			not applicable		reset
		!NULL			INT_MAX			return swheader->image_head_;
		!NULL			<0			return C address of line at offset -(*current_offset)
	
		peek
		----
		0	return next line and set state appropriately..
		1	return next line and leave state unchanged.
		*/
	
	        if (!current_offset){
			switer->reset();	
			switer->init_read();
			switer->reset();	
			fprintf (stderr, "reseting in switer_get_nextline\n");
			SWITER_E_DEBUG("NULL current_offset: returning NULL");
			return (char*)(NULL);
		}
						// return char pointer to first line.
		if (*current_offset == INT_MAX){
			ret = static_cast<char*>(
			static_cast<swStructIter*>((void*)cplob_val(switer->iteratorsM, 0))->
				get_swstruct()->get_swdefinition()->get_mem_addr());
			SWITER_E_DEBUG3("INT_MAX returning %p [%s]", (void*)ret, ret?ret:"");
			return ret;
		}
	
						// return current line
		if (*current_offset < 0) {
			ret = switer_get_current_line(this_obj, -(*current_offset));
			SWITER_E_DEBUG3("<0 returning %p [%s]", (void*)ret, ret?ret:"");
			return ret;
		}
	
		if (*current_offset == 0) {
			if (peek) {
				switer->undo_mark(&sv);	
			}
			swm=static_cast<swStructIter*>((void*)cplob_val(switer->iteratorsM, 0))->get_swstruct()->get_swdefinition();
			if (peek) {
				switer->undo_back(&sv);	
			} else {
				*current_offset=swm->get_ino();
			}
			ret = swm->get_parserline();
			SWITER_E_DEBUG3("==0 returning %p [%s]", (void*)ret, ret?ret:"");
			return ret;
		}
		
		if (peek) {
			switer->undo_mark(&sv);	
		}
		if (switer->stack_index_ <= 0) switer->set_stack_index(1);
		swit=static_cast<swStructIter*>((void*)cplob_val(switer->iteratorsM, switer->stack_index_-1));
		swm=swit->find_next_by_ino(switer, *current_offset);
		if (peek) {
			switer->undo_back(&sv);
		}	
		if (!swm){
			/*fprintf (stderr, "BOOOM\n"); */
			/* *current_offset=-1; */
			SWITER_E_DEBUG("returning NULL");
			return (char*)(NULL);
		}
		if (!peek)
			*current_offset=swm->get_ino();
		ret = swm->get_parserline();
		SWITER_E_DEBUG3("END returning %p [%s]", (void*)ret, ret?ret:"");
		return ret;
	}
	
	static char * 
	switer_get_current_line(void * this_obj, int ino)
	{
		swMetaData * swm;
		struct swIter::save_set sv;
	
		(static_cast<swIter*>(this_obj))->undo_mark(&sv);
		swm=switer_get_attribute(this_obj, ino);
		(static_cast<swIter*>(this_obj))->undo_back(&sv);
		if (!swm) return (char*)(NULL);
		return swm->get_parserline();
	}
	
	void init_()
	{
		vtos_=ptos_=stack_index_=0;
		sv_.sv_vtos_=ptos_=sv_.sv_stack_index_=0;
		last_popped_index_=last_popped_level_=-1;
		sv_.sv_last_popped_index_=sv_.sv_last_popped_level_=-1;
		iteratorsM=cplob_open(8);
		cplob_additem(iteratorsM, 0, static_cast<char*>(NULL));
	}
	
	swStructIter * pushif_i(swStruct * sw)
	{
		if (vtos_ < ptos_){
			swStructIter * st=peek(vtos_++);
			stack_index_=vtos_;
			return st;
		}
		swStructIter * swit=new swStructIter(sw);
		push(swit);
		return swit;
	}
	
	
	swStructIter * popif_i(swStruct * sw, int inx)
	{
		int i=0;
		swStructIter * swit= static_cast<swStructIter *>(NULL);
		while(	
			(inx<vtos_) &&
			(swit=static_cast<swStructIter*>((void*)cplob_val(iteratorsM, inx))) && 
			(swit->get_swstruct() != sw)
		     ) {
				i++;
		     		inx++;
			}
		if (inx >= vtos_) {
			return NULL;
		}
		return swit;
	}
	
	void init_read(void)
	{
		char * line;
		int i_inode=0;
		swMetaData * swmd;
		line=switer_get_nextline(this, &i_inode, 0);
		while (line) {
			swmd=swIter::switer_get_attribute(static_cast<void*>(this), i_inode);
			if (!swmd) {
				fprintf(stderr,"exception on switer::init_read: %d i_inode number not found.\n", i_inode);
			}
			line=switer_get_nextline(this, &i_inode, 0);	
		}
	}
};	
#endif
