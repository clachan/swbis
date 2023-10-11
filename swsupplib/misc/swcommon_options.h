/* swcommon.h - A common namespace for misc. functions.
 *
 * Copyright (C) 1998 James H. Lowe, Jr. <jhlowe@acm.org>
 *
 */

/*
 * COPYING TERMS AND CONDITIONS:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef swcommon_options_200212j_h
#define swcommon_options_200212j_h

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swextopt.h"

   struct extendedOptions optionsArray[] = {
{1, 0, 0, "allow_downdate", 		"false", 		NULL, SWC_U_I},
{1, 0, 0, "allow_incompatible", 	"false", 		NULL, SWC_U_I|SWC_U_V|SWC_U_G},
{1, 0, 0, "allow_multiple_versions", 	"false", 		NULL, SWC_U_G},
{1, 0, 0, "ask",		 	"false", 		NULL, SWC_U_I|SWC_U_G},
{1, 0, 0, "autoreboot",		 	"false", 		NULL, SWC_U_I},
{1, 0, 0, "autorecover",	 	"false", 		NULL, SWC_U_I},
{0, 0, 0, "autoselect_dependencies", 	"as_needed", 		NULL, SWC_U_I|SWC_U_C|SWC_U_R|SWC_U_V|SWC_U_G},
{1, 0, 0, "autoselect_dependents", 	"false", 		NULL, SWC_U_G},
{1, 0, 0, "check_contents", 		"true", 		NULL, SWC_U_V},
{1, 0, 0, "check_permissions", 		"true", 		NULL, SWC_U_V},
{1, 0, 0, "check_requisites",	 	"true", 		NULL, SWC_U_V},
{1, 0, 0, "check_scripts",		"true", 		NULL, SWC_U_V},
{1, 0, 0, "check_volatile", 		"false", 		NULL, SWC_U_V},
{1, 0, 0, "compress_files", 		"false", 		NULL, 0|SWC_U_C},
{0, 0, 0, "compression_type",		"gzip", 		NULL, 0|SWC_U_C},
{1, 0, 0, "defer_configure",	 	"false", 		NULL, SWC_U_I},
{0, 0, 0, "distribution_source_directory",	"-", 		NULL, SWC_U_I|SWC_U_C},
{0, 0, 0, "distribution_target_directory",	"/", 		NULL, SWC_U_P|SWC_U_C|SWC_U_L|SWC_U_R|SWC_U_V},
{0, 0, 0, "distribution_target_serial",	"-",	 		NULL, SWC_U_P},
{1, 0, 0, "enforce_dependencies",	"true", 		NULL, SWC_U_I|SWC_U_C|SWC_U_R|SWC_U_V},
{1, 0, 0, "enforce_dsa",		"true", 		NULL, SWC_U_P|SWC_U_I|SWC_U_C},
{1, 0, 0, "enforce_locatable",		"true", 		NULL, SWC_U_I|SWC_U_V},
{1, 0, 0, "enforce_scripts",		"true", 		NULL, SWC_U_I|SWC_U_R|SWC_U_G},
{0, 0, 0, "files",			NULL,	 		NULL, 0},
{1, 0, 0, "follow_symlinks",		"false", 		NULL, SWC_U_P},
{0, 0, 0, "installed_software_catalog",	"var/lib/swbis/catalog",NULL, SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "logfile",			"/var/log/sw.log",	NULL, SWC_U_I|SWC_U_C|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "loglevel",			"1",			NULL, SWC_U_I|SWC_U_C|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "media_capacity",		"0",			NULL, SWC_U_P},
{0, 0, 0, "media_type",			"serial",		NULL, SWC_U_P},
{0, 0, 0, "psf_source_file",		"-",			NULL, SWC_U_P},
{0, 0, 0, "one_liner",			"title revision tag",	NULL, SWC_U_L},
{1, 0, 0, "reconfigure",		"false",		NULL, SWC_U_G},
{1, 0, 0, "recopy",			"false",		NULL, 0|SWC_U_C},
{1, 0, 0, "reinstall",			"false",		NULL, SWC_U_I},
{1, 0, 0, "select_local",		"true",			NULL, SWC_U_I|SWC_U_C|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "software",			"",			NULL, 0},
{0, 0, 0, "targets",			"",			NULL, 0},
{1, 0, 0, "uncompress_files",		"false",		NULL, 0|SWC_U_C},
{0, 0, 0, "verbose",			"1",			NULL, SWC_U_P|SWC_U_I|SWC_U_C|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{1, 0, 0, "swbis_cksum",		"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_file_digests",		"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_file_digests_sha2",	"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_files",		"false",		NULL, SWC_U_P},
{0, 0, 0, "swbis_signer_pgm",		"GPG",			NULL, SWC_U_P},
{1, 0, 0, "swbis_sign",			"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_archive_digests",	"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_archive_digests_sha2",	"false",		NULL, SWC_U_P},
{0, 0, 0, "swbis_gpg_name",		"",			NULL, SWC_U_P},
{0, 0, 0, "swbis_gpg_path",		"~/.gnupg",		NULL, SWC_U_P},
{1, 0, 0, "swbis_gzip",			"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_bzip2",		"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_numeric_owner",	"false",		NULL, SWC_U_P},
{1, 0, 0, "swbis_absolute_names",	"false",		NULL, SWC_U_P},
{0, 0, 0, "swbis_format",		"ustar",		NULL, SWC_U_P|SWC_U_I},
{1, 0, 0, "swbis_no_analysis_phase",	"false",		NULL, SWC_U_X},
{0, 0, 0, "swbis_archive_reader", 	"tar",			NULL, SWC_U_X},
{1, 0, 0, "swbis_no_audit",	 	"true",			NULL, SWC_U_C},
{0, 0, 0, "swbis_local_pax_write_command", "tar",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_remote_pax_write_command","tar",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_local_pax_read_command",	"tar",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_remote_pax_read_command",	"tar",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_local_pax_remove_command",	"gtar",		NULL, SWC_U_R}, /* Not used */
{0, 0, 0, "swbis_remote_pax_remove_command",	"gtar",		NULL, SWC_U_R}, /* Not used */
{1, 0, 0, "swbis_quiet_progress_bar",	"false",		NULL, SWC_U_C|SWC_U_I},
{1, 0, 0, "swbis_no_remote_kill",	"false",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{1, 0, 0, "swbis_no_getconf",		"true",		NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_shell_command",	"detect",			NULL, SWC_U_C|SWC_U_I|SWC_U_L|SWC_U_R|SWC_U_V|SWC_U_G},
{1, 0, 0, "swbis_enforce_file_md5",	"false",		NULL, SWC_U_I},
{1, 0, 0, "swbis_allow_rpm",		"false",		NULL, SWC_U_C|SWC_U_I|SWC_U_L},
{0, 0, 0, "swbis_remote_shell_client",	"/usr/bin/ssh",		NULL, SWC_U_C|SWC_U_I|SWC_U_L},
{1, 0, 0, "swbis_install_volatile",	"true",			NULL, SWC_U_I},
{0, 0, 0, "swbis_volatile_newname",	"",			NULL, SWC_U_I},
{1, 0, 0, "swbis_any_format",		"false",		NULL, SWC_U_L|SWC_U_I|SWC_U_P|SWC_U_C},
{1, 0, 0, "swbis_forward_agent",	"true",			NULL, SWC_U_L|SWC_U_C|SWC_U_I|SWC_U_V|SWC_U_G},
{1, 0, 0, "swbis_ignore_scripts",	"false",		NULL, SWC_U_I|SWC_U_G},
{0, 0, 0, "swbis_sig_level",		"0",			NULL, SWC_U_R|SWC_U_I|SWC_U_L|SWC_U_V|SWC_U_G},
{1, 0, 0, "swbis_enforce_all_signatures","false",		NULL, SWC_U_R|SWC_U_I|SWC_U_L|SWC_U_V|SWC_U_G},
{0, 0, 0, "swbis_remove_volatile",	"true",			NULL, SWC_U_R},
{0, 0, 0, "swbis_check_mtime",		"true",			NULL, SWC_U_V},
{1, 0, 0, "swbis_check_duplicates",	"true",			NULL, SWC_U_P},
{1, 0, 0, "swbis_check_owners",		"true",			NULL, SWC_U_V},
{1, 0, 0, "swbis_replacefiles",		"false",		NULL, SWC_U_I},
{1, 0, 0, NULL, NULL, NULL, 0}
};

#endif
