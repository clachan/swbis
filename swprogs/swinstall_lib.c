/*  swinstall_lib.c -- the top-level routines of swinstall

 Copyright (C) 2004,2005,2006,2007,2008 Jim Lowe
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "strob.h"
#include "cplob.h"
#include "vplob.h"
#include "swlib.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swcommon0.h"
#include "swcommon.h"
#include "swparse.h"
#include "swfork.h"
#include "swgp.h"
#include "etar.h"
#include "swssh.h"
#include "progressmeter.h"
#include "swevents.h"
#include "to_oct.h"
#include "tarhdr.h"
#include "swinstall.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swicat.h"
#include "strar.h"
#include "swi.h"
#include "atomicio.h"
#include "shlib.h"
#include "swutilname.h"
#include "swproglib.h"

extern struct g_pax_read_command g_pax_read_commands[];

#define SWBIS_DEFAULT_REVISION "0.0"

static
int
construct_controlsh_script(GB * G, STROB * buf, SWI * swi)
{
	char * tag;
	STROB * tmp;
	STROB * tmp2;
	STROB * taglist;

	taglist = strob_open(100);
	tmp = strob_open(100);
	tmp2 = strob_open(100);
	strob_strcpy(buf, "");

	/*
	 * Construct the ch'dir command to SW_ROOT
	 */

	if (G->g_no_script_chdirM) {
		strob_sprintf(tmp2, STROB_NO_APPEND, "\n");
	} else {
		strob_sprintf(tmp2, STROB_NO_APPEND, 
			"# here chdir  cd $SW_ROOT\n"
			"cd \"$SW_ROOT\" || exit 1\n"
		);
	}

	swicat_write_auto_comment(buf, SW_A_CONTROL_SH);
	swicat_env(buf, swi, NULL, NULL);

	swicat_construct_controlsh_taglist(swi, "*", taglist);

	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"SWBCS_TAGLIST=\"%s\"\n"
		"case \"$#\" in\n"
		"	2)\n"
		"		;;\n"
		"	*)\n"
		"		echo \"usage: " SW_A_CONTROL_SH " selection tag\" 1>&2\n"
		"		exit 1\n"
		"		;;\n"
		"esac\n"
		"SWBCS_SWSEL_PAT=\"$1\"\n"
		"SWBCS_SCRIPT_TAG=\"$2\"\n"
		"SWBCS_GOT_MATCH=n\n"
		"SWBCS_MATCH=\"\"\n"
		CSHID
		"for fqc in $SWBCS_TAGLIST\n"
		"do\n"
		"	case \"$fqc\" in\n"
		"		${SWBCS_SWSEL_PAT})\n"
		"		case $SWBCS_GOT_MATCH in y) echo SW_SELECTION_NOT_FOUND_AMBIG 1>&2; exit 22;; esac\n"
		"		SWBCS_MATCH=\"$fqc\"\n"
		"		SWBCS_GOT_MATCH=y\n"
		"		;;\n"
		"	esac\n"
		"done\n"
		CSHID
		"case $SWBCS_GOT_MATCH in\n"
        	"	n)\n"
		"		echo SW_SELECTION_NOT_FOUND 1>&2\n"
		"		exit 1\n"
		"		;;\n"
		"esac\n"
		, strob_str(taglist)
		);

	strob_sprintf(buf, STROB_DO_APPEND,
		"# Set the interpreter default value\n"
		"INTERPRETER=\"sh\"\n"
		"export INTERPRETER\n"
		);

	strob_sprintf(buf, STROB_DO_APPEND,
		"case \"$SWBCS_MATCH\" in\n"
		);

	tag = strob_strtok(tmp, strob_str(taglist), " ");
	while(tag) {
		swpl_write_case_block(swi, buf, tag);
		tag = strob_strtok(tmp, NULL, " ");
	}
	
	strob_sprintf(buf, STROB_DO_APPEND, "esac\n");

	/*
	 * Now finally the environment is set, so now
	 * run the script.
	 */

	strob_sprintf(buf, STROB_DO_APPEND,
		"\n"
		CSHID
		"# Now run the script\n"
		"\n"
		"# Now set the INTERPRETER\n"
		"#\n"
		"# SWPATH is set above\n"
		"export CONFPATH\n"
		"CONFPATH=\"$SWPATH\"\n"
		"case \"$CONFPATH\" in\n"
		"\"\")\n"
		"CONFPATH=\"$PATH\"\n"
		";;\n"
		"esac\n"

		"# /bin/bash is the first choice\n"
		"case \"${INTERPRETER}\" in\n"
		"\"\"|sh)\n"
		"#\n"
		"# Find PGM in CONFPATH\n"
		"export PGM=bash\n"
		"# Here is a shell implementation of which(1)\n"
		"location=`IFS=: ; set --; set -- \"$CONFPATH\"; for i in $*; do test -x \"$i/$PGM\"; res=$?; case \"$res\" in 0) echo \"$i\"; exit 0;;  esac; done; exit 1; `\n"
		"case \"$?\" in\n"
		"0)\n"
		"INTERPRETER=\"$location\"/$PGM\n"
		";;\n"
		"esac\n"
		";;\n"
		"esac\n"

		"# sh as found in CONFPATH is the next choice\n"
		"case \"${INTERPRETER}\" in\n"
		"\"\"|sh)\n"
		"#\n"
		"# Find PGM in CONFPATH\n"
		"export PGM=sh\n"
		"# Here is a shell implementation of which(1)\n"
		"location=`IFS=: ; set --; set -- \"$CONFPATH\"; for i in $*; do test -x \"$i/$PGM\"; res=$?; case \"$res\" in 0) echo \"$i\"; exit 0;;  esac; done; exit 1; `\n"
		"case \"$?\" in\n"
		"0)\n"
		"INTERPRETER=\"$location\"/$PGM\n"
		";;\n"
		"esac\n"
		";;\n"
		"esac\n"

		"# /usr/xpg4/bin/sh is the last choice\n"
		"case \"${INTERPRETER}\" in\n"
		"\"\"|sh)\n"
		"test -x /usr/xpg4/bin/sh\n"
		"case \"$?\" in\n"
		"0)\n"
		"INTERPRETER=/usr/xpg4/bin/sh\n"
		";;\n"
		"esac\n"
		";;\n"
		"esac\n"

		"case \"${INTERPRETER}\" in\n"
		"\"\"|sh)\n"
		"# No interpreter found\n"
		"echo \"swbis: control.sh (tag=$SW_CONTROL_TAG): interpreter [${INTERPRETER}] not found\" 1>&2\n"
		"exit 1\n"
		";;\n"
		"esac\n"

		"#\n"
		"# All the checks are done, Now execute the script\n"
		"#\n"
		"%s\n"
		"export SW_CONTROL_DIRECTORY\n"
		"\"${INTERPRETER}\" \"${SW_CONTROL_DIRECTORY}/${SWBCS_SCRIPT_NAME}\"\n"
		"swbcs_ret=$?\n"
		"case \"$swbcs_ret\" in\n"
		"	0) ;;\n"
		"	1) ;;\n"
		"	2) ;;\n"
		"	3) ;;\n"
		"	*) swbcs_ret=1 ;;\n"
		"esac\n"
		"exit $swbcs_ret\n",
		strob_str(tmp2)
		);

	strob_close(taglist);
	strob_close(tmp);
	strob_close(tmp2);
	return 0;
}

static
int
write_tar_controlsh_file(GB * G, SWI * swi, int ofd,
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str
	)
{
	int ret = 0;
	STROB * name;
	STROB * data;

	name = strob_open(100);
	data = strob_open(100);

	strob_strcpy(name, SW_A_controlsh);

	ret = construct_controlsh_script(G, data, swi);
	SWLIB_ASSERT(ret == 0);

	ret = swpl_load_single_file_tar_archive(G,
		swi,
		ofd, 
		catalog_path,
		pax_read_command,
		alt_catalog_root,
		event_fd,
		id_str,
		strob_str(name),
		strob_str(data), (AHS*)NULL);

	strob_close(name);
	strob_close(data);
	return ret;
}


static
int
copy_attributes(GB * G, SWHEADER * infoheader, struct new_cpio_header * file_hdr)
{
	int ret;
	ret = swheader_fileobject2filehdr(infoheader, file_hdr);

	/* FIXME, also copy the extended attributes into file_hdr */

	return ret;
}


static
int
write_tar_session_options_file(GB * G, SWI * swi, int ofd,
	char * catalog_path,
	char * pax_read_command,
	int alt_catalog_root,
	int event_fd,
	char * id_str
	)
{
	int ret = 0;
	STROB * name;
	STROB * data;

	name = strob_open(100);
	data = strob_open(100);

	strob_strcpy(name, SW_A_session_options);
	swpl_write_session_options_file(data, swi);
	ret = swpl_load_single_file_tar_archive(G,
		swi,
		ofd, 
		catalog_path,
		pax_read_command,
		alt_catalog_root,
		event_fd,
		id_str,
		strob_str(name),
		strob_str(data), (AHS*)NULL);

	strob_close(name);
	strob_close(data);
	return ret;
}

static
int
check_is_all_digits(char * value) {
	char * terminating_chars;
	char * w;
	w = value;
	if (strlen(value) == 0) return 1;
	terminating_chars = " ";
	while (*w  &&  isdigit(*w)) w++;
	if (*w == '\0' || strchr(terminating_chars, *w)) return 0;
	return -1;
}

static
int
detect_and_correct_numeric_ids(AHS * ahs)
{
	struct new_cpio_header * h;
	char * username;
	char * groupname;
	unsigned long uid;
	unsigned long gid;

	h = ahs->file_hdrM;
	username = ahsStaticGetTarUsername(h);
	groupname = ahsStaticGetTarGroupname(h);
	uid = h->c_uid;
	gid = h->c_gid;

	if (check_is_all_digits(username) == 0) {
		if (swlib_atoi(username, NULL) == (int)uid) {
			ahsStaticSetTarUsername(h, "");
		} 
	}

	if (check_is_all_digits(groupname) == 0) {
		if (swlib_atoi(groupname, NULL) == (int)gid) {
			ahsStaticSetTarGroupname(h, "");
		} 
	}
	return 0;
}

static
int
sanity_compare(AHS * ahs1, AHS * ahs2, char * filename, int do_be_verbose)
{
	int ret = 0;
	int cpiotype1, cpiotype2;
	struct new_cpio_header * h1 = ahs1->file_hdrM;
	struct new_cpio_header * h2 = ahs2->file_hdrM;

	cpiotype1 = (h1->c_mode & CP_IFMT);
	cpiotype2 = (h2->c_mode & CP_IFMT);

	if (cpiotype1 != cpiotype2) {
		ret ++;
		if (do_be_verbose)
		fprintf(stderr, "%s: attribute mismatch: %s: att=type: storage=[%d] [%d]\n",
			swlib_utilname_get(), filename,
			(int)(cpiotype1),
			(int)(cpiotype2));
	}

       	if  ((h2->usage_maskM) & TARU_UM_MODE) {
		if ((07777 & h1->c_mode) != (07777 & h2->c_mode)) {
			ret ++;
			if (do_be_verbose)
			fprintf(stderr, "%s: attribute mismatch: %s: att=mode: storage=[%o] [%o]\n",
				swlib_utilname_get(), filename,
				(int)(h1->c_mode),
				(int)(h2->c_mode));
		}
	} else {
		if (do_be_verbose)
		fprintf(stderr, "%s: warning: INFO file does not contain a mode attribute: %s\n",
			swlib_utilname_get(), filename);
	}

       	if  ((h2->usage_maskM) & TARU_UM_UID) {
 	      	if (
			(h1->c_uid != ULONG_MAX && h2->c_uid != ULONG_MAX) &&
			h1->c_uid != h2->c_uid
		) {
			ret ++;
			if (do_be_verbose)
			fprintf(stderr, "%s: attribute mismatch: %s: att=uid: storage=[%d] INFO=[%d]\n",
				swlib_utilname_get(), filename,
				(int)(h1->c_uid),
				(int)(h2->c_uid));
		}
	} else {
		if (do_be_verbose)
		fprintf(stderr, "%s: warning: INFO file does not contain a uid attribute: %s\n",
			swlib_utilname_get(), filename);
	}

       	if  ((h2->usage_maskM) & TARU_UM_GID) {
 	      	if (
			(h1->c_gid != ULONG_MAX && h2->c_gid != ULONG_MAX) &&
			h1->c_gid != h2->c_gid
		) {
			ret ++;
			if (do_be_verbose)
			fprintf(stderr, "%s: attribute mismatch: %s: att=gid: storage=[%d] INFO=[%d]\n",
				swlib_utilname_get(), filename,
				(int)(h1->c_gid),
				(int)(h2->c_gid));
		}
	} else {
		if (do_be_verbose)
		fprintf(stderr, "%s: warning: INFO file does not contain a gid attribute: %s\n",
			swlib_utilname_get(), filename);
	}

       	if ( ((h2->usage_maskM) & TARU_UM_MTIME) && h1->c_mtime != h2->c_mtime) {
		/* This is now only a warning. */
		/* ret ++; FIXME:  policy at wrong level */
		if (do_be_verbose)
		fprintf(stderr, "%s: Warning: attribute mismatch: %s: att=mtime: storage=[%lu] INFO=[%lu]\n",
			swlib_utilname_get(), filename,
			(h1->c_mtime),
			(h2->c_mtime));
	}

       	if (h1->c_filesize != h2->c_filesize &&
		(
			(cpiotype1 != CP_IFLNK) &&
			(cpiotype2 != CP_IFLNK) &&
			(cpiotype2 != CP_IFLNK) &&
			(h1->c_is_tar_lnktype != 1) &&
			(h1->c_is_tar_lnktype != 1)
		)
	) {
		/*
		 * Don't do a file size comparison if the type is SYMTYPE or LNKTYPE
		 */
		ret ++;
		if (do_be_verbose)
		fprintf(stderr, "%s: attribute mismatch: %s: att=size: storage=[%d] INFO=[%d]\n",
			swlib_utilname_get(), filename,
			(int)(h1->c_filesize),
			(int)(h2->c_filesize));
		SWLIB_FATAL("mismatch of size attribute is fatal");
	}

       	if ((h1->c_dev_maj || h2->c_dev_maj) && 
		h1->c_dev_maj != h2->c_dev_maj) {
		ret ++;
		if (do_be_verbose)
		fprintf(stderr, "%s: attribute mismatch: %s: att=major: storage=[%d] INFO=[%d]\n",
			swlib_utilname_get(), filename,
			(int)(h1->c_dev_maj),
			(int)(h2->c_dev_maj));
	}

       	if ((h1->c_dev_min || h2->c_dev_min) &&
       		h1->c_dev_min != h2->c_dev_min) {
		ret ++;
		if (do_be_verbose)
		fprintf(stderr, "%s: attribute mismatch: %s: att=minor: storage=[%d] INFO=[%d]\n",
			swlib_utilname_get(), filename,
			(int)(h1->c_dev_min),
			(int)(h2->c_dev_min));
	}

       	if  ((h2->usage_maskM) & TARU_UM_GROUP) {
		ret += swpl_compare_name(
			ahsStaticGetTarGroupname(h1),
			ahsStaticGetTarGroupname(h2),
			SW_A_group, filename);
	} else {
		if (do_be_verbose)
		fprintf(stderr, "%s: warning: INFO file does not contain a group attribute: %s\n",
			swlib_utilname_get(), filename);
	}


       	if  ((h2->usage_maskM) & TARU_UM_OWNER) {
		ret += swpl_compare_name(
			ahsStaticGetTarUsername(h1),
			ahsStaticGetTarUsername(h2),
			SW_A_owner, filename);
	} else {
		if (do_be_verbose)
			fprintf(stderr, "%s: warning: INFO file does not contain a owner attribute: %s\n",
			swlib_utilname_get(), filename);
	}	

	if (1 || (h1->c_is_tar_lnktype != 1 && h2->c_is_tar_lnktype != 1)) {
		/*
		 * If either is a hard link, don't do this comparison.
		 * The linkname in a tar header does not match the
		 * link_source attribute in the INFO file since the tar
		 * archive may have control directories in the linkname.
		 * This is done so that the IEEE layout tar archive is
		 * self consistent.
		 */

		/*
		 * Actually, don't do this test at all since
		 * the linkname is rewritten.
		 * FIXME there should be sanity check anyway.
		 */
		/*
		ret += swpl_compare_name(
			ahsStaticGetTarLinkname(h1),
			ahsStaticGetTarLinkname(h2),
			SW_A_link_source, filename);
		*/
	}

	if (strcmp(filename, ".") == 0)
		ret += swpl_compare_name(
			"/" /* filename */,
			ahsStaticGetTarFilename(h2),
			SW_A_path, "/");
	else	
		ret += swpl_compare_name(
			filename /* ahsStaticGetTarFilename(h1)*/ ,
			ahsStaticGetTarFilename(h2),
			SW_A_path, filename);

	return ret;
}

static
int
determine_if_do_configure(SWI * swi, struct extendedOptions * opta)
{
	char * value;
	char * target_path;
	SWI_PRODUCT * current_product;
	SWI_XFILE * current_fileset;
	SWHEADER * indexheader;
	SWHEADER_STATE state;

	target_path = swi->swi_pkgM->target_pathM;

	/*
	 * check is the target_path is "/"
	 */

	if (target_path == NULL || strcmp(target_path, "/") != 0) {
		/*
		 * requirements for running configure script by swinstall
		 * are not met
		 */
		return 0;
	}

	current_product = swi_package_get_product(swi->swi_pkgM, 0 /* FIXME the first one */);
	current_fileset = swi_product_get_fileset(current_product, 0 /* FIXME the first one */);

	indexheader = swi_get_global_index_header(swi);
	swheader_store_state(indexheader, &state);
	swheader_reset(indexheader);

	/*
	 * FIXME, support more than one fileset
	 */

	swheader_set_current_offset(indexheader, current_fileset->baseM.header_indexM);

	/*
	 * Now check the is_reboot attribute
	 */
	value = swheader_get_single_attribute_value(indexheader, SW_A_is_reboot);
	swheader_restore_state(indexheader, &state);
	if (value != NULL && swextopt_is_value_true(value)) {
		/*
		 * don't configure now, per spec
		 */
		return 0;
	}

	/*
	 * Now check the defer_configure extended option
	 */

	if (swextopt_is_option_true(SW_E_defer_configure, opta)) {
		return 0;
	}

	return 1; /* Do configure, if a configure script is included */
}

static
char * 
get_directory(SWI * swi)
{
	SWI_PRODUCT * product;
	char * value;
	SWHEADER_STATE state;
	SWHEADER * indexheader;
	char * ret = "/"; /* default value */

	indexheader = swi_get_global_index_header(swi);
	swheader_store_state(indexheader, &state);
	swheader_reset(indexheader);

	/* FIXME, select the correct product, not the first */
	product = swi_package_get_product(swi->swi_pkgM, 0); /* FIXME */
	swheader_set_current_offset(indexheader, product->p_baseM.header_indexM);

	value = swheader_get_single_attribute_value(indexheader, SW_A_directory);
	if (value == NULL) 
		value = ret;
	swheader_restore_state(indexheader, &state);
	return value;
}

static
uintmax_t
get_delivery_size(SWI * swi, int * result)
{
	uintmax_t size;
	uintmax_t ret;
	int count;
	char * obj;
	char * line;
	char * value;

	SWHEADER * indexheader = swi_get_global_index_header(swi);
	SWHEADER * infoheader = swi_get_fileset_info_header(swi, 0, 0);

	/*
	 * Get the size of the tar archive of the fileset
	 * which is the fileset size attribute plus 2048 bytes 
	 * for each file to account for the tar header and
	 * slack block.  This assumes the fileset size attribute
	 * is a sum of the file sizes.
	 */

	/*
	 * Get the size attribute from the fileset.
	 */
	*result = 0;
	swheader_reset(indexheader);
        obj = swheader_get_object_by_tag(indexheader, SW_A_fileset, "*" );
        line = swheader_get_attribute(indexheader, SW_A_size, NULL);
	value = swheaderline_get_value(line, NULL);

	if (!value) {
		SWLIB_FATAL("fileset.size attribute is missing");
	}

	size = strtoumax(value, NULL, 10);

	count = swpl_get_fileset_file_count(infoheader);
	if (count < 0) {
		SWLIB_FATAL("");
	}

	size = 	size + 			/* the fileset size value */
		(count * 2048) +	/* header and last block for every file + 1024 extra pad,
					   this should *more* than account for extended headers and
					   special device files and directories that are not counted
					   in the fileset size */
		(10 * 512);		/* trailer blocks, 10 is arbitrary */

	/*
	 * Now convert size to be evenly divisible by 512
	 * size = ((size / 512) + 1) * 512;
	 */
	ret = swpl_get_whole_block_size(size);
	return ret;
}

static
int
write_attribute_file(SWI * swi, ETAR * etar, int ofd, char * filename, char * file_data)
{
	int ret;
	int retval;

	retval = 0;
	SWLIB_ASSERT(strlen(file_data) < 64); /* Sanity check and enforce the Posix Limit */
	swpl_init_header_root(etar);
	etar_set_size(etar, strlen(file_data) + 1 /* +1 is the newline */);
	if (etar_set_pathname(etar, filename)) SWLIB_FATAL("name too long");
	swpl_set_detected_catalog_perms(swi, etar, REGTYPE);
	etar_set_chksum(etar);

	/*
	 * write the tar header for the file.
	 * The file name is <filename>
	 */
	ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
			ofd,
			(void*)(etar_get_hdr(etar)),
			(size_t)(TARRECORDSIZE));
	if (ret != TARRECORDSIZE) { SWBIS_ERROR_IMPL(); SWI_internal_fatal_error(); }
	retval += ret;

	/*
	 * now write the data block which the value of the vendor_tag.
	if (strlen(file_data) == 0)
		return retval;
	 */

	memset((void*)(swi->tarbufM), '\0', TARRECORDSIZE);
	strncpy((char*)(swi->tarbufM), file_data, TARRECORDSIZE - 2);
	strcat((char*)(swi->tarbufM), "\n");
	ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
			ofd, (void*)(swi->tarbufM), (size_t)(TARRECORDSIZE));
	if (ret != TARRECORDSIZE) { SWBIS_ERROR_IMPL(); SWI_internal_fatal_error(); }
	retval += ret;
	return retval;
}

static
int
write_catalog_archive_member(SWI * swi, int ofd,
		int sig_block_start, int sig_block_end)
{
	int ret;
	int retval;
	XFORMAT	* xformat;
	STROB * buf;
	int size;
	struct tar_header * catalog_tar_hdr;
	ETAR * etar;
	char * qualifier;
	char * location;
	char * vendor_tag;

	catalog_tar_hdr = (struct tar_header *)malloc(TARRECORDSIZE);
	xformat = swi->xformatM;
	buf = strob_open(100);
	etar = etar_open(swi->xformatM->taruM->taru_tarheaderflagsM);
	etar_init_hdr(etar);

	/*
	 * write the ../export/ directory itself
	 */

	swpl_init_header_root(etar);
	strob_strcpy(buf, SWINSTALL_INCAT_NAME);

	etar_set_mode_ul(etar, (unsigned int)(0755));
	swpl_set_detected_catalog_perms(swi, etar, DIRTYPE);
	if (etar_set_pathname(etar, strob_str(buf))) SWLIB_FATAL("name too long");
	etar_set_typeflag(etar, DIRTYPE);
	etar_set_size(etar, 0);
	etar_set_chksum(etar);

	ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
				ofd,
				(void*)(etar_get_hdr(etar)),
				(size_t)(TARRECORDSIZE));

	if (ret != TARRECORDSIZE) {
		SWBIS_ERROR_IMPL();
		SWI_internal_fatal_error();
		return -1;
	}

	retval = ret;

	/*
	 * Now write the catalog.tar file 
	 */
	swpl_init_header_root(etar);

	/*
	 * set the tar header name and file size
	 */
	strob_strcpy(buf, "");
	swlib_unix_dircat(buf, SWINSTALL_INCAT_NAME);
	swlib_unix_dircat(buf, SWINSTALL_CATALOG_TAR);
	size = swpl_write_catalog_data(swi, swi->nullfdM, sig_block_start, sig_block_end);
	etar_set_size(etar, size);
	if (etar_set_pathname(etar, strob_str(buf))) SWLIB_FATAL("name too long");
	swpl_set_detected_catalog_perms(swi, etar, REGTYPE);
	etar_set_chksum(etar);

	/*
	 * write the tar header
	 */
	ret = atomicio((ssize_t (*)(int, void *, size_t))uxfio_write,
				ofd,
				(void*)(etar_get_hdr(etar)),
				(size_t)(TARRECORDSIZE));

	if (ret != TARRECORDSIZE) {
		SWBIS_ERROR_IMPL();
		SWI_internal_fatal_error();
	}

	/*
	 * write the catalog data.
	 */
	if (swpl_write_catalog_data(swi, ofd, sig_block_start, sig_block_end) != size) {
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		retval = -1;
	} else {
		retval +=  (size + ret);
	}
	
	/*
	 * write the location attribute file.
	 */
	location = swi->swi_pkgM->locationM;
	retval += write_attribute_file(swi, etar, ofd, SW_A_location, location);

	/*
	 * write the qualifier attribute file.
	 */
	qualifier = swi->swi_pkgM->qualifierM;
	retval += write_attribute_file(swi, etar, ofd, SW_A_qualifier, qualifier);
	
	/*
	 * write the vendor_tag attribute file.
	 */
	vendor_tag = strar_get(swi->distdataM->vendor_tagsM, 0 /*product index*/);
	if (!vendor_tag || strlen(vendor_tag) == 0) {
		vendor_tag = "";
	}
	retval += write_attribute_file(swi, etar, ofd, SW_A_vendor_tag, vendor_tag);

	etar_close(etar);
	strob_close(buf);
	free(catalog_tar_hdr);
	return retval;
}

static
int
send_catalog_tarfile(SWI * swi, int ofd, 
	char * catalog_path, char * pax_read_command,
	int alt_catalog_root, int event_fd,
	int sig_block_start, int sig_block_end, char * id_str)
	
{
	int ret = 0;
	int dataret = 0;
	int stdin_file_size;
	STROB * tmp;

	tmp = strob_open(10);

	/*
	 * Here is the task scriptlet.
	 */

	swpl_write_chdir_catalog_fragment(tmp, catalog_path, id_str);

	strob_sprintf(tmp, STROB_DO_APPEND,
		"dd 2>/dev/null | %s\n"
		"sw_retval=$?\n"	/* This line is required */
		"dd of=/dev/null 2>/dev/null\n"
		,
		pax_read_command
		);

	stdin_file_size = write_catalog_archive_member(swi, swi->nullfdM, sig_block_start, sig_block_end);
	if (stdin_file_size < 0) {
		SWBIS_ERROR_IMPL();
		return -1;
	}
	stdin_file_size += (TARRECORDSIZE+TARRECORDSIZE); /* trailer blocks */

	if (stdin_file_size % TARRECORDSIZE) {
		SWBIS_ERROR_IMPL();
		SWI_internal_error();
		return -1;
	}

	/*
	 * Here is the directory that is chdir'ed into.
	 */

	swicol_set_task_idstring(swi->swicolM, SWBIS_TS_Load_catalog);
	ret = swicol_rpsh_task_send_script2(
		swi->swicolM,
		ofd, 
		stdin_file_size,
		swi->swi_pkgM->target_pathM,
		strob_str(tmp), SWBIS_TS_Load_catalog
		);

	/*
	 * Now write the actual data to the stdin of the
	 * posix shell, It *must* be exactly stdin_file_size
	 * bytes in length.
	 */
	if (ret == 0) {
		ret = write_catalog_archive_member(swi, ofd, 
			sig_block_start, sig_block_end);

		if (ret < 0) {
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}
		dataret += ret;

		/*
		 * Now write the trailer blocks
		 */	
		ret = etar_write_trailer_blocks(NULL, ofd, 2);
		if (ret < 0) {
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}
		dataret += ret;

		/*
		 * Assert what must be true.
		 */
		if (dataret != stdin_file_size) {
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}

		/*
		 * Reap the events from the event pipe.
		 */
		ret = swicol_rpsh_task_expect(swi->swicolM,
					event_fd,
					SWICOL_TL_9 /*time limit*/);
		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);
	}	
	strob_close(tmp);
	return ret;
}


static
int
construct_execution_phase_script(GB * G, CISF_PRODUCT * cisf, SWI * swi,
			STROB * buf, char * pax_read_command)
{
	int do_configure;
	int ret;
	char * xxx;
	char * v_target;
	char * v_command;
	

	/*
	 * determine if its proper to run the configure scripts
	 */
	do_configure = determine_if_do_configure(swi, G->optaM);

	strob_sprintf(buf, STROB_DO_APPEND,
	"( # subshell 001\n"
		CSHID
	);

	/*
	 * construct the preinstall portion of the execution script
	 */
	ret = swpl_construct_script(G, cisf, buf, swi, SW_A_preinstall);

	strob_sprintf(buf, STROB_DO_APPEND,
	"# FIXME: why isn't sw_retval being set\n"
	"sw_retval=\"${sw_retval:=0}\"\n"
	"exit \"$sw_retval\"\n"
	") 1</dev/null # subshell 001\n"
	"sw_retval=$?\n"
	);

	/*
	 * Now write the actual command to install the fileset
	 */
	xxx = TEVENT(2, -1, SWI_TASK_CTS, "Clear to Send: status=0");

	if (G->g_justdbM == 0) {
		v_target = swi->swi_pkgM->target_pathM;
		v_command = pax_read_command;
	} else {
		v_target = ".";
		v_command = "tar tf - 1>/dev/null 2>&1";
	}

	strob_sprintf(buf, STROB_DO_APPEND,
		CSHID
		"\n"
		"cd \"%s\"\n"
		"case $? in\n"
		"	0)\n"
		"	%s\n"
		"	umask 077\n"
		"	dd 2>/dev/null | %s\n"
		"	sw_retval=$?\n"
		"	dd of=/dev/null 2>/dev/null\n"
		"	;;\n"
		"	*)\n"
		"	sw_retval=1\n"
		"	;;\n"
		"esac\n"
		"\n"
		,
		v_target,
		xxx,
		v_command);

	/*
	 * construct the postinstall portion of the execution script
	 */
	strob_sprintf(buf, STROB_DO_APPEND,
		"case $sw_retval in # Case005\n"
		"0)\n"
		);

	strob_sprintf(buf, STROB_DO_APPEND,
	"( # subshell 002\n"
		CSHID
	);
	
	ret = swpl_construct_script(G, cisf,  buf, swi, SW_A_postinstall);

	swpl_construct_configure_script(G, cisf, buf, swi, do_configure);

	strob_sprintf(buf, STROB_DO_APPEND,
	"exit \"$sw_retval\"\n"
	") 1</dev/null # subshell 002\n"
	"sw_retval=$?\n"
	);

	strob_sprintf(buf, STROB_DO_APPEND,
		";;\n"
		"esac  # Case005\n"
		);

	return 0;
}

static
void
construct_directory_ownership_script(SWI * swi, STROB * dir_owner_buf, char * isc_path)
{
	mode_t mode;
	STROB * tmp;
	STROB * tmp2;
	STROB * path2;
	char * s;

	tmp = strob_open(32);
	tmp2 = strob_open(32);
	path2 = strob_open(32);

	/* set a failsafe value */
	strob_strcpy(dir_owner_buf, "true");
	
	strob_strcpy(tmp, swi->swi_pkgM->catalog_entryM);
	E_DEBUG2("swi->swi_pkgM->catalog_entryM is [%s]", swi->swi_pkgM->catalog_entryM);

	s = strstr(strob_str(tmp), isc_path);
	if (s != strob_str(tmp)) {
		/* error */
		return;
	}
	s += (strlen(isc_path));
	swlib_squash_leading_slash(s);

        mode = swi->swi_pkgM->installed_catalog_modeM;
	strob_strcpy(tmp, "");
	s = strob_strtok(tmp2, s, "/");
	while(s) {
		if (swi->swi_pkgM->installed_catalog_ownerM == NULL) return;
		if (swi->swi_pkgM->installed_catalog_groupM == NULL) return;
		if (strlen(swi->swi_pkgM->installed_catalog_ownerM) == 0) return;
		if (strlen(swi->swi_pkgM->installed_catalog_groupM) == 0) return;
		swlib_unix_dircat(path2, s);
		strob_sprintf(tmp, 1, "chown '%s':'%s' '%s/%s'\n",
			swi->swi_pkgM->installed_catalog_ownerM,
			swi->swi_pkgM->installed_catalog_groupM,
			isc_path,
			strob_str(path2));
		if (mode & S_ISVTX) {
			strob_sprintf(tmp, 1, "chmod o+t '%s/%s'\n",
				isc_path,
				strob_str(path2));
		}
		s = strob_strtok(tmp2, NULL, "/");
	}	
	if (strob_strlen(tmp) > 10)  /* silly sanity check */
		strob_strcpy(dir_owner_buf, strob_str(tmp));
	strob_close(path2);
	strob_close(tmp2);
	strob_close(tmp);
	return;
}

static
int
send_files_with_no_storage(GB * G, SWI * swi, SWHEADER * infoheader, XFORMAT * xformat, int fileset_fd, int vofd,
				int * pstatus, char * target_path, char * location,
				char * directory, uintmax_t * pstatbytes, char * tar_format)
{
	int ret;
	int retval;
	int tartype;
	int header_ret;
	char * att;
	char * next_line;
	char * keyword;
	char * current_path;
	char * l_current_path;
	AHS * ahs;
	char *current_type;
	char * current_link_source;
	STROB * relocated_current_path;

	relocated_current_path = strob_open(10);
	retval = 0;
	ret = 0;
	ahs = ahs_open();
	*pstatus = 0;

	E_DEBUG2("tar_format=[%s]", tar_format);
	swheader_store_state(infoheader, NULL);
	swheader_reset(infoheader);
	while((next_line=swheader_get_next_object(infoheader, (int)UCHAR_MAX, (int)UCHAR_MAX))) {
		keyword = swheaderline_get_keyword(next_line);
		SWLIB_ASSERT(keyword != NULL);
		if (strcmp(keyword, SW_A_control_file) == 0) {
			/* skip control files */
			continue;
		}
		if (swheaderline_get_flag1(next_line) == 0) {
			/* These files have no file in the storage section */

			ahs_init_header(ahs);
			ret = swheader_fileobject2filehdr(infoheader, ahs_vfile_hdr(ahs));
			if (ret) {
				SWI_internal_error();
				*pstatus = 2; /* error */
				break;
			}	
			tartype = ahs_get_tar_typeflag(ahs);

			att = swheader_get_attribute(infoheader, SW_A_type, NULL);
			SWLIB_ASSERT(att != NULL);
			current_type = swheaderline_get_value(att, NULL);

			att = swheader_get_attribute(infoheader, SW_A_path, NULL);
			SWLIB_ASSERT(att != NULL);
			current_path = swheaderline_get_value(att, NULL);
			
			swlib_apply_location(relocated_current_path, current_path, location, directory);

			swpl_sanitize_pathname(strob_str(relocated_current_path));
			swlib_squash_leading_dot_slash(strob_str(relocated_current_path));
			swlib_squash_leading_slash(strob_str(relocated_current_path));
			l_current_path = strob_str(relocated_current_path);

			ahsStaticSetTarFilename(ahs_vfile_hdr(ahs), l_current_path);

			if (*current_type == SW_ITYPE_f) {
				/* REGTYPE */
				*pstatus = 1; /* error */
				continue;
			} else if (
				*current_type == SW_ITYPE_d || /* directory */
				*current_type == SW_ITYPE_h || /* hard link */
				*current_type == SW_ITYPE_s || /* sym link */
				*current_type == SW_ITYPE_c || /* char device */
				*current_type == SW_ITYPE_b || /* block device */
				0
			) {
				detect_and_correct_numeric_ids(ahs);
				ahs_copy(xformat_ahs_object(xformat) /*to*/ , ahs /*from*/);

				if (*current_type == SW_ITYPE_h || *current_type == SW_ITYPE_s) {
					att = swheader_get_attribute(infoheader, SW_A_link_source, NULL);
					SWLIB_ASSERT(att != NULL);
					current_link_source = swheaderline_get_value(att, NULL);
					if (!current_link_source) {
						*pstatus = 3;
						SWI_internal_error();
						break;
					}

					E_DEBUG("HERE");
					if ( strcmp(tar_format, "ustar") == 0 &&
					     strlen(current_link_source) > TARLINKNAMESIZE) {
						sw_e_msg(G, "tar link name too long for ustar format, try -H gnu\n");
						/* SWI_internal_error(); */
						*pstatus = 4;
						break;
					}
					if (strcmp(target_path, "/") != 0) {
						swpl_sanitize_pathname(current_link_source);
					}
					taru_set_new_linkname(xformat->taruM, (struct tar_header *)NULL, current_link_source);
					ahsStaticSetPaxLinkname(
						ahs_vfile_hdr(xformat_ahs_object(xformat)),
						 current_link_source);
				} else {
					taru_set_new_linkname(xformat->taruM, (struct tar_header *)NULL, "");
					ahsStaticSetPaxLinkname(ahs_vfile_hdr(xformat_ahs_object(xformat)), "");
				}

				if (pstatbytes) {
					off_t meter_len;
					*pstatbytes = 0;
					meter_len = (off_t)0;
					start_progress_meter(STDOUT_FILENO, l_current_path, meter_len, pstatbytes);
					*swlib_pump_get_ppstatbytes() = pstatbytes;
				}

				if (fileset_fd != vofd) {
					/*
					 * This code path is used in preview mode.
					 */
					xformat_set_ofd(xformat, fileset_fd);
					header_ret = xformat_write_header(xformat);
					xformat_set_ofd(xformat, vofd);
				} else {
					/*
					 * Normal fast path.
					 */
					header_ret = xformat_write_header(xformat);
				}	

				if (header_ret < 0) {
					SWI_internal_error();
					*pstatus = 4;
					break;
				}
				retval += header_ret;


			} else {
				/* FIXME, need to support other types */	
				;
			}
		}
	}
	swheader_restore_state(infoheader, NULL);
	strob_close(relocated_current_path);
	ahs_close(ahs);
	return retval;
}

int
swinstall_lib_determine_catalog_path(STROB * buf, SWI_DISTDATA * distdata,
		int product_number, char * isc_path, int instance_id)
{
	char * prodtag;
	char * prodrev;
	char * prodvendortag;
	int idx;
	int ret;
	STROB * tmp;
	
	E_DEBUG("");
	E_DEBUG2("fp: isc_path: %s", isc_path);
	tmp = strob_open(32);
	strob_strcpy(tmp, isc_path);
	swlib_unix_dircat(tmp, distdata->catalog_bundle_dir1M);

	idx = product_number;  /* for now, always zero */
	prodtag = strar_get(distdata->product_tagsM, idx);
	prodrev = strar_get(distdata->product_revisionsM, idx);
	prodvendortag = strar_get(distdata->vendor_tagsM, idx);

	if (prodtag != NULL) {
		strob_strcpy(buf, strob_str(tmp));
		if (swlib_is_sh_tainted_string(prodtag)) { 
			return 7;
		}
		if (swlib_is_sh_tainted_string(prodrev)) { 
			return 8;
		}
		swlib_unix_dircat(buf, prodtag);
		if (prodrev) {
			swlib_unix_dircat(buf, prodrev);
		} else {
			return 9;
		}
		strob_sprintf(buf, STROB_DO_APPEND, "/%d", instance_id);
		ret = 0;
	} else {
		strob_strcpy(buf, "");
		ret = 1;
	}
	strob_close(tmp);
	return ret;
}

static
int
arfinstall_tar(GB * G, SWI * swi, int ofd,
	int * g_signal_flag, char * target_path, char * catalog_path, VPLOB * swspecs,
	int altofd, int opt_preview, char * pax_read_command_key,
	int alt_catalog_root, int event_fd, struct extendedOptions * opta,
	int keep_old_files, uintmax_t * pstatbytes,
	int allow_missing_files, SWICAT_E * up_e)
{
	int format;
	int vofd;
	int padamount;
	int read_header_ret;
	int retval = 0;
	int cmpret = 0;
	int size_result;
	int taru_ls_verbose_level = -1;
	int signal_flag;
	int * p_signal_flag;
	char * current_path;
	char * l_current_path;
	char * current_type;
	char * current_link_source;
	char * name;
	char * pax_read_command;
	int ifd;
	int fileset_fd;
	int ret;
	int ret2;
	int parseret;
	int is_catalog;
	int data_amount;
	int files_loading_verbose;
	int filecount;
	int optacc_a;
	int atoi_error;
	int files_missing_from_storage;
	int checkinstall_status;
	int do_check_abort;
	int do_enforce_scripts;
	mode_t modemask;
	STROB * resolved_path;
	STROB * namebuf;
	STROB * newnamebuf;
	STROB * catalogdir;
	STROB * tmp;
	STROB * pathtmp;
	STROB * tmp_swpath_ex;
	STROB * execution_phase_script;
	STROB * current_path_expanded;
	STROB * relocated_current_path;
	STROB * relocated_current_linksource;
	STROB * shell_lib_buf;
	STROB * dir_owner_buf;
	AHS * info_ahs2 = ahs_open();
	char * obj;
	char * tar_format;
	int tarheaderflags;
	struct new_cpio_header * file_hdr;
	XFORMAT * xformat = swi->xformatM;
	SWPATH * swpath = swi->swpathM;
	SWPATH_EX * swpath_ex = NULL;
	SWHEADER * infoheader = (SWHEADER*)NULL;
	int sig_block_start;
	int sig_block_end;
	int package_is_signed = -1;
	struct tar_header *ptar_hdr;
	int did_catalog_tar = 0;
	int did_sigfiles = 0;
	int curpos;
	int did_fileset = 0;
	uintmax_t fileset_write_ret = 0;
	uintmax_t payload_size;
	uintmax_t filesize;
	int eoa;
	int header_ret;
	char * log_level_str;
	int log_level;
	char * md5buf;
	char * filemd5;
	char * is_volatile;
	SWI_PRODUCT * current_product;
	SWI_XFILE * current_fileset;
	char * newname_suffix;
	char * p_location;
	char * p_qualifier;
	char * p_directory;
	SWVERID * swverid;
	CISF_PRODUCT * cisf;
	SWI_FILELIST * fl;

	tar_format = swbisoption_get_opta(opta, SW_E_swbis_format);
	E_DEBUG2("tar_format=[%s]", tar_format);

	E_DEBUG2("fp: catalog_path=[%s]", catalog_path);
	signal_flag = 0;
	p_signal_flag = &signal_flag;
	if (g_signal_flag)
		p_signal_flag = g_signal_flag;
	do_check_abort = 0;
	checkinstall_status = -1; /* unset value */
	files_missing_from_storage = 0;
	do_enforce_scripts = swextopt_is_option_true(SW_E_enforce_scripts, opta);

	swpath_ex = swpath_shallow_create_export();
	shell_lib_buf = strob_open(100);
	catalogdir = strob_open(100);
	newnamebuf = strob_open(100);
	tmp_swpath_ex = strob_open(100);
	dir_owner_buf = strob_open(100);
	namebuf = strob_open(100);
	tmp = strob_open(10);
	current_path_expanded = strob_open(10);
	relocated_current_path = strob_open(10);
	relocated_current_linksource = strob_open(10);
	pathtmp = strob_open(10);
	resolved_path = strob_open(10);
	execution_phase_script = strob_open(10);
	tarheaderflags = xformat_get_tarheader_flags(xformat);
	file_hdr = taru_make_header();

	swpath_reset(swpath);
	
	swi->swicolM->verbose_levelM = swi->verboseM;
	swicol_set_verbose_level(swi->swicolM, swi->verboseM);
	format = xformat_get_format(xformat);
	ifd = xformat_get_ifd(xformat);
	if (ifd < 0) {
		SWI_internal_error();
		return -32;		
	}
	taru_set_header_recording(xformat->taruM, 1/*ON*/);
	xformat->taruM->linkrecord_disableM = 1;
	
	if (opt_preview && altofd >= 0) {
		fileset_fd = altofd;
	} else if (opt_preview) {
		fileset_fd = swi->nullfdM;
	} else {
		fileset_fd = ofd;
	}
	
	if (opt_preview) {
		vofd = swi->nullfdM;
	} else {
		vofd = ofd;
	}

	xformat_set_ofd(xformat, vofd);

	ptar_hdr = (struct tar_header*)(strob_str(xformat->taruM->headerM));
	
	log_level_str = get_opta(opta, SW_E_loglevel);
	SWLIB_ASSERT(log_level_str != NULL);
	log_level = swlib_atoi(log_level_str, &atoi_error);	

	files_loading_verbose = (
				(keep_old_files ? keep_old_files : swi->verboseM > SWC_VERBOSE_3) ||
				(log_level >= 2) ||
				(0)
				);

	/* if files_loading_verbose is true, then a 1>&2 is added to the tar command
	   and this results in the file names being compared to error messages, this
	   is harmless but it gives a false appearance of error */
	
	pax_read_command = swc_get_pax_read_command(g_pax_read_commands, pax_read_command_key, 
					files_loading_verbose, 
					keep_old_files, DEFAULT_PAX_R);


	if (swextopt_is_option_true(SW_E_swbis_enforce_file_md5, opta)) {
		md5buf = (char*)(xformat->taruM->md5bufM->str_);
	} else {
		md5buf = NULL;
	}

	/*
	 * FIXME: Eventually support multiple products and filesets.
	 */

	swpl_enforce_one_prod_one_fileset(swi);

	/*
	 * Set the current product and fileset
	 */
	current_product = swi_package_get_product(swi->swi_pkgM, D_ZERO /* the first one */);
	current_fileset = swi_product_get_fileset(current_product, D_ZERO /* the first one */);

	/*
	 * Construct the CISF_PRODUCT object
	 */

	cisf = swpl_cisf_product_create(current_product);

	/*
	 * Initialize the CISF object according to assumptions
	 * of one product and one fileset  FIXME
	 */	

	swpl_cisf_init_single_single(cisf, swi);
	swpl2_audit_cisf_bases(G, swi, cisf);

	/*
	 * Get the fileset size.
	 */
	payload_size = get_delivery_size(swi, &size_result);
	if (size_result) {
		SWBIS_ERROR_IMPL();
		return -31;
	}

	/* get the location spec */
	p_location = (char*)NULL;
	if ((swverid=vplob_val(swspecs, 0)) != NULL) {
		p_location = swverid_get_verid_value(swverid, SWVERID_VERIDS_LOCATION, 1);
	}
	if (p_location == (char*)NULL || strlen(p_location) == 0)
		p_location = "/";

	/* get the qualifier spec */
	if ((swverid=vplob_val(swspecs, 0)) != NULL) {
		p_qualifier = swverid_get_verid_value(swverid, SWVERID_VERIDS_QUALIFIER, 1);
		if (p_qualifier == NULL)
			p_qualifier = "";
	} else {
		p_qualifier = "";
	}
	
	swi->swi_pkgM->locationM = strdup(p_location);	
	swi->swi_pkgM->qualifierM = strdup(p_qualifier);	

	/* get the value of the product.directory attribute */
	p_directory = get_directory(swi);

	infoheader = swi_get_fileset_info_header(swi, 0, 0);
	filecount = swpl_get_fileset_file_count(infoheader);

	fileset_write_ret = 0;
	is_catalog = -1;
	optacc_a = 0;	
	
	/*
	 * ================================
	 * make the catalog entry directory
	 * ================================
	 */

	/* Construct a umask value from the detected mode */

	if (swi->swi_pkgM->installed_catalog_modeM > 0) {
		mode_t mode;
		mode = swi->swi_pkgM->installed_catalog_modeM;
		modemask = (~mode) & (0777);
		/* fprintf(stderr, "%0o %o\n", (unsigned int)modemask, (unsigned int)mode); */
	} else {
		/* default */
		modemask = 02;
	}

	construct_directory_ownership_script(swi, dir_owner_buf,
			get_opta_isc(G->optaM, SW_E_installed_software_catalog));

	E_DEBUG2("swi->swi_pkgM->catalog_entryM=[%s]", swi->swi_pkgM->catalog_entryM);
	if (opt_preview == 0 && retval == 0 && G->g_to_stdout == 0) {
		swicol_set_task_idstring(swi->swicolM, SWBIS_TS_make_catalog_dir);
		strob_sprintf(tmp, 0,
			"(umask %o; mkdir -p \"%s\")</dev/null\n"
			"sw_retval=$?\n"
			"(\n"
			"%s\n"
			")</dev/null\n"
			"dd count=1 bs=512 of=/dev/null 2>/dev/null\n"
			,
			(unsigned int)modemask,
			swi->swi_pkgM->catalog_entryM,
			strob_str(dir_owner_buf)
			);

		ret = swicol_rpsh_task_send_script2(
			swi->swicolM,
			ofd, 
			512, 			/* stdin_file_size */
			target_path, 		/* directory to run in*/
			strob_str(tmp),  	/* the task script */
			SWBIS_TS_make_catalog_dir
			);

		if (ret != 0) {
			sw_e_msg(G, "error from SWBIS_TS_make_catalog_dir\n");
			/* FIXME, set abort condition? */
		}

		ret = etar_write_trailer_blocks(NULL, ofd, 1);
		if (ret < 0) {
			SWBIS_ERROR_IMPL();
			SWI_internal_error();
			return -1;
		}

		ret = swicol_rpsh_task_expect(swi->swicolM,
					event_fd,
					SWICOL_TL_5 /*time limit*/);

		if (ret != 0) {
			return -1;
		}
	}

	if (opt_preview == 0 && retval == 0 && G->g_to_stdout == 0) {
		ret = swpl_session_lock(G, swi->swicolM, target_path, ofd,  event_fd);
		if (ret < 0) {
			sw_e_msg(G, "error from swpl_session_lock: status=%d\n", ret);
			return -1;
		} else if (ret > 0) {
			if (G->g_force_locks == 0)
				return -1;
		}
	}

	/*
	 * =====================================
	 * Loop over the Catalog section files.
	 * =====================================
	 */

	while ((read_header_ret = xformat_read_header(xformat)) > 0 &&
			(*p_signal_flag == 0)) {
		
		E_DEBUG("HERE");
		if (xformat_is_end_of_archive(xformat)){
			/*
			 * This is an unexpected error unless their is no
			 * storage section such as the case of the installed
			 * software catalog or a package with no files.
			 */
			if (swi->does_have_payloadM == 0) {
				break;
			} else {
				fprintf(stderr, "%s: premature end of archive\n",
					swlib_utilname_get());
				return -36;
			}
		}

		optacc_a++;  /* Used for Optimization Hack */

		curpos = uxfio_lseek(ifd, 0L, SEEK_CUR);
		if (curpos < 0) {
			SWBIS_ERROR_IMPL();
		}
		xformat_get_name(xformat, namebuf);
		name = strob_str(namebuf);
		data_amount = xformat_get_filesize(xformat);	
	
		parseret = swpath_parse_path(swpath, name);
		if (parseret < 0) {
			SWBIS_ERROR_IMPL();
			return -38;
		}

		/*
		 * Show the parsed path components
		 */
		swpath_shallow_fill_export(swpath_ex, swpath); 
		if (swi->verboseM >= SWC_VERBOSE_0) {
			sw_d_msg(G,"%s", swpath_ex_print(swpath_ex, tmp_swpath_ex, ""));
		}
		is_catalog = swpath_get_is_catalog(swpath);

		if (is_catalog == SWPATH_CTYPE_DIR) {
			/*
			 * Leading package directories
			 */
			;
		} else if (did_catalog_tar == 0 &&
			is_catalog == SWPATH_CTYPE_CAT &&
			strstr(swpath_get_pathname(swpath), SW_A_INDEX)
		) {
			/*
			 * Write out the catalog.tar file
			 *  <catalog_path>/<export>/catalog.tar
			 */

			/*
			 * Examine the catalog for signature.
			 */
			E_DEBUG("HERE");
			did_catalog_tar = 1;
			swi_examine_signature_blocks(swi->swi_pkgM->dfilesM,
					&sig_block_start, &sig_block_end);
			if (sig_block_start < 0) {
				package_is_signed = 0;
			} else {
				package_is_signed = 1;
			}

			if (opt_preview == 0 && retval == 0) {
				/* TS_Load_catalog */
				ret = send_catalog_tarfile(swi, vofd, 
					catalog_path, pax_read_command,
					alt_catalog_root, event_fd,
					sig_block_start, sig_block_end, SWBIS_TS_Load_catalog);
			} else {
				ret = 0;
			}

			if (ret) {
				retval++;
				E_DEBUG2("retval=%d", retval);
			}
	
			curpos = uxfio_lseek(ifd, curpos, SEEK_SET);
			if (curpos < 0) {
				SWBIS_ERROR_IMPL();
			}

			/*
			 * Exhaust the member data to /dev/null
			 */
			ret = xformat_copy_pass(xformat, swi->nullfdM, ifd);
			if (ret < 0) {
				SWBIS_ERROR_IMPL();
			}
		} else if (
			did_sigfiles == 0 &&
			package_is_signed == 0 &&
			is_catalog == SWPATH_CTYPE_CAT 
		) {
			/*
			 * =====================================
			 * Send empty payload to the task shell if
			 * there are no signatures.
			 * =====================================
			 */
			E_DEBUG("HERE");
			did_sigfiles = 1;
			if (opt_preview == 0 && retval == 0) {
				ret = swpl_send_null_task(swi->swicolM, vofd, event_fd, SWBIS_TS_Load_signatures, SW_SUCCESS);
			} else {
				ret = 0;
			}
			if (ret) {
				retval++;
				E_DEBUG2("retval=%d", retval);
			}
			ret = xformat_copy_pass(xformat, swi->nullfdM, ifd);
		} else if (
			did_sigfiles == 0 &&
			package_is_signed > 0 &&
			is_catalog == SWPATH_CTYPE_CAT &&
			strstr(swpath_get_pathname(swpath), SW_A_signature)
		) {
			/*
			 * Write out all the signatures.
			 *	 <catalog_path>/export/catalog.tar.sig
			 *	 <catalog_path>/export/catalog.tar.sigN
			 */
			E_DEBUG("HERE");
			did_sigfiles = 1;
			filesize = xformat_get_filesize(xformat);	

			if (opt_preview == 0 && retval == 0) {
				/* TS_Load_signatures */
				ret = swpl_send_signature_files(swi, vofd,
					catalog_path, pax_read_command,
					alt_catalog_root, event_fd, ptar_hdr,
					sig_block_start, sig_block_end, filesize);
			} else {
				ret = 0;
			}
			if (ret) {
				retval++;
				E_DEBUG2("retval=%d", retval);
			}

			curpos = uxfio_lseek(ifd, curpos, SEEK_SET);
			if (curpos < 0) {
				SWBIS_ERROR_IMPL();
			}

			E_DEBUG("");
			ret = xformat_copy_pass(xformat, swi->nullfdM, ifd);
			if (ret < 0) {
				SWBIS_ERROR_IMPL();
			}
		} else if (is_catalog == SWPATH_CTYPE_CAT) {
			/*
			 * =====================================
			 * Read past the remainder of the catalog files.
			 * =====================================
			 */
			E_DEBUG("HERE");
			ret = xformat_copy_pass(xformat, swi->nullfdM, ifd);
			if (ret < 0) {
				SWBIS_ERROR_IMPL();
			}
		} else if (did_fileset == 0 && is_catalog == SWPATH_CTYPE_STORE) {
			/*
			 * Now ready to deal with the storage section.
			 */		
			E_DEBUG("HERE");
			break;
		}
	} /* while */

	E_DEBUG("HERE");
	if (read_header_ret < 0) {
		SWBIS_ERROR_IMPL();
		return -50;
	}
	E_DEBUG("HERE");

	/*
	 * Good, made it this far..
	 * Now, the catalog.tar and signature files are loaded
	 * We could check the signature here before unpacking the catalog.tar
	 * file.
	 */

	/*
	 * ==========================================
	 * Send the task script to unpack the catalog
	 * ==========================================
	 */

	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		E_DEBUG("HERE");
		/* TS_catalog_unpack */
		ret = swpl_unpack_catalog_tarfile(G, swi, vofd, catalog_path, pax_read_command, alt_catalog_root, event_fd);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}
	E_DEBUG2("retval = %d", retval);

	E_DEBUG("HERE");
	/*
	 * ==========================================
	 * Send the task script to install the 
	 * session_options file.
	 * ==========================================
	 */

	if (opt_preview == 0 && retval == 0) {
		/* TS_session_options */
		E_DEBUG("HERE");
		ret = write_tar_session_options_file(G,
			swi,
			vofd,
			catalog_path,
			pax_read_command,
			alt_catalog_root,
			event_fd,
			SWBIS_TS_Load_session_options
			);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}
	E_DEBUG2("retval = %d", retval);
	
	/*
	 * ==========================================
	 * Send the task script to load the the 
	 * control.sh file.
	 * ==========================================
	 */

	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/* TS_load_controlsh */
		E_DEBUG("HERE");
		ret = write_tar_controlsh_file(G,
			swi,
			vofd,
			catalog_path,
			pax_read_command,
			alt_catalog_root,
			event_fd,
			SWBIS_TS_Load_control_sh);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}

	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/*
		 * TS_analysis_002
		 */
		E_DEBUG("HERE");
		ret = swpl_run_check_script(G, SW_A_checkinstall, SWBIS_TS_Analysis_002, swi, vofd, event_fd, &checkinstall_status);
		if (ret < 0) {
			/* internal error */
			retval++;
			E_DEBUG2("retval = %d", retval);
		} else {
			/* process the checkinstall enforcement policy 
			   based on the script exit status */
			
			if (checkinstall_status == SW_SUCCESS) {
				;	
			} else if (checkinstall_status == SW_WARNING) {
				;
			} else if (checkinstall_status == SW_ERROR) {
				if (do_enforce_scripts) {
					do_check_abort = 1;
				}
			} else if (checkinstall_status == SW_DESELECT) {
				/* even if do_enforce_scripts == 0, enforce this status
				   as meaning deselect, only if forced override */
				if (G->g_force == 0) {
					do_check_abort = 1;
				}
			} else if (checkinstall_status == -1) {
				/* OK, probably no checkinstall script */
				;
			} else {
				/* unrecognized status */
				sw_e_msg(G, "unrecognized exit status for checkinstall: %d\n", checkinstall_status);
				retval++;
			}

		}

		/* TS_restore_catalog_entry */
		if (up_e && do_check_abort && retval == 0) {
			/* restore the catalog entry */
			ret = swpl_restore_catalog_entry(G, swi->swicolM, up_e,
				vofd, get_opta_isc(G->optaM, SW_E_installed_software_catalog));
			retval++;
		} else {
			/* fulfill the task with nothing successfully */			
			ret = swpl_send_nothing_and_wait(swi->swicolM, vofd, event_fd,
					SWBIS_TS_restore_catalog_entry,
					SWICOL_TL_8, SW_SUCCESS);
		}

		/* SWBIS_TS_continue */
		if (do_check_abort) { 
			retval++;
			ret = SW_ERROR;
		} else {
			ret = SW_SUCCESS;

		}
		ret = swpl_send_nothing_and_wait(swi->swicolM, vofd, event_fd,
			SWBIS_TS_continue, SWICOL_TL_8, ret);

		if (ret != 0) retval++;
	}

	/*
	 * ===================================================
	 * Send the INSTALLED file to set the state to transient
	 * ===================================================
	 */

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/* SWBIS_TS_Load_INSTALLED_90 */
		/*
		 * Audit the control_scripts that must be run
		 */
		E_DEBUG("HERE");
		swpl_audit_execution_scripts(G, swi, swpl_get_script_array());
		
		swpl_update_fileset_state(swi, "*", SW_STATE_TRANSIENT);

		ret = swpl_write_tar_installed_software_index_file(G, swi,
			vofd,
			catalog_path,
			pax_read_command,
			alt_catalog_root,
			event_fd,
			SWBIS_TS_Load_INSTALLED_90, SW_A__INSTALLED, NULL);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}
	E_DEBUG2("retval = %d", retval);

	/*
	 * =============================================
	 * Send the task script for loading the fileset.
	 * =============================================
	 */
	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	did_fileset = 1;	
	strob_strcpy(newnamebuf, target_path);
	ret = 0;
	if (opt_preview == 0 && retval == 0) {
		/* TS_execution_phase */
		E_DEBUG("HERE");
		ret = construct_execution_phase_script(G, cisf, swi,
				 execution_phase_script,
				pax_read_command);

		swicol_set_task_idstring(swi->swicolM, SWBIS_TS_Load_fileset);
		swicol_set_event_fd(swi->swicolM, event_fd);
		ret = swicol_rpsh_task_send_script2(
			swi->swicolM,
			vofd, 
			payload_size,  /* fileset size plus padding */
			swi->swi_pkgM->target_pathM,
			strob_str(execution_phase_script), SWBIS_TS_Load_fileset);
		if (ret == 0) {
			/* wait for clear-to-send */
			ret = swicol_rpsh_wait_cts(swi->swicolM, event_fd);
		}
	}
	if (ret) {
		retval++;
		E_DEBUG2("retval=%d", retval);
	}

	if (strlen(swpath_get_pathname(swpath))) {
		/*
		 * Now seek back 512 bytes.
		 * This is not required if the control directoires are
		 * in the package, but since HP-UX packages don't have the
		 * control directories as archive members then this is
		 * required.
		 */
		/*
	 	 * uh-oh we've dived into the storage section
		 * and there are no control directory archive members.
		 * Meaning the first header of an actual storage file
		 * has just been read.  This happens with HP-UX
		 * packages.
		 */
		E_DEBUG("HERE HP style package");
		if (uxfio_lseek(ifd, -512, SEEK_CUR) < 0) {
			SWI_internal_fatal_error();
		}
	}

	/*
	 * =====================================
	 * Loop over Storage section files.
	 * =====================================
	 */
	E_DEBUG2("retval=%d", retval);
	E_DEBUG2("*p_signal_flag = %d", *p_signal_flag);
	E_DEBUG("HERE at Loop over Storage section files");
	taru_ls_verbose_level = -1;
	while (
		xformat_read_header(xformat) > 0 &&
		*p_signal_flag == 0 &&
		retval == 0
	) {
		E_DEBUG("HERE: ******************************************** Storage");
		if (xformat_is_end_of_archive(xformat)){
			break;
		}
		optacc_a++;  
		E_DEBUG("");
		xformat_get_name(xformat, namebuf);
		name = strob_str(namebuf);
		E_DEBUG2("***************************************** name=[%s]", name);
		parseret = swpath_parse_path(swpath, name);
		E_DEBUG("");
		if (parseret < 0) {
			SWBIS_ERROR_IMPL();
			return -55;
		}
		is_catalog = swpath_get_is_catalog(swpath);
		E_DEBUG2("HERE: Storage: Name is %s", name);
		
		/*
		 * Show the parsed path components
		 */
		swpath_shallow_fill_export(swpath_ex, swpath); 

		sw_d_msg(G,"%s", swpath_ex_print(swpath_ex, tmp_swpath_ex, ""));

		/*
		 * The path is obtained from the archive path
		 */
		current_path = swpath_get_pathname(swpath);
		swlib_expand_escapes(NULL, NULL, current_path, current_path_expanded);
		E_DEBUG("");

		/* An entry in INFO for a archive member of "." has a path of "/" */
		if (strcmp(strob_str(current_path_expanded), ".") == 0) {
			strob_strcpy(current_path_expanded, "/");
		}
		E_DEBUG("");

		/*
		 * Sanitize and check the filename
		 */
		swpl_sanitize_pathname(current_path);
		swlib_squash_leading_dot_slash(current_path);

		E_DEBUG2("current_path before applying location: %s", current_path);
		/*
		 * apply the location and directory attributes
		 */
		swlib_apply_location(relocated_current_path, current_path, p_location, p_directory);
		E_DEBUG2("current_path after applying location: %s", current_path);
	
		swpl_sanitize_pathname(strob_str(relocated_current_path));
		swlib_squash_leading_dot_slash(strob_str(relocated_current_path));
		l_current_path = strob_str(relocated_current_path);

		E_DEBUG("");
		if (optacc_a == 1) {
			/*
			 * Look for the current file at the
			 * current position of the infoheader.
			 * This is an optimization path.
			 */
			E_DEBUG2("HERE: Storage: current_path_expanded is [%s]", strob_str(current_path_expanded));
			obj = swheader_get_object_by_tag(infoheader, SW_OBJ_file, strob_str(current_path_expanded));
		} else {
			/*
			 * this is the failsafe code path
			 */
			E_DEBUG("HERE: Storage");
			obj = (char*)NULL;
		}	

		if (obj == (char*)(NULL)) {
			E_DEBUG2("HERE: obj is NULL for %s", name);
			swheader_reset(infoheader);
		}

		/* Now make a test for storage section control directories */
		E_DEBUG2("HERE: Testing if control directory: %s", l_current_path);
	
		if (l_current_path && strlen(l_current_path)) {
			E_DEBUG2("HERE: Storage: current_path_expanded is [%s]", strob_str(current_path_expanded));
			if (obj == (char*)NULL) {
				E_DEBUG("");
				obj = swheader_get_object_by_tag(infoheader, SW_OBJ_file, strob_str(current_path_expanded));
			}

			if (obj == (char*)NULL) {
				if (xformat_get_tar_typeflag(xformat) == DIRTYPE) {
					/* allow extra leading directories in the storage
					   section since they are unavoidable when lifting
					   the source from the directory form of a 
					   distribution */
					continue;
				} else {
					/*
					* This is a hard error, a storage file must have metadata.
					*/
					fprintf(stderr, "%s: File object not found for path=%s\n", swlib_utilname_get(), current_path);
					fprintf(stderr, "%s: This may be a implementation error or an invalid package\n", swlib_utilname_get());
					SWI_internal_error();
					return -58;
				}
			}

			E_DEBUG("");
			current_type = swheader_get_attribute(infoheader, SW_A_type, NULL);
			current_type = swheaderline_get_value(current_type, NULL);

			if (!current_type) { 
				SWI_internal_error();
				return -59;
			}


			/* Set the flag1 to indicate this software definitiion was
				represented in the storage structure and used */

			swheaderline_set_flag1(obj);
	
			/*
			 * * Here's how to print the file object from the INFO file.
			 * swheaderline_write_debug(obj, STDERR_FILENO);
			 * while((next_attr=swheader_get_next_attribute(infoheader)))
			 * 	swheaderline_write_debug(next_attr, STDERR_FILENO);
			 *  
			 * * Reading the header advances the index pointer, therefore you have
			 * * to seek back or reset and find the object again.
			 *
			 * swheader_reset(infoheader);
			 * obj = swheader_get_object_by_tag(infoheader, SW_OBJ_file, swpath_get_pathname(swpath));
			 */
			
			/*
			 * The metadata in the INFO file trumps the metadata in the
			 * storage file tar header.  For now, just compare and warn if
			 * any are different.  Eventually, in order to be compliant, the
			 * storage section tar header is re-written with the file stats from
			 * the INFO file.
			 */
					
			/*
			 * translate the INFO file attributes to a file_hdr object.
			 */
			E_DEBUG("");
			ahs_init_header(info_ahs2);
			E_DEBUG("");

			/*
 			 * Copy the attributes from the INFO file to the file_hdr struct
 			 */
			ret = copy_attributes(G, infoheader, ahs_vfile_hdr(info_ahs2));
			if (ret) {
				/*
				* FIXME, handle this error better.
				*/
				SWI_internal_error();
				return -62;
			}

			/*
			 * Find the "md5sum" attribute.
			 */
			filemd5 = swpl_get_attribute(infoheader, SW_A_md5sum, (int *)NULL);

			/*
			 * Find the "is_volatile" attribute.
			 */
			is_volatile = swpl_get_attribute(infoheader, SW_A_is_volatile, (int *)NULL);	

			if (swextopt_is_value_true(is_volatile)) {
				/*
				 * The INFO file had is_volatile set to true.
				 * Apply the volatile file policy here.
				 */
				if (swextopt_is_option_true(SW_E_swbis_install_volatile, opta)) {
					/*
					 * Install volatile file.
					 * Determine the suffix from the user options.
					 */

					newname_suffix = get_opta(opta, SW_E_swbis_volatile_newname);
					if (newname_suffix && strlen(newname_suffix)) {
						strob_strcpy(pathtmp, l_current_path);
						/* swpl_safe_check_pathname(newname_suffix); */
						strob_strcat(pathtmp, newname_suffix);
						l_current_path = strob_str(pathtmp);
					} else {
						/*
						 * no suffix specified for volatile files
						 */
						;
					}
				} else {
					/*
					 * skip this file because its volatile and policy excludes it.
					 * we must move the file pointer, just dump the data to * /dev/null
					 */		
					ret = xformat_copy_pass_md5(xformat,
						swi->nullfdM,
						ifd, md5buf);
					SWLIB_ASSERT(ret >= 0);
					optacc_a = 0;
					continue;
				}
			} else {
				/*
				 * nothing to do if not volatile
				 */
				;
			}

			if (	md5buf &&
				(!filemd5 && *current_type == SW_ITYPE_f)
			) {
				sw_e_msg(G, "md5sum attribute not found: %s\n", current_path);
			}

			/*
			 * Compare the header stats from the storage section archive
			 * member to the stats from the INFO file.
			 */
			E_DEBUG("");
			if (sanity_compare(xformat_ahs_object(xformat), 
					info_ahs2,
					swpath_get_pathname(swpath), swi->verboseM >= SWC_VERBOSE_5)) {
				cmpret++;
			}

			/*
			 * handle INFO files where uid and uname are the same string
			 */
			detect_and_correct_numeric_ids(info_ahs2);

			/*
			 * Impose the stats from the INFO file.
			 */				
			ahs_copy(xformat_ahs_object(xformat), info_ahs2);

			E_DEBUG("");
			/*
			 * Set the sanitized, relocated name.
			 */
			ahsStaticSetTarFilename(
				ahs_vfile_hdr(xformat_ahs_object(xformat)),
				l_current_path);

			/*
			 * Set the link name.
			 */
			if (*current_type == SW_ITYPE_h || *current_type == SW_ITYPE_s) {
				current_link_source = swheader_get_attribute(infoheader, SW_A_link_source, NULL);
				current_link_source = swheaderline_get_value(current_link_source, NULL);
				if (!current_link_source) { SWI_internal_error(); return -68; }
				if (swlib_check_clean_relative_path(current_link_source) == 0) {
					/*
 					 * If path is relative do not apply location modification
 					 * FIXME: is this the right thing to do ?
 					 *
 					 * FIXME: maybe need to determine if part of the 'directory' attribute
 					 * is part of the relative path. and also process any ../ to see if they
 					 * intersect any of the 'directory''s levels
 					 */
					E_DEBUG2("relative linksource [%s]", current_link_source);
					strob_strcpy(relocated_current_linksource, current_link_source);
				} else {
					/*
 					 * Link source is absolute
 					 */
					if (
						(!p_directory || strcmp(p_directory, "/") == 0) &&
						(!p_location || strcmp(p_location, "/") == 0)
					) {
						/*
 						 * Both are nul or '/' (root default),
 						 * Do nothing, this is a special case to preserve the linksource
 						 * exactly as it is in the package
 						 */
						E_DEBUG2("absolute linksource: [%s]", current_link_source);
						strob_strcpy(relocated_current_linksource, current_link_source);
					} else {
						/*
 						 * apply relocation
 						 */
						swlib_apply_location(relocated_current_linksource, 
							current_link_source, p_location, p_directory);
						E_DEBUG2("current_path after applying location: %s",
							strob_str(relocated_current_linksource));
						if (strcmp(target_path, "/") == 0) {
							/* swpl_safe_check_pathname(current_link_source); */
						} else {
							swpl_sanitize_pathname(strob_str(relocated_current_linksource));
						}
						swlib_squash_all_leading_slash(strob_str(relocated_current_linksource));
					}
				}

				taru_set_new_linkname(xformat->taruM,
						(struct tar_header *)NULL,
						strob_str(relocated_current_linksource));
				ahsStaticSetPaxLinkname(
					ahs_vfile_hdr(xformat_ahs_object(xformat)),
					strob_str(relocated_current_linksource));
			} else {
				taru_set_new_linkname(xformat->taruM, (struct tar_header *)NULL, "");
				ahsStaticSetPaxLinkname(ahs_vfile_hdr(xformat_ahs_object(xformat)), "");
			}

			if (pstatbytes) {
				off_t meter_len;
				*pstatbytes = 0;
				if (*current_type == SW_ITYPE_f) {
					meter_len = (off_t)(info_ahs2->file_hdrM->c_filesize);
					if (meter_len && meter_len < 512) meter_len = 512;
				} else {
					meter_len = (off_t)0;
				}	
				start_progress_meter(STDOUT_FILENO, l_current_path, meter_len, pstatbytes);
				*swlib_pump_get_ppstatbytes() = pstatbytes;
			}

			/*
			 * Write the tar header to the target for real.
			 */
			if (fileset_fd != vofd) {
				/*
				 * This code path is used in preview mode and writing the
				 * archive to stdout.
				 */
				xformat_set_ofd(xformat, fileset_fd);
				E_DEBUG("");
				header_ret = xformat_write_header(xformat);
				xformat_set_ofd(xformat, vofd);
			} else {
				/*
				 * Normal fast path.
				 */
				header_ret = xformat_write_header(xformat);
			}	
	
			if (header_ret < 0) {
				SWI_internal_error();
				return -76;
			}	
			fileset_write_ret += header_ret;
			
			if (*current_type == SW_ITYPE_f) {
				E_DEBUG("Entering xformat_copy_pass_md5");
				ret = xformat_copy_pass_md5(xformat,
						fileset_fd,
						ifd, md5buf);
				E_DEBUG("Finished xformat_copy_pass_md5");
			} else {
				ret = 0;
			}

			if (ret < 0) {
				fprintf(stderr, "error sending file data for file %s\n", l_current_path);
				return -77;
			} else {
				if (pstatbytes) {
					*pstatbytes = ret;
				}
				if (*current_type == SW_ITYPE_f) {

					/*
					 * Sanity check
					 */

					if (ret < (int)(info_ahs2->file_hdrM->c_filesize)) {
						SWBIS_ERROR_IMPL();
					}
				}
				fileset_write_ret += ret;
			}

			if (pstatbytes) {
				update_progress_meter(SIGALRM);
				stop_progress_meter();
			}

			if (
				(opt_preview && swi->verboseM >= SWC_VERBOSE_2) ||
				(swi->verboseM >= SWC_VERBOSE_2 && header_ret > 0) ||
				(log_level >= SWC_VERBOSE_2)
			) {
				int do_write_at_level;

				/*
				 * write a long ls listing.
				 */

				/*
				 * Read and decode the tar header that was just written
				 * to the target.
				 */

				taru_read_in_tar_header2(xformat->taruM,
					ahs_vfile_hdr(xformat_ahs_object(xformat)),
					-1,
					strob_str(xformat->taruM->headerM),
					&eoa,
					xformat_get_tarheader_flags(xformat), TARRECORDSIZE);
		
				/*
				 * Print and write the line.
				 */

				do_write_at_level = opt_preview ? 1 : SWC_VERBOSE_3;
				if (log_level >= 2 && (&(G->g_logspec))->logfdM >= 0) {
					do_write_at_level = 1;
				}
		
				if (taru_ls_verbose_level < 0)	
					taru_ls_verbose_level = swpl_determine_tar_listing_verbose_level(swi);	

				taru_print_tar_ls_list(tmp,
					ahs_vfile_hdr(xformat_ahs_object(xformat)),
					taru_ls_verbose_level);

				if (swi->swc_idM == SWC_U_L) {
					/* swlist utility */
					uxfio_write(G->g_t_efd, strob_str(tmp), strob_strlen(tmp));
				} else {
					/* send to logger process */
					swlib_doif_writef(swi->verboseM, do_write_at_level,
						&G->g_logspec, G->g_t_efd, "%s", strob_str(tmp));
				}
			}

			if (md5buf && *current_type == SW_ITYPE_f) {
				if (filemd5 == NULL) {
					sw_e_msg(G, "Warning: md5sum attribute not found in INFO file for %s\n", current_path);
				}
				if (strlen(md5buf) && filemd5) {
					if (strcmp(md5buf, filemd5)) {
						sw_e_msg(G, "md5sum mismatch archive=%s INFO=%s: %s\n", md5buf, filemd5, current_path);
						return -78;
					} else {
						sw_d_msg(G, "%s %s\n", filemd5, current_path);
					}
				}
			}

			if (fileset_write_ret % 512) {
				SWBIS_ERROR_IMPL();
				return -79;
			}
		} else {
			/*
			 * This code path occurs for storage section control directories.
			 */	
			E_DEBUG3("HERE: control directory or empty path: [%s] [%s]", strob_str(namebuf), current_path);
			ret = 0;
		}
		optacc_a = 0;
		if (ret < 0) {
			SWBIS_ERROR_IMPL();
		}
		E_DEBUG("HERE: Storage");
	} /* while */

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (cmpret) {
		fprintf(stderr,
		"swinstall: ==Warning== A mismatch between the INFO file and storage section\n"
		"swinstall: ==Warning== file attributes was detected.  Currently, swinstall\n"
		"swinstall: ==Warning== *does* override all storage section attributes with\n"
		"swinstall: ==Warning== the INFO file attributes. This warning will be removed\n"
		"swinstall: ==Warning== in a future release.\n");
	}

	/* Now make sure that there were no files missing
		from the storage section, the swheaderline flag1 
		is set for all files that were found in the 
		storage section */

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (*p_signal_flag == 0 && retval == 0) {
		/* The routine swheaderline_set_flag1() called above
		   tags the file objects as they are installed */
		ret = swpl_assert_all_file_definitions_installed(swi, infoheader);
	} else {
		ret = 0;
	}

	if (ret) {
		/* some files are missing, it is acceptable that
		   directories and non-REGTYPE files not appear in the
		   storage section.  Send tar headers for these files based on
		   the attributes in the INFO file and if all are non-REGTYPE then
		   avert this error. */
		ret2 = 1;
		ret = send_files_with_no_storage(G, swi, infoheader, xformat, fileset_fd, vofd, &ret2, target_path,
						p_location, p_directory, pstatbytes, tar_format);
		fileset_write_ret += ret;
	} else {
		ret2 = 0;
	}

	if (ret2) {
		/*
		 * this is an error
		 */
		E_DEBUG("STORAGE FILE error");
		if (allow_missing_files == 0) {
			sw_e_msg(G, "storage section has missing files: error status %d\n", ret2);
		}
		files_missing_from_storage = 1;
	}

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (opt_preview == 0) {
		if (fileset_write_ret > (payload_size - (uintmax_t)(1024))) {
			/*
			 * This is a bad error.
			 * No room left to send the tar trailer blocks.
			 */
			SWBIS_ERROR_IMPL();
			return -81;	
		}
	}

	/*
	 * Now send the tar trailer blocks and padding that
	 * the remote dd(1) process is exepecting.
	 */

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (swi->verboseM > SWC_VERBOSE_10)
	{
		STROB * xx_tmp2 = strob_open(10);
		STROB * xx_tmp3 = strob_open(10);

		swlib_umaxtostr(payload_size, xx_tmp2);
		swlib_umaxtostr(fileset_write_ret, xx_tmp3);
		fprintf(stderr, "payload size = %s fileset_write_size =%s \n",
			swlib_umaxtostr(payload_size, xx_tmp2),
			swlib_umaxtostr(fileset_write_ret, xx_tmp3));
		fprintf(stderr, "pad amount = %lu\n",  (unsigned long)(payload_size - fileset_write_ret));
		strob_close(xx_tmp2);
		strob_close(xx_tmp3);
	}

	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (retval == 0) {
		padamount = (int)(payload_size - fileset_write_ret);
		ret = swlib_pad_amount(vofd, padamount);
		E_DEBUG("HERE");
		if (ret != padamount) {
			if (*p_signal_flag == 0) {
				/* this may happen when ctl-c is given by the user */
				SWBIS_ERROR_IMPL();
				fprintf(stderr, "%s: swlib_pad_amount returned [%d]\n", swlib_utilname_get(), padamount);
			}
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}

	/*
	 * This waits on the fileset loading task.
	 */
	E_DEBUG2("retval=%d", retval);
	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/*
		 * TS_execution_phase
		 */

		E_DEBUG("HERE");
		ret = swicol_rpsh_task_expect(swi->swicolM,
				event_fd,
				SWICOL_TL_1d /*time limit of 1 day*/);
		if (ret != 0) {
			E_DEBUG("HERE");
			sw_e_msg(G, "error loading fileset\n");
			return -85;
		}

		if (swi->debug_eventsM)
			swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, -1);

		/*
		 * Now we must read the events stack and update the script
		 * retsults.
		 */

		E_DEBUG("HERE");
		swpl_update_execution_script_results(swi, swi->swicolM, swpl_get_script_array());
		E_DEBUG("HERE");
		swpl2_update_execution_script_results(swi, swi->swicolM, cisf); 

		E_DEBUG("HERE");

	} else {
		E_DEBUG2("retval=%d", retval);
		E_DEBUG("HERE opt_preview mode");
		if (
			opt_preview
		) {
			/*
		 	 * This satisfies the preview task shell
			 * which serves to make the script wait until
			 * it receives this script.
			 */
			/* TS_preview */
			E_DEBUG("HERE at " SWBIS_TS_preview_task);
			ret = swpl_send_nothing_and_wait(swi->swicolM, ofd, event_fd, SWBIS_TS_preview_task , SWICOL_TL_8, SW_SUCCESS);
			E_DEBUG("HERE at " SWBIS_TS_preview_task);
			if (ret) { 
				retval++;
				E_DEBUG2("retval = %d", retval);
			}
		}
	}

	/*
	 * ====================================================
	 * Send the task script to remove the unpacked catalog
	 * ====================================================
	 *
	 * This removes the directory, for example
	 *    var/lib/swbix/catalog/foo/foo/1.1/0/export
	 */

	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/* TS_catalog_dir_remove */
		ret = swpl_remove_catalog_directory(swi, vofd, catalog_path, pax_read_command, alt_catalog_root, event_fd);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}

	/*
	 * ===================================================
	 * Send the INSTALLED file
	 * ===================================================
	 */

	E_DEBUG("HERE");
	if (opt_preview == 0 && retval == 0) {
		/*  SWBIS_TS_Load_INSTALLED_91 */
		swpl_update_fileset_state(swi, "*", SW_STATE_INSTALLED);
		ret = swpl_write_tar_installed_software_index_file(G, swi,
			vofd,
			catalog_path,
			pax_read_command,
			alt_catalog_root,
			event_fd,
			SWBIS_TS_Load_INSTALLED_91, SW_A__INSTALLED, NULL);
		if (ret) {
			retval++;
			E_DEBUG2("retval = %d", retval);
		}
	}


	/*
	 * ===================================================
	 * Rename the _INSTALLED to INSTALLED (make it live)
	 * ===================================================
	 */

	if (opt_preview == 0 && retval == 0) {
		ret = swpl_run_make_installed_live(G, swi, ofd, target_path);
		if (ret != 0) {
			return -1;
		}
	}

	/*
	 * ===================================================
	 * Done
	 * ===================================================
	 */
	E_DEBUG("HERE");

	if (swi->debug_eventsM) {
		swicol_show_events_to_fd(swi->swicolM, STDERR_FILENO, 0); 
	}

	strob_close(shell_lib_buf);
	strob_close(catalogdir);
	strob_close(newnamebuf);
	strob_close(namebuf);
	strob_close(tmp);
	strob_close(current_path_expanded);
	strob_close(tmp_swpath_ex);
	strob_close(dir_owner_buf);
	strob_close(resolved_path);
	strob_close(execution_phase_script);
	strob_close(relocated_current_path);
	strob_close(relocated_current_linksource);
	taru_free_header(file_hdr);
	ahs_close(info_ahs2);

	if (allow_missing_files) {
		E_DEBUG2("HERE returning %d", retval);
		return retval;
	} else {
		E_DEBUG2("HERE returning %d", retval || files_missing_from_storage);
		if (files_missing_from_storage) {
			sw_e_msg(G, "missing files, use --allow-missing-files to allow\n");
		}
		return retval || files_missing_from_storage;
	}
}

static
int
write_target_install_script(GB * G,
		int ofd, 
		char * fp_targetpath, 
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
		int opt_preview,
		char * g_sh_dash_s,
		int alt_catalog_root
		)
{
	int retval = 0;
	int ret;
	int is_target_trailing_slash;
	int cmd_verbose = 0;
	char * xx;
	char * pax_read_command;
	char * opt_char_preview;
	char * opt_char_to_stdout;
	char * opt_force_lock;
	char * opt_allow_no_lock;
	char * ignore_scripts;
	STROB * lockfrag_buffer;
	STROB * unlockfrag_buffer;
	STROB * catlockbuffer;
	STROB * buffer_new;
	STROB * tmp;
	STROB * tmpexit;
	STROB * to_devnull;
	STROB * set_vx;
	STROB * subsh, * subsh2;
	STROB * shell_lib_buf;
	char * target_dirname, *source_basename, *target_basename;
	char * targetpath;
	char * x_targetpath;
	char umaskbuf[10];
	char * debug_task_shell;

	SWLIB_ASSERT(fp_targetpath != NULL);
	targetpath = strdup(fp_targetpath);

	if (swlib_check_safe_path(targetpath)) { return 2; }
	if (swlib_check_safe_path(sourcepath)) { return 3; }
	
	lockfrag_buffer = strob_open(64);
	unlockfrag_buffer = strob_open(64);
	buffer_new = strob_open(64);
	catlockbuffer = strob_open(64);
	tmp = strob_open(10);
	tmpexit = strob_open(10);
	to_devnull = strob_open(10);
	set_vx = strob_open(10);
	subsh = strob_open(10);
	subsh2 = strob_open(10);
	shell_lib_buf = strob_open(10);

	if (G->g_do_task_shell_debug == 0) {
		debug_task_shell="";
	} else {
		debug_task_shell="x";
	}

	if (swextopt_is_option_true(SW_E_swbis_ignore_scripts, G->optaM)) {
		ignore_scripts="yes";
	} else {
		ignore_scripts="no";
	}

	if (G->g_force_locks) {
		opt_allow_no_lock = "true";
		opt_force_lock = "true";
	} else {
		opt_allow_no_lock = "";
		opt_force_lock = "";
	}

	/*
	 * Run sanity checks on the distribution tag
	 * and product tags and revisions.
	 */

	if (swlib_is_sh_tainted_string(installed_software_catalog)) { return 4; }
	if (swlib_is_sh_tainted_string(distdata->dist_tagM)) { return 5; }
	if (swlib_is_sh_tainted_string(distdata->catalog_bundle_dir1M)) { return 6; }

	/*
	 * Always treat the installed_software catalog path
	 * as being relative to the target_path
	 */

	strob_strcpy(catlockbuffer, installed_software_catalog);
	swlib_unix_dircat(catlockbuffer, distdata->catalog_bundle_dir1M);

	/*
	 * catlockbuffer contents:
	 * <installed_software_catalog>/<bundle_tag>
	 * This is the name that is used as the lock file name
	 */

	if (G->g_to_stdout == 0) {
		opt_char_to_stdout = "";
	} else {
		opt_char_to_stdout = "True";
	}
	
	if (opt_preview == 0) {
		opt_char_preview = "";
	} else {
		opt_char_preview = "True";
	}

	if (vlv <= SWC_VERBOSE_4 && keep_old_files == 0) {
		strob_strcpy(to_devnull, "2>/dev/null");
	}

	if (vlv >= SWC_VERBOSE_7 && keep_old_files == 0) {
		cmd_verbose = SWC_VERBOSE_1;
	}
	
	if (vlv >= SWC_VERBOSE_SWIDB) {
		strob_strcpy(set_vx, "set -vx\n");
	}

	pax_read_command = swc_get_pax_read_command(g_pax_read_commands, pax_read_command_key, 
					cmd_verbose, 
					keep_old_files, DEFAULT_PAX_R);

	swc_print_umask(umaskbuf, sizeof(umaskbuf));

	is_target_trailing_slash = (strlen(targetpath) && 
				targetpath[strlen(targetpath) - 1] == '/');

	sw_d_msg(G, "swc_write_target_copy_script : source_type [%s]\n", strob_str(control_message));

	/*
	 * Squash the trailing slash.
	 */

	swlib_squash_double_slash(targetpath);
	if (strlen(targetpath) > 1) {
		if (targetpath[strlen(targetpath) - 1] == '/' ) {
			targetpath[strlen(targetpath) - 1] = '\0';
		}
	}

	strob_strcpy(tmp, "");	
	swlib_dirname(tmp, targetpath);
	target_dirname = strdup(strob_str(tmp));
	swlib_basename(tmp, sourcepath);
	source_basename = strdup(strob_str(tmp));
	swlib_basename(tmp, targetpath);
	target_basename = strdup(strob_str(tmp));

	/*
	 * Unpack the archive using pax
	 * The source control string was 
	 * SWBIS_SWINSTALL_SOURCE_CTL_DIRECTORY
	 */

	x_targetpath = targetpath;
	
	strob_strcpy(buffer_new, "");

	if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "detect") == 0) {
		swpl_bashin_detect(buffer_new);
	} else if (strcmp(get_opta(G->optaM, SW_E_swbis_shell_command), "posix") == 0) {
		swpl_bashin_posixsh(buffer_new);
	} else {
		swpl_bashin_testsh(buffer_new, get_opta(G->optaM, SW_E_swbis_shell_command));
	}

	strob_sprintf(buffer_new, STROB_DO_APPEND,
	"IFS=\"`printf ' \\t\\n'`\"\n"
	"trap '/bin/rm -f ${LOCKPATH}.lock; exit 1' 1 2 15\n"
	"echo " SWBIS_TARGET_CTL_MSG_125 ": " KILL_PID "\n"
	"echo " SWBIS_TARGET_CTL_MSG_129 ": \"`pwd | head -1`\"\n"
	CSHID
	"%s\n" /* Session Begins */
	"%s"
	"opt_allow_no_lock=\"%s\"\n"
	"opt_force_lock=\"%s\"\n"
	"sw_retval=0\n"
	"export PATH\n"
	"export isc_path\n"
	"export swutilname\n"
	"export swbis_ignore_scripts\n"
	"export opt_preview\n"
	"export opt_to_stdout\n"
	"export LOCKPATH\n"
	"export LOCKENTRY\n"
	"swutilname=swinstall\n"
	"LOCKENTRY=$$\n"
	"PATH=`getconf PATH`:$PATH\n"
	"swbis_ignore_scripts=\"%s\"\n"
	"%s"
	"%s\n"			/* shls_bashin from shell_lib.sh */
	"%s\n"			/* shls_false_ from shell_lib.sh */
	"%s\n"			/* lf_ lock routine */
	"%s\n"			/* lf_ lock routine */
	"%s\n"			/* lf_ lock routine */
	"%s\n"			/* lf_ lock routine */
	"%s\n"			/* shls_make_dir_absolute */
	"opt_preview=\"%s\"\n"
	"opt_to_stdout=\"%s\"\n"
	"LOCKPATH=\"%s\"\n"
	"umask %s\n"
	"blocksize=\"%s\"\n"
	"isc_path=\"%s\"\n"
	"xtarget=\"%s\"\n"
	"wcwd=\"`pwd`\"\n"
	"export sh_dash_s\n"
	"d_sh_dash_s=\"%s\"\n"
	"case \"$5\" in PSH=*) eval \"$5\";; *) unset PSH ;; esac\n"
	"sh_dash_s=\"${PSH:=$d_sh_dash_s}\"\n"
	"debug_task_shell=\"%s\"\n"
	CSHID
	"if test -f \"$xtarget\" -o -p \"$xtarget\" -o -b \"$xtarget\" -o -c \"$xtarget\"; then\n"
			/*
			* error
			* The target must be a directory or be non-existent.
			*/
	"	sw_retval=1\n"
	"	if test \"$sw_retval\" != \"0\"; then\n"
	"		 dd count=1 of=/dev/null 2>/dev/null\n"
	"	fi\n"
	"	%s\n" 				/* SW_SESSION_ENDS: status=1 */
	"	exit $sw_retval\n"
	"fi\n"
	CSHID
	"#\n"
	"# Start of code to make absolute\n"
	"#\n"
	"case \"$xtarget\" in\n"
	"/*)\n"
	";;\n"
	"*)\n"
	"mda_pwd=\"$wcwd\"\n"
	"mda_target_path=\"$xtarget\"\n"
	"shls_make_dir_absolute\n"
	"xtarget=\"$mda_target_path\"\n"
	"# xtarget is now absolute\n"
	";;\n"
	"esac\n"
	"#\n"
	"# END of code to make absolute\n"
	"#\n"
	"cd \"$xtarget\" 2>/dev/null\n"
	"sw_retval=$?\n"
	"cd_retval=$sw_retval\n"
	"if test \"$xtarget\" = '.'; then\n"
	"	sw_retval=1\n"
	"else\n"
	"	if test $sw_retval !=  \"0\"; then\n"
	"		if test \"$opt_preview\" != \"True\"; then\n"
	"			mkdir -p \"$xtarget\"\n"
	"			sw_retval=$?\n"
	"		else\n"
	"			xtarget=/\n"
	"			sw_retval=1\n"
	"		fi\n"
	"	else\n"
	"		# cd \"$xtarget\"\n"
	"		sw_retval=1\n"
	"	fi\n"
	"fi\n"
	"case \"$sw_retval\" in\n"
	"	0)\n"
	"	%s\n"  /* SOC_CREATED */
	"	;;\n"
	"esac\n"
	CSHID

	"case \"$cd_retval\" in\n"
	"0)\n"
	"sw_retval=0\n"
	";;\n"
	"*)\n"
	"cd \"$xtarget\" 2>/dev/null\n"
	"sw_retval=$?\n"
	";;\n"
	"esac\n"

	"if test -d \"$isc_path\"; then\n"
	"	echo 1>/dev/null\n"
	"else\n"
	"	if test \"$opt_preview\" != \"True\"; then\n"
	"		(umask 027; mkdir -p \"$isc_path\"; exit $?)\n"
	"		sw_retval=$?\n"
	"	fi\n"
	"fi\n"

	/* ---------------- */

	"swexec_status=0\n"
	"export swexec_status\n"
	CSHID
	"case \"$sw_retval\" in\n"
	"	0)\n"
	"		# the value of catpath has no affect\n" 
	"		catpath=\"" SW_UNUSED_CATPATH  "\"\n" 
	"		echo " SWBIS_TARGET_CTL_MSG_128 ": $catpath\n"
	"		;;\n"
	"	*)\n"
	"		swexec_status=1\n"
	"		echo " SWBIS_TARGET_CTL_MSG_508 "\n"
	"		exit 1\n"
	"		;;\n"
	"esac\n"
	CSHID
	"shls_bashin2 \"" SWBIS_TS_uts "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_uts ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_get_catalog_perms ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	"case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"swexec_status=$sw_retval\n"

	"case $sw_retval in 0) ;; *) swexec_status=1; sw_retval=1; ;; esac\n"
	"case $swexec_status in	0) %s ;; *) shls_false_ ;; esac\n" 	/* SW_ANALYSIS_BEGINS */
	"case $sw_retval in 0) ;; *) swexec_status=1; sw_retval=1; ;; esac\n"
	"case \"$sw_retval\" in\n"
	"	0)\n"
	"		;;\n"
	"	*)\n"
	"		swexec_status=1\n" 			/* error! Remove the catalog directory */
	"		sw_retval=1\n"
	"		rmdir \"$catpath\" 2>/dev/null\n"
	"		;;\n"
	"esac\n"
	"case \"$opt_to_stdout\" in\n"
	"True)\n"
	"	;;\n"
	"*)\n"

	CSHID
	"	shls_bashin2 \"" SWBIS_TS_Get_iscs_listing "\"\n"
	"       sw_retval=$?\n"
	"       case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Get_iscs_listing ";; *) shls_false_;; esac\n"
	"       sw_retval=$?\n"
	"	case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"       swexec_status=$sw_retval\n"
	
	"	shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing ";; *) shls_false_;; esac\n"
	"	sw_retval=$?\n"
	"	case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"	swexec_status=$sw_retval\n"

	CSHID
	"	shls_bashin2 \"" SWBIS_TS_Get_iscs_entry "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Get_iscs_entry  ";; *) shls_false_;; esac\n"
	"	sw_retval=$?\n"
	"	case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"	swexec_status=$sw_retval\n"

	"	shls_bashin2 \"" SWBIS_TS_check_OVERWRITE "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) $sh_dash_s " SWBIS_TS_check_OVERWRITE";; *) shls_false_;; esac\n"
	"	sw_retval=$?\n"
	"	case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"	swexec_status=$sw_retval\n"

	"	shls_bashin2 \"" SWBIS_TS_Do_nothing "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Do_nothing ";; *) shls_false_;; esac\n"
	"	sw_retval=$?\n"
	"	case \"$opt_preview\" in \"\") ;; *) sw_retval=0;; esac\n"
	"	swexec_status=$sw_retval\n"
	
	CSHID
	"	shls_bashin2 \"" SWBIS_TS_remove_catalog_entry "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) wwf_did=1; $sh_dash_s " SWBIS_TS_remove_catalog_entry  ";; *) wwf_did=0; shls_false_;; esac\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in\n"
	"		0)\n"
	"			;;\n"
	"		*)\n"
	"			case $wwf_did in\n"
	"				1)\n"
	"				echo \"swinstall: catalog entry removal failed\" 1>&2\n"
	"				sw_retval=0 # ignore this error ??\n"
	"				;;\n"
	"			esac\n"
	"			;;\n"
	"	esac\n"
	"	swexec_status=$sw_retval\n"

	"	;;\n"
	"esac\n"

	CSHID
	"shls_bashin2 \"" SWBIS_TS_check_status "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_check_status ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	CSHID
	"case \"$opt_preview\" in\n"
	"True)\n"
	"	shls_bashin2 \"" SWBIS_TS_preview_task "\"\n"
	"	sw_retval=$?\n"
	"	case $sw_retval in 0) $sh_dash_s " SWBIS_TS_preview_task ";; *) shls_false_;; esac\n"
	"	# Here is where the preview option scuttles the rest of the script\n"
	"	# by setting swexec_status=1\n"
	"	swexec_status=1\n"
	"	opt_to_stdout=\"True\"\n"
	"	;;\n"
	"esac\n"
	"case \"$opt_to_stdout\" in\n"
	"True)\n"
	"	swexec_status=1\n"
	"	;;\n"
	"*)\n"
	"	;;\n"
	"esac\n"

	CSHID
	"shls_bashin2 \"" SWBIS_TS_make_catalog_dir "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_make_catalog_dir ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"%s\n" /* <<<<----- here's the code to lock a session */

	"shls_bashin2 \"" SWBIS_TS_Load_catalog "\"\n"
	"sw_retval=$?\n"
	CSHID
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_catalog ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_Load_signatures "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_signatures ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_Catalog_unpack "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Catalog_unpack ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_Load_session_options "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_session_options   ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
 	
	"shls_bashin2 \"" SWBIS_TS_Load_control_sh "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_control_sh  ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	
	"shls_bashin2 \"" SWBIS_TS_Analysis_002 "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Analysis_002 ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_restore_catalog_entry "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_restore_catalog_entry ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_continue "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_continue ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	
	CSHID
	"if [ \"$opt_preview\" = \"\" ]; then\n"
	"	%s\n"							/* SW_ANALYSIS_ENDS */
	"fi\n"
	
	"case $swexec_status in	0) %s ;; *) shls_false_ ;; esac\n" 	/* echo SW_EXECUTION_BEGINS */
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	
	"shls_bashin2 \"" SWBIS_TS_Load_INSTALLED_90 "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_INSTALLED_90  ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_Load_fileset "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_fileset   ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	"shls_bashin2 \"" SWBIS_TS_Catalog_dir_remove "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Catalog_dir_remove  ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	
	"shls_bashin2 \"" SWBIS_TS_Load_INSTALLED_91 "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_Load_INSTALLED_91 ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"
	
	"shls_bashin2 \"" SWBIS_TS_make_live_INSTALLED  "\"\n"
	"sw_retval=$?\n"
	"case $sw_retval in 0) $sh_dash_s " SWBIS_TS_make_live_INSTALLED ";; *) shls_false_;; esac\n"
	"sw_retval=$?\n"
	"swexec_status=$sw_retval\n"

	CSHID
	"%s\n" /* <<<<----- here's the code to unlock a session */

	"case $swexec_status in	0) %s ;; *) shls_false_ ;; esac\n" 	/* SW_EXECUTION_ENDS */
	"case $sw_retval in 0) ;; *) swexec_status=1; sw_retval=1; ;; esac\n"
	"if [ \"$opt_preview\" ]; then\n"
	"	swexec_status=0\n"
	"	sw_retval=0\n"
	"fi\n"
	"sleep %d\n"
	"%s\n" /* SWI_MAIN_SCRIPT_ENDS */
	"%s\n" /* Session Ends */
	"%s\n"
	,
/*_% */	TEVENT(2, vlv, SW_SESSION_BEGINS, ""),
/*_% */	swicol_brace_marks(subsh, "target", 'L', nhops, vlv),
/*_% */	opt_force_lock,
/*_% */	opt_allow_no_lock,
/*_% */	ignore_scripts,
/*_% */	strob_str(set_vx),
/*_% */	shlib_get_function_text_by_name("shls_bashin2", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_false_", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_name", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_make_lockfile_entry", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_test_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("lf_remove_lock", shell_lib_buf, NULL),
/*_% */	shlib_get_function_text_by_name("shls_make_dir_absolute", shell_lib_buf, NULL),
/*_% */	opt_char_preview,
/*_% */	opt_char_to_stdout,
/*_% */	strob_str(catlockbuffer),
/*_% */	umaskbuf,
/*_% */	blocksize,
/*_% */	installed_software_catalog,
/*_% */	x_targetpath, 
/*_% */	g_sh_dash_s,
/*_% */	debug_task_shell,	/* debug_task_shell */	
/*_% */	TEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
/*_% */	TEVENT(2, vlv, SW_SOC_CREATED, x_targetpath),
/*_% */	TEVENT(2, vlv, SW_ANALYSIS_BEGINS, ""),
	swpl_shellfrag_session_lock(lockfrag_buffer, vlv),
/*_% */	TEVENT(2, 1/*verbose level*/, SW_ANALYSIS_ENDS, "status=$sw_retval"),
/*_% */	TEVENT(2, vlv, SW_EXECUTION_BEGINS, ""),
	swpl_shellfrag_session_unlock(unlockfrag_buffer, vlv),
/*_% */	TEVENT(2, vlv, SW_EXECUTION_ENDS, "status=$sw_retval"),
/*_% */	delaytime,
	TEVENT(2, -1, SWI_MAIN_SCRIPT_ENDS, "status=0"),
/*_% */	TEVENT(2, vlv, SW_SESSION_ENDS, "status=$sw_retval"),
/*_% */	swicol_brace_marks(subsh2, "install_target", 'R', nhops, vlv)
	);

	xx = strob_str(buffer_new);
	ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, xx, strlen(xx));
	if (ret != (int)strlen(xx)) {
		return 1;
	}
        if (G->g_target_script_name) {
                swlib_tee_to_file(G->g_target_script_name, -1, xx, -1, 0);
        }
	free(targetpath);
	free(source_basename);
	free(target_dirname);
	free(target_basename);

	strob_close(lockfrag_buffer);
	strob_close(unlockfrag_buffer);
	strob_close(catlockbuffer);
	strob_close(tmp);
	strob_close(tmpexit);
	strob_close(set_vx);
	strob_close(to_devnull);
	strob_close(subsh);
	strob_close(subsh2);
	strob_close(shell_lib_buf);
	
	if (ret <= 0) retval = 1;
	sw_d_msg(G, "swc_write_target_copy_script : retval = [%d]\n", retval);
	/*
	 *  retval:
	 *	0 : Ok
	 *	1 : Error
	 */
	return retval;
}

static
int
report_file_conflicts(GB * G, SWI * swi, int opt_preview, int replacefiles, int keepfiles, int is_upgrade, char * name)
{
	int retval;
	retval = 0;
	
	E_DEBUG2("HERE : replacefiles %d", replacefiles);
	E_DEBUG2("HERE : keepfiles %d", keepfiles);
	E_DEBUG2("HERE : is_upgrade %d", is_upgrade);
	E_DEBUG2("HERE : %s", name);
	if (replacefiles == 0 && keepfiles == 0 && is_upgrade == 0) {
		if (opt_preview == 0)
			retval = 1;
		sw_l_msg(G, SWC_VERBOSE_1, "SW_FILE_ERROR: file exists: %s\n", name);
	} else if (is_upgrade == 0) {
		sw_l_msg(G, SWC_VERBOSE_1, "SW_FILE_WARNING: file exists: %s\n", name);
	}
	E_DEBUG2("HERE : returning %d", retval);
	return retval;
}

/* ---------------------------------------------------------------- */
/* ------------------ PUBLIC ---- PUBLIC -------------------------- */
/* ----- Routines below are likely called from swinstall.c -------- */
/* ---------------------------------------------------------------- */

int
swinstall_write_target_install_script(GB * G,
		int ofd,
		char * fp_targetpath, 
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
		int opt_preview,
		char * g_sh_dash_s,
		int alt_catalog_root
		)
{
	return
	write_target_install_script(
		G,
		ofd,
		fp_targetpath, 
		control_message, 
		sourcepath,
		delaytime,
		keep_old_files,
		nhops,
		vlv,
		pax_read_command_key,
		hostname,
		blocksize,
		distdata,
		installed_software_catalog,
		opt_preview,
		g_sh_dash_s,
		alt_catalog_root);
}

int
swinstall_arfinstall(GB * G, SWI * swi, int ofd, int * g_signal_flag,
	char * target_path, char * catalog_prefix, VPLOB * swspecs,
	int section, int altofd, int opt_preview, char * pax_read_command_key,
	int alt_catalog_root, int event_fd, struct extendedOptions * opta,
	int keep_old_files, uintmax_t * pstatbytes,
	int allow_missing_files, SWICAT_E * up_e)
{
	int ret;
	int format;
	XFORMAT * xformat = swi->xformatM;

	if (swi->swi_pkgM->target_pathM) {
		free(swi->swi_pkgM->target_pathM);
		swi->swi_pkgM->target_pathM = NULL;
	}

	if (swi->swi_pkgM->catalog_entryM) {
		free(swi->swi_pkgM->catalog_entryM);
		swi->swi_pkgM->catalog_entryM = NULL;
	}
	swi->swi_pkgM->target_pathM = strdup(target_path);
	swi->swi_pkgM->catalog_entryM = strdup(catalog_prefix);

	E_DEBUG("HERE");
	if (swi->swc_idM == SWC_U_L) {
		;
	} else {
		if (
			swlib_check_clean_path(swi->swi_pkgM->target_pathM) ||
			0
		) {
			E_DEBUG2("HERE path is [%s]", swi->swi_pkgM->target_pathM);
			return 10;
		}
	}
	
	E_DEBUG("HERE");
	if (strlen(swi->swi_pkgM->catalog_entryM)) {
		/* Disable the check for zero length strings since this is what
		   happens when swlist uses this routine in list mode */
		if (
			swlib_check_clean_path(swi->swi_pkgM->catalog_entryM) ||
			0
		) {
			return 3;
		}
	}

	E_DEBUG("HERE");
	format = xformat_get_format(xformat);

	swicol_set_targetpath(swi->swicolM, target_path);

	{ /* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */ 
		/*
		* Scratch test area
		*/
		/*
		STROB * tmp = strob_open(10);
		swextopt_write_session_options(tmp, opta, SWC_U_I);
		fprintf(stderr, "%s", strob_str(tmp));
		strob_close(tmp);
		*/
	} /* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */ 

	E_DEBUG("HERE");
	if (format == arf_tar || format == arf_ustar) {
		/*
		 * All tar formats
		 */
		ret = arfinstall_tar(G, swi, ofd, g_signal_flag,
				target_path, catalog_prefix, swspecs,
				altofd, opt_preview, pax_read_command_key,
				alt_catalog_root, event_fd, opta,
				keep_old_files, pstatbytes, allow_missing_files, up_e);
	} else {
		/*
		 * all other formats
		 */
		fprintf(stderr, "%s: cpio formats currently not supported\n",
			swlib_utilname_get());
		ret = 1;
	}
	return ret;
}

int
swinstall_analyze_overwrites(GB * G, SWI * swi, int overwrite_fd, int opt_keep_old_files, int is_upgrade)
{
	int retval;
	STROB * buf;
	int n;
	int opt_preview;
	char * s;
	int replacefiles;

	E_DEBUG("");
	retval = 0;
	if (overwrite_fd < 0) return 0;
	buf = strob_open(32);
	opt_preview = G->g_opt_previewM;
	replacefiles = swextopt_is_option_true(SW_E_swbis_replacefiles, G->optaM);

	E_DEBUG("");
	uxfio_lseek(overwrite_fd, 0, SEEK_SET);
	while ((n=swgp_read_line(overwrite_fd, (STROB*)buf, DO_NOT_APPEND)) > 0) {
		s = strob_str(buf);
		while(swlib_squash_trailing_char(s, '\n') == 0);
		E_DEBUG("");
		retval += report_file_conflicts(G, swi, opt_preview, replacefiles, opt_keep_old_files, is_upgrade, s);
	}
	strob_close(buf);
	E_DEBUG2("returning %d", retval);
	return retval;
}
