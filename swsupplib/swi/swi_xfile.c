/* swi_xfile.c -- Fileset/Dfiles/Pfiles Object

   Copyright (C) 2005 Jim Lowe
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS:
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "swheader.h"
#include "swheaderline.h"
#include "atomicio.h"
#include "ls_list.h"
#include "swevents.h"
#include "swi_common.h"
#include "swi_base.h"
#include "swi_xfile.h"
#include "debug_config.h"


SWI_FILE_MEMBER * 
swi_xfile_construct_file_member(SWI_XFILE * xfile, char * name, int fd, SWVARFS * swvarfs)
{
	char * data;
	char * newdata;
	int mem_fd;
	int len;
	SWI_FILE_MEMBER * s;
	E_DEBUG2("name=[%s]", name);
	s = swi_file_member_create();

	mem_fd = swlib_open_memfd();
	swi_com_assert_value(mem_fd >= 0, __FILE__, __LINE__);

	s->baseM.is_activeM = 0;
	s->header_start_offsetM = swvarfs->current_header_offsetM; 

	s->data_start_offsetM = swvarfs->current_data_offsetM;
	swlib_pipe_pump(mem_fd, fd);
	s->data_end_offsetM = uxfio_lseek(mem_fd, 0L, SEEK_CUR) + s->data_start_offsetM;
	data = swi_com_get_fd_mem(mem_fd, &len);

	if (len != (s->data_end_offsetM - s->data_start_offsetM)) {
		/*
		 * Sanity check.
		 */
		SWI_internal_error();
	}

	s->pathnameM = strdup(name);
	s->lenM = len;
	newdata = (char*)malloc(len + 1);
	swi_com_assert_pointer((void*)newdata, __FILE__, __LINE__);
	*(newdata + len) = '\0'; 
	memcpy(newdata, data, len+1);
	s->dataM = newdata;
	E_DEBUG2("data=[%s]", (char*)(s->dataM));
	uxfio_close(mem_fd);
	return s;
}

void
swi_xfile_delete(SWI_XFILE * s)
{
	int index = 0;
	SWI_FILE_MEMBER * file;

	if (s->baseM.b_tagM) free(s->baseM.b_tagM);
	if (s->package_pathM) free(s->package_pathM);
	if (s->control_dirM) free(s->control_dirM);
	if (s->swi_scM) swi_scripts_delete(s->swi_scM);
	if (s->info_headerM) 
		swheader_close(s->info_headerM);
	if (s->swdef_fdM > 0) uxfio_close(s->swdef_fdM);
	s->swdef_fdM = -1;
	file = (SWI_FILE_MEMBER*)cplob_val(s->archive_filesM, index++);
	while (file) {
		swi_file_member_delete(file);
		file = (SWI_FILE_MEMBER*)cplob_val(s->archive_filesM, index++);
	}
	free((void*)(cplob_release(s->archive_filesM)));	
	free(s);
}

SWI_XFILE *
swi_xfile_create(int type, SWHEADER * global_index_header, SWPATH_EX * current)
{
	SWI_XFILE * s = (SWI_XFILE *)malloc(sizeof(SWI_XFILE));
	E_DEBUG("");
	swi_com_assert_pointer((void*)s, __FILE__, __LINE__);
	#ifdef SWSWINEEDDEBUG
		{
		STROB * tmp = strob_open(100);
		fprintf(stderr, "type=%d %s", type,
			swpath_ex_print(current, tmp, "swi_xfile_create"));	
		strob_close(tmp);
		}
	#endif

	if (type == SWI_XFILE_TYPE_FILESET) {
		swi_vbase_init(s, SWI_I_TYPE_XFILE, global_index_header, current);
	} else {
		swi_vbase_init(s, SWI_I_TYPE_XFILE, global_index_header, NULL);
	}

	s->xfileM = (void*)s;
	s->typeM = type;

	s->package_pathM = NULL;
	s->control_dirM = NULL;
	s->swi_scM = swi_scripts_create();
	s->info_headerM = NULL;
	s->archive_filesM = cplob_open(1);
	s->swdef_fdM = -1;
	s->did_parse_def_fileM = 0;
	swi_xfile_set_state(s, SW_STATE_UNSET);
	s->is_selectedM = 1;

	return s;
}

int
swi_write_fileset(SWI_XFILE * xfile, int ofd)
{
	return 0;
}

void
swi_xfile_set_state(SWI_XFILE * s, char * state)
{
	strncpy(s->stateM, state, sizeof(s->stateM) -1);
	s->stateM[sizeof(s->stateM)-1] = '\0';
}

SWHEADER *
swi_xfile_get_infoheader(SWI_XFILE * xfile)
{
	return xfile->info_headerM;
}

SWI_FILE_MEMBER * 
swi_xfile_get_control_file_by_path(SWI_XFILE * xfile, char * path)
{
	int index = 0;
	SWI_FILE_MEMBER ** files;

	E_DEBUG("Searching non-ieee control scripts");
	files = (SWI_FILE_MEMBER **)cplob_get_list(xfile->archive_filesM);
	while(*files && index < SWI_MAX_OBJ) {
		if (swlib_basename_compare(path, (*files)->pathnameM) == 0) {
			E_DEBUG2("found one, returning %p", (void*)(*files));
			return (*files);
		}
		index++;
	}
	E_DEBUG("returning NULL");
	return NULL;
}

SWI_CONTROL_SCRIPT * 
swi_xfile_get_control_script_by_path(SWI_XFILE * xfile, char * path)
{
	int index = 0;
	SWI_SCRIPTS * sci = xfile->swi_scM;
	SWI_CONTROL_SCRIPT ** scripts = sci->swi_coM;
	SWI_FILE_MEMBER ** files;

	/*
	 * search control scripts
	 */
	
	E_DEBUG("ENTERING");
	E_DEBUG("Searching IEEE control scripts");
	while(*scripts && index < SWI_MAX_OBJ) {
		if (swlib_basename_compare(path, (*scripts)->afileM->pathnameM) == 0) {
			E_DEBUG2("found one, returning %p", (void*)((*scripts)->afileM));
			return (*scripts);
		}
		index++;
		scripts++;
	}

	E_DEBUG("returning NULL");
	return NULL;
}

SWI_CONTROL_SCRIPT * 
swi_xfile_get_control_script_by_id(SWI_XFILE * xfile, int id)
{
	int index = 0;
	SWI_SCRIPTS * sci = xfile->swi_scM;
	SWI_CONTROL_SCRIPT ** scripts = sci->swi_coM;
	while(*scripts && index < SWI_MAX_OBJ) {
		if (id == (*scripts)->sidM) {
			E_DEBUG2("found one, returning %p", (void*)(*scripts));
			return (*scripts);
		}
		index++;
		scripts++;
	}
	E_DEBUG("returning NULL");
	return NULL;
}

SWI_CONTROL_SCRIPT * 
swi_xfile_get_control_script_by_tag(SWI_XFILE * xfile, char * tag)
{
	int index = 0;
	SWI_SCRIPTS * sci = xfile->swi_scM;
	SWI_CONTROL_SCRIPT ** scripts = sci->swi_coM;
	
	E_DEBUG("ENTERING");
	while(*scripts && index < SWI_MAX_OBJ) {
		if (strcmp(tag, (*scripts)->baseM.b_tagM) == 0) {
			E_DEBUG2("found one, returning %p", (void*)(*scripts));
			return (*scripts);
		}
		index++;
		scripts++;
	}
	E_DEBUG("returning NULL");
	return NULL;
}

int
swi_examine_signature_blocks(SWI_XFILE * dfiles,
		int * sig_block_start, int * sig_block_end)
{
	/*
	 * Look for signature files to determine the total
	 * length
	 */
	int index;
	char * f;
	SWI_FILE_MEMBER * s;
	CPLOB * cplob = dfiles->archive_filesM;

	*sig_block_start = -1;
	*sig_block_end = -1;
	index = 0;
	s = (SWI_FILE_MEMBER*)cplob_val(cplob, index);
	while (s) {
		f = strstr(s->pathnameM, "/" SW_A_signature);
		if (f && *(f+10) == '\0') {
			if (*sig_block_start == -1) *sig_block_start = s->header_start_offsetM;
			*sig_block_end = s->data_start_offsetM + s->lenM;
		}
		index++;
		s = (SWI_FILE_MEMBER*)cplob_val(cplob, index);
	}
	return 0;
}

int
swi_xfile_has_posix_control_file(SWI_XFILE * xfile, char * tag)
{
	SWI_CONTROL_SCRIPT * ret; 
	ret = swi_xfile_get_control_script_by_tag(xfile, tag);
	return ret ? 1 : 0;
}
