/* sw_keyword.h:  keyword list
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



#ifndef SW_KEYWORD_H_CPP_19980731
#define SW_KEYWORD_H_CPP_19980731

#include "swuser_config.h"

#define SWLEX_SW_ATTRIBUTE_KEYWORD (1<<1)
#define SWLEX_SW_OBJECT_KEYWORD  (1<<2)
#define SWLEX_SW_EXTENDED_KEYWORD  (1<<3)
#define SWLEX_RPM_ATTRIBUTE_KEYWORD  (1<<16)

struct swbis_keyword {
   char * name;
   int flag;
};

struct swbis_keyword sw_keywords[] =
{
{"FAT_Hidden",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"FAT_Readonly",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"RPMTAG_ARCH",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_ARCHIVESIZE",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_BUILDARCHS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_BUILDHOST",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_BUILDROOT",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_BUILDTIME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CHANGELOGNAME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CHANGELOGTEXT",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CHANGELOGTIME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CONFLICTFLAGS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CONFLICTNAME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_CONFLICTVERSION",	SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_COPYRIGHT",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_DEFAULTPREFIX",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_DESCRIPTION",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_DISTRIBUTION",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_EXCLUDEARCH",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_EXCLUDEOS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_EXCLUSIVEARCH",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_EXCLUSIVEOS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_EXTERNAL_TAG",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEFLAGS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEGROUPNAME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILELINKTOS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEMD5S",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEMODES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEMTIMES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILENAMES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILERDEVS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILESIZES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILESTATES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEUSERNAME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_FILEVERIFYFLAGS",	SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_GIF",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_GROUP",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_ICON",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_INSTALLPREFIX",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_INSTALLTIME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_NAME",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_OS",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PACKAGER",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PATCH",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_POSTIN",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_POSTINPROG",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_POSTUN",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_POSTUNPROG",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PREIN",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PREINPROG",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PREUN",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PREUNPROG",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_PROVIDES",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_RELEASE",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_REQUIREFLAGS",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_REQUIRENAME",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_REQUIREVERSION",	SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_ROOT",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_RPMVERSION",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_SERIAL",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_SIZE",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_SOURCE",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_SOURCERPM",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_SUMMARY",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_URL",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_VENDOR",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_VERIFYSCRIPT",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_VERSION",		SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"RPMTAG_XPM",			SWLEX_RPM_ATTRIBUTE_KEYWORD},
{"URL",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"all_filesets",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"ancestor",      		SWLEX_SW_ATTRIBUTE_KEYWORD},   /* XDSA C701 */
{"applied_patch",		SWLEX_SW_ATTRIBUTE_KEYWORD},   /* XDSA C701 */
{"architecture", 		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"archive_offset",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"archive_path",      		SWLEX_SW_ATTRIBUTE_KEYWORD},   /* XDSA C701 */
{"archive_source",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"archive_type",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"bundle",       		SWLEX_SW_OBJECT_KEYWORD},
{"catalog",      		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"category",      		SWLEX_SW_OBJECT_KEYWORD|SWLEX_SW_ATTRIBUTE_KEYWORD},      /* XDSA C701, Hewlett-Packard */
{"category_tag",      		SWLEX_SW_ATTRIBUTE_KEYWORD},   /* XDSA C701 */
{"checkinstall",    		SWLEX_SW_EXTENDED_KEYWORD},
{"checkremove",			SWLEX_SW_EXTENDED_KEYWORD},
{"chksum",       		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"class",        		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"compressed_chksum",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"compressed_size",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"compression_state",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"compression_type",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"configure",    		SWLEX_SW_EXTENDED_KEYWORD},
{"configured_instances",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"contents",     		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"control_directory",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"control_file",			SWLEX_SW_OBJECT_KEYWORD|SWLEX_SW_EXTENDED_KEYWORD},
{"copyright",    		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"copyright.*",    		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* Multi-Language support */
{"corequisites", 		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"create_date",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"create_time",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"date",         		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"description",  		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"description.*",  		SWLEX_SW_ATTRIBUTE_KEYWORD},  /* Multi-Language support */
{"dfiles",       		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"directory",			SWLEX_SW_ATTRIBUTE_KEYWORD|SWLEX_SW_EXTENDED_KEYWORD},
{"distribution", 		SWLEX_SW_OBJECT_KEYWORD},
{"exclude",			SWLEX_SW_EXTENDED_KEYWORD},
{"exrequisites",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"file",				SWLEX_SW_OBJECT_KEYWORD|SWLEX_SW_EXTENDED_KEYWORD},
{"file_permissions",		SWLEX_SW_EXTENDED_KEYWORD},
{"fileset",			SWLEX_SW_OBJECT_KEYWORD},
{"fix",				SWLEX_SW_EXTENDED_KEYWORD},
{"gid",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"group",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"host",				SWLEX_SW_OBJECT_KEYWORD},
{"hostname",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"interpreter",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"install_time",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"installed_software",		SWLEX_SW_OBJECT_KEYWORD},
{"instance_id",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"is_exclusive", 		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"is_kernel",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"is_locatable",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"is_reboot",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"is_sparse",      		SWLEX_SW_ATTRIBUTE_KEYWORD},   /* XDSA C701 */
{"is_volatile",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"layout_version",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"link_source",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"location",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"machine_type",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"major",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"md5sum",       		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"media",			SWLEX_SW_OBJECT_KEYWORD},
{"media_sequence_number",	SWLEX_SW_ATTRIBUTE_KEYWORD},
{"media_type",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"minor",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"mode",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"mod_date",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"mod_time",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"mtime",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"number",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"os_name",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"os_release",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"os_version",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"owner",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"packager",			SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"path",				SWLEX_SW_ATTRIBUTE_KEYWORD}, /* |SWLEX_SW_EXTENDED_KEYWORD}, */
{"patch_state",      		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* XDSA C701 */
{"patches",			SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"pfiles",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"postinstall",			SWLEX_SW_EXTENDED_KEYWORD},
{"postkernel",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"preinstall",			SWLEX_SW_EXTENDED_KEYWORD},
{"preremove",			SWLEX_SW_EXTENDED_KEYWORD},
{"prerequisites",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"pristine_source_package",	SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"product",			SWLEX_SW_OBJECT_KEYWORD},
{"postremove",			SWLEX_SW_EXTENDED_KEYWORD},
{"qualifier",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"release",			SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"request",			SWLEX_SW_EXTENDED_KEYWORD},
{"result",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"revision",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"rpm_fileflags",		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"rpm_provides",		 	SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */	
{"rpm_verifyflags",		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"saved_files",      		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* XDSA C701 */
{"sequence_number",		SWLEX_SW_ATTRIBUTE_KEYWORD},
{"sha1sum",       		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"sha512sum",       		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"signature",			SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"size",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"source",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"sourcerpm",                    SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"source_package",		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"space",			SWLEX_SW_EXTENDED_KEYWORD},
{"state",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"state_info",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"subproduct",			SWLEX_SW_OBJECT_KEYWORD},
{"superseded_by",		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* XDSA C701 */
{"supersedes",      		SWLEX_SW_ATTRIBUTE_KEYWORD}, /* XDSA C701 */
{"tag",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"the_term_vendor_is_misleading", SWLEX_SW_ATTRIBUTE_KEYWORD},  /* swbis */
{"title",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"title.*",			SWLEX_SW_ATTRIBUTE_KEYWORD}, /* Multi-Language support */
{"type",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"uid",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"unconfigure",			SWLEX_SW_EXTENDED_KEYWORD},
{"unpreinstall",			SWLEX_SW_EXTENDED_KEYWORD},
{"unpostinstall",		SWLEX_SW_EXTENDED_KEYWORD},
{"url",				SWLEX_SW_ATTRIBUTE_KEYWORD}, /* swbis */
{"user",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"uuid",				SWLEX_SW_ATTRIBUTE_KEYWORD},
{"vendor",			SWLEX_SW_OBJECT_KEYWORD},
{"vendor_tag",			SWLEX_SW_ATTRIBUTE_KEYWORD},
{"verify",			SWLEX_SW_EXTENDED_KEYWORD},
{NULL, 0}
};

#endif
