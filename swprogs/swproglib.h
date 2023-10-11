/*  swproglib.h - 
 Copyright (C) 2006 James H. Lowe, Jr.

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

#ifndef swproglib_h_200609
#define swproglib_h_200609

#include "swuser_config.h"
#include "cplob.h"
#include "vplob.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "etar.h"
#include "swgp.h"
#include "swssh.h"
#include "swevents.h"
#include "strar.h"
#include "swutilname.h"
#include "swi.h"
#include "swicol.h"
#include "swicat.h"
#include "swgpg.h"
#include "swicat_e.h"
#include "globalblob.h"

#define D_ZERO		0
#define D_ONE		1
#define D_TWO		2
#define SWPL_PRODUCT_0	D_ONE
#define SWPL_FILESET_0	D_TWO
#define CISF_ID_PRODUCT	D_ONE
#define CISF_ID_FILESET	D_TWO

#define SCAR_ARRAY_LEN 20

#define SWP_RP_STATUS_NO_INTEGRITY	54
#define SWP_RP_STATUS_NO_GNU_TAR	55
#define SWP_RP_STATUS_NO_LOCK		56
#define SWP_RP_STATUS_NO_PERMISSION	57

#define SWLIST_PMODE_TEST1	"test1"  /* provides a scratch test area for distributions */
#define SWLIST_PMODE_DEP1	"dep1"   /* dependecy check format that is used internally 
					    by swinstall */
#define SWLIST_PMODE_PROD	"products"
#define SWLIST_PMODE_DIR	"directory"
#define SWLIST_PMODE_FILE	"files"
#define SWLIST_PMODE_CAT	"list_catalog"
#define SWLIST_PMODE_INDEX	"INDEX"   /* POSIX -v mode */
#define SWLIST_PMODE_ATT	"ATT"   /* POSIX -a mode */
#define SWLIST_PMODE_UTS_ATT	"UTS_ATT"   /* POSIX -a mode for uts attributes only */


/* CISFBA base array, this should eventually replace
   the array g_script_array[SCAR_ARRAY_LEN+1] of (*SCAR) objects */

typedef struct {
	VPLOB * base_arrayM; /* array of (CISF_BASE*) objects */
} CISFBA;

typedef struct {
	int typeidM;		/* D_TWO fileset , or D_ONE product */
	SWI_XFILE * ixfileM;	/* the fileset pointer from the (*SWI)swi object */
	SWHEADER_STATE * atM;	/* location in the INSTALLED (or INDEX)  *SWHEADER object */
	int cf_indexM;		/* the index j, in swi->swi_pkgM->swi_coM[i]->swi_coM[j] */
} CISF_BASE;

/* CISF_FILESET: this struct associates the location in INSTALLED (or INDEX)
   *SWHEADER object with the corresponding fileset object in (SWI*),
   namely the pointer held in swi->swi_pkgM->swi_coM[i]->swi_coM[ii]  */

typedef union /*struct*/ {
	CISF_BASE cisf_baseM;
} CISF_FILESET;

/* CISF_PRODUCT: this struct maps the location in INSTALLED with the product_index
   swi->swi_pkgM->swi_coM[i] and it also stores the objects.  It
   stores the list of CISF_FILESET pointers associated with this product */


			/* FIXME, this data structure assumes that 
			   there is only one product being operated on
			   (due to  CISFBA * cbaM of which there only
			    should be one */
typedef struct {
	CISF_BASE cisf_baseM;
	SWI * swiM;		/* Main object, just to have it easily accessable */
	SWI_PRODUCT * productM;	/* The installed product */
	VPLOB * isetsM;		/* USE: list of (*CISF_FILESET) pointers that are installed */
	CISFBA * cbaM;		/* USE: array of (CISF_BASE*) objects including this product
				     and *all* the filesets, wheather installed or selected
				    for swinstall, it contains the list of scripts that have
				   been run.
				   */
} CISF_PRODUCT;


/* The SCAR struct is deprecated, it will eventually be replaced
   with the CISF* data structures.  The CISF data structures model
   selections and installed software (as opposed to assuming all
   (or exactly one) filesets and exactly one product */

typedef struct {
	char * tagM;			/* The script tag: postinstall, preinstall, etc.  */
	char * tagspecM;		/* the tag portion of the sw spec: <prod_tag>.<fileset_tag> */
	SWI_CONTROL_SCRIPT * scriptM;   /* the SWI_* script object */
} SCAR;

void swpl_init_header_root(ETAR * etar);
SCAR ** swpl_get_script_array(void);
void 	swpl_scary_init_script_array(void);
SCAR * 	swpl_scary_create(char * tag, char * tagspec, SWI_CONTROL_SCRIPT * script);
void 	swpl_scary_delete(SCAR * scary);
SCAR * 	swpl_scary_add(SCAR ** array, SCAR * script);
void 	swpl_scary_show_array(SCAR ** array);
SWI_CONTROL_SCRIPT * swpl_scary_find_script(SCAR ** array, char * tag, char * tagspec);
void 	swpl_audit_execution_scripts(GB * G, SWI * swi, SCAR ** scary_scripts);
int 	swpl_write_case_block(SWI * swi, STROB * buf, char * tag);
uintmax_t swpl_get_whole_block_size(uintmax_t);
int 	swpl_write_session_options_filename(STROB * name, SWI * swi);
int 	swpl_write_session_options_file(STROB * buf, SWI * swi);
int swpl_write_single_file_tar_archive(SWI * swi, int ofd, char * name, char * data, AHS *);
int swpl_load_single_file_tar_archive (GB * G,
	SWI * swi,
	int ofd, 
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str,
	char * name,
	char * data, AHS *);

int swpl_write_tar_installed_software_index_file (GB * G, SWI * swi, int ofd,
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str, char * sw_a_installed, AHS *);

int swpl_construct_script_cases(GB * G, STROB * buf, SWI * swi, 
		SWI_CONTROL_SCRIPT * script, 
		char * tagspec,
		int error_event_value,
		int warning_event_value);

int swpl_construct_configure_script(GB * G, CISF_PRODUCT * cisf, STROB * buf, SWI * swi, int do_configure);

int swpl_construct_script(GB * G, CISF_PRODUCT * cisf,  STROB * buf, SWI * swi, char * script_tag);

int swpl_construct_analysis_script(GB * G, char * script_name, STROB * buf, SWI * swi, SWI_CONTROL_SCRIPT ** p_script);

int swpl_construct_controlsh_script(GB * G, STROB * buf, SWI * swi);

int swpl_write_tar_session_options_file(GB * G, SWI * swi, int ofd,
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str);

int swpl_compare_name(char * name1, char * name2, char * att, char * filename);

void swpl_safe_check_pathname(char * s);

void swpl_sanitize_pathname(char * s);

char * swpl_get_attribute(SWHEADER * header, char * att, int * len);

void swpl_enforce_one_prod_one_fileset(SWI * swi);

int swpl_does_have_prod_postinstall(SWI * swi);

int swpl_get_fileset_file_count(SWHEADER * infoheader);

unsigned long int swpl_get_delivery_size(SWI * swi, int * result);

int swpl_write_out_signature_member(SWI * swi, struct tar_header * ptar_hdr,
		struct new_cpio_header * file_hdr,
		int ofd, int signum,
		int *package_ret, char * installer_sig);

int swpl_write_out_all_signatures(SWI * swi, struct tar_header * ptar_hdr,
		struct new_cpio_header * file_hdr,
		int ofd, int, int, unsigned long filesize);

int swpl_write_catalog_data(SWI * swi, int ofd, int sig_block_start, int sig_block_end);

int swpl_write_catalog_archive_member(SWI * swi, int ofd,
		struct tar_header *ptar_hdr,
		int sig_block_start,
		int sig_block_end);

int swpl_send_nothing_and_wait(SWICOL * swicol, int ofd, int event_fd, char * msgtag, int tl, int retcode);
int swpl_send_abort(SWICOL * swicol, int ofd, int event_fd, char * msgtag);
int swpl_send_success(SWICOL * swicol, int ofd, int event_fd, char * msgtag);

int swpl_send_signature_files(SWI * swi, int ofd,
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd,
	struct tar_header * ptar_hdr,
	int sig_block_start, int sig_block_end,
	unsigned long filesize);

int swpl_common_catalog_tarfile_operation(SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd,
	char * script, char * message);

void swpl_update_fileset_state(SWI * swi, char * swsel, char * state);

int swpl_update_execution_script_results(SWI * swi, SWICOL * swicol, SCAR ** array);

int swpl_unpack_catalog_tarfile(GB * G, SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd);

int swpl_remove_catalog_directory(SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd);

int swpl_get_utsname_attributes(GB * G, SWICOL * swicol, SWUTS * uts, int ofd, int event_fd);

int swpl_determine_tar_listing_verbose_level(SWI * swi);
int swpl_assert_all_file_definitions_installed(SWI * swi, SWHEADER * infoheader);
int swpl_send_success(SWICOL * swicol, int ofd, int event_fd, char * msgtag);
int swpl_send_null_task(SWICOL * swicol, int ofd, int event_fd, char * msgtag, int retcode);
int swpl_send_null_task2(SWICOL * swicol, int ofd, int event_fd, char * msgtag, char * status_expression);
VPLOB * swpl_get_dependency_specs(GB * G, SWI * swi, char * requisite_keyword, int product_number, int fileset_number);

int swpl_get_catalog_entries(GB * G,
			VPLOB * swspecs,
			VPLOB * pre_swspecs,
			VPLOB * co_swspecs,
			VPLOB * ex_swspecs,
			char * target_path,
			SWICOL * swicol,
			int target_fd0,
			int target_fd1,
			struct extendedOptions * opta,
			char * pgm_mode);

int swpl_do_list_catalog_entries2(GB * G,
			VPLOB * swspecs,
			VPLOB * pre_swspecs,
			VPLOB * co_swspecs,
			VPLOB * ex_swspecs,
			char * target_path,
			SWICOL * swicol,
			int target_fd0,
			int target_fd1,
			struct extendedOptions * opta,
			char * pgm_mode);

int swpl_test_pgm_mode(char * pgm_mode, char * test_mode);
VPLOB * swpl_get_same_revision_specs(GB * G, SWI * swi, int product_number, char * location);

int swpl_get_catalog_tar(GB * G,
	SWI * swi,
	char * target_path,
	VPLOB * upgrade_specs,
	int target_fd0,
	int target_fd1);


void swpl_tty_raw_ctl(GB * G, int c);

char * swpl_get_samepackage_query_response(GB * G, SWICOL * swicol,
			char * target_path, int target_fd0, int target_fd1,
			struct extendedOptions * opta, VPLOB * packagespecs,
			int * p_retval, int make_dummy_response);

int swpl_report_status(SWICOL * swicol, int ofd, int event_fd);

SWICAT_REQ * swpl_analyze_samepackage_query_response(GB * G, char * response_image, SWICAT_SL ** p_sl);
char * swpl_shellfrag_session_lock(STROB * buf, int vlv);
char * swpl_shellfrag_session_unlock(STROB * buf, int vlv);
int swpl_session_lock(GB * G, SWICOL * swicol, char * target_path, int ofd, int event_fd);
char * swpl_looper_payload_routine(STROB * buf, int vlv, char * locked_region);
int swpl_signature_policy_accept(GB * G, SWGPG_VALIDATE * w, int verbose_level, char * swspec_string);
int swpl_remove_catalog_entry(GB * G, SWICOL * swicol, SWICAT_E * e, int ofd, char * iscpath);
int swpl_restore_catalog_entry(GB * G, SWICOL * swicol, SWICAT_E * e, int ofd, char * iscpath);
char * swpl_rename_suffix(STROB * buf);
void swpl_tag_volatile_files(SWI_FILELIST * fl);
int swpl_show_file_list(GB * G, SWI_FILELIST * fl);
int
swpl_make_verify_command_script_fragment(GB * G,
		STROB * scriptbuf,
		SWI_FILELIST * fl,
		char * pax_write_command_key, int be_silent);
int
swpl_retrieve_files(GB * G, SWI * swi, SWICOL * swicol, SWICAT_E * e, SWI_FILELIST * file_list,
	int ofd, int ifd, int archive_fd, char * pax_write_command_key, int ls_fd, int ls_verbose, FILE_DIGS * digs);
int swpl_ls_fileset_from_iscat(SWI * swi, int ofd, int ls_verbose, int * available_attributes, int skip_volatile, int check_contents);
void swpl_bashin_detect(STROB * buf);
int swpl_check_package_signatures(GB * G, SWI * swi, int * NumChecked);
char * swpl_make_package_signature(GB * G, SWI * swi);
void swpl_set_detected_catalog_perms(SWI * swi, ETAR * etar, int tar_type);
int swpl_get_catalog_perms(GB * G, SWI * swi, int ofd, int event_fd);
char * swpl_make_environ_transfer_image(STROB * buf);
int swpl_run_check_script(GB * G, char * script_name, char * id_string, SWI * swi, int ofd, int event_fd, int * p_check_status);
CISF_PRODUCT * swpl_cisf_product_create(SWI_PRODUCT *);
void swpl_cisf_product_delete(CISF_PRODUCT *);
CISF_FILESET * swpl_cisf_fileset_create(void);
void swpl_cisf_fileset_delete(CISF_FILESET *);
void swpl_cisf_init_single_single(CISF_PRODUCT * x, SWI * swi);
void swpl_cisf_init_base(CISF_BASE * x);
int swpl2_construct_control_script(GB * G, CISF_BASE * base, STROB * buf, SWI * swi, char * script_tag, char * parent_tag);
VPLOB * swpl2_get_cisf_base_array(void);
void swpl2_audit_cisf_bases(GB * G, SWI * swi, CISF_PRODUCT * cisf);
int swpl2_update_execution_script_results(SWI * swi, SWICOL * swicol, CISF_PRODUCT * cisf);
void swpl_update_state_by_cisf_base(CISF_BASE * cfb, char * state);
SWI_CONTROL_SCRIPT * swpl2_find_by_id(int id, CISFBA * base_array, CISF_BASE ** bpp);
void swpl2_cisf_base_array_add(CISF_PRODUCT * cisf, CISF_BASE * b);
CISF_BASE * swpl2_cisf_base_array_get(CISF_PRODUCT * cisf, int ix);
int swpl_run_make_installed_live(GB * G, SWI * swi, int ofd, char * target_path);
int swpl_load_single_status_value (GB * G, SWI * swi, int ofd, int event_fd, char * id_str, int status_msg);
int swpl2_normalize_configure_script_results(SWI * swi, char * script_name, CISF_PRODUCT * cisf);
void swpl_print_file_list_to_buf(SWI_FILELIST * fl, STROB * buf);
int swpl_run_check_overwrite(GB * G, SWI_FILELIST * fl, SWI * swi, int ofd, char * target_path, int mem_fd);
void swpl_write_chdir_catalog_fragment(STROB * tmp, char * catalog_path, char * id_str);
void swpl_agent_fail_message(GB *G, char * current_arg, int status);
void swpl_bashin_testsh(STROB * buf, char * shell);
void swpl_bashin_posixsh(STROB * buf);
int swpl_run_get_prelink_filelist(GB * G, SWICOL * swicol, int ofd, int * pfp_prelink_fd);

#endif
