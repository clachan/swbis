/* swpackagefile.cxx
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

#include "swuser_config.h"
#include "swuser_assert_config.h"
#include "swpackagefile.h"

mode_t swPackageFile::default_tar_mode_dir = (00|S_IRWXU|(S_IRWXG&(S_IXGRP|S_IRGRP))|(S_IRWXO&(S_IXOTH|S_IROTH)));
mode_t swPackageFile::default_tar_mode_regfile = (00|(S_IRWXU&(~S_IXUSR))|(S_IRWXG&(S_IRGRP))|(S_IRWXO&(S_IROTH)));

int swPackageFile::Initor::use_count_=0;
int swPackageFile::Initor::did_create_=0;
uxFormat * swPackageFile::uxformatM = NULL;
STROB * swPackageFile::tmpM = NULL;


static
void
apply_umask(mode_t * mode, char * cumask)
{
	mode_t umask;
	unsigned long ul_umask;
	SWPACKAGEFILE_DEBUG("");
	taru_otoul(cumask, &ul_umask);
	umask = (mode_t)(ul_umask);
	(*mode) &= (~umask);
	return;
}

static
void
add_if_vremoval(swDefinition * swdef, swMetaData * swmd, char * name)
{
	swMetaData * existing_swmd;	
	
	SWPACKAGEFILE_DEBUG("");
	existing_swmd = swdef->findAttribute(name);
	if (existing_swmd && existing_swmd->get_status() > 0) {
		//
		// Dont add
		//
	} else if (existing_swmd  && !existing_swmd->get_status()) {
		swdef->list_replace(swmd);
	} else if (!existing_swmd) {
		swdef->list_add(swmd);
	}
	return;
}

int swPackageFile::check_no_stat_optimization(swDefinition * swdef,
				char fileArray[][file_permsLength],
					int fileArrayWasSet[]) 
{
	int i;
	int ret;

	ret = 0;
	for (i=0; i<lastE; i++) {
		switch(i) {
			//
			//  These are the required attributes to
			//  
			case linksourceE:
				if ( fileArrayWasSet[i] == 0 ) {
					// Linksource Not set
					if (fileArrayWasSet[typeE] && *fileArray[typeE] == SW_ITYPE_s) {
						// Type is Link, Not OK
						ret++;
					} else {
						// Type is not a Link, OK
						;
					}
				}
				break;	
			case uidE:
				if (fileArrayWasSet[i] == 0 && fileArrayWasSet[ownerE] == 0) ret++;
				break;	
			case gidE:
				if (fileArrayWasSet[i] == 0 && fileArrayWasSet[groupE] == 0) ret++;
				break;	
			case modeE:
			case typeE:
				if (fileArrayWasSet[i] == 0) {
					//
					// Missing this attribute
					//
				       	ret++;
					SWPACKAGEFILE_DEBUG2("test failed: [%d]", i);
				}
				break;	
			case mtimeE:
			case sizeE:
			case groupE:
			case ownerE:
			case majorE:
			case minorE:
			case volaE:
			case compE:
			case cksumE:
			case md5sumE:
			case sha1sumE:
			case sha512sumE:
			case ctimeE:
			case nlinkE:
			case inoE:
			case swbiE:
			case umaskE:
			default:
				break;	
		}
	}

	//
	// 0  Means the PSF has all the file attributes, do optimize by not calling stat()
	// >0 Means have to stat the file, too bad
	//
	if ( 
		*fileArray[typeE] == SW_ITYPE_f ||
		*fileArray[typeE] == SW_ITYPE_d ||
		*fileArray[typeE] == SW_ITYPE_h ||
		*fileArray[typeE] == SW_ITYPE_s ||
		0
	) {
		//
		// Restrict the optimizatin to regular files and directories and symlinks
		//
		return ret;
	} else {
		return 1;
	}

	// Never gets here
	return 100;
}


int swPackageFile::initialize_file_stats_array(swDefinition * swdef,
				char fileArray[][file_permsLength],
					int fileArrayWasSet[]) 
{
	int i;
	swMetaData * swmd;	
	char * value;
	
	SWPACKAGEFILE_DEBUG("");
	for (i=0; i<=lastE; i++) {
		*fileArray[i]='\0';
		fileArrayWasSet[i] = 0;
	}

	for (i=0; i<lastE; i++) {
		switch(i) {
			case modeE: 	// 1
			case ownerE: 	// 4
			case uidE: 	// 5
			case groupE:	// 6
			case gidE:	// 7
			case majorE:	// 16
			case minorE:	// 15
			case typeE:	// 0
			case mtimeE:
			case linksourceE:
				//
				//  Preserve these attributes that are
				//  already set from the PSF file.
				//
				swmd = swdef->findAttribute(namesM[i]);
				if (swmd) {
					value = swmd->get_value((int*)(NULL));
					if (i == linksourceE) {
						if (strlen(value) >= file_permsLength -1) {
							fprintf(stderr, "linksource too long: %s\n", value);
							exit(1);
						}
					}
					SWPACKAGEFILE_DEBUG3("INIT: setting attribute [%s] == [%s]", namesM[i], value);
					::swlib_strncpy(fileArray[i], value, file_permsLength);
					fileArrayWasSet[i] = 1;
				} else {
					SWPACKAGEFILE_DEBUG2("INIT: NOT setting attribute [%s]", namesM[i]);
					;
				}
				break;
			case volaE:
			case compE:
			case cksumE:
			case md5sumE:
			case sha1sumE:
			case sha512sumE:
			case ctimeE:
			case nlinkE:
			case inoE:
			case swbiE:
			case umaskE:
			case sizeE:
			default:
				break;	
		}
	}
	return 0;
}

int
swPackageFile::apply_file_stats(swDefinition * swdef, SWVARFS * swvarfs, char *source, int cksumflags ) {
	int ret;
	struct stat st;
	char fileArray			[lastE+1][file_permsLength];
	int  fileArrayWasSet		[lastE+1];
	swMetaData * swmd;
	swMetaData * swmd2;
	SWVARFS * swvarfs1 = (SWVARFS*)xFormat_get_swvarfs();
	int level = swdef->get_level();
	char * umask_value;
	char * value;
	char * set_type;
	int i;

	SWPACKAGEFILE_DEBUG("ENTERING");
	SWPACKAGEFILE_DEBUG2("source=[%s]", source);
	if (source == NULL) {
		//
		// Determine what to use as the effective source from the contents
		// of the definition.
		//
		fprintf(stderr, "swPackageFile::apply_file_stats: missing source attribute, fatal.\n");
		return -1;
	}

	swdef->set_path_attribute(source);

	SWLIB_ASSERT(swvarfs == swvarfs1);

	if (	swdef->get_no_stat() ||
		swdef->get_storage_status() == SWDEF_STATUS_DUP ||
		0
	) {
		//
		// don't set the implicit file stats
		//
		SWPACKAGEFILE_DEBUG3("returning get_no_stat=[%d] storage_status=[%d]",
				swdef->get_no_stat(), swdef->get_storage_status());
 		return 0;
	}

	initialize_file_stats_array(swdef, fileArray, fileArrayWasSet);

	swmd = swdef->findAttribute("umask");
	if (swmd) {
		/*
		* This umask value came from a file_permissions extended
		* defintiion and was applied to subsequent file (object keyword)
		* definitions in the PSF.
		* It is applies elsewhere in the case of subsequent file 
		* (explicit extended) definitions.
		*/
		SWPACKAGEFILE_DEBUG("");
		umask_value = swmd->get_value((int*)(NULL));
	} else {
		SWPACKAGEFILE_DEBUG("");
		umask_value = (char*)NULL;
	}
	
	cksumflags = swdef->apply_cksum_policy_specialization(cksumflags);  // cksum are on for control_files.

	SWPACKAGEFILE_DEBUG2("running [%s]", source);

	//
	// Here is the main stat,  whether it is lstat for stat depends on the setting
	// via swvarfs_set_stat_syscall(swvarfs, SWVARFS_VSTAT_STAT) e.g. or SWVARFS_VSTAT_LSTAT
	//

	if (    
		check_no_stat_optimization(swdef, fileArray, fileArrayWasSet) == 0 &&
		(
		 	//
			// Allow optimization on directories and sym links
			//
		 	(fileArrayWasSet[typeE] && *fileArray[typeE] == SW_ITYPE_d) ||
		 	(fileArrayWasSet[typeE] && *fileArray[typeE] == SW_ITYPE_h) ||
		 	(fileArrayWasSet[typeE] && *fileArray[typeE] == SW_ITYPE_s) 
		)
	) {	
		//
		// Good, got optimization permission, all the stats are set the the PSF
		// Now, transfer them to &st, then keep running, the remainder of the
		// function will be none-the-wiser.
		//
		SWPACKAGEFILE_DEBUG("avoiding stat()");
		if (swMetaData::fileStats2statbuf(&st, fileArray, fileArrayWasSet) != 0) {
			//
			// failed for some reason.
			// Fall back and run the real stat
			//
			SWPACKAGEFILE_DEBUG("optimization failed");
			if (xFormat_u_lstat(source, &st) != 0) {
				SWPACKAGEFILE_DEBUG("");
				return -9980;
			}
		}
	} else {
		SWPACKAGEFILE_DEBUG("");
		if (xFormat_u_lstat(source, &st) != 0) {
			// FIXME  9991 is a special code with special meaning, should at least be an
			// abtracted constant of some form.
			return -9991;
		}
	}

	/********************************************************************************	
	  ******************************************************************************
		2014-03-29 (March 2014): the following code is obsolete atleast until I remember
		the motivation for creating it in the first place. JL

	swmd = swdef->findAttribute("type");

	// JL if (swmd) swmd->write_fd(2);
	// fprintf(stderr, "JL fileArrayWasSet[typeE] is [%d]\n", fileArrayWasSet[typeE]);
	// fprintf(stderr, "JL swmd is %p\n", swmd);

	if (swmd) {
		//
		// Here the user explicitly set a type 
		// we will respect it for the case of a symlink
		// wrt the follow_symlinks extended option.
		//

		XFORMAT * xformat;
		SWVARFS * swvarfs;
		char * current_stat;
		char itype;
		
		SWPACKAGEFILE_DEBUG("type attribute was set");
		xformat = xFormat_get_xformat();
		swvarfs = xformat_get_swvarfs(xformat);
		current_stat = swvarfs_get_stat_syscall(swvarfs);

		set_type = swmd->get_value((int*)(NULL));
		if (set_type == NULL) return -9992;

		if (
			S_ISLNK(st.st_mode) && strcmp(current_stat, SWVARFS_VSTAT_LSTAT) == 0
		) {
			//
			// The source is a symlink and the current stat function is lstat
			// re-stat with stat()
			//
			SWPACKAGEFILE_DEBUG("re-stating -- running stat");
			swvarfs_set_stat_syscall(swvarfs, SWVARFS_VSTAT_STAT);
			if (xFormat_u_lstat(source, &st) != 0) {
				return -9992;
			}
			swvarfs_set_stat_syscall(swvarfs, current_stat);
			itype = getFileTypeFromMode(st.st_mode);
			if (*set_type != itype) {
				//
				// the actual file contradicts the
				// type set in the PSF.  Let this be an error
				//
				fprintf(stderr, "swPackageFile::file type attribute in PSF does not match source.\n");
				return -1;
			}
		}
	}
	*****************************************************************************************
	**********************************************************************************************/

	if (umask_value) {
		SWPACKAGEFILE_DEBUG("");
		apply_umask(&(st.st_mode), umask_value);
	}
	
	ret = swMetaData::swmetadata_decode_file_stats(swvarfs1, source, source,
				&st, fileArray, fileArrayWasSet, cksumflags);
	if (ret < 0) {
		SWPACKAGEFILE_DEBUG("");
		// error
		return ret;
	} else if (ret > 0) {
		// ignore file
		ret = 0;
	}

	for (i=0; i<lastE; i++) {
		if (fileArrayWasSet[i]) {
			swmd = new swAttribute(const_cast<char*>(namesM[i]), fileArray[i]);
			swmd->set_level(level + 1);
			switch(i) {
				case ctimeE:
				case mtimeE:
				case sizeE:
				case typeE:
				case linksourceE:
					swdef->list_replace_if_not_explicitly_set(swmd);
					break;
				case volaE:
				case compE:
				case cksumE:
				case md5sumE:
				case sha1sumE:
				case sha512sumE:
					SWPACKAGEFILE_DEBUG3("setting %s to value [%s]", namesM[i], swmd->get_value(NULL));
					swdef->list_replace(swmd);
					break;
				case ownerE:
				case groupE:
					SWPACKAGEFILE_DEBUG("");
					add_if_vremoval(swdef, swmd, namesM[i]);
					break;
				case majorE:
				case minorE:
					SWPACKAGEFILE_DEBUG2("storage status is %d", swdef->get_storage_status());
					SWPACKAGEFILE_DEBUG("setting minor/major");
					/***********************
					swmd2 = swdef->findAttribute(SW_A_type);
					if (swmd2) {
						value = swmd2->get_value(NULL);
						if (value &&
							(
							strcmp(value, "c") == 0 ||
							strcmp(value, "b") == 0
							)
						) {
							SWPACKAGEFILE_DEBUG("NOT setting minor/major because of type");
							break;
						}
					}
					******************/
					if (
						*fileArray[typeE] == 'c' ||
						*fileArray[typeE] == 'b'
					) {
						SWPACKAGEFILE_DEBUG("");
						swdef->list_add_if_new(swmd);
					} else {
						SWPACKAGEFILE_DEBUG("");
						swmd->vremove();
					}
					break;
				case uidE:
				case gidE:
					SWPACKAGEFILE_DEBUG("");
					swdef->list_add_if_new(swmd);
					break;
				case modeE:
					SWPACKAGEFILE_DEBUG("");
					swdef->list_add_if_new(swmd);
					break;
				case nlinkE:
				case inoE:
				case swbiE:
				case umaskE:
					SWPACKAGEFILE_DEBUG("");
					swdef->list_replace(swmd);
					swmd->vremove();
					break;
			}
		}
	}
	swdef->set_no_stat(1);
	swdef->apply_file_stat_specialization(fileArray, fileArrayWasSet);  // some stats are turned off in control_files.
	SWPACKAGEFILE_DEBUG("");
	return 0;
}
