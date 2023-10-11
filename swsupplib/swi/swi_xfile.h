/*  swi.h -  Fileset/Dfiles/Pfile object
 
 * COPYING TERMS AND CONDITIONS:
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef swi_xfile_h_20041223
#define swi_xfile_h_20041223

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "uxfio.h"
#include "strob.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swicol.h"
#include "swi_common.h"
#include "swi_afile.h"

#define SWI_XFILE_TYPE_FILESET		0	/* type: 0 fileset; 1 dfiles; 2 pfiles  */
#define SWI_XFILE_TYPE_DFILES		1
#define SWI_XFILE_TYPE_PFILES		2

typedef struct {		/* The fileset/pfiles/dfiles  object	*/
	SWI_BASE baseM;
	void * xfileM;		/* (SWI_XFILE*) points to this		*/
	char * package_pathM;	/* The package path for this object	*/
	char * control_dirM;	/* the fileset control dir		*/
	int typeM;		/* type: 0 fileset; 1 dfiles; 2 pfiles  */
	char stateM[12];	/* fileset state 			*/
	SWI_SCRIPTS * swi_scM;	/* control scripts 			*/
	SWHEADER * info_headerM; /* INFO file access object		*/
	int INFO_header_indexM;	/* INFO access object index             */
	CPLOB * archive_filesM;	/* List of pointers to SWI_FILE_MEMBERs */
	int swdef_fdM;		/* INFO file uxfio fd			*/
	int did_parse_def_fileM;/* did parse INFO for this object 	*/
	int is_selectedM;	/* is selected, applies to filesets	*/
	int INDEX_ordinalM;	/* Order in INDEX file.  0 is first	*/
} SWI_XFILE;			/* Is a dfile, pfile, of fileset object */

SWI_XFILE * swi_xfile_create(int type, SWHEADER * swheader, SWPATH_EX *);

void swi_xfile_delete(SWI_XFILE * s);

SWI_FILE_MEMBER * swi_xfile_contstruct_file_member(SWI_XFILE * xfile, char * name, int fd);

int swi_examine_signature_blocks(SWI_XFILE * dfiles, int * sig_block_start, int * sig_block_end);

SWI_FILE_MEMBER * swi_xfile_get_control_file_by_path(SWI_XFILE * xfile, char * path);

SWI_CONTROL_SCRIPT * swi_xfile_get_control_script_by_path(SWI_XFILE * xfile, char * path);

SWI_CONTROL_SCRIPT * swi_xfile_get_control_script_by_tag(SWI_XFILE * xfile, char * tag);
SWI_CONTROL_SCRIPT * swi_xfile_get_control_script_by_id(SWI_XFILE * xfile, int id);

SWI_FILE_MEMBER * swi_xfile_contstruct_file_member(SWI_XFILE * xfile, char * name, int fd);

void swi_xfile_set_state(SWI_XFILE * s, char * state);

SWI_FILE_MEMBER * swi_xfile_construct_file_member(SWI_XFILE * xfile, char * name, int fd, SWVARFS * swvarfs);
int swi_xfile_has_posix_control_file(SWI_XFILE * xfile, char * tag);
SWHEADER * swi_xfile_get_infoheader(SWI_XFILE * xfile);

#endif
