/* swextopt.h - sw exteneded options
  
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


#ifndef swextopt_200212j_h
#define swextopt_200212j_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usgetopt.h"
#include "strob.h"

/* #define 	SWBISLIBDIR "/usr/lib" This now comes from the Makefile */

#define SW_E_TRUE "True"
#define SW_E_FALSE "False"

#define SYSTEM_DEFAULTS_FILE 		SWBISLIBDIR "/swbis/swdefaults"
#define SYSTEM_SWBISDEFAULTS_FILE 	SWBISLIBDIR "/swbis/swbisdefaults"

struct extendedOptions {
		char is_boolM;
		char util_setM;
		char option_setM;
		char * optionNameM;
		char * defaultValueM;
		char * valueM;
		int app_flags;
};

#define SWC_U_ASK		SWC_U_A
#define SWC_U_COPY		SWC_U_C 
#define SWC_U_INSTALL		SWC_U_I 
#define SWC_U_CONFIG		SWC_U_G
#define SWC_U_LIST		SWC_U_L 
#define SWC_U_MODICY		SWC_U_M 
#define SWC_U_PACKAGE		SWC_U_P 
#define SWC_U_REMOVE		SWC_U_R 
#define SWC_U_VERIFY		SWC_U_V 
#define SWC_U_NOTAPPLICABLE	SWC_U_X 

#define SWC_U_A  (1 << 0) 	/* applicability_flags   swask     */
#define SWC_U_C  (1 << 1) 	/* applicability_flags   swcopy    */
#define SWC_U_I  (1 << 2) 	/* applicability_flags   swinstall */
#define SWC_U_G  (1 << 3) 	/* applicability_flags   swconfig  */
#define SWC_U_L  (1 << 4) 	/* applicability_flags   swlist    */
#define SWC_U_M  (1 << 5) 	/* applicability_flags   swmodify  */
#define SWC_U_P  (1 << 6) 	/* applicability_flags   swpackage */
#define SWC_U_R  (1 << 7) 	/* applicability_flags   swremove  */
#define SWC_U_V  (1 << 8) 	/* applicability_flags   swverify  */
#define SWC_U_X  (1 << 9) 	/* applicability_flags   not applicable  */

#define FILE_URL	"file://"


enum eOpts {	/* NOTE: the order must match the optionsArray 
		  found in swsupplib/misc/swcommon_options.h */
	SW_E_allow_downdate,
	SW_E_allow_incompatible,
	SW_E_allow_multiple_versions,
	SW_E_ask,
	SW_E_autoreboot,
	SW_E_autorecover,
	SW_E_autoselect_dependencies,
	SW_E_autoselect_dependents,
	SW_E_check_contents,
	SW_E_check_permissions,
	SW_E_check_requisites,
	SW_E_check_scripts,
	SW_E_check_volatile,
	SW_E_compress_files,
	SW_E_compression_type,
	SW_E_defer_configure,
	SW_E_distribution_source_directory,
	SW_E_distribution_target_directory,
	SW_E_distribution_target_serial,
	SW_E_enforce_dependencies,
	SW_E_enforce_dsa,
	SW_E_enforce_locatable,
	SW_E_enforce_scripts,
	SW_E_files,
	SW_E_follow_symlinks,
	SW_E_installed_software_catalog,
	SW_E_logfile,
	SW_E_loglevel,
	SW_E_media_capacity,
	SW_E_media_type,
	SW_E_psf_source_file,
	SW_E_one_liner,
	SW_E_reconfigure,
	SW_E_recopy,
	SW_E_reinstall,
	SW_E_select_local,
	SW_E_software,
	SW_E_targets,
	SW_E_uncompress_files,
	SW_E_verbose,
	SW_E_swbis_cksum,
	SW_E_swbis_file_digests,
	SW_E_swbis_file_digests_sha2,
	SW_E_swbis_files,
	SW_E_swbis_signer_pgm,
	SW_E_swbis_sign,
	SW_E_swbis_archive_digests,
	SW_E_swbis_archive_digests_sha2,
	SW_E_swbis_gpg_name,
	SW_E_swbis_gpg_path,
	SW_E_swbis_gzip,
	SW_E_swbis_bzip2,
	SW_E_swbis_numeric_owner,
	SW_E_swbis_absolute_names,
	SW_E_swbis_format,
	SW_E_swbis_no_analysis_phase,
	SW_E_swbis_archive_reader,
	SW_E_swbis_no_audit,
	SW_E_swbis_local_pax_write_command,
	SW_E_swbis_remote_pax_write_command,
	SW_E_swbis_local_pax_read_command,
	SW_E_swbis_remote_pax_read_command,
	SW_E_swbis_local_pax_remove_command,
	SW_E_swbis_remote_pax_remove_command,
	SW_E_swbis_quiet_progress_bar,
	SW_E_swbis_no_remote_kill,
	SW_E_swbis_no_getconf,
	SW_E_swbis_shell_command,
	SW_E_swbis_enforce_file_md5,
	SW_E_swbis_allow_rpm,
	SW_E_swbis_remote_shell_client,
	SW_E_swbis_install_volatile,
	SW_E_swbis_volatile_newname,
	SW_E_swbis_any_format,
	SW_E_swbis_forward_agent,
	SW_E_swbis_ignore_scripts,
	SW_E_swbis_sig_level,
	SW_E_swbis_enforce_all_signatures,
	SW_E_swbis_remove_volatile,
	SW_E_swbis_check_mtime,
	SW_E_swbis_check_duplicates,
	SW_E_swbis_check_owners,
	SW_E_swbis_replacefiles,
	SW_E_swbis_last_option
	};  /* NOTE: the order must match the optionsArray
		 found in swsupplib/misc/swcommon_options.h*/

void debug_writeBooleanExtendedOptions(int ofd, struct extendedOptions * opta);
void swextopt_write_session_options(STROB * buf, struct extendedOptions * opta, int SWC_FLAG);
int swextopt_is_value_true(char * s);
int swextopt_is_value_false(char * s);
int swextopt_is_option_true(enum eOpts nopt, struct extendedOptions * options);
int swextopt_is_option_set(enum eOpts nopt, struct extendedOptions * options);
int parseDefaultsFile(char * utility_name, char * defaults_filename, struct extendedOptions * options, int doPreserveOptions);
int initExtendedOption(void);
char * getLongOptionNameFromValue(struct option * arr, int val);
int getEnumFromName(char * optionname, struct extendedOptions * peop);
int setExtendedOption(char * optionname, char * value, struct extendedOptions * peop, int optSet, int is_util_option);
int swextopt_writeExtendedOptions(int ofd, struct extendedOptions * eop, int SWC_FLAG);
void swextopt_writeExtendedOptions_strob(STROB * tmp, struct extendedOptions * eop, int SWC_FLAG, int do_shell_protect);
char * getExtendedOption(char * optionname, struct extendedOptions * peop, int * pis_set, int * isOptSet, int *isUtilSet);
void set_opta(struct extendedOptions * opta, enum eOpts nopt, char * value);
void set_opta_initial(struct extendedOptions * opta, enum eOpts nopt, char * value);
char * swbisoption_get_opta(struct extendedOptions * opta, enum eOpts nopt);
char * get_opta(struct extendedOptions * opta, enum eOpts nopt);
char * get_opta_isc(struct extendedOptions * opta, enum eOpts nopt);
int parse_options_file(struct extendedOptions * opta, char * filename, char * util_name);
char * initialize_options_files_list(char * usethis);
int swextopt_parse_options_files(struct extendedOptions * opta, char * option_files, char * util_name, int reqd, int show_only);
int swextopt_combine_directory(STROB * result, char * soc_spec, char * directory);
int swextopt_get_status(void);
void set_opta_boolean(struct extendedOptions * opta, enum eOpts nopt, char * value);
#endif
