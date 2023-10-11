/* swdefinitionfile.h
 *
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


#ifndef swdefinitionfile_19980901jhl_h
#define swdefinitionfile_19980901jhl_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <tar.h>
#include "swstructdef.h"
#include "swdefinition.h"
#include "swcatalogfile.h"
#include "swattribute.h"
#include "swptrlist.h"
#include "swparser.h"
#include "swfilemap.h"
extern "C" {
#include "swutilname.h"
}


class swDefinitionFile: public swCatalogFile {
	int parserOutputLenM;
	swparser * parserM;
       	swPtrList<swDefinition> * swdefinition_listM;  
	int did_setM;
	static swFileMap * swfilemapM;
	
	swDefinition * currentM;	// Used by the iterator.
	swDefinition * headM;
	swDefinition * tailM;
	STROB * sbufM;
public:
	swDefinitionFile (char * path, char * source): swCatalogFile(path, source){ init(); }

	swDefinitionFile (char * path): swCatalogFile(path, path){ init(); }

	virtual ~swDefinitionFile (void) { 
		if (did_setM == 0) delete swdefinition_listM;
		::strob_close(sbufM); 
	}

	int open_parser(int ifd, int ofd) {
		return open_parser_common(ifd, ofd, -1);
	}

	int open_parser(int ifd) {
		return open_parser_common(ifd, get_mem_fd(), -1);
	}

	int open_parser(swPackageFile * package) {
		return open_parser_common
			(package->xFormat_get_ifd(), get_mem_fd(), package->xFormat_get_filesize());
	}
       	
	swPtrList<swDefinition> * get_swdefinition_list(void)  {
       		return  swdefinition_listM;  
	}
	
	void set_swdefinition_list(swPtrList<swDefinition> * list)  {
		did_setM = 1;
       		swdefinition_listM  = list; 
	}
	
	swparser * getParserObject(void) {
		return parserM;
	}

	int open_parser(char * filename) {
		struct stat st;
		int fd; 
		if (! ::strcmp (filename,"-"))  {
			fd=STDIN_FILENO;
		} else { 
			fd = open (filename, O_RDONLY );
		}
		if (fd <  0) return -1;
		if ( (fstat(fd, &st) < 0) || !S_ISREG(st.st_mode)) {
			close(fd);
			fprintf(stderr,"file not regular file: %s\n", filename);
			return -2;
		}
		return open_parser_common(fd, get_mem_fd(), -1);
	}

	//
	// *Depricated*
	int run_parser(void) {
		return run_parser_(0, 0, (int*)NULL);
	}
	
	//
	// *Depricated*
	int run_parser(int atlevel, int mark_up_flag) {
		return run_parser_(atlevel, mark_up_flag, (int*)(NULL));
	}

	int run_parser(int atLevel, int markUpFlag, int * swFileMap_index_up) {
		return run_parser_(atLevel, markUpFlag, swFileMap_index_up);
	}
	
	void close_parser() {
		delete parserM;
		parserM = NULL;
	}

	void swdeffile_list_add(swDefinition * node){
		if (swlib_get_verbose_level() > SWPACKAGE_VERBOSE_V3)
		{
			int efd = STDERR_FILENO;  // FIXME
			swMetaData * p;
			int eret, ret = 0;
			int type = node->get_type();
			if (type == swstructdef::sdf_object_kw) {
				// Write the object
				ret = node->swAttribute::write_fd_debug(efd, "swpackage: swdeffile_list_add: ");
				p = node->get_next_node();
				while (p) {
					eret=p->write_fd_debug(efd, "swpackage: swdeffile_list_add: ");
					if (eret > 0)
						ret += eret;
					else 
						return;
					p=p->get_next_node();
				}
			} else {
				fprintf(stderr,"%s: unexpected result in swDefinition::write_fd_debug type = %d %s.\n",
					swlib_utilname_get(), (int)(node->get_type()), (node->get_parserline()));
					return;
			}
		}
		swdefinition_listM->list_add(node);
	}

	int swdeffile_list_del(int i){
		return swdefinition_listM->list_del(i);
	}

	int swdeffile_list_del(swDefinition* node){
		return swdefinition_listM->list_del(node);
	}

	int swdeffile_get_index_from_pointer(swDefinition * p){
		return swdefinition_listM->get_index_from_pointer(p);
	}
	
	int swdeffile_get_definition_list_length() {
		return swdefinition_listM->length();
	}

	swDefinition * swdeffile_get_pointer_from_index(int index){
		return swdefinition_listM->get_pointer_from_index(index);
	}

	//
	// Linked list Interface.
	// ----------------------------------------------------------------
	//
	
	static char * swdeffile_linki_nextline(void * the_this_object, int * current_offset, int peek);
	//static int swdeffile_linki_write_fd(swDefinitionFile * swdef, int ofd);
	virtual int swdeffile_linki_write_fd(int ofd);
	int swdeffile_linki_write_fd_debug(int ofd, char * prefix);
	swDefinition * find_same_prior_file(swDefinition * newswdef, int start_index);
	
	swMetaData *
	swdeffile_linki_find_by_offset(swDefinition * relhead, int offset)
	{
		swMetaData * ret;
		ret = swdeffile_linki_find_n(relhead, offset, 
			&swMetaData::get_p_offset, //int (swMetaData::*fp_get_n)(void), 
			&swDefinition::find_by_p_offset //swMetaData *(swDefinition::*fp_find_n)(int)
			);
		return ret;
	}
	
	swMetaData *
	swdeffile_linki_find_by_ino(swDefinition * relhead, int inode)
	{
		int ino;
		swDefinition *p;
		swMetaData *ret;

		if (!relhead) relhead = headM;
		p = relhead;

		if (inode == 0) return headM;

		while (p) {
			ino = p->get_ino();
			if (ino == inode) {
				currentM = p;
				return p;
			}
			if ((ret = p->find_by_ino(inode))) {
				currentM = p;
				return ret;
			}
			p = p->get_next();
		}
		return static_cast<swMetaData*>(NULL);
	}

	swMetaData *
	swdeffile_linki_find_by_parserline(swDefinition * relhead, char * keyline)
	{
		char * line;
		swDefinition *p;
		swMetaData *ret;
		int do_fine_grained;	
	
		do_fine_grained = (*swheaderline_get_type_pointer(keyline) != SWPARSE_MD_TYPE_OBJ);

		if (!relhead) relhead = headM;
		p = relhead;
	
		while (p) {
			line = p->get_parserline();				
			if (line == keyline) {
				currentM = p;
				return p;
			}
			if (do_fine_grained) {
				if ((ret = p->find_by_parserline(keyline))) {
					currentM = p;
					return ret;
				}
			}
			p = p->get_next();
		}
		return static_cast<swMetaData*>(NULL);
	}


	void swdeffile_linki_set_head(swDefinition * head) {
		headM = head;
	}

	void swdeffile_linki_set_tail(swDefinition * tail) {
		tailM = tail;
	}
	
	swDefinition *  swdeffile_linki_get_head(void) {
		return headM;
	}

	int
	swdeffile_linki_init(void)
	{
		swDefinition * swdef;
		swDefinition * oldswdef = NULL;
		int index = 0;
		
		swdef = swdefinition_listM->get_pointer_from_index(index);

		if (swdef) swdef->setup_contained_by();
		swdeffile_linki_set_head(swdef);
		if (headM) headM->set_prev(NULL);
		while (swdef) {
			swdef->setup_contained_by();
			swdef->set_prev(oldswdef);
			oldswdef = swdef;
			swdef = swdefinition_listM->get_pointer_from_index(++index);
			oldswdef->set_next(swdef);
		}
		swdeffile_linki_set_tail(oldswdef);
		return 0;
	}

	int 
	swdeffile_linki_insert_after(swDefinition * parent, swDefinition * w)
	{
		swDefinition * next;
		swdeffile_list_add(w);
		if (parent) {	
			next = parent->get_next();

			w->set_next(next);
			w->set_prev(parent);
	
			next->set_prev(w);
			parent->set_next(w);
		} else {
			w->set_prev(NULL);
			w->set_next(headM);
			headM->set_prev(w);
			swdeffile_linki_set_head(w);
		}
		return 0;
	}
	
	int 
	swdeffile_linki_append(swDefinition * w) {
		swdeffile_list_add(w);

		if (headM == NULL) {
			w->set_prev(NULL);
			w->set_next(NULL);
			swdeffile_linki_set_head(w);
		} else {
			if (tailM) tailM->set_next(w);
			w->set_prev(tailM);
			w->set_next(NULL);
		}
		tailM = w;	
		return 0;
	}
	
	int 
	swdeffile_linki_preppend(swDefinition * w) {

		swdeffile_list_add(w);

		if (headM == NULL) {
			w->set_prev(NULL);
			w->set_next(NULL);
			tailM = w;
		} else {
			w->set_prev(NULL);
			w->set_next(headM);
		}
		swdeffile_linki_set_head(w);
		return 0;
	}

	//
	// Get the object after the object with inode (get_ino()) == *inode.
	// *inode == 0 is a special case. Zero (0) being a name for the first
	// object.  The parser line is returned and (*inode) is to the get_ino()
	// value of the returned object.
	//
	char * 
	swdeffile_linki_get_nextline(int * inode, int peek_only)
	{
		char * line;
		swMetaData * ret = static_cast<swMetaData*>(NULL);
		swMetaData * next_object = static_cast<swMetaData*>(NULL);

		if (headM == static_cast<swDefinition*>(NULL)) {
			*inode = 0;
			return NULL;
		}

		if (currentM != static_cast<swDefinition*>(NULL)) {
			//if (currentM->get_ino() == *inode) {
				ret = swdeffile_linki_find_by_ino(currentM, *inode);
			//}
		}

		if (ret == static_cast<swMetaData*>(NULL)) {	
			ret = swdeffile_linki_find_by_ino(headM, *inode);
		}

		if (ret) {
			if (*inode != 0) {
				//
				// Get the next object after the current
				// object (given by *inode).
				//
				next_object = ret->get_next_node();
			} else {
				//
				// Special, return the first object.
				// *inode == 0
				//
				next_object = ret;
			}

			if (next_object != static_cast<swMetaData*>(NULL)) {
				ret = next_object;
			} else {
				//
				// Normal. last attribute in a definition.
				//
				// Now must check next definition.
				//
				swDefinition * this_definition = static_cast<swDefinition*>(ret->get_contained_by()); // FIXME: DownCast
				if (this_definition) {
					ret = this_definition->get_next(); // Next definition in linked list.
				} else {
					if (ret->get_type() == swstructdef::sdf_object_kw) {
						//
						// Definition with no attributes.
						//
						ret = (static_cast<swDefinition*>(ret))->get_next();     // XXEXP 2003-01-19
					} else {	
						//
						// end of swdefinitionfile
						//
						ret = NULL;
					}
				}
			}
		}
		if (ret) {
			if (!peek_only) {
				*inode = ret->get_ino();
			}
			line = ret->get_parserline();
			return line;
		} else {
			return static_cast<char*>(NULL);	
		}
	}

	//
	// End of Linked list Interface.
	// ----------------------------------------------------------------
	//

	int xFormat_write_file_by_func(int(swDefinitionFile::*fout)(int)) {
		XFORMAT * xux;
	        struct  new_cpio_header * hdr0;
		intmax_t ret=0;
		intmax_t ret1;

	        hdr0 = xFormat_vfile_hdr();
		xux = xFormat_get_xformat();
		ret1 = xFormat_write_header();
		if (ret1 < 0) return ret1;
		ret += ret1;

		ret1 = swDefinitionFile::write_archive_member_data
				(hdr0, xux->ofdM, (int(swDefinitionFile::*)(int))fout, xux->output_format_codeM);
		if (ret1 < 0) return -ret;
		ret += ret1;
	
		xux->bytes_writtenM += ret1;
	        return ret;
	}

	virtual intmax_t xFormat_write_file(int (swDefinitionFile::*fout)(int)) {
		int nullfd;
		intmax_t ret;
		int source_fd;
		int filesize;
		char * name;

		nullfd = UXFIO_NULL_FD;
		name = swfile_get_package_filename();	
		source_fd = swfile_open_public_image_fd();
		
		filesize = (this->*fout)(nullfd);
		if (filesize < 0) {
			return filesize;
		}

		xFormat_set_filesize(filesize);
		xFormat_set_filetype_from_tartype(REGTYPE);
		xFormat_set_name(swfile_get_package_filename());

		ret = swDefinitionFile::xFormat_write_file_by_func(fout);
		return ret;
	}

	virtual int write_fd(int fd);
	virtual int write_fd_debug(int fd, char * prefix);
	
	void list_add (swMetaData * node) {
		swdeffile_list_add(static_cast<swDefinition*>(node));
	}
	virtual swDefinition * makeDefinition (char * parser_line, int * retvalp);

	int generateDefinitions(void);
	int get_parser_output_len(void){return parserOutputLenM;}
	int get_next_line_offset(void) { return get_next_line_offset(-1); }
	int get_next_line_offset(int swfilemapfd);

	static char * doExpandFileReference(char * attribute_value, off_t * length);
	int swFileMapPush(int off, int len) { return swfilemapM->swFileMapPush(off, len); }
	int swFileMapPop(void) {
		int a, b, c;
		return swFileMapPop(&a, &b, &c);
	}
	int swFileMapPop(int *off, int * coff, int *len) { return swfilemapM->swFileMapPop(off, coff, len); }
	int swFileMapPeek(int *off, int * coff, int *len) { return swfilemapM->swFileMapPeek(off, coff, len); }
	int swFileMapPeekByIndex(int swmapindex, int *off, int * coff, int *len) { 
		return swfilemapM->swFileMapPeekByIndex(swmapindex, off, coff, len); 
	}
	int swFileMapSetCurrentOffset(int offset) { return swfilemapM->swFileMapSetCurrentOffset(offset); }
	int swFileMapReset(void) { return swfilemapM->swFileMapReset(); }
        int determine_status_for_writing(swDefinition * swmd);

private:

	intmax_t write_archive_member_data(struct new_cpio_header *file_hdr0,
		      		int out_file_des, int (swDefinitionFile::*fout)(int), enum archive_format archive_format_out)
	{
		intmax_t retval = 0;
		intmax_t ret = 0;

		if (( ((file_hdr0->c_mode & CP_IFMT) == CP_IFREG))   //  && (tmpp == (char*)NULL || !strlen(tmpp)))
		) {
			ret = (intmax_t)(this->*fout)(out_file_des);
			retval = ret;
			if (ret < 0) {
				fprintf(stderr, "error in swDefinitionFile::write_archive_member_data\n");
			} else {
				if (ret != (intmax_t)(file_hdr0->c_filesize)) {
					fprintf(stderr, "ASSERTION FAILED: swDefinitionFile %s != ",
						::swlib_imaxtostr(ret, NULL));
					fprintf(stderr, "%s\n", swlib_imaxtostr(file_hdr0->c_filesize, NULL));
					exit(33);
				}
				ret = taru_tape_pad_output(out_file_des, file_hdr0->c_filesize, archive_format_out);
				if (ret >= 0) {
					retval += ret;
				} else {
					fprintf(stderr, "error: taru_tape_pad_output\n");
				}
			}
		}
		//fprintf(stderr, "write archive_member_data  ret = %d\n", retval);
		return retval;
	}


	swMetaData *
	swdeffile_linki_find_n(swDefinition * relhead, int offset, 
		int (swMetaData::*fp_get_n)(void), 
		swMetaData *(swDefinition::*fp_find_n)(int))
	{
		int ino;
		swDefinition *p;
		swMetaData *ret;
		int (swMetaData::*get_n)(void) = fp_get_n;
		swMetaData *(swDefinition::*find_n)(int) = fp_find_n;


		if (!relhead) relhead = headM;
		p = relhead;

		if (offset == 0) return headM;

		while (p) {
			ino = (p->*get_n)();
			if (ino == offset) {
				currentM = p;
				return p;
			}
			if ((ret = ((p->*find_n)(offset)))) {
				currentM = p;
				return ret;
			}
			p = p->get_next();
		}
		return static_cast<swMetaData*>(NULL);
	}


    void init(void);
    int merge_duplicates(swDefinition * swmd);
    int run_parser_(int atlevel, int mark_up_flag, int * memfileindex_up);
    int open_parser_common (int ifd, int ofd, int filesize);
};
#endif
