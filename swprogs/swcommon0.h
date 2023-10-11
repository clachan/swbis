/* swcommon0.h - lowest level common header file
  
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


#ifndef swcommon0_200212j_h
#define swcommon0_200212j_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include "sw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DEFAULT_PAX_W 
#define DEFAULT_PAX_W "pax"
#endif
#ifndef DEFAULT_PAX_R 
#define DEFAULT_PAX_R "pax"
#endif
#ifndef DEFAULT_PAX_REM 
#define DEFAULT_PAX_REM "tar"
#endif

/* volatile  */  /* int global_lastsig; */

#define SWINSTALL_VOLATILE_SUFFIX	".swbisnew"
#define SWREMOVE_VOLATILE_SUFFIX	".swbisold"


#define PAX_READ_COMMANDS_LEN	10
#define PAX_WRITE_COMMANDS_LEN	10
#define SW_UTILNAME		"swutility"
#define REPORT_BUGS		"<bug-swbis@gnu.org>"
#define SWC_PID_ARRAY_LEN	30
#define SWC_TARGET_FD_ARRAY_LEN	10
#define SWC_SCRIPT_SLEEP_DELAY	0 /* Seconds to sleep a the end of the script */
				/* in order to help ssh not drop any bytes. */
				/* Using recent GNU/Linux releases this can be */
				/* set to zero '0' */

#define CMD_TAINTED_CHARS  "'|\"*?;&<>`$"
#define SW_FAILED_ALL_TARGETS		1
#define SW_FAILED_SOME_TARGETS		2

#define SWBIS_TARGET_CTL_MSG_125 	"125 target script started"
#define SWBIS_TARGET_CTL_MSG_128 	"128 catalog path determined"
#define SWBIS_TARGET_CTL_MSG_129 	"129 current directory"
#define SWBIS_TARGET_CTL_MSG_508 	"508 target script error"
#define SWBIS_SOURCE_CTL_ARCHIVE	"130 source is from archive"
#define SWBIS_SOURCE_CTL_DIRECTORY 	"140 source is from directory"
#define SWBIS_SOURCE_CTL_CLEANSH 	"150 source is from cleansh"

#define SWINSTALL_DO_SOURCE_CTL_MESSAGE		(swutil_x_mode && local_stdin == 0)
#define SWBIS_SWINSTALL_SOURCE_CTL_ARCHIVE	SWBIS_SOURCE_CTL_ARCHIVE
#define SWBIS_SWINSTALL_SOURCE_CTL_DIRECTORY 	SWBIS_SOURCE_CTL_DIRECTORY
#define SWBIS_SWINSTALL_SOURCE_CTL_CLEANSH	SWBIS_SOURCE_CTL_CLEANSH


#define SWCOPY_DO_SOURCE_CTL_MESSAGE		(swutil_x_mode && local_stdin == 0)
#define SWBIS_SWCOPY_SOURCE_CTL_ARCHIVE		SWBIS_SOURCE_CTL_ARCHIVE
#define SWBIS_SWCOPY_SOURCE_CTL_DIRECTORY 	SWBIS_SOURCE_CTL_DIRECTORY


/* These are the various task script.  Task scripts
   are shell scripts handled by the routines of
   swsupplib/misc/swicol.c */

#define SWBIS_TS_Load_signatures        "Load signatures"
#define SWBIS_TS_Load_fileset           "load fileset"
#define SWBIS_TS_Load_catalog           "Load catalog and attributes"
#define SWBIS_TS_Load_INSTALLED         "Load INSTALLED file"
#define SWBIS_TS_Load_control_sh        "load " SW_A_CONTROL_SH
#define SWBIS_TS_Load_session_options   "load session options"
#define SWBIS_TS_Get_catalog_selections   "Get catalog selections"
#define SWBIS_TS_Abort  		"Programmed Abort" /* Do nothing unsucessfully */
#define SWBIS_TS_Do_nothing		"Do nothing" /* Do nothing sucessfully */
#define SWBIS_TS_uts			"get utsnames"
#define SWBIS_TS_Get_iscs_listing	"Get catalog selections listing"
#define SWBIS_TS_Get_iscs_entry		"Get catalog entry"
#define SWBIS_TS_load_index_file_90     "Load INDEX file loc90"
#define SWBIS_TS_Load_INSTALLED_90      SWBIS_TS_Load_INSTALLED " location 90"
#define SWBIS_TS_Load_INSTALLED_91      SWBIS_TS_Load_INSTALLED " location 91"
#define SWBIS_TS_Load_INSTALLED_92      SWBIS_TS_Load_INSTALLED " location 92"
#define SWBIS_TS_Catalog_dir_remove     "Catalog directory removal" 
#define SWBIS_TS_Catalog_unpack         "unpack catalog.tar"
#define SWBIS_TS_Analysis_002         	"Run analysis phase 002"
#define SWBIS_TS_Analysis_001         	"Run analysis phase 001"
#define SWBIS_TS_make_catalog_dir	"Make catalog entry directory"
#define SWBIS_TS_preview_task		"Preview Task"
#define SWBIS_TS_make_live_INSTALLED	"Make INSTALLED status live"
#define SWBIS_TS_make_locked_session	"Lock Session"
#define SWBIS_TS_check_loop  		"Check Loop"
#define SWBIS_TS_continue		"Programmed Continue"
#define SWBIS_TS_report_status 		"Report Status"
#define SWBIS_TS_remove_catalog_entry	"Remove catalog entry"
#define SWBIS_TS_restore_catalog_entry	"Restore catalog entry"
#define SWBIS_TS_remove_files		"Remove files"
#define SWBIS_TS_retrieve_files_archive "Retrieve files archive"
#define SWBIS_TS_post_verify		"Run post Verify"
#define SWBIS_TS_get_catalog_perms	"Get catalog permissions"
#define SWBIS_TS_run_configure		"Run configure script"
#define SWBIS_TS_Load_INSTALLED_transient  SWBIS_TS_Load_INSTALLED " transient"
#define SWBIS_TS_Load_management_host_status "Load manangement host status"
#define SWBIS_TS_check_OVERWRITE	"Check overwrites"
#define SWBIS_TS_check_status		"Check Status"
#define SWBIS_TS_Get_prelink_filelist	"Get prelink file list"   /* swverify */




#define SWPARSE_AT_LEVEL	0
#define VERBOSE_LEVEL2		2
#define VERBOSE_LEVEL3		3
#define LCEXIT(arg)		swc_lc_exit(G, __FILE__, __LINE__, arg)
#define LC_RAISE(arg)		swc_lc_raise(G, __FILE__, __LINE__, arg)
#define LOCAL_RSH_BIN 		"rsh"
#define REMOTE_RSH_BIN 		"rsh"
#define LOCAL_SSH_BIN 		"ssh"
#define REMOTE_SSH_BIN 		"ssh"
#define SWC_PID_ARRAY_LEN		30
#define MAX_CONTROL_MESG_LEN		2000

struct g_pax_write_command {
	char * idM;
	char * commandM;
};

struct g_pax_read_command {
	char * idM;
	char * commandM;
	char * verbose_commandM;
	char * keep_commandM;
};

struct g_pax_remove_command {
	char * idM;
	char * commandM;
	char * verbose_commandM;
};

typedef struct {
	int codeM;
} ERRORCODE;

ERRORCODE * createErrorCode(void);
void destroyErrorCode(ERRORCODE * EC);
void swc_setErrorCode(ERRORCODE * EC, int code, char *);
int swc_getErrorCode(ERRORCODE * EC);

int swc0_set_arf_format(char * optarg, int * format_arf, int * do_oldgnutar,
	int * do_bsdpax3, int * do_oldgnuposix, int * do_gnutar, int * do_paxexthdr);
  
int swc0_process_w_option(STROB * tmp, CPLOB * w_arglist, char * i_optarg,
	int * w_argc_p);

void swc0_create_parser_buffer(void);

#endif
