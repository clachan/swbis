/* swmetadata_i.h
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


#ifndef swmetadata_19980601jhl_h
#define swmetadata_19980601jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>
#include "swstructdef.h"
#include "swattributemem.h"

extern "C" {
#include "swparse.h"
#include "swfdio.h"
#include "strob.h"
#include "swsdflt.h"
#include "uxfio.h"
#include "swheaderline.h"
#include "swvarfs.h"
#include "swlib.h"
#include "swutillib.h"
#include "debug_config.h"
}

#define SWMETADATANEEDDEBUG 1
#undef SWMETADATANEEDDEBUG 


#ifdef SWMETADATANEEDDEBUG
#define SWMETADATA_DEBUG(format) SWBISERROR("SWMETADATA DEBUG: ", format)
#define SWMETADATA_DEBUG2(format, arg) SWBISERROR2("SWMETADATA DEBUG: ", format, arg)
#define SWMETADATA_DEBUG3(format, arg, arg1) SWBISERROR3("SWMETADATA DEBUG: ", format, arg, arg1)
#else
#define SWMETADATA_DEBUG(arg)
#define SWMETADATA_DEBUG2(arg, arg1)
#define SWMETADATA_DEBUG3(arg, arg1, arg2)
#endif

#define SWFILE_DO_CKSUM   (1 << 0)	// Do create POSIX cksum
#define SWFILE_DO_FILE_DIGESTS  (1 << 1)	// Do create md5 digest.
#define SWFILE_DO_FILE_DIGESTS_SHA2  (1 << 2)	// Do create md5 digest.
#define SWBIS_NO_STAT_INDICATOR "\x1C"    // FS source names that begin with this are not lstat'ed., Internal use only.
#define SWBIS_NO_STAT_INDICATOR_C '\x1C'    // FS source names that begin with this are not lstat'ed., Internal use only.

class swPackage;

class swMetaData
{
	int offset_;    // physical offset.
	int inode_;     // virtual offset that is used to identify.
	char is_set_explicit_; 
	char level_;
	char kw_type_;
	unsigned char status_;   // 0 is included in write_fd, non-zero is excluded.
	unsigned char verbose_state_;   // used to prevent multiple identical warning messages
	swMetaData * contained_byM;

	swMetaData * next_;
    
	static int use_count_;
	static swstructdef * defaults_;
	static swAttributeMem * data_p_;
	static int logical_modeM;	// 0 no censoring, 1 means get_value() returns NULL if status_ is non-zero. 
  public:

	enum swExtEnum {file_permsLength = 500};  // FIXME, this is the length limit for the linksource, owner, et.al.
	enum swFileOpts {typeE, modeE, mtimeE, ctimeE, ownerE, uidE, 
			groupE, gidE, umaskE, sizeE, volaE, compE, 
			cksumE, md5sumE, sha1sumE, sha512sumE, linksourceE, minorE, majorE, inoE, nlinkE, swbiE, lastE};
	static char * namesM[23];
     
	swMetaData(char * keyword, char * value);
	swMetaData (void);
	virtual ~swMetaData (void);

	void set_find_mode_logical(int c) { logical_modeM = c; }
	int  get_find_mode_logical(void) { return logical_modeM; }
     
	swMetaData * get_last_node (void) {
		swMetaData *p, *pp;
		p = pp = get_next_node();
		while ( p )
		{
			pp = p;
			p = p->get_next_node();
		}
		return pp;
	}
    
        virtual int get_no_stat(void) { return 0; }
	virtual void set_no_stat(int c) {  }

	inline swMetaData * get_next_node (void) { return next_; }
	inline int get_is_explicit(void) { return (int)is_set_explicit_; }
	inline void set_is_explicit(void) { is_set_explicit_ = (char)1; }

	void * get_mem_addr (void) {
		return (void*)(((UXFIO*)(data_p_->uxfio_addrM))->bufM);
	}

	int get_mem_fd (void) {
		return data_p_->swAttributeMem::get_mem_fd();
	}

	int get_mem_offset (void) {
		return data_p_->swAttributeMem::get_mem_offset();
	}

	int get_p_offset (void) { return offset_; }

	swMetaData * set_next_node (swMetaData * p) {
		next_ = p;
		return next_;
	}

	inline char * get_parserline(void) { 
		//return static_cast<char*>(get_mem_addr()) + get_p_offset(); 
		return static_cast<char*>(((UXFIO*)(data_p_->uxfio_addrM))->bufM) + offset_;
	}

	swMetaData * get_last_node_in_object (swMetaData * swi) {
		int target_level;
		swMetaData * p = swi, *pp;
		if (!p) return static_cast<swMetaData*>(NULL);
     
		target_level = p->get_level();
     
		pp=p;
		while (p && p->get_level() == target_level) {
			pp=p;
			p=pp->get_next_node ();
		}
		return pp;
	}

	void   delete_list(swMetaData * header) {
		swMetaData *p, *pp;
		p = pp = header->get_next_node();
		delete header; 
		while ( p )
		{
			pp = p;
			p = p->get_next_node();
			delete p;
		}
	}

	void  list_add (swMetaData * newnode) {
		next_ = newnode;
	}

	virtual int list_add_if_new (swMetaData * newnode);
	virtual void  list_replace (swMetaData * newnode);
   	virtual void list_replace_if_not_explicitly_set(swMetaData * swmd); 
	virtual void list_insert (swMetaData * header, swMetaData * newnode);

	virtual void set_ino(int off);       //identifying serial number, constant.
     
	inline int get_ino(void) { return inode_; }

	virtual char * getObjectName(void) { return (char *)NULL; }

	virtual void set_p_offset (int off); //physical offset, changes.
	virtual void set_type (int type);
        int get_status(void){return (int)status_;}
	void set_status(int c){status_= (unsigned char)c;}
	virtual int  get_type (void);
	virtual int  get_level (void);
	void set_level_ (int level) { level_ = level; }
	virtual void set_level (int level);
	virtual void set_contained_by(swMetaData * referer) { contained_byM = referer; }
	virtual swMetaData * get_contained_by(void) { return contained_byM; }

	virtual int write_fd(int fd);
	virtual int write_fd_debug(int fd, char * prefix) { return 0; }
	virtual int insert(char * line, swMetaData * location);    
	virtual int insert(char * keyword, char * value);
	void vremove(void) { status_ = (unsigned char)(0x01); }
	
	swMetaData* findAttribute(char * object_keyword);

	swMetaData* find_object (char * object_keyword, int occurance_number );
	static int determine_type (char type);
	static int determine_type (char * typestring);

	virtual char * get_value (int * length);
	virtual char * get_keyword (void);
     
	virtual int add (char * keyword, char * value, swMetaData * newnode, int level);
    
	virtual char * set_value (char * value);
	virtual char * set_value (swMetaData * value);
	virtual char * set_keyword (char * key);
     
	virtual int get_next_line_offset (void);

	struct swsdflt_defaults  *defaults	(void);
	int  	is_sw_keyword 		(char* object_keyword, char* keyword);
	int 	get_attr_group 		(int index);
	struct swsdflt_defaults  * return_entry	(char * object, char * keyword );
	int 	return_entry_index 	(char * object, char * keyword);
	char * 	return_entry_keyword 	(char * buf, int entry_index);
	int 	return_entry_group 	(int index);

	static int did_sense_hard_link(SWVARFS * swvarfs, struct stat * st);
	static int swmetadata_local_open(SWVARFS * swvarfs, char * name, int * virt_whence_set);
	static int swmetadata_local_close(SWVARFS * swvarfs, int u_fd);
	static int swmetadata_decode_file_stats(SWVARFS * swvarfs, 
			char *source, char * hl_path,
				struct stat *st,
					char array[][swMetaData::file_permsLength],
						int array_is_set[],
							int cksumflags);
	static char getFileTypeFromMode(mode_t mode);
	static mode_t getModeFromFileType(mode_t mode, int ch);
	static int fileStats2statbuf(struct stat * statbuf, char array[][file_permsLength], int array_is_set[]);

  private: 	//  ----- private functions --------
	int add_(char * keyword, char * value, swMetaData * newnode, int level, int off);
	char * set_value_ (char * value, int off);
	static void setTarLinkname(char * dest, char * linkname, int size);
	void init_mem(void);  
	void init(void);  

	void handle_utf_verbose_top(void) {
		if (verbose_state_ == 0) {
			swparse_set_do_not_warn_utf();
		} else {
			verbose_state_ = 0;
		        swparse_unset_do_not_warn_utf();
   		}
	}

	void handle_utf_verbose_bot(void) {
		if (verbose_state_ == 0) {
			swparse_set_do_not_warn_utf();
		} else {
			; //	
		}
	}
};
#endif
