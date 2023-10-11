/*  swinstall.h - Items common to swinstall.c and swinstall_lib.c 
   Copyright (C) 2004 James H. Lowe, Jr.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef swinstall_h_200402
#define swinstall_h_200402

#include "swuser_config.h"
#include "cplob.h"
#include "vplob.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "swssh.h"
#include "swevents.h"
#include "strar.h"
#include "swutilname.h"
#include "swi.h"
#include "swicat.h"
#include "swicat_e.h"
#include "swicol.h"
#include "globalblob.h"

#ifndef DEFAULT_PAX_W 
#define DEFAULT_PAX_W "pax"
#endif
#ifndef DEFAULT_PAX_R 
#define DEFAULT_PAX_R "pax"
#endif

#define SW_UNUSED_CATPATH "reserved/version/1.0"

typedef struct {
	char * tagM;			/* The script tag */
	char * tagspecM;		/* the tag portion of the sw spec: <prod_tag>.<fileset_tag> */
	SWI_CONTROL_SCRIPT * scriptM;   /* the SWI_* script object */
} SCARY;

int sw_main_get_stderr_fd(void);
int swinstall_write_target_install_script(GB * , int ofd, char * fp_targetpath, 
		STROB * control_message, 
		char * sourcepath,
		int delaytime,
		int keep_old_files,
		int nhops,
		int vlv,
		char * pax_read_command_key,
		char * hostname,
		char * blocksize,
		SWI_DISTDATA * distdata,
		char * installed_software_catalog,
		int opt_preview, char * sh_dash_s, int alt_catalog_root);

int swinstall_arfinstall(GB * , SWI * swi, int ofd,
	int * deadman, char * target_path, char * catalog_path,
	VPLOB * swspecs, int section, int altofd, int opt_preview,
	char * pax_read_command, int alt_catalog_root, int event_fd,
	struct extendedOptions * eopt, int keep_old_files, uintmax_t *, int allow_missing_files, SWICAT_E * up_e);

int swinstall_get_utsname_attributes(GB * , SWI * swi, int ofd, int event_fd);
int swinstall_lib_write_trailer_blocks(int ofd, int nblocks);
int swinstall_lib_determine_catalog_path(STROB * buf, SWI_DISTDATA * distdata,
		int product_number, char * isc_path, int instance_id);
int swinstall_analyze_overwrites(GB * G, SWI * swi, int overwrite_fd, int keep, int is_upgrade);

#endif
