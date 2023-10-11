/* swdefinitionfile.cxx
 */ 

/*
 * Copyright (C) 1998,1999,2008  James H. Lowe, Jr.
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

extern "C" {
#include "swuser_config.h"
}

#include <assert.h>
#include "swparser.h"
#include "swdefinitionfile.h"

extern "C" {
#include "swheaderline.h"
int fnmatch (const char *, const char *, int); 
}

extern "C" {
#include "cplob.h"
#include "debug_config.h"
}

swFileMap * swDefinitionFile::swfilemapM;

int 
swDefinitionFile::write_fd_debug(int fd, char * prefix) {
	int ret=0,i=0;
	int eret;
	swDefinition *swmd;
	while ((swmd=swdeffile_get_pointer_from_index(i++))) {
		eret = swmd->write_fd_debug(fd, prefix); 
		if (eret<0) return -ret;
		ret+=eret; 
	}
	return ret;
}

int 
swDefinitionFile::write_fd(int fd) {
	int ret=0,i=0;
	int eret;
	swDefinition *swmd;
	while ((swmd=swdeffile_get_pointer_from_index(i++))) {
		eret = swmd->write_fd(fd); 
		if (eret<0) return -ret;
		ret+=eret; 
	}
	return ret;
}

swDefinition * 
swDefinitionFile::makeDefinition(char * parser_line, int * retvalp) {
	swMetaData * attributes;
	swMetaData * swdef;

	if ((char)(*parser_line) == 'F') {
		return NULL;
	}
	if (swMetaData::determine_type(parser_line) != swstructdef::sdf_object_kw) return NULL;
	
	swdef=swDefinition::swdefinition_factory(parser_line);
	if (!swdef) {
		return NULL;
	}
	
	attributes = generate_attribute_list(swdef->get_level() + 1, retvalp);
	//
	// The Extended Definition Expansion returns NULL here.
	// Null is deemed not an error.
	//
	if (attributes) swdef->set_next_node(attributes);
	return static_cast<swDefinition*>(swdef);  // FIXME Down Cast, Bad.
}
	
int 
swDefinitionFile::generateDefinitions(void) {
	int retval = 0, retvalDef=0;
	char  *p,  *parser_line; 
	int offset, start_offset, current_offset;
	int oldoffset;
	int u_type=0, u_level;
	swDefinition * swdef = static_cast<swDefinition*>(NULL);
	int len; 

	set_next_node(NULL); 
	p = (char*)get_mem_addr();

	if (swFileMapPeek(&start_offset, &current_offset, &len) < 0) return -1;
	if (len <= 0) {
		fprintf (stderr,"error in generateDefinitions(): bad length: %d\n", len);
		return -2;
	}
	uxfio_lseek(get_mem_fd(), start_offset, SEEK_SET);

	do {
		offset=get_next_line_offset();
		if (offset >= 0) {
			parser_line = p + offset; 
			u_type=swheaderline_get_type(parser_line);
			//
			// Can't use swheaderline_get_keyword(parser_line) because
			// adds a null and get_next_line_offset() doesn't expect nulls in
			// the parsed output.
			// u_keyword = swheaderline_get_keyword(parser_line);
			//
			u_level = swheaderline_get_level(parser_line);
	 
			if (u_type == 'F') {
				continue;
			} else if (u_type == 'E') {
				//
				// seek back because generate_attribute_list() must read this
				// same line again.
				//
				E_DEBUG("making type E");
				oldoffset = uxfio_lseek(get_mem_fd(), 0, SEEK_CUR);
				uxfio_lseek(get_mem_fd(), offset, SEEK_SET);
				generate_attribute_list(u_level, &retval);
				uxfio_lseek(get_mem_fd(), oldoffset, SEEK_SET);
			} else if (u_type == 'O') {
				E_DEBUG("making type O");
				swdef = makeDefinition(parser_line, &retvalDef);
				assert(swdef);
				swdeffile_list_add(swdef); 
				#ifdef FILENEEDDEBUG
					fprintf(stderr, "HERE ********************\n");
					swdef->write_fd(2);
				#endif
			} else {
				retval--;	
				fprintf (stderr,"error in generateDefinitions(): Unrecognized attribute type.\n");
			}
			p = (char*)get_mem_addr();
		} else {
			continue;
		}
	} while (retval == 0 && offset >= 0 && u_type != 0 && ((offset - start_offset) < len ));
	// swFileMapPop(&start_offset, &current_offset, &len);
	return (retval ? retval : retvalDef);
}


//
// Seeks to next line in physical order.
//
int 
swDefinitionFile::get_next_line_offset(int swfilemapfd) {
	int 	offset;
	int	amount;
	int 	next_offset;
	int 	v_current_offset;
	const int  RBLK=512; 
	char  	buf[RBLK+2];
	int 	v_offset = 0;
	int	v_len = 0;
	int 	value_len;
	char * 	value;
	char * 	keyword;
	int	eoatt;
	int	seek_adjustment;

	if (swfilemapfd >= 0)
		swFileMapPeekByIndex(swfilemapfd, &v_offset, &v_current_offset, &v_len);
	else	
		swFileMapPeek(&v_offset, &v_current_offset, &v_len);
	
	//
	// Seek to next line and return the current line.
	// and avoid reading past the original length of the
	// parsed output. (the file may have been added to.)
	//
	
	offset=uxfio_lseek(get_mem_fd(), 0, SEEK_CUR);

	if (offset < 0) {
		fprintf(stderr,"internal error in swDefinitionFile::get_next_line_offset()\n");
		return -2;	
	}
	buf[RBLK]='\0'; 

	//
	// Check to see if we are at the end of the original
	// parsed output.
	//
	amount = v_offset + v_len - offset;
 	if (amount > RBLK) {
		amount = RBLK;
	}

	if (amount <= 10 && amount != 0) {
		//fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=1a.\n");
		return -2;	
	} else if (amount == 0) {
		//
		// Normal termination.
		//
		return -1;
	} else {
		;
	}

	//
	// Nibble at the line, 10 bytes is a safe amount.
	//
	if (uxfio_sfread(get_mem_fd(), buf, 10) != 10) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=2.\n");
		return -3;
	}

	//
	// It ought to contain a space unless the value length is billions of bytes long.
	//
	if (!strchr(buf, (int)' ')) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=3.\n");
		return -4;
	}

	//
	// It ought to begin with a digit.
	//
	if (!isdigit((int)(*buf))) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=3a.\n");
		return -4;
	}

	//
	// Get the value length.
	//
	value_len = swheaderline_get_value_length(buf);
	if (value_len < 0) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=4.\n");
		return -4;
	}


	//
	// Make the rest of the nibble.
	//    The attribute name better had be less than  RBLK bytes long!!.
	//
	if (uxfio_sfread(get_mem_fd(), buf+10, amount-10) != amount-10) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=5.\n");
		return -5;
	}

	//
	// Determine total length of attribute.
	//
	keyword = swheaderline_get_keyword(buf);
	if (!keyword) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=5a. buf=[%s]\n", buf);
		return -5;
	}

	//
	// Get the value pointer, don't use swheaderline_get_value()
	// because this places a \0 at the end of the value.
	//
	value = swheaderline_get_value_pointer(buf, NULL);
	if (!value) {
		eoatt = (int)(keyword - buf) + strlen(keyword) + 1;
	} else {
		eoatt = (int)(value - buf) + value_len + 1;
	}
	

	//
	// Now adjust file to the start of the next line.
	//
	seek_adjustment = eoatt - amount;
	if (seek_adjustment > 0) {
		//
		// Have not read the whole value, must do so otherwise
		// things blow up.
		//
		swlib_read_amount(get_mem_fd(), seek_adjustment); 	
		next_offset = uxfio_lseek(get_mem_fd(), 0, SEEK_CUR);
	} else {
		next_offset = uxfio_lseek(get_mem_fd(), seek_adjustment, SEEK_CUR);
	}

	if (next_offset < 0) {
		fprintf(stderr,"Internal error in swDefinitionFile::get_next_line_offset(), location=6.\n");
		return -6;
	}

	swFileMapSetCurrentOffset(next_offset);
	return offset;
}

char *
swDefinitionFile::doExpandFileReference(char * attribute_value, off_t * len) {
	int fd;
	STROB * contents;
	struct stat st;
	char * filename = attribute_value;

	filename ++;	// skip over the '<'
	while(isspace(static_cast<int>(*filename))) filename++;
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return static_cast<char*>(NULL);
	}
	if ( (fstat(fd, &st) < 0) || !S_ISREG(st.st_mode)) {
		close(fd);
		return static_cast<char*>(NULL);
	}
	*len = st.st_size;
	contents = strob_open(*len + 1);
	if (read(fd, static_cast<void*>(strob_str(contents)), (size_t)(*len)) != *len) {
		fprintf(stderr,"read error: %s\n", filename);
		close(fd);
		strob_close(contents);
		return static_cast<char*>(NULL);
	}
	close(fd);
	return strob_release(contents);
}

//
// This is the function that can be registered with the swheader object.
//    (See ./tests/testheader4.cxx)
// Uses the link list interface.
// Get the next line after (*current_inode) and set *current_inode
// to that value.
//

char * 
swDefinitionFile::swdeffile_linki_nextline(void * this_obj, int * current_inode, int peek)
{
	swMetaData * swm;
	swDefinitionFile * tob = static_cast<swDefinitionFile *>(this_obj);	
	char * ret;

	E_DEBUG3("ENTER this=%p current_p=%p", this_obj, (void*)current_inode);
	E_DEBUG3("ENTER current_inode=%d  peek=%d", current_inode ? (*current_inode):-999999, peek);
	
	// 
	//* Interface:
	//* current_inode		*current_inode		Action
	//* ------------------      ------------------     --------
	//* NULL			not applicable		reset
	//* !NULL			INT_MAX			return swheader->image_head_;
	//* !NULL			<0			return C address of line at offset -(*current_inode)
	//* !NULL			0			return first object.
	//* 
	//* peek
	//* ----
	//* 0	return next line and set state appropriately..
	//* 1	return next line and leave state unchanged.
	//

	//
	// *current_inode is NULL.
	// Reset.
	//
        if (current_inode == static_cast<int*>(NULL)){
		tob->currentM = tob->headM;
		// fprintf (stderr, "reseting in get_nextline\n");
		E_DEBUG("NULL current_inode: returning NULL");
		return (char*)(NULL);
	}
	
	//
	// *current_inode == INT_MAX
	// return char pointer to first line.
	//
	if (*current_inode == INT_MAX){
		ret = static_cast<char*>(tob->headM->get_mem_addr());
		E_DEBUG3("INT_MAX returning %p [%s]", (void*)ret, ret?ret:"");
		return ret;
	}

	//
	// *current_inode < 0
	// return current line
	//
	if (*current_inode < 0) {
		E_DEBUG3("<0 returning %p [%s]", (void*)ret, ret?ret:"");
		swm = tob->swdeffile_linki_find_by_ino(tob->headM, -(*current_inode));
		if (swm == NULL) {
			ret = NULL;
		} else {
			ret = swm->get_parserline();
		}
		return ret;
	}

	//
	// Return first element.
	//
	//if (*current_inode == 0) {
	//	swm = headM;
	//	*current_inode=swm->get_ino();
	//	ret = swm->get_parserline();
	//	E_DEBUG3("==0 returning %p [%s]", (void*)ret, ret?ret:"");
	//	return ret;
	//}

	ret = tob->swdeffile_linki_get_nextline(current_inode, peek);

	E_DEBUG3("END returning %p [%s]", (void*)ret, ret?ret:"");
	return ret;
}

int 
swDefinitionFile::swdeffile_linki_write_fd_debug(int fd, char * prefix) {
	int ret=0;
	int eret;
	swDefinition *swmd = headM;
	while (swmd) {
		eret = swmd->write_fd_debug(fd, prefix); 
		if (eret<0) return -ret;
		ret+=eret; 
		swmd = swmd->get_next();
	}
	return ret;
}

int 
swDefinitionFile::swdeffile_linki_write_fd(int fd) {
	int ret=0;
	int eret;
	int do_write;
	swDefinition *swmd = headM;

	while (swmd) {
		E_DEBUG("running: determine_status_for_writing(swmd)");
		do_write = determine_status_for_writing(swmd);
		if (do_write) {
			E_DEBUG("do_write is true");
			eret = swmd->write_fd(fd); 
		} else {
			E_DEBUG("do_write is false");
			eret = 0;
		}
		if (eret < 0) return -ret;
		ret += eret; 
		swmd = swmd->get_next();
	}
	return ret;
}

int
swDefinitionFile::determine_status_for_writing(swDefinition * swmd) { 
	int ret;
	int ret2;
	E_DEBUG("Entering");
	if (swmd->get_storage_status() == SWDEF_STATUS_DUP0) {
		// there is a duplicate later on, go find it
		// and merge it with swmd
		E_DEBUG("status is SWDEF_STATUS_DUP0, merging");
		ret2 = merge_duplicates(swmd);
		if (ret2 != 0) {
			//
			// error
			//
			fprintf(stderr,
				"%s: fatal error merging file definitions, status code: %d\n",
				swlib_utilname_get(), ret2);
			exit(47);
		}
		ret = 1;
	} else if (swmd->get_storage_status() == SWDEF_STATUS_DUP) {
		// skip it 
		E_DEBUG("status is SWDEF_STATUS_DUP, skip it");
		ret = 0;
		;
	} else {
		ret = 1;
	}
	return ret;
}

// --------------------------------------------------------
// --------------- Private Functions ----------------------
// --------------------------------------------------------

int
swDefinitionFile::merge_duplicates(swDefinition * relhead) { 
	swDefinition *swmd;
	char * ipath;
	char * path;
	char * isource;
	char * source;

	E_DEBUG("Entering");
	swmd = relhead;
	ipath = swmd->find(SW_A_path);
	isource = swmd->find(SW_A_source);
	if (!ipath) return -1;
	if (!isource) return -2;
			
	swmd = swmd->get_next();
	while (swmd) {
		E_DEBUG("New Loop");
		if (swmd->get_storage_status() == SWDEF_STATUS_DUP) {
			path = swmd->find(SW_A_path);
			if (!path) return -3;
			if (
				strcmp(path, ipath) == 0
			) {
				E_DEBUG2("Got match to merge for path [%s]", path);
				source = swmd->find(SW_A_source);
				if (!source) return -4;
				E_DEBUG3("source=[%s] isource=[%s]", source, isource);
				if (swlib_dir_compare(source, isource, 0) == 0) {
					E_DEBUG("calling merge");
					relhead->merge(swmd,
						0, /* 0 =: replace only attributes
						      explicitly set */
						1  /* 1 =: do replace */
						);
				} else {
					//
					// Changing the source is not supported
					//
					return -5;
				}
			}
		}
		swmd = swmd->get_next();
	}
	return 0;
}

void swDefinitionFile::init(void) { 
	headM = static_cast<swDefinition*>(NULL);
	tailM = static_cast<swDefinition*>(NULL);
	currentM = static_cast<swDefinition*>(NULL);
	if (swfilemapM == static_cast<swFileMap*>(NULL)) swfilemapM = new swFileMap();
	swdefinition_listM=new swPtrList<swDefinition>();
	sbufM = strob_open(2);
	did_setM = 0;
}

int swDefinitionFile::run_parser_(int atlevel, int mark_up_flag, int * pfd) {
	int offset = uxfio_lseek(get_mem_fd(), 0L, SEEK_CUR);
	int current_end_offset = uxfio_lseek(get_mem_fd(), 0L, SEEK_END);
	int len;
	int vfd;

	uxfio_lseek(get_mem_fd(), offset, SEEK_SET);
	len = parserM->run_parser(atlevel, mark_up_flag);
	vfd = swFileMapPush(current_end_offset, len);
	if (pfd) *pfd = vfd;
	return len;
}

int swDefinitionFile::open_parser_common(int ifd, int ofd, int filesize) {
	char  typestring[20];
  
	swstructdef::return_entry_keyword(typestring, get_type());
	parserM = new swparser(ifd, typestring, ofd);
	parserM->set_inputfd(ifd);   // Sets persitence on the output_fd.
	parserM->set_outputfd(ofd);  // Sets persitence on the output fd.
	if (filesize >= 0) {
		uxfio_fcntl(ifd, UXFIO_F_SET_VEOF, filesize);
	}
	return 0;
}
