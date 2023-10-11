/* swextendeddef.h  -- Process PSF Extended Definitions.

 Copyright (C) 2003,2004,2005,2007,2010 Jim Lowe
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

#ifndef swextdef_20070211_h
#define swextdef_20070211_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "swuser_config.h"
#include "swmetadata.h"
#include "swattributemem.h"
#include "swstructdef.h"
#include "swdefinition.h"
#include "swdefinitionfile.h"
#include "swpsfi.h"
extern "C" {
#include "misc-fnmatch.h"
#include "swlib.h"
#include "uxfio.h"
#include "strob.h"
#include "strar.h"
#include "swvarfs.h"
#include "swheaderline.h"
#include "taru.h"
}

#define SWEXTDEFNEEDDEBUG 1
#undef SWEXTDEFNEEDDEBUG

#ifdef SWEXTDEFNEEDDEBUG
#define SWEXTDEF_DEBUG(format) SWBISERROR("SWEXTDEF DEBUG: ", format)
#define SWEXTDEF_DEBUG2(format, arg) SWBISERROR2("SWEXTDEF DEBUG: ", format, arg)
#define SWEXTDEF_DEBUG3(format, arg, arg1) SWBISERROR3("SWEXTDEF DEBUG: ", format, arg, arg1)
#else
#define SWEXTDEF_DEBUG(arg)
#define SWEXTDEF_DEBUG2(arg, arg1)
#define SWEXTDEF_DEBUG3(arg, arg1, arg2)
#endif 

class swExtendedDef : private swMetaData
{

	char fileArrayM			[lastE+1][file_permsLength];
	int  fileArrayWasSetM		[lastE+1];
	
	char fileDefPermsArrayM		[lastE+1][file_permsLength];
	int  fileDefPermsArrayWasSetM	[lastE+1];
	
	char statArrayM			[lastE+1][file_permsLength];
	int  statArrayWasSetM		[lastE+1];

	STROB * cwdM;			// Current working directory.
	
	int	fileset_sindexM;	// Start index of current fileset. Used to check for files
					// previously defined (so that the new defintiion replaces or
					// merges with the previous one).
	
	int	fileset_recursive_startM; // Start index at start of recursive processing.

	int	excl_sindexM;		// Used by exclude keyword processing: Beginning location 
					// in psf, 'exclude' keyword has no effect before this point.

	int   	excl_qindexM;		// Used by exclude keyword: index in Mapped source attribute value to the
					// original source name that was provided in the PSF by the user.

	STROB * distribution_targetM;	// Top level path, set as arge to swpackage.
	STROB * directory_asourceM;	// Modified.
	STROB * directory_sourceM;	// Always Equal to value in PSF.
	STROB * directory_pathM;
	STROB * file_sourceM;
	STROB * file_pathM;
	STROB * file_linksource_pathM;
	SWVARFS * swvarfsM;

	int	directoryActiveM;	// True or False, true if directory source is set.
	int	directoryDestActiveM;	// True or False, true is directory path is set.
	int	filePermissionsActiveM;	// True or False,
	int 	file_recursiveM; 	// True or False,
	int	is_volatileM;		// True or False,
	int	did_cwd_prefixM;	// True or False,  Did the directory.source value get prefixed with the cwd.
	int	do_prefix_distribution_targetM;
	int	do_create_cksumM;
	int	do_create_md5sumM;	// sha1 and md5
	int	do_create_sha2M;	// sha512
	int	non_existing_sourceM;
	int     verbose_levelM;

	STROB * excludeM;
	STROB * includeM;
	
	swDefinition * swdefFormedM;	// True only when finished.
	int did_open_swvarfsM;
	struct stat stM;		// Utility buffer.

	int follow_symlinksM;

	static const int EXPSET = 2;
	// ===============================================================================
	// ===============================================================================
public: // ===============================================================================
	
	
	swExtendedDef(void * swvarfs) : swMetaData() {
		directory_asourceM = directory_sourceM = directory_pathM = file_sourceM = file_pathM = (STROB*)NULL;
		excludeM = includeM = file_linksource_pathM = (STROB*)NULL;
		construction_init_();
		soft_init_();
		init_default_fileperm_opts();
		init_file_opts();
		init_directory();
		init_file_recursive();
		set_swvarfs((SWVARFS*)swvarfs);
		verbose_levelM = ::swlib_get_verbose_level();
	}
	
	swExtendedDef(void) : swMetaData() {
		distribution_targetM = directory_asourceM = directory_sourceM = (STROB*)NULL;
		directory_pathM = file_sourceM = file_pathM = (STROB*)NULL;
		excludeM = includeM = file_linksource_pathM = (STROB*)NULL;
		construction_init_();
		soft_init_();
		init_default_fileperm_opts();
		init_file_opts();
		init_directory();
		init_file_recursive();
		set_swvarfs((SWVARFS*)NULL);
	}
	
	~swExtendedDef(void) { 
		if (distribution_targetM != static_cast<STROB*>(NULL)) strob_close(distribution_targetM);
		if (directory_sourceM != static_cast<STROB*>(NULL)) strob_close(directory_sourceM);
		if (directory_asourceM != static_cast<STROB*>(NULL)) strob_close(directory_asourceM);
		if (directory_pathM != static_cast<STROB*>(NULL)) strob_close(directory_pathM);
		if (file_sourceM != static_cast<STROB*>(NULL)) strob_close(file_sourceM);
		if (file_pathM != static_cast<STROB*>(NULL)) strob_close(file_pathM);
		if (file_linksource_pathM != static_cast<STROB*>(NULL)) strob_close(file_linksource_pathM);
		if (excludeM != static_cast<STROB*>(NULL)) strob_close(excludeM);
		if (includeM != static_cast<STROB*>(NULL)) strob_close(includeM);
		if (cwdM != static_cast<STROB*>(NULL)) strob_close(cwdM);
	}

	swDefinition * getFormedDefinition(void) { return swdefFormedM; }
	int filePermissionsActive(void) { return filePermissionsActiveM; }
	int directoryActive(void) { return directoryActiveM; }
	int directoryDestActive(void) { return directoryDestActiveM; }

	// Public
	//
	void
	set_follow_symlinks(int s)
	{
		SWEXTDEF_DEBUG("ENTER");
		follow_symlinksM = s;
	}
	
	void
	set_digests2_creation(int s)
	{
		SWEXTDEF_DEBUG("ENTER");
		do_create_sha2M = s;
	}
	
	void
	set_digests1_creation(int s)
	{
		SWEXTDEF_DEBUG("ENTER");
		do_create_md5sumM = s;
	}
	
	// Public
	//
	void
	set_cksum_creation(int s)
	{
		SWEXTDEF_DEBUG("ENTER");
		do_create_cksumM = s;
	}
	
	// Public
	//
	void
	set_distribution_target(char * dtarget)
	{
		SWEXTDEF_DEBUG("ENTER");
		strob_strcpy(distribution_targetM, dtarget);
	}

	// Public
	//
	void
	set_swvarfs(SWVARFS * sv)
	{
		SWEXTDEF_DEBUG("ENTER");
		swvarfsM = sv;
	}
	
	// Public
	//
	void
	reset(void) {	
		SWEXTDEF_DEBUG("ENTER");
		soft_init_();
		init_default_fileperm_opts();
		init_file_opts();
		init_directory();
		init_file();
		init_file_recursive();
	}
	
	// Public
	//
	int
	processIncludeFile(swDefinitionFile* psf, char * filename, swDefinition * swdef) {
		int level = swdef->get_level();
		int len;
		swPSFi *swpsfi = new swPSFi();
		
		SWEXTDEF_DEBUG("ENTER");
		swpsfi->open_parser(filename);
		len = swpsfi->run_parser(level, SWPARSE_FORM_MKUP_LEN);
		if (len < 0) {
			fprintf(stderr,"swpackage: parse error: %s\n", filename);
			return len;
		}
		if (psf->generateDefinitions()) {
			fprintf(stderr,"swpackage: error in generateDefinitions() processing %s\n", filename);
	 	}	
		psf->swFileMapPop();
		delete swpsfi;
		return 0;
	}

	// Public
	//
	// Process the file keyword.
	// 5.2.13.2 Recursive File Definition  Lines 774-786
	// file *
	//
	int
	processRecursiveFile(swDefinitionFile* psf, swDefinition * swdef) {
		SWVARFS * swvarfs;
		struct stat st;
		char * dir_source;
		char * rel_path;
		char * path;
		char * old_follow_state;
		int ret=0;

		SWEXTDEF_DEBUG("");
		fileset_recursive_startM = -1;
		dir_source = strob_str(directory_sourceM);

		if (swvarfsM == NULL) {
			SWEXTDEF_DEBUG("");
			swvarfs = swvarfs_open(dir_source, UINFILE_DETECT_FORCEUXFIOFD, (mode_t)(NULL));
			if (swvarfs == static_cast<SWVARFS*>(NULL)) {
				return -2;
			}
			did_open_swvarfsM = 1;
			swvarfsM = swvarfs;
		} else {
			SWEXTDEF_DEBUG("");
			swvarfs = swvarfsM;
			if (swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM && strcmp(dir_source, ".") == 0) {
				/*
				* FIXME If sourcing on an archive ignore "."... Because it doesn't work
				*/
				;
			} else {
				if (swvarfs_setdir(swvarfsM, dir_source)) {
					SWBIS_E_FAIL2("swvarfs_setdir [%s]", dir_source);
					return -2;
				}
			}
		}

		old_follow_state = swvarfs_get_stat_syscall(swvarfs);

		SWEXTDEF_DEBUG2("follow_symlinks is %d", follow_symlinksM);
		if (follow_symlinksM) {
			swvarfs_set_stat_syscall(swvarfs, "stat" /* stat or lstat */);
		}

		//
		// 5.2.13.1  line 771  make sure it exists.
		//
		SWEXTDEF_DEBUG("");
		path = dir_source;
		if (swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM && strcmp(path, ".") == 0) {
			/*
			* FIXME If sourcing on an archive ignore "."... Because it doesn't work
			*/
			SWEXTDEF_DEBUG("running swvarfs_get_next_dirent");
			path = swvarfs_get_next_dirent(swvarfs, &st);
		} else {
			SWEXTDEF_DEBUG2("running local_lstat_swvarfs on path: %s", path);
			if (local_lstat_swvarfs(path, &st) < 0) {
				fprintf(stderr,"swpackage: lstat error [%s] : %s\n", path, errno ? strerror(errno) : "");
				return -3;
			}
			//
			// The next line excludes the first path, which is the
			// directory path argument of the directory keyword from
			// the recursive list.  This is done so that the user is able
			// to specify this directory explicitly, just prior to the
			// recursive specification in the PSF file.
			//
			//
			// However, doing this breaks the bit-for-bit compatibillty test
			// with GNU tar (because the '.' is missing I think) so don't do this
			// for now.  I also think it is wrong per spec. 
			//

			// The above statements are contridicted here.
			/* path = swvarfs_get_next_dirent(swvarfs, &st); */
		}

		SWEXTDEF_DEBUG("");
		while(path != NULL && !ret) {
			//
			// translate spaces to non-break space 0xa0
			//
			SWEXTDEF_DEBUG2("while path loop: path=%s", path);
			protect_space(path);
			
			//
			// Strip off the dir_source component.
			//
			rel_path = makePathRelative(path, dir_source);
			if (rel_path) {
				path = rel_path;
			} else {
				fprintf(stderr,"swpackage: internal error returned from makePathRelative().\n");
				return -1;
			}

			if (strlen(path) == 0) {
				path = "./";
			}
			
			SWEXTDEF_DEBUG("");
			ret = processExplicitFile_i(path, swdef, &st, 1, 1, NULL);
			if (ret == 0) {
				SWEXTDEF_DEBUG("");
				assert(swdefFormedM);
				swdefFormedM->set_no_stat(1);
				recursive_list_unique_add(psf, swdefFormedM);
				if (fileset_recursive_startM < 0) {
					fileset_recursive_startM = psf->swdeffile_get_index_from_pointer(swdefFormedM);
				}
				set_start_index(psf, swdefFormedM);
			} else if (ret < 0) {
				// error
				fprintf(stderr,"swpackage: error processing file: %s\n", path);
			} else if (ret > 0) {
				// Ignore, such as a socket
				fprintf(stderr,"swpackage: %s: socket ignored\n", path);
			}
			if ((ret=swvarfs_dirent_err(swvarfs))) {
				fprintf(stderr,"swpackage: internal error status returned from swvarfs_get_next_dirent().\n");
				return (ret);
			}
			path = swvarfs_get_next_dirent(swvarfs, &st);
		}
		
		swvarfs_set_stat_syscall(swvarfs, old_follow_state);

		if (did_open_swvarfsM) {
			SWEXTDEF_DEBUG("");
			swvarfs_close(swvarfsM);
			swvarfsM = NULL;
			did_open_swvarfsM = 0;
		}
		return (ret);
	}

	// Public
	//
	// Process the file keyword.
	//
	int
	processFile(swDefinitionFile * psf, char * value, swDefinition * swdef) {
		int retval = -1;
		swDefinition * prior_file;
		char * p;
		
		SWEXTDEF_DEBUG("");
		p = value;
		while (isspace(*p)) p++;
		if (*p == '<') {
			//
			// Including files 5.2.13.6 Lines 915-917
			//
			SWEXTDEF_DEBUG("");
			if(!includeM) {
				includeM = strob_open(2);
			}
			p++; while (isspace(*p)) p++;
			strob_strcpy(includeM, p);
			retval = processIncludeFile(psf, p, swdef);
		} else if (*p == '*') {
			//
			// Recursive file definition 5.2.13.2
			//
			SWEXTDEF_DEBUG("");
			file_recursiveM = 1;
			retval = processRecursiveFile(psf, swdef);
		} else {
			//
			// Explicit file definition 5.2.13.3
			//
			SWEXTDEF_DEBUG("");
			retval = processExplicitFile(value, swdef);
			if (retval < 0) {
				SWEXTDEF_DEBUG("");
				fprintf(stderr, "swpackage: error processing file: %s\n",
					value /*strob_str(file_pathM)*/);
				//fprintf(stderr,"Fatal Internal Software Error indicated by return from processExplicitFile().\n");
				//exit(41);
			} else if (retval > 0) {
				// ignore socket
				fprintf(stderr, "swpackage: %s: socket ignored\n", value /*strob_str(file_pathM)*/);
				retval = 0;
			} else {
				assert(swdefFormedM);
				//
				// 
				// 5.2.13.3 Lines 792-793
				// Check to see if path attribute matches any previous files, and if
				// so merge the attributes.  (Duplicate files).
				//
				SWEXTDEF_DEBUG("");
				if ((prior_file = find_same_prior_file(psf, swdefFormedM, -1)) != NULL) {
					SWEXTDEF_DEBUG("");
					#ifdef SWEXTDEFNEEDDEBUG
						fprintf(stderr, "merging>>>>>\n");
						swdefFormedM->write_fd(2);
						fprintf(stderr, "<<<<<<<\n");
						fprintf(stderr, "prior file is>>>>>\n");
						prior_file->write_fd(2);
						fprintf(stderr, "<<<<<<<\n");
					#endif
					/* set_start_index(psf, priorFile); */
					prior_file->merge(swdefFormedM, 0, 1 /* do replace */);
					#ifdef SWEXTDEFNEEDDEBUG
						fprintf(stderr, "after merge>>>>>\n");
						prior_file->write_fd(2);
						fprintf(stderr, "after merge\n");
					#endif
				} else {
					SWEXTDEF_DEBUG("");
					psf->swdeffile_list_add(swdefFormedM);
					set_start_index(psf, swdefFormedM);
				}
			}
		}
		SWEXTDEF_DEBUG("");
		return retval;
	}

	// Public
	//
	int
	attach_filePaths(swDefinition * swdef, char * source_att, char * source, char * path) {
		int level = swdef->get_level() + 1;
		int mem_fd = swdef->get_mem_fd();
		swAttribute * swatt;

		SWEXTDEF_DEBUG("");
		swatt = swAttribute::make_newAttribute(mem_fd, source_att, /*"source",*/  source);
		swatt->set_is_explicit();
		swatt->set_level(level);
		swdef->list_add(swatt);
		
		if (path) {
			SWEXTDEF_DEBUG("");
			swlib_squash_double_slash(path);
			swatt = swAttribute::make_newAttribute(mem_fd, SW_A_path, path);
			swatt->set_is_explicit();
			swatt->set_level(level);
			swdef->list_add(swatt);
		}
		return 0;
	}

	// Public
	//
	int
	processExtendedControl_file(char * keyword, char * value, swDefinition * parent_swdef) {
		int level = parent_swdef->get_level();
		char * source, *path;
		swAttribute * swatt;
		swDefinition * swdef;

		SWEXTDEF_DEBUG2("ENTER %s", value);
		swdef = swDefinition::make_newDefinition(parent_swdef, "control_file");
		assert(swdef);	
		
		parse_SourcePath(value, &source, &path);
		level+=2;
		if (strcmp(keyword, "control_file")) {
			SWEXTDEF_DEBUG("");
			swatt = swAttribute::make_newAttribute(parent_swdef->get_mem_fd(), SW_A_tag, keyword);
			if (path == NULL) {
				SWEXTDEF_DEBUG("");
				path = keyword;
			}
		} else {
			SWEXTDEF_DEBUG("");
			swatt = swAttribute::make_newAttribute(parent_swdef->get_mem_fd(), SW_A_tag, swlib_basename(NULL, source));
			if (path == NULL) {
				SWEXTDEF_DEBUG("");
				path = swlib_basename(NULL, source);
			}
		}
		
		swatt->set_level(level);
		swdef->list_add(swatt);
		attach_filePaths(swdef, SW_A_source, source, path);
		swdefFormedM = swdef;
		return 0;
	}
	
	// Public
	//
	// 5.2.13.4 Default File Permissions.  Lines 886-908
	// file_permission [-m mode] [-u umask] [-o [owner[,]][uid]] [-g [group[,]][gid]]
	//
	int
	processFilePermissions(swDefinitionFile * psf, char * value) {
		SWEXTDEF_DEBUG2("ENTER %s", value);
		//
		// reset at every definition.
		//
		SWEXTDEF_DEBUG("");
		init_default_fileperm_opts();
		if (!value || strlen(value) == 0) {
			//
			// Handle special case of empty string value.
			//
			SWEXTDEF_DEBUG("");
			filePermissionsActiveM = 0;
			return 0;
		}
		if (parse_FileOpts(value, fileDefPermsArrayM, fileDefPermsArrayWasSetM, 0) == static_cast<char*>(NULL)) {
			SWEXTDEF_DEBUG("");
			filePermissionsActiveM = 0;
			return -1;
		} else {
			SWEXTDEF_DEBUG("");
			filePermissionsActiveM = 1;
			return 0;
		}
		SWEXTDEF_DEBUG("");
		// Never gets here.
		return -1;
	}
	
	// Public
	//
	// 5.2.13.3 Explicit File Desfinition. Lines 788-884
	// file [-t type] [-m mode] [-o [owner[,]][uid]] [-g [group[,]][gid]] [-n] [-v] source [path]
	//
	int
	processExplicitFile(char * value, swDefinition * parent_swdef) {
		int do_no_stat = 1;
		int ret = processExplicitFile_i(value, parent_swdef, static_cast<struct stat *>(NULL), 1, 0, &do_no_stat);
		// The NULL st buf causes the stat to be done in stat_source_file_if().
		if (swdefFormedM && ret == 0)
			swdefFormedM->set_no_stat(do_no_stat); 
		if (ret > 0) {
			// Not an error, the file was ignored
			;
		}
		return ret;
	}

	// Public
	//
	// Process Directory keyword.  Section 5.2.13.1  Lines 762-772
	//
	int
	processDirectory(swDefinitionFile * psf, char * value) {
		char *path, *source;
		
		directoryDestActiveM=0;
		directoryActiveM=0;
		excl_qindexM = -1;
		parse_SourcePath(value, &source, &path);
		strob_strcpy(directory_sourceM, source);
		strob_strcpy(directory_asourceM, source);

		SWEXTDEF_DEBUG2("ENTER %s", value);
		if (::strcmp(strob_str(directory_sourceM), ".") != 0) {
			if (make_AbsolutePath(directory_asourceM, 1, strob_str(cwdM), &did_cwd_prefixM) == static_cast<char*>(NULL)) {
				fprintf(stderr,"swpackage: Source directory %s does not exist.\n", strob_str(directory_asourceM));
				return -1;
			}
		}
		//	
		// If there is a path then store it, otherwise set directory_pathM
		// to null so to indicate there is no path.
		//
		if (path != static_cast<char*>(NULL)) {
			strob_strcpy(directory_pathM, path);
			//
			//
			// 5.2.13.1:  the dest directory must be an absolute path.
			//
			if (*path != '/') {
				fprintf(stderr, "swpackage: The destination directory [%s] must be an absolute path.\n", path);
				return -1;
			}
			directoryDestActiveM=1;
		} else {
			strob_strcpy(directory_pathM, "");
		}

		directoryActiveM=1;
		return 0;
	}

	// Public
	//
	// 5.2.13.5 Exclude Files
	//

	int
	processExcludes(swDefinitionFile* psf, char * value) {
		int i;
		STROB * f_source;
		STROB * excl_pat;
		STRAR * pat_list;
		swDefinition * swdef;
		char * source;
		char * isource;
		char * def_source;
		char * mdef_source;
		int is_dir;
		int t_dflag;
		int l_dflag;
		int does_have_trailing_slash;
		int num_of_trailing_slash;
		int first_match;
		int ret;
		char * type;

		num_of_trailing_slash = 0;
		does_have_trailing_slash = 0;
		is_dir = 0;
		t_dflag = 0;
		l_dflag = 0;
		i = excl_sindexM;
		first_match = 0;
		f_source = strob_open(100);
		excl_pat = strob_open(100);
		pat_list = strar_open();

		SWEXTDEF_DEBUG2("value is [%s]", value);

		//
		// /* This just cleans up 'value' of extraneous whitespace etc.*/
		// /* 'isource' is not used */
		//
		parse_SourcePath(value, &source, &isource);

		SWEXTDEF_DEBUG2("source is now [%s]", source);
	
		//
		// 'source' is the name to exclude from the PSF
		//
		// Apply a special case of trailing slash, DIR/, which is treated as DIR/*
		//

		//
		// Detect and remove trailing slashes
		//
		if (source && strlen(source)) {
			swlib_toggle_all_trailing_slash("drop", source, &num_of_trailing_slash);
		}

		SWEXTDEF_DEBUG2("num_of_trailing_slash = [%d]", num_of_trailing_slash);
		if (num_of_trailing_slash == 1) {
			//
			// One trailing slash means the same thing as *NO* trailing slash
			//
			// we will treat this as a pattern to exclude the 
			// entire directory contents and the directory entry itself
			// It does this by adding a second globbing pattern.
			//
			// NOTE that if num_of_trailing_slash > 1, then this is
			// not done because this is assinged a special meaning
			// to exclude the directory entry alone.
			//
			SWEXTDEF_DEBUG("does have trailing slash");
			does_have_trailing_slash = 1;
		}

		//  
		//  num_of_trailing_slash >=2 has special meaning and means
		//  exclude the directory but not its contents

		//
		// Now form the pattern
		//

		//
		// filter out the common path components between
		// source and dest_path of the directory mapping directive, if active.
		//

		if (directoryDestActive() && *source == '/') {
			//
			// If directoryDestActive() AND the exclude source is an absolute path
			// then strip away the leading commong component
			//
			char * t;

			SWEXTDEF_DEBUG("directoryDestActive and /");
			t = strstr(source, strob_str(directory_pathM));
			if (t) {
				t += strob_strlen(directory_pathM);
			} else {
				t = source;
			}
			strob_strcpy(f_source, t);	
		} else {
			strob_strcpy(f_source, source);	
		}

		SWEXTDEF_DEBUG3("concatenating [%s] [%s]", strob_str(excl_pat), strob_str(f_source));
		strob_strcpy(excl_pat, strob_str(directory_sourceM));
		swlib_unix_dircat(excl_pat, strob_str(f_source));

		if (num_of_trailing_slash == 1) {
			//
			// Trailing slashes were removed above
			// put back the correct number
			//
			strob_strcat(excl_pat, "/");
		}

		SWEXTDEF_DEBUG2("excl_pat is now [%s]", strob_str(excl_pat));

		swlib_toggle_trailing_slashdot("drop", strob_str(excl_pat), &t_dflag);
		swlib_toggle_leading_dotslash("drop", strob_str(excl_pat), &l_dflag);
		
		SWEXTDEF_DEBUG2("excl_pat is now [%s]", strob_str(excl_pat));
		strar_add(pat_list, strob_str(excl_pat));

		while ((swdef=psf->swdeffile_get_pointer_from_index(i++)) != static_cast<swDefinition*>(NULL)) {
			if(strcmp("file", swdef->get_keyword())) {
				//
				// Sanity check
				// 
				fprintf(stderr,"swpackage: Error: Non file object found in list in processExcludes().\n");
				return -1;
			}		
			def_source = swdef->find(SW_A_source);
			if (def_source == static_cast<char *>(NULL)) {
				//
				// Sanity check
				// 
				fprintf(stderr,"swpackage: warning: source attribute not found\n");
				return -1;
			}

			SWEXTDEF_DEBUG2("def_source [%s]", def_source);
			//
			// Strip off absolute part if directory_sourceM is relative.
			//
			if (*strob_str(directory_sourceM) != '/' && excl_qindexM > 0) {
				mdef_source = def_source + ((excl_qindexM > (int)strlen(def_source)) ? strlen(def_source) : excl_qindexM);
			} else {
				mdef_source = def_source;
			}

			SWEXTDEF_DEBUG3("comparing for exclusion : fnmatch [%s] [%s]", strob_str(excl_pat), mdef_source);
		
			//
			// Process the pattern list through fnmatch
			//
			ret = process_fnmatch_list(pat_list, mdef_source);

			if (ret == 0) {
				//
				// Got match
				//
				SWEXTDEF_DEBUG3("fnmatch result: GOT MATCH [%s] [%s]", strob_str(excl_pat), mdef_source);
				type = swdef->find(SW_A_type);
				if (type == static_cast<char *>(NULL)) {
					//
					// Sanity check
					// 
					fprintf(stderr,"swpackage: error: type attribute not found in processExcludes().\n");
					return -1;
				}
				if (
					first_match == 0 && 
					*type == 'd' && 
					num_of_trailing_slash <= 1 &&
					(directoryDestActive()==0 || does_have_trailing_slash == 1)
				) {
					//
					// Add trailing "/*"
					// as a second pattern in order to match the directory contents
					//
					SWEXTDEF_DEBUG2("ADDING /* to pattern list, now [%s]", strob_str(excl_pat));
					swlib_unix_dircat(excl_pat, "/*");
					strar_add(pat_list, strob_str(excl_pat));
					first_match = 1;
				}

				//
				//  swdeffile_list_del(--i) shortens the list, hence we need to
				//  decrement the index to get the next item on the list
				//
				//  Delete the entry because it matched
				//
				psf->swdeffile_list_del(--i);
			} else if (ret < 0) {
				//
				// error
				// FIXME
				fprintf(stderr, "swpackage: exclude keyword: fnmatch error: ''%s''\n",
						swlib_printable_safe_ascii(strar_get(pat_list, 0)));
				return -1
				;
			} else {
				//
				// No Match
				//
				;
				SWEXTDEF_DEBUG3("fnmatch result: NO MATCH [%s] [%s]", strob_str(excl_pat), mdef_source);
			}
		}
		strob_close(excl_pat);
		strar_close(pat_list);
		return 0;
	}

	// Public
	//
	int 
	processExtendedDefinition(char * parserline, swDefinitionFile * psf, swDefinition * swdef){
		int retval = 0;
		char * keyword = swheaderline_get_keyword(parserline);
		char * value = swheaderline_get_value(parserline, (int*)(NULL));

		SWEXTDEF_DEBUG("ENTER");
		swdefFormedM=NULL;
		assert(keyword);
		assert(value);

		SWEXTDEF_DEBUG2("keyword is %s", keyword);
		SWEXTDEF_DEBUG2("value is %s", value);

		if (!::strcmp("file", keyword)) {
			retval = processFile(psf, value, swdef);
		} else if (!::strcmp("directory", keyword)) {
			retval = processDirectory(psf, value);
		} else if (!::strcmp("file_permissions", keyword)) {
			retval = processFilePermissions(psf, value);
		} else if (!::strcmp("exclude", keyword)) {
			retval = processExcludes(psf, value);
		} else {
			//
			// It can be a control_file extended definition.
			//
			int gr = swstructdef::return_entry_group(swstructdef::return_entry_index(SW_A_product, keyword));
			if (strcmp(keyword, "control_file") == 0 || gr == swstructdef::CONTROLFILE_EXT) {
				if ((retval = processExtendedControl_file(keyword, value, swdef)) == 0) { 
					assert(swdefFormedM);
					psf->swdeffile_list_add(swdefFormedM);
				}
			} else {
				retval = -1;
				fprintf(stderr,"swpackage: extended control_file keyword not recognized.\n");	
			}
		}
		return retval;
	}
	
	// Public
	//
	// Add attributes to a definition if not already present.
	//
	int
	addDefaultFilePermissions(swDefinition * swdef) {
		swAttribute * swatt;
		int mem_fd = swdef->get_mem_fd();
		int current_offset = uxfio_lseek(mem_fd, 0, SEEK_CUR);
		int level = swdef->get_level();
		int i;
		
		level++;
		for (i=0; i<lastE; i++) {
			//
			// Add attributes to the swdefinition.
			//
			if (swdef->find(namesM[i]) == static_cast<char*>(NULL)) {
				if (*fileDefPermsArrayM[i] != '\0') {
					swatt = new swAttribute(namesM[i], fileDefPermsArrayM[i]);
					swatt->set_level(level);
					if (i == umaskE) {
						swatt->vremove();
					}
					swdef->list_add(swatt);
				}
			}
		}
		//
		// Set the file back the way it was.
		//
		uxfio_lseek(mem_fd, current_offset, SEEK_SET);
		return 0;
	}

	// Public
	//
	void
	set_fileset_start_index(swDefinitionFile * psf) {
		fileset_sindexM = psf->swdeffile_get_definition_list_length();
		return;
	}
 
	 // ================================================================
	 // ================================================================
private: // ================================================================
	 // ================================================================
	 // ================================================================

	// Private
	int
	process_fnmatch_list(STRAR * pat_list, char * mdef_source) {
		int i;
		char * pat;
		int ret;

		SWEXTDEF_DEBUG2("BEGIN mdef_source is [%s]", mdef_source);
		ret = 1;
		i = 0;
		pat = strar_get(pat_list, i++);
		while(pat && ret) {
			SWEXTDEF_DEBUG3("LOOP pat[%d] is [%s]", i-1, pat);
			SWEXTDEF_DEBUG2("mdef_source = [%s]", mdef_source);
			//
			// Check for the case
			//   where pat=/dir/A/B/C/  and mdef_source=/dir/A/B/C
			// As an implementation extension call this a match
			// (fnmatch wil not).   This is used to exlude a single
			// directory with out excluding the contents of /dir/A/B/C
			//
			
			SWEXTDEF_DEBUG3("mdef_source  :  pat  [%s] [%s]", mdef_source, pat);
			SWEXTDEF_DEBUG3("strlen(mdef_source) == strlen(pat)-1  [%d] [%d]", (int)strlen(mdef_source), (int)(strlen(pat)-1));
			if (
				pat &&
				strlen(pat) > 1 &&
				*(pat+strlen(pat)-1) == '/' &&
				strlen(mdef_source) == strlen(pat)-1 &&
				strpbrk(pat, "*?") == NULL &&
				strncmp(pat, mdef_source, strlen(pat)-1) == 0
			) {
				//
				// Got a match for the special case to exlude
				// a directory but not its contents
				//
				SWEXTDEF_DEBUG("\nGot special match 0001\n");
				SWEXTDEF_DEBUG("BREAK 1");
				ret = 0;
				break;
			}
			if (
				strcmp(".", pat) == 0 &&
				(
					strcmp(mdef_source, "." ) == 0 ||
					strcmp(mdef_source, "/" ) == 0 ||
					strcmp(mdef_source, "/." ) == 0
				)
			) {
				SWEXTDEF_DEBUG("Got special match for [.]");
				ret = 0;
				break;	
			}

			SWEXTDEF_DEBUG3("CALLING ---- fnmatch(%s, %s)", pat, mdef_source);
			ret = fnmatch(pat, mdef_source, 0);
			if (ret == 0) {
				//
				// match
				//
				SWEXTDEF_DEBUG3("        ---- fnmatch(%s, %s) MATCH", pat, mdef_source);
			       	break;
			} else if (ret == FNM_NOMATCH) {
				//
				// no match
				//
				SWEXTDEF_DEBUG3("        ---- fnmatch(%s, %s) NO MATCH", pat, mdef_source);
			} else {
				//
				// error
				// FIXME
				return -1;
				;
			}
			pat = strar_get(pat_list, i++);
		}
		return ret;
	}


	int
	pattern_is_wild(char * p) {
		if (strpbrk(p, "?*[")) return 1;
		else return 0;
	}

	void
	recursive_list_unique_add(swDefinitionFile * psf, swDefinition * swdef) {
		//
		//  Check for duplicates starting at fileset_sindexM
		//		thru fileset_recursive_startM 
		//
		swDefinition * prior_file;
		SWEXTDEF_DEBUG("");
		prior_file = find_same_prior_file(psf, swdef, fileset_recursive_startM);
		if (prior_file) {
			SWEXTDEF_DEBUG2("found prior file: [%s]", swdef->find(SW_A_path));
			#ifdef SWEXTDEFNEEDDEBUG
				fprintf(stderr, "merging>>>>>\n");
				swdef->write_fd(2);
				fprintf(stderr, "<<<<<<<\n");
				fprintf(stderr, "prior file is>>>>>\n");
				prior_file->write_fd(2);
				fprintf(stderr, "<<<<<<<\n");
			#endif
			prior_file->merge(swdef, 1/*merge unconditionally*/, 1 /* do replace*/);
		} else {
			psf->swdeffile_list_add(swdef);
			/* set_start_index(psf, swdefFormedM); */
		}
	}

	int
	is_file_attribute_sufficient(int * do_no_stat, int is_set[], int is_default_set[])
	{
		int retval = 0;
		//
		// This routine implements Lines 810-812 Section 5.2.13.3
		//
		if (
			(is_set[modeE] || (filePermissionsActiveM && is_default_set[modeE])) &&
			(is_set[ownerE] || (filePermissionsActiveM && is_default_set[ownerE])) &&
			(is_set[groupE] || (filePermissionsActiveM && is_default_set[groupE])) &&
			1
		) {
			retval = 1;
			if (do_no_stat) *do_no_stat = 1;
		}
		return retval;
	}
	
	int
	apply_default_values(int type)
	{
		int i;
		int itmp;
		mode_t mode;
		unsigned int umask;
		char * ptmp;
		char  usernamebuf[TARU_SYSDBNAME_LEN+1];

		//
		// Apply default values in a way that is overridden by the
		// file_permissions defaults if active.
		//
		umask = my_get_umask();
		for (i=0; i<lastE; i++) {
			//
			// If default is active or explicitly set then
			// do not apply a default.
			//
			// The defaults below are implementation defined (per spec)
			// hence they are only meant to assign resonable values, not
			// correct values (since correct is not defined).
			//
	
			if (fileDefPermsArrayWasSetM[i])
				continue;
			if (fileArrayWasSetM[i])
				continue;
			switch(i) {
				case typeE:
					// assumed to be already set.
					break;
				case umaskE:
					break;
				case modeE:
					mode = swlib_apply_mode_umask(type, umask, (mode_t)(0));
					ptmp = get_file_cell_pointer(modeE);
					snprintf(ptmp, file_permsLength-1, "%o", (unsigned int)mode);
					ptmp[file_permsLength - 1] = '\0';
					break;
				case uidE:
					ptmp = get_file_cell_pointer(uidE);
					snprintf(ptmp, file_permsLength-1, "%d", (int)getuid());
					ptmp[file_permsLength - 1] = '\0';
					break;
				case gidE:
					ptmp = get_file_cell_pointer(gidE);
					snprintf(ptmp, file_permsLength-1, "%d", (int)getgid());
					ptmp[file_permsLength - 1] = '\0';
					break;
				case ownerE:
					if (taru_get_tar_user_by_uid(getuid(), usernamebuf) == 0) {
						copy_file_cell(fileArrayM, fileArrayWasSetM, ownerE, usernamebuf);
					} else {
						//
						// This should never happen
						//
						return -1;
					}
					break;
				case groupE:
					if (taru_get_tar_group_by_gid(getgid(), usernamebuf) == 0) {
						copy_file_cell(fileArrayM, fileArrayWasSetM, groupE, usernamebuf);
					} else {
						//
						// This should never happen
						//
						return -1;
					}
					break;
				case sizeE:
				case volaE:
				case compE:
				case cksumE:
				case md5sumE:
				case sha1sumE:
				case sha512sumE:
				case mtimeE:
				case ctimeE:
				case inoE:
				case nlinkE:
				case majorE:
				case minorE:
				case linksourceE:
				case swbiE:
					*fileArrayM[i]='\0';
					fileArrayWasSetM[i] = 0;
					break;
				default:
					SWEXTDEF_DEBUG("");
					fprintf(stderr,"swpackage: internal error in apply_default_values at i=%d.\n", i);
					break;	
			
			}
		}
		return 0;
	}

	void
	soft_init_(void) {
		swdefFormedM = NULL;
		// did_open_swvarfsM = 0;
		fileset_sindexM = 0;			// Zero is the unset value;
		fileset_recursive_startM = 0;		// Zero is the unset value;
		excl_sindexM = 0;			// Zero is the unset value;
		excl_qindexM = -1;			// -1 is the unset value.
		do_prefix_distribution_targetM = 0;
		cwdM = strob_open(254);
		if (!getcwd(strob_str(cwdM), 250)) {
			exit(3);
		}
		non_existing_sourceM = 0;
	}
	
	void
	construction_init_(void) {
		distribution_targetM = strob_open(10);
		do_create_cksumM = 0;
		do_create_md5sumM = 0;
		do_create_sha2M = 0;
		did_open_swvarfsM = 0;
		non_existing_sourceM = 0;
		follow_symlinksM = 0;
	}

	void
	init_directory(void){
		directoryActiveM=0;
		directoryDestActiveM = 0;
		did_cwd_prefixM = 0;
		if (directory_asourceM) strob_close(directory_asourceM);	
		if (directory_sourceM) strob_close(directory_sourceM);	
		if (directory_pathM) strob_close(directory_pathM);	
		directory_asourceM = strob_open(2);
		directory_sourceM = strob_open(2);
		directory_pathM = strob_open(2);
		non_existing_sourceM = 0;
	}

	void
	init_file_recursive(void){
		file_recursiveM = 0;
	}

	void
	init_file(void){
		is_volatileM = 0;
		if (file_sourceM) strob_close(file_sourceM);	
		if (file_pathM) strob_close(file_pathM);	
		if (file_linksource_pathM) strob_close(file_linksource_pathM);	
		file_sourceM = strob_open(2);
		file_pathM = strob_open(2);
		file_linksource_pathM = strob_open(2);
	}

	void
	init_default_fileperm_opts(void){
		filePermissionsActiveM = 0;
		int i; 
		for (i=0; i<=lastE; i++) {
			*fileDefPermsArrayM[i]='\0';
			fileDefPermsArrayWasSetM[i] = 0;
		}
	}
	
	void
	init_statArray(void){
		int i; for (i=0; i<=lastE; i++) {
			*statArrayM[i]='\0';
			statArrayWasSetM[i] = 0;
		}
	}
	

	void
	init_file_opts(void){
		int i; for (i=0; i<=lastE; i++) {
			*fileArrayM[i]='\0';
			fileArrayWasSetM[i] = 0;
			*statArrayM[i]='\0';
			statArrayWasSetM[i] = 0;
		}
	}

	void
	copy_file_cell(char array[][file_permsLength], int is_set[], int cell, char * value) {
		if (value) {
			strncpy(array[cell], value, file_permsLength -1);
			array[cell][file_permsLength -1] = '\0';
			if (is_set) is_set[cell] = 1;
		} else {
			if (is_set) is_set[cell] = 0;
			strncpy(array[cell], "", file_permsLength -1);
		}
	}
	
	mode_t
	my_get_umask(void) {
		mode_t mode = 0;
		mode = umask(mode);
		umask(mode);
		return mode;
	}
	
	char *
	file_cell_value(char array[][file_permsLength], int cell) {
		return array[cell];
	}
	
	int
	file_cell_was_default_set(int cell) {
		return fileDefPermsArrayWasSetM[cell];
	}
	
	void
	set_file_was_set(int array_is_set[], int index) {
		//
		// Avoid losing a setting of EXPSET, which means is was
		// really, really explicitly set.
		//
		if (array_is_set[index]) return;
		array_is_set[index] = 1;
	}
	
	int
	file_cell_was_explicitly_set(int cell) {
		return fileArrayWasSetM[cell] > 1;
	}

	int
	local_access(char * name, int mode) {
		int ret;
		ret = local_lstat_swvarfs(name, &stM);
		/*
		 * FIXME use 'mode' to determine return value
		 */
		return ret;
	}
	
	int
	local_lstat_swvarfs(char * name, struct stat * st) {
		int ret;
		ret = swvarfs_u_lstat(swvarfsM, name, st);
		return ret;
	}

	int
	local_open(char * name) {
		int u_fd;
		if (swvarfsM && swvarfs_get_format(swvarfsM) != UINFILE_FILESYSTEM) {
			u_fd = swvarfs_u_open(swvarfsM, name);
		} else {
			u_fd = open(name, O_RDONLY, 0);
		}
		return u_fd;
	}

	int
	local_close(int u_fd) {
		int ret;
		if (swvarfsM && swvarfs_get_format(swvarfsM) != UINFILE_FILESYSTEM) {
			ret = swvarfs_u_close(swvarfsM, u_fd);
		} else {
			ret = close(u_fd);
		}
		return ret;
	}

	// 
	// Return cell virtual value, taking filePermissions
	// definition into account.
	//
	char *
	get_file_cell_value(int cell) {
		char * value = fileArrayM[cell];
		if (*value == '\0') {
			value = fileDefPermsArrayM[cell];
		}
		return value;
	}
	
	//	
	// Return pointer to statArray cell.
	//
	char *
	get_stat_cell_pointer(int cell){ 
		return statArrayM[cell]; 
	}
	
	//	
	// Return pointer to fileArray cell.
	//
	char *
	get_file_cell_pointer(int cell){ 
		return fileArrayM[cell]; 
	}

	int
	set_source_prefix_length(char * asource, char * source) {
		int ret = 0;
		int index;
		char * s = strstr(asource, source);
		if (s == static_cast<char*>(NULL)) {
			fprintf(stderr,"swpackage: internal error set_source_prefix_length.\n");
			exit(1);
		}
		//
		// If source is empty, don't set excl_qindexM
		//
		if (*source == '\0') { 
			return 2;
		}
		index = s - asource;
		if (excl_qindexM >= 0) {
			if (index != excl_qindexM) {
				fprintf(stderr,
				"swpackage: internal message: set_source_prefix_length(): source prefix is inconsistent.\n");
				ret = 3;
			}
		}
		excl_qindexM = index;
		return ret;
	}

	swDefinition *
	find_same_prior_file(swDefinitionFile * psf, swDefinition * newswdef, int stop_index) {
		int start_index = fileset_sindexM;
		int list_index = start_index;
		char * newpath;
		char * path;
		swDefinition * swdef;
		swDefinition * ret_swdef = NULL;
		
		//
		// Look for a file definition with the same path as swdef.<path>
		// starting at index=fileset_sindexM
		//

		SWEXTDEF_DEBUG("");
		if ((newpath = newswdef->find(SW_A_path)) == NULL) {
			// fprintf(stderr,"swpackage: Error: path attribute not found in find_same_prior_file().\n");
			// return NULL;
		}

		SWEXTDEF_DEBUG("entering while loop");
		while (
			(stop_index  < 0 || list_index < stop_index) &&
			(swdef=psf->swdeffile_get_pointer_from_index(list_index++)) != 
				static_cast<swDefinition*>(NULL)
		)
		{
			SWEXTDEF_DEBUG("in while loop");
			if(strcmp("file", swdef->get_keyword())) {
				// 
				// skip objects that are not file objects.
				// It seems the start index is the "fileset" object.
				//
				// Do a further sanity check for this and issue a warning
				// if not a fileset but don't quit.
				//
				if(
					strcmp(SW_A_fileset, swdef->get_keyword()) != 0 &&
					strcmp(SW_A_control_file, swdef->get_keyword()) != 0
				) {
					fprintf(stderr,
					"swpackage: Internal Exception: warning: Non file object found in list in find_same_prior_file() %s.\n",
						swdef->get_keyword());
				}
				continue;
			}
			path = swdef->find(SW_A_path);
			if (path && newpath) {
				if (::swlib_dir_compare(path, newpath, 0) == 0) {
					SWEXTDEF_DEBUG2("Got merge candidate [%s]", path);
					ret_swdef = swdef;
				}
			}
		}
		if (ret_swdef) {
			;
			#ifdef SWEXTDEFNEEDDEBUG
			SWEXTDEF_DEBUG("Found file to merge");
			ret_swdef->write_fd(2);
			#endif
		}
		SWEXTDEF_DEBUG2("retval=[%p]", ret_swdef);
		return ret_swdef;
	}
	
	char *
	normalize_leading_slash(char * s) {
		if (*s == '/') return s+1;
		if (!strncmp(s, "./", 2)) return s+2;
		return s;
	}

	
	int
	compare_exclude_no_wild(char * fp_attr_source, char * fp_xsource, int is_dir) {
		char * s, *end;
		char * attr_source;
		char * xsource;

		//
		// Normalize the leading parts striping a leading '/' or './'
		//
		
		SWEXTDEF_DEBUG("In compare_exclude");
		SWEXTDEF_DEBUG2("fp_attr_source=[%s]", fp_attr_source);
		SWEXTDEF_DEBUG2("fp_xsource=[%s]", fp_xsource);
		if (directoryActiveM && did_cwd_prefixM && *fp_attr_source == '/') {
			//
			// Need to strip off the leading portion of fp_attr_source which
			// is the contents of directory_asourceM.
			//
			char * ds;
			SWEXTDEF_DEBUG("");
			ds = strstr(fp_attr_source, strob_str(cwdM));
			if (ds == NULL || ds != fp_attr_source) {
				SWEXTDEF_DEBUG("Do not exclude");
				return 1;  // Don't exclude.
			} else {
				fp_attr_source += strob_strlen(cwdM);
				if (*fp_attr_source == '/') fp_attr_source++;
				SWEXTDEF_DEBUG2("NOW fp_attr_source=[%s]", fp_attr_source);
			}
		} else if (directoryActiveM) {
			//
			// Need to strip off the directory source part
			//
			if (strstr(fp_attr_source, strob_str(directory_sourceM)) == fp_attr_source) {
				fp_attr_source += strob_strlen(directory_sourceM);
				if (*fp_attr_source == '/') fp_attr_source++;
				SWEXTDEF_DEBUG2("NOW fp_attr_source=[%s]", fp_attr_source);
			} 
		}

		attr_source = normalize_leading_slash(fp_attr_source);
		xsource = normalize_leading_slash(fp_xsource);
		SWEXTDEF_DEBUG3("attr_source=[%s] xsource=[%s]", attr_source, xsource);

		if (is_dir) {
			s = strstr(attr_source, xsource);
			if (s) {
				//
				// Need to find out where it is.
				//
				end = s + strlen(xsource);
				if (attr_source != s) {
					if ((*end == '\0' || *end == '/')) {
						if (*(s-1) == '/') {
							//
							// Since leading / is stripped 
							// this must be an internal path component.
							// Don't exclude.
							//
							return 1;
						}
						SWEXTDEF_DEBUG("Do exclude");
						return 0;	// really a directory, exclude.
					} else {
						SWEXTDEF_DEBUG("Do not exclude");
						return 1;	// Matched a path component substring, don't exclude.
					}
				} else {
					if (*end == '\0'  ||  *end == '/') {
						SWEXTDEF_DEBUG("Do exclude");
						return 0;	// really a directory, exclude.
					} else {
						SWEXTDEF_DEBUG("Do not exclude");
						return 1;	// Matched a path component substring, don't exclude.
					}
				}
			} else {
				SWEXTDEF_DEBUG("Do not exclude");
				return 1;	// didn't match, don't exclude.
			}
		} else {
			SWEXTDEF_DEBUG3("do strcmp [%s] [%s]", attr_source, xsource);
			return strcmp(attr_source, xsource);
		}
	}
	
	int
	compare_exclude(char * fp_attr_source, char * fp_xsource, int is_dir) {
		int ret;	
		SWEXTDEF_DEBUG("entering");
		if (pattern_is_wild(fp_xsource)) {
			ret = fnmatch(fp_xsource, fp_attr_source, 0);
			if (ret == 0) {
				ret = compare_exclude_no_wild(fp_attr_source, fp_attr_source, 1);
			}
			SWEXTDEF_DEBUG2("fnmatch returned %d", ret);
		} else {		
			ret = compare_exclude_no_wild(fp_attr_source, fp_xsource, is_dir);
			SWEXTDEF_DEBUG2("compare_exclude_no_wild returned %d", ret);
		}
		SWEXTDEF_DEBUG2("leaving with return value=%d", ret);
		return ret;
	}
	
	void
	set_start_index(swDefinitionFile * psf, swDefinition * swdef) {
		if (!excl_sindexM) {
			excl_sindexM = psf->swdeffile_get_index_from_pointer(swdef);
			if (excl_sindexM < 0) {
				SWBIS_E_FAIL("negative index");
				swdef->write_fd(2);
			}
			assert(excl_sindexM >= 0);
		}
	}

	char *
	makePathRelative(char * path, char * prefix) {
		char * s;
		char * ret;
		
		SWEXTDEF_DEBUG3("path=[%s]  prefix=[%s]", path, prefix);
		s = strstr(path, prefix);
		if (s == path && strcmp(path, prefix) == 0) {
			ret = path + strlen(prefix);
			SWEXTDEF_DEBUG("");
			//
			// This a empty string.
			//
		} else if (s == path) {
			SWEXTDEF_DEBUG("");
			ret = path + strlen(prefix);
			while( *ret == '/') ret++;
		} else if (strcmp(prefix, ".") == 0) {
			SWEXTDEF_DEBUG("");
			ret = path;
		} else {
			// SWBIS_E_FAIL("");
			SWEXTDEF_DEBUG("");
			ret = static_cast<char*>(NULL);
		}
		SWEXTDEF_DEBUG("");
		return ret;	
	}

	//
	// transform `strb' to an absolute path using `path_prefix'.
	//
	char *
	make_AbsolutePath(STROB * strb, int checkAccess, char * path_prefix, int * did_modify) {
		char *p = strob_str(strb);
		if (*p != '/' && path_prefix && strlen(path_prefix)) {
			int n = strlen(p);
			int m = strlen(path_prefix);
			
			strob_set_length(strb, m+n+2);
			p = strob_str(strb);
			memmove(p+m+1, p, n+1);
			*(strob_str(strb) + m) = '/';
			memcpy(p, path_prefix, strlen(path_prefix));
			*did_modify = 1;
		} else {
			*did_modify = 0;
		}
		if (checkAccess) {
			if (local_access(p, F_OK)) {
				return static_cast<char*>(NULL);
			}
		}
		return p;
	}
	
	// 5.2.13.3 Lines 818-826.
	// Apply directory mapping to file_path, if ipath is NULL use source 
	// as path.  `ipath' is the explicitFile `path' value.
	// `source' is the explicitFile `source' value.
	// 	
	char *
	make_ActivePath(char * path, char * source, STROB * file_path) {
		STROB * tmp = strob_open(24);
		STROB * psrc = strob_open(24);
		char * ipath = NULL;
		
		if (path) {
			ipath = path;
			if (*ipath != '/') { 	
				// path is relative and destination is active.
				// Prefix the dest path.
				//
				// Per 5.2.13.3 Lines 820-822.
				//
				strob_strcpy(psrc, strob_str(directory_pathM));
				swlib_add_trailing_slash(psrc);
				strob_strcat(psrc, ipath);
				ipath = strob_str(psrc);
			}  else {
				// Do nothing.
				ipath = path;
				;
			}
		} else {
			//
			// path not specified.  See 5.2.13.3 Lines 823-825.
			//
			ipath = source;	

			//
			// If directory mapping active. See 5.2.13.3 Lines 825, 826.
			//
			if (directoryDestActive()) {  // And implicitly directoryActive() is true.
				if (*ipath != '/') {
					// Map the source to the cwdM of directory source.
					// get_QualifiedAbsoluteSourcePath(psrc, directory_pathM, ipath); 
				
					// Apply 5.2.13.3 path: Lines 820-822
					strob_strcpy(psrc, strob_str(directory_pathM));
					swlib_add_trailing_slash(psrc);
					strob_strcat(psrc, ipath);
					ipath=strob_str(psrc);
				} else {
					//
					// 'source' used as 'path' and directoryDestActive is true.
					//  Do mapping iff the directorySource is a prefix of 'ipath'.
					//  per 5.2.13.3 Lines 825-826.
					//
					strob_strcpy(psrc, strob_str(directory_sourceM));
					swlib_add_trailing_slash(psrc);

					// Now see if strob_str(psrc) is a prefix in ipath.
					
					if (
						(
							strstr(ipath, strob_str(psrc)) == ipath 
						) ||
						(
							//
							// Case where ipath is directory w/o trailing /
							// and equal to directory_sourceM.
							//
							(strstr(strob_str(psrc), ipath) == strob_str(psrc)) &&
							(strob_strlen(psrc) - strlen(ipath) == 1)
						)
					) {
						//
						// Do mapping.
						//
						
						//
						// Copy the non prefixable portion of ipath.
						//
						strob_strcpy(tmp, ipath + strob_strlen(psrc) - 1);
						
						strob_strcpy(psrc, strob_str(directory_pathM));
						swlib_add_trailing_slash(psrc);
						strob_strcat(psrc, strob_str(tmp));
						
						ipath=strob_str(psrc);
						swlib_squash_double_slash(ipath);
					} else {
						;		
					}

				}
			}
			else if (directoryActive()) {
				//
				// Apply 5.2.13.3 path: Lines 824-826.
				//
				// Use the source as the path.
				//
				strob_strcpy(psrc, strob_str(directory_sourceM));
				strob_strcat(psrc, "/");
				strob_strcat(psrc, ipath);

				ipath=strob_str(psrc);
			}
		}
		
		//
		// Only apply restrictions of Lines 822-823 iff 'path' is specified
		//   (i.e. non NULL)
		//
		if (path != NULL && *ipath != '/' && directoryDestActive() == 0) {
			//
			// Per Lines 822-824.
			//
			fprintf(stderr,
				"swpackage: error: path is relative and destination directory not active. path=[%s] source=[%s]\n", 
					ipath, source);
			strob_close(psrc);
			strob_close(tmp);
			return static_cast<char*>(NULL);
		}
		
		strob_strcpy(file_path, ipath);
		strob_close(psrc);
		strob_close(tmp);
		return strob_str(file_path);
	}
	
	//
	// Return the value of `psrc' that is `isource' transformed to an
	// absolute path based on either the directory source value or the 
	// current working directory; If already absolute path, then do nothing.
	//
	char *
	get_QualifiedAbsoluteSourcePath(STROB * psrc, STROB * map_source, char * source) {
		// Implement 5.2.13.3 source : Lines 803-817.
		int did_modify = 0;	

		if (*source != '/') {	// Not an absolute path.
			if (directoryActive()) {
				//
				// Lines 805-806
				// directory_sourceM may not be an absolute path.
				//
				strob_strcpy(psrc, strob_str(map_source));
				//make_AbsolutePath(psrc, 0, strob_str(cwdM), &did_modify);
				make_AbsolutePath(psrc, 0, "", &did_modify);
				if (*source != '\0') swlib_add_trailing_slash(psrc);
				strob_strcat(psrc, source);
				swlib_squash_all_dot_slash(strob_str(psrc));
				swlib_squash_trailing_slash(strob_str(psrc));
			} else {  
				//
				// Lines 807-809
				// Directory not active.  Transform to an absolute Path
				// base on cwd.
				//
				strob_strcpy(psrc, source);
				make_AbsolutePath(psrc, 0, strob_str(cwdM), &did_modify);
				swlib_squash_all_dot_slash(strob_str(psrc));
				swlib_squash_trailing_slash(strob_str(psrc));
			}
		} else {
			strob_strcpy(psrc, source);
		}
		return strob_str(psrc);
	}

	//
	// Set or maintain open state on strob object.
	//
	static void
	path_open_if(STROB ** strb){
		if (*strb == static_cast<STROB*>(NULL)) { *strb = strob_open(56); }
	}
	
	static void
	squashEscapedSpace(char * src) {
		char * s;
		s = src;
		while (*s) {
			if (*s == '\\' && *(s+1) == '\x20') {
				memmove(s, s+1, strlen(s+1) + 1);
				*s = '\xA0';
			}
			s++;
		}	
	}

	static int
	is_path_true(char * path) {	
		return (path && ::strlen(path));
	}

	static void 
	squashEscapedBackslashes(char * src) {
		char * s;
		s = src;
		while (*s) {
			if (*s == '\\' && *(s+1) == '\\') {
				memmove(s, s+1, strlen(s+1) + 1);
			}
			s++;
		}	
	}
	
	static void 
	unprotect_space(char * p) {
		//
		// translate non-break spaces to  space.
		//
		while (p && *p) { 
			if (*p == (char)(0xa0)) *p = (char)('\x20');
			p++;
		}
	}
		
	static void 
	protect_space(char * p) {
		//
		// translate spaces to non-break space 0xa0
		//
		while (p && *p) {
			if (*p == '\x20') *p = (char)('\xA0');
			p++;
		}
	}
	
	//
	// Parse the source [path] portion of the attribute value.
	//
	void 
	parse_SourcePath(char * value, char ** source, char ** path) {
		char *p, *w;
	

		squashEscapedBackslashes(value);
		squashEscapedSpace(value);

		p=value;
		while(isspace(*p)) p++;
		w = p + 1;
		*source = p;
		while (*w  &&  *w != '\x20'  &&  *w != '\x09') w++;
		if (*w) {	
			*w = '\0';
			w++;
			while(isspace(*w)) w++;
			if (*w)
				*path = w;	
			else
				*path = (char*)(NULL);
		} else {
			*path = (char*)(NULL);
		}
		if (*path) unprotect_space(*path);
		if (*source) unprotect_space(*source);
	}

	int
	check_is_all_digits(char * value, char * terminating_chars) {
		char * w = value;

		//
		// Verifies that the value is all digits.
		// returns number of digits, -1 on error
		//

		while (*w  &&  isdigit(*w)) w++;
		if (*w == '\0' || strchr(terminating_chars, *w)) {
			return (int)(w-value);
		}

		return -1;
	}
	
	//
	// Parse the Explicit File Option value.
	//	
	char *
	bound_value(char * value, char ** aux, int *p_has_comma) {
		char *s=value;
		char *w;
	
		SWEXTDEF_DEBUG2("BEGIN value=[%s]", value);
		if (aux) *aux = NULL;
		if (p_has_comma) *p_has_comma = 0;

		//
		// skip over leading spaces
		//
		while (isspace((int)(*s))){ s++; }
		w = s;
		if (*s == '-') {
			//
			// Empty arg. OK by Std.
			// A owner,group, uid or gid may not begin with a dash.
			//
			s--;
			*s = '\0';
			SWEXTDEF_DEBUG2("aux = [%s]", (aux!=NULL ? *aux : ""));
			return s;
		}

		//
		// march down the value (atom) until a separator is encounterd
		//

		while (*w  &&  *w != '\x20'  &&  *w != '\x09' && *w != ',') w++;
	
		//
		// If the separator was a ',' then record the pointer
		//
	
		if (aux) {
			if (*w == ',') {
				if (p_has_comma) {
					*p_has_comma = 1;
				}
				if (*(w+1) == '\x20' || *(w+1) == '\x09') {
					//
					// case for 
					// file -o owner,
					//
					// Here, the numeric id is not givem
					// return it as an empty string by leaving
					// *aux set to *w, (*w will become '/0').
					//
					*aux = w;
				} else	{
					//
					// case for 
					// file -o owner,uid
					//
					// *w, which is now ',', will become NUL.
					// Here we are intensionally stepping
					// past a (to be) '\0' terminator because
					// then next atom is presumably the all-digits
					// numeric system id.
					//
					*aux = w+1;
				}
			} else {
				*aux = NULL;
			}
		}
		*w = '\0';
		SWEXTDEF_DEBUG2("aux = [%s]", (aux!=NULL ? *aux : ""));
		return s;
	}

	int 
	parse_name_id_string(char * value, char * aux_value,
				int is_file_perm, 
				int nameix,
				int idix,
				char array[][file_permsLength],
				int array_is_set[],
				int * p_skip_amount,
				int has_comma
			)
	{
		int ret;
		char * val;
		STROB * vxa;

		SWEXTDEF_DEBUG2("BEGIN value=[%s]", value);
		SWEXTDEF_DEBUG2("BEGIN aux_value=[%s]", aux_value ? aux_value: "<nil>");
		
		vxa = NULL;
		copy_file_cell(array, array_is_set, idix, NULL);
		copy_file_cell(array, array_is_set, nameix, NULL);
		
		if (strlen(value) == 0) {
			SWEXTDEF_DEBUG2("value=[%s]", value);
			if (is_file_perm) {
				//
				// Section 5.2.13.4 Line 886
				// According to synopsis syntax for file_permissions
				// use data base values.
				//
				SWEXTDEF_DEBUG("");
				copy_file_cell(array, array_is_set, idix, NULL);
				copy_file_cell(array, array_is_set, nameix, NULL);
			} else {
				//
				// Section 5.2.13.3 Line 788
				// According to synopsis syntax this is an error
				//
				SWEXTDEF_DEBUG("");
				return 2;	
			}
		} else {
			//
			// value is not 0 length
			//
			SWEXTDEF_DEBUG2("value=[%s]", value);
			ret = check_is_all_digits(value, " ,");
			if ( ret > 0 && aux_value == NULL) {
				//
				// value has all digits, treat it
				// as a uid.
				//
				// Section 5.2.13.3 Lines 853-877.
				//	
				SWEXTDEF_DEBUG("");
				SWEXTDEF_DEBUG("handling -o uid");
				copy_file_cell(array, array_is_set, idix, value);
				if (is_file_perm == 0) {
					SWEXTDEF_DEBUG("not is_file_permissions_context");
					array_is_set[idix] = EXPSET;
				}
				copy_file_cell(array, array_is_set, nameix, TARU_C_DO_NUMERIC);
			} else if ( ret < 0 && aux_value == NULL) {
				//
				// Here the value is not all digits and there is
				// no aux_value.
				// This handles the case of `-o owner'
				//
				SWEXTDEF_DEBUG("");
				SWEXTDEF_DEBUG("handling -o owner");
				copy_file_cell(array, array_is_set, nameix, value);
				if (is_file_perm == 0) {
					SWEXTDEF_DEBUG("not is_file_permissions_context");
					array_is_set[nameix] = EXPSET;
				}
			} else if ( ret < 0 && aux_value != NULL && has_comma == 1) {
				//
				// This handles the case of `-o owner,uid' and set the
				// value for the case of `-o owner,' where the setting of
				// the implied uid is set below
				//
				SWEXTDEF_DEBUG2("handling -o owner,uid  value=%s", value);
				copy_file_cell(array, array_is_set, nameix, value);
				if (is_file_perm == 0) {
					SWEXTDEF_DEBUG("not is_file_permissions_context");
					array_is_set[nameix] = EXPSET;
				}
			} else if ( ret == 0 && aux_value == NULL) {
				//
				// This will never happen the the strlen(value) was
				// tested above.
				//
				fprintf(stderr, "swpackage: internal error 13 in swextendeddef.h\n");
				exit(1);	
			}
		}
		*p_skip_amount = strlen(value);
		if (aux_value && strlen(aux_value)) {
			SWEXTDEF_DEBUG2("aux_value is [%s]", aux_value);
			ret = check_is_all_digits(aux_value, " ");
			val = NULL;
			if ( ret < 0) {
				//
				// Error, not all digits.
				//
				fprintf(stderr, "swpackage: invalid uid in extended definition.\n");
				return 1;
			} else if (ret == 0 && has_comma) {
				//
				// This is the case of for example
				//    -o admin, -g admin,
				// Here, get the uid of the user name and use it as if
				// it was specified.
				//
				SWEXTDEF_DEBUG2("looking up id for [%s]", value);
				uid_t xid;	
				if (taru_get_uid_by_name(value, &xid) < 0) {
					fprintf(stderr, "swpackage: user name not in system: %s\n", value);
					fprintf(stderr, "swpackage: setting id to nobody\n");
					if (taru_get_uid_by_name(AHS_USERNAME_NOBODY, &xid) < 0) {
						fprintf(stderr, "swpackage: %s name not in system\n", AHS_USERNAME_NOBODY);
						fprintf(stderr, "swpackage: setting id to %d\n", (int)AHS_UID_NOBODY);
						xid = (uid_t)AHS_UID_NOBODY;
					}
				}
				if (vxa == NULL)
					vxa = strob_open(12);
				SWEXTDEF_DEBUG3("Found id as [%d] for name [%s]", (int)xid, value);
				strob_sprintf(vxa, 0, "%d", (int)xid);
				val = strob_str(vxa);
			} else if (ret == 0 && !has_comma) {
				fprintf(stderr, "swpackage: invalid uid in extended definition.\n");
				return 1;
			} else if (ret > 0) {
				//
				// This is the normal case, the user supplied a number after the comma
				//
				val = bound_value(aux_value, (char**)NULL, (int*)NULL);
			} else {
				//
				// Never gets here
				return -2;
			}
			SWEXTDEF_DEBUG("aux value");
			SWEXTDEF_DEBUG2("setting from aux value, [%s]", val);
			copy_file_cell(array, array_is_set, idix, val);
			if (is_file_perm == 0) {
				SWEXTDEF_DEBUG2("is_file_perm == 0, setting array_is_set[%d] = EXPSET", idix);
				array_is_set[idix] = EXPSET;
			}
			(*p_skip_amount) ++;  	// Skip over the comma.
			(*p_skip_amount) += strlen(val);
			if (vxa) strob_close(vxa);
		}
		return 0;
	}

	//
	// Parse the file options.
	// file [-t type] [-m mode] [-o [owner[,]][uid]] [-g [group[,]][gid]] [-n] [-v] 
	// and return pointer to the 'source [path]' portion.
	//
	char *
	parse_FileOpts(char * defvalue, char array[][file_permsLength], int array_is_set[], int parseSourcePath) {
		char *p;
		char * value = NULL;	
		char * aux_value;
		STROB  * val = strob_open(40);
		int skip_amount = 0;
		int error_code = 0;
		int is_file_permissions_context = 0;
		int did_umode = 0;
		int umask_value;
		int ret;
		int has_comma;

		SWEXTDEF_DEBUG("");
		aux_value = NULL;
		if (array == fileDefPermsArrayM) {
			//
			// The syntax for 'file_permissions' keyword is slightly different.
			//
			is_file_permissions_context = 1;
		}

		//
		// the algorithm below looks ahead past a NULL, so make sure there are atleast two.
		//
		strob_setlen(val, strlen(defvalue) + 10);
		memset(strob_str(val), (int)('\0'), strlen(defvalue) + 10);
		strob_strcpy(val, defvalue);
	
		p = strob_str(val);
		while (*p && error_code == 0) {
			value = NULL;	
			if (*p == '-' && islower(*(p+1)) && (isspace(*(p+2)) || *(p+2) == '\0')) {
				p++;	// Step over the dash '-'
				switch(*p) {
					case 't':
						SWEXTDEF_DEBUG("");
						p+=2;  // Step over the option letter and a space.
						value = bound_value(p, (char**)NULL, (int*)NULL);
						copy_file_cell(array, array_is_set, typeE, value);
						array_is_set[typeE] = EXPSET;
						skip_amount = strlen(value);
						break;
				
					case 'm':
						SWEXTDEF_DEBUG("");
						p+=2;
						value = bound_value(p, (char**)NULL, (int*)NULL);
						if (is_file_permissions_context) {
							if (did_umode) {
								fprintf(stderr, "swpackage: either mode or umask may be specified.\n");
								error_code = 2;
							} else {
								copy_file_cell(array, array_is_set, umaskE, NULL);
							}
							did_umode = 1;
						}
						if (strlen(value) == 0 || check_is_all_digits(value, " ") < 0) {
							fprintf(stderr, "swpackage: invalid mode [%s].\n", value);
							error_code = 2;
						}
						copy_file_cell(array, array_is_set, modeE, value);
						if (!is_file_permissions_context) {
							array_is_set[modeE] = EXPSET;
						}
						SWEXTDEF_DEBUG2("---- %d ----\n", array_is_set[modeE]);
						skip_amount = strlen(value);
						break;
				
					case 'o':
						//
						// If neither are set, use the database.
						// Lines 860-861.
						//
						SWEXTDEF_DEBUG("");
						p+=2;
						value = bound_value(p, &aux_value, &has_comma);
						skip_amount = 0; // this to avoid a warning
						ret = parse_name_id_string(value, aux_value,
								is_file_permissions_context, 
								ownerE,
								uidE,
								array,
								array_is_set,
								&skip_amount, has_comma);
						if (ret) {
							error_code = 4;
						}
						break;
					case 'g':
						SWEXTDEF_DEBUG("");
						p+=2;
						value = bound_value(p, &aux_value, &has_comma);
						ret = parse_name_id_string(value, aux_value,
								is_file_permissions_context, 
								groupE,
								gidE,
								array,
								array_is_set,
								&skip_amount, has_comma);
						if (ret) {
							error_code = 4;
						}
						break;
					case 'u':
						SWEXTDEF_DEBUG("");
						p+=2;
						value = bound_value(p, (char**)NULL, (int*)NULL);
						if (!is_file_permissions_context) {
							//
							// Error.
							//
							error_code = 2;
							fprintf(stderr, "swpackage: umask cannot be specified\n");
						} else {
							if (did_umode) {
								error_code = 2;
								fprintf(stderr, "swpackage: either mode or umask may be specified.\n");
							} else {
								copy_file_cell(array, array_is_set, modeE, NULL);
							}
						}
						if (strlen(value) < 3) {
							fprintf(stderr, "swpackage: bad umask string.\n");
							umask_value = 0;
							error_code = 2;
						} else {
							umask_value = *value + *(value+1) + *(value+2);	
						}
						if (umask_value > 144) { // '0' + '0' + '0' = 144d
							copy_file_cell(array, array_is_set, umaskE, value);
						} else if (umask_value == 144) {
							//
							// this is "-u 000"
							//
							array_is_set[umaskE] = 0;
							strcpy(array[umaskE], "");
						} else {
							//
							// error.
							//
							fprintf(stderr, "swpackage: bad umask value.\n");
							error_code = 2;
						}
						did_umode = 1;
						skip_amount = strlen(value);
						break;
					case 'n':     // Not compressable. 
						SWEXTDEF_DEBUG("");
						p++;
						*p = '\0';
						value = NULL;
						skip_amount = 1;
						copy_file_cell(array, array_is_set, compE, "not_compressible");
						if (!is_file_permissions_context)
							array_is_set[compE] = EXPSET;
						break;
					case 'v':     // Volatile, in contents, attributes or existence.
						SWEXTDEF_DEBUG("");
						p++;
						*p = '\0';
						value = NULL;
						skip_amount = 1;
						copy_file_cell(array, array_is_set, volaE, "true");
						is_volatileM = 1;
						break;

					default:
						SWEXTDEF_DEBUG("");
						p++;
						*p = '\0';
						value = p;
						skip_amount = strlen(value);
						fprintf(stderr, 
						"swpackage: error , unrecognized option in extended file permissions: %s\n", defvalue);
						strob_close(val);
						return static_cast<char*>(NULL);
						break;	

				} // switch 
			} else {
				// Must be at the end of the options.
				break;
			}
			if (value) {
				p+=skip_amount;
			} else {

			}
			while (isspace((int)(*p))){ p++; }
			if ((*p) == '\0') {
				p++;
				while (isspace((int)(*p))){ p++; }
			} else {
				if (skip_amount == 0) {
					;
				} else {
					// Must be at the end of the options.
					break;
				}
			}
		} // While loop.

		if (error_code) {
			strob_close(val);
			return static_cast<char*>(NULL);
		}
		//
		// return a pointer in defvalue to the remaining 'source [path]' portion.
		//
		p = defvalue + (p - strob_str(val));	
		strob_close(val);
		return p;
	}
	
	// 
	// Fill in the attributes from a source file as neccessary 
	// per 5.2.13.3 Lines 810-812 (p.158).  If source is NULL but type is set
	// then set default values.
	//	
	int
	stat_source_file_if(char *source, char * hl_path, struct stat *sti) {
		int retval;
		char * p_owner;
		char *p_mode;
		char *p_minor;
		char *p_major;
		char *p_ino;
		char *p_nlink;
		char *p_group;
		char * p_type;
		char * p_uid;
		char * p_gid;
		char * p_swbi;
		char * hlif_path; 
		mode_t mode;
		mode_t default_umask = 022;
		struct stat stbuf;
		struct stat *st;
		SWEXTDEF_DEBUG2("Begin: source: %s", source);
	
		retval = 0;
		if (sti != static_cast<struct stat*>(NULL)) {
			SWEXTDEF_DEBUG("sti is NULL");
			st = sti;
		} else {
			SWEXTDEF_DEBUG("");
			st = &stbuf;
			if (source) {
				SWEXTDEF_DEBUG2("running local_lstat_swvarfs() on source: %s", source);
				if (local_lstat_swvarfs(source, st)) {
					fprintf(stderr,"swpackage: stat error: %s\n", source);
					return -1;
				}
			} else {
				fprintf(stderr,"swpackage: internal error : source null\n");
				return -2;
			}
		}

		if (source == static_cast<char*>(NULL)) {
			//
			// FIXME, this code branch probably should not exist here
			//

			//
			// Set some default attribute values based on the type.
			//
			SWEXTDEF_DEBUG("source is NULL");
			p_mode = get_stat_cell_pointer(modeE);
			p_minor = get_stat_cell_pointer(minorE);
			p_major = get_stat_cell_pointer(majorE);
			p_ino = get_stat_cell_pointer(inoE);
			p_nlink = get_stat_cell_pointer(nlinkE);
			p_type = get_stat_cell_pointer(typeE);
			p_owner = get_stat_cell_pointer(ownerE);
			p_group = get_stat_cell_pointer(groupE);
			p_uid = get_stat_cell_pointer(uidE);
			p_gid = get_stat_cell_pointer(gidE);
			p_swbi = get_stat_cell_pointer(swbiE); 	// INternal attribute used as a flag.

			// fprintf(stderr, "swpackage: setting default file attributes.\n");
			strcpy(p_ino, "0");
			strcpy(p_nlink, "1");
			
			if (*p_type == '\0') {
				fprintf(stderr,"swpackage: internal error in stat_source_file_if().\n");
				return -2;
			}
			mode = swlib_apply_mode_umask(*p_type, default_umask, (mode_t)(0));
			snprintf(p_mode, file_permsLength-1, "%o", (unsigned int)mode);
			p_mode[file_permsLength - 1] = '\0';
			if (*p_owner == '\0') {
				SWEXTDEF_DEBUG("");
				strncpy(p_owner, "daemon", 7);
				strncpy(p_uid, "2", 6);
				statArrayWasSetM[uidE] = 1;
				statArrayWasSetM[ownerE] = 1;
			}	
			if (*p_group == '\0') {
				SWEXTDEF_DEBUG("");
				strncpy(p_group, "daemon", 7);
				strncpy(p_gid, "2", 6);
				statArrayWasSetM[gidE] = 1;
				statArrayWasSetM[groupE] = 1;
			}	
			statArrayWasSetM[swbiE] = 1;
			strncpy(p_swbi, "false", 10);
		} else {
			int cksumflags = 0;

			SWEXTDEF_DEBUG("");
			if (do_create_cksumM) {
				cksumflags |= SWFILE_DO_CKSUM;
			}
			if (do_create_md5sumM) {
				cksumflags |= SWFILE_DO_FILE_DIGESTS;
			}
			if (do_create_sha2M) {
				cksumflags |= SWFILE_DO_FILE_DIGESTS_SHA2;
			}
	
			SWEXTDEF_DEBUG("");
			init_statArray();

			if (hl_path && strlen(hl_path)) {
				hlif_path = hl_path;
			} else {
				hlif_path = source;
			}

			SWEXTDEF_DEBUG("Entering swmetadata_decode_file_stats");
			retval = swmetadata_decode_file_stats(swvarfsM, source,
				hlif_path, st, statArrayM,
				statArrayWasSetM, cksumflags);

			if (retval < 0) {
				fprintf(stderr, "swpackage: error processing file stats for %s\n", source);
				return -1;
			}

			if (do_create_cksumM) {
				copy_file_cell(fileArrayM, fileArrayWasSetM, cksumE, statArrayM[cksumE]);
			}
			if (do_create_md5sumM) {
				copy_file_cell(fileArrayM, fileArrayWasSetM, md5sumE, statArrayM[md5sumE]);
			}
			if (do_create_sha2M) {
				copy_file_cell(fileArrayM, fileArrayWasSetM, sha512sumE, statArrayM[sha512sumE]);
			}
			copy_file_cell(fileArrayM, fileArrayWasSetM, sha1sumE, statArrayM[sha1sumE]);
		}
		return retval;
	}
	
	//
	// Section 5.2.13.3 owner, uid, group, gid  Lines 854-862, 869,876
	//
	int
	handle_ownerships(	char array[][file_permsLength], int is_set[],
				char statarray[][file_permsLength], int stat_is_set[],
				int name_index, int id_index) {
		int ret;
		uid_t xuid;
		gid_t xgid;
		int uid;

		if (is_set[name_index] && is_set[id_index] == 0) {
			SWEXTDEF_DEBUG("");
			if (name_index == swMetaData::ownerE) {
				SWEXTDEF_DEBUG("");
				ret = taru_get_uid_by_name(array[name_index], &xuid);
				uid = (int)xuid;
			} else if (name_index == swMetaData::groupE) {
				SWEXTDEF_DEBUG("");
				ret = taru_get_gid_by_name(array[name_index], &xgid);
				uid = (int)xgid;
			} else {
				SWEXTDEF_DEBUG("");
				return -1;
			}

			if (ret == 0) {
				SWEXTDEF_DEBUG("");
				snprintf(array[id_index], file_permsLength, "%d", (int)(uid));
				array[id_index][file_permsLength - 1] = '\0';
				is_set[id_index] = 1;
			} else {
				SWEXTDEF_DEBUG("");
				// fprintf(stderr, 
				// 	"swpackage: warning : %s [%s] not found in system database.\n", 
				// 		swMetaData::namesM[name_index], array[name_index]);
				is_set[id_index] = 0;
			}
		} else if (is_set[name_index] == 0 && is_set[id_index]) {
			SWEXTDEF_DEBUG("");
			is_set[name_index] = 0;
		} else if (is_set[name_index] == 0 && is_set[id_index] == 0) {
			SWEXTDEF_DEBUG("");
			copy_file_cell(array, is_set, name_index, statarray[name_index]);
			copy_file_cell(array, is_set, id_index, statarray[id_index]);
		} else if (is_set[name_index] && is_set[id_index]) {
			SWEXTDEF_DEBUG("");
			;		
		}
		return 0;
	}
	
	//
	// Generate file attributes taking them from the source or defaultFilePermissions
	// definitions if required.
	//
	int 
	generate_ExplicitFileAttributes(char * source, char * hl_path, struct stat *stpo, int do_stat) {
		int retval;
		int ret;
		int i;
		mode_t def_umask;
		mode_t mode;
		unsigned int ui_def_umask;
		unsigned int ui_mode;
		
		retval = 0;
		SWEXTDEF_DEBUG("Begin");
		SWEXTDEF_DEBUG2("source=[%s]", source);
		//
		// Section 5.2.13.3
		// Lines 810-812.  If file_permissions keyword is active ...
		//
		if (
		    (
		        filePermissionsActiveM == 1
		    ) ||
		    (   file_cell_was_explicitly_set(ownerE) ||	   // Section 5.2.13.3, Lines 811-812.
		    	file_cell_was_explicitly_set(modeE) || 
			file_cell_was_explicitly_set(groupE)
		    )
		) {
			SWEXTDEF_DEBUG("filePermissionsActiveM and perms were explicitly set");
			for (i=0; i<lastE; i++) {
				SWEXTDEF_DEBUG("transferring from defaults array");
				//
				// Transfer the values from defaults array to explicit array.
				// if not already set.
				// If a cell was explicitly set, then copy it.
				//
				if (file_cell_was_explicitly_set(i) == 0 && file_cell_was_default_set(i)) {
					SWEXTDEF_DEBUG("");
					copy_file_cell(fileArrayM, static_cast<int*>(NULL), i, fileDefPermsArrayM[i]);
					set_file_was_set(fileArrayWasSetM, i);
				}
			}
		} 

		if (do_stat) {
			SWEXTDEF_DEBUG("in do_stat");
			ret = stat_source_file_if(source, hl_path, stpo);
			if (ret < 0) {
				fprintf(stderr,"swpackage: stat_source_file_if returned error.\n");
				exit(2); // FIXME
			} else if (ret > 0) {
				// ignore socket
				SWEXTDEF_DEBUG("ignoring socket");
				retval = ret;
			} else {
				retval = ret;
			}
		}

		if (
		    (
		        filePermissionsActiveM == 1
		    ) ||
		    (   file_cell_was_explicitly_set(ownerE) ||	   // Section 5.2.13.3, Lines 811-812.
		    	file_cell_was_explicitly_set(modeE) || 
			file_cell_was_explicitly_set(groupE)
		    )
		) {
			SWEXTDEF_DEBUG("AGAIN: filePermissionsActiveM and perms were explicitly set");
			SWEXTDEF_DEBUG("");
			if (fileArrayWasSetM[modeE] == 0 && fileDefPermsArrayWasSetM[umaskE]) {
				//
				// Apply the umask to make the mode.
				//
				// 
				// Section 5.2.13.4(885)  Lines 896-902
				// apply the umask to the mode.
				//
				// If the mode was explicitly set then apply it to this value.
				// If not explicitly set then make up a mode '666' or '777'
				//

				SWEXTDEF_DEBUG("applying umask");
				sscanf(fileDefPermsArrayM[umaskE], "%o", &ui_def_umask);
				def_umask = (mode_t)ui_def_umask;
				if (statArrayWasSetM[modeE]) {
					SWEXTDEF_DEBUG("");
					sscanf(statArrayM[modeE], "%o", &ui_mode);
					mode = (mode_t)ui_mode;
				} else {
					SWEXTDEF_DEBUG("");
					mode = (mode_t)(0);
				}
				mode = swlib_apply_mode_umask(*fileArrayM[typeE], def_umask, mode);
				snprintf(fileArrayM[modeE], file_permsLength - 1, "%4o", (unsigned int)mode);
				fileArrayM[modeE][file_permsLength - 1] = '\0';
				set_file_was_set(fileArrayWasSetM, modeE);
			}
		} 

		//
		// 5.2.13.3 -o [owner][,uid] Lines 853-881. 
		// Now apply the verbage of Lines 853-881 by adjusting fileArrayM[i] and
		// fileArrayWasSetM[i].
		// Transfer certain values from statArrayM to fileArrayM.
		// To prevent an attribute from appearing in the INFO file definition set
		// the statArrayWasSetM[i] to zero.
		//

		SWEXTDEF_DEBUG("transfer from stat array");
		handle_ownerships(fileArrayM, fileArrayWasSetM, statArrayM, statArrayWasSetM, ownerE, uidE);
		handle_ownerships(fileArrayM, fileArrayWasSetM, statArrayM, statArrayWasSetM, groupE, gidE);

		for (i=0; i<lastE; i++) {
			switch(i) {
				case ctimeE:
				case mtimeE:
				case sizeE:
				case modeE:
				case umaskE:
				case typeE:
				case linksourceE:
				case majorE:
				case minorE:
				case inoE:
				case nlinkE:
				case swbiE:
						//
						// Transfer the value from statArray to the fileArray.
						//
						SWEXTDEF_DEBUG("");
						if (fileArrayWasSetM[i] == 0) {
							SWEXTDEF_DEBUG3("copying from stat array [%s] = [%s]", namesM[i], statArrayM[i]);
							copy_file_cell(fileArrayM, static_cast<int*>(NULL), i, statArrayM[i]);
							set_file_was_set(fileArrayWasSetM, i);
							if (  0 ||
								//
								// These values are not in a stat buf or don't need preservation.
								//
								i == ctimeE || /* FIXME ?? why is this here */
								i == umaskE ||
								0
							) {
								fileArrayWasSetM[i] = 0;
							}
						}
						SWEXTDEF_DEBUG("");
						break;
				//	
				// Handled above.
				//
				case uidE:
				case gidE:
				case ownerE:
				case groupE:
						SWEXTDEF_DEBUG("");
						break;
			}
		}

		SWEXTDEF_DEBUG("");
		*fileArrayM[umaskE]='\0';
		return retval;
	}

	//
	// Attach the fileArray explicit file attributes to the swdefinition.
	//
	int
	attach_ExplicitFileAttributes(swDefinition * swdef, char * source, int recursive_context){
		int mem_fd = swdef->get_mem_fd();
		int current_offset = uxfio_lseek(mem_fd, 0, SEEK_CUR);
		int level = swdef->get_level();	
		swAttribute * swatt;
		char * value;
		unsigned long ulmode;
		unsigned int mode;
		int  cell; 
		char type = 0;
		unsigned int umask=0;

		SWEXTDEF_DEBUG("");
		for (cell=0; cell<lastE; cell++) {
			value = get_file_cell_pointer(cell);

			// Handle default.
			switch(cell) {
				case typeE:
					SWEXTDEF_DEBUG("");
					type = (char)(*value);
					break;
				case umaskE:
					SWEXTDEF_DEBUG("");
					if (*value != '\0') {
						if (!sscanf(value, "%o", (unsigned int*)&umask)) {
							fprintf(stderr,"swpackage: internal error scanning umask.\n");
						}
					} else {
						umask = 0;
					}
					break;
				case modeE:
					SWEXTDEF_DEBUG("");
					if (*value != '\0') {
						/*
						// if (!sscanf(value, "%o", (unsigned int*)&mode)) {
						// 	fprintf(stderr,"internal error scanning mode.\n");
						// }
						*/
						SWEXTDEF_DEBUG("");
						taru_otoul(value, &ulmode);
						mode = (unsigned int)ulmode;
						SWEXTDEF_DEBUG2(" mode value is [%o]", mode);
					} else {
						SWEXTDEF_DEBUG("");
						mode = 0;
						SWEXTDEF_DEBUG2(" mode value is [%o]", mode);
					}

					//
					// type and umask are already set because of the ordering of the
					// enumeration.
					//
					SWEXTDEF_DEBUG2("mode value before is [%o]", mode);
					SWEXTDEF_DEBUG2("file type is [%c]", (int)type);
					SWEXTDEF_DEBUG2("umask is [%o]", umask);

					//
					// Check here to see if umask was set explicitly, if not set
					// then assign the Unix process value return by umask()
					//
					
					if (file_cell_was_explicitly_set(modeE)) {
						;
						SWEXTDEF_DEBUG("mode already set");
						// nothing to do
					} else {
						//
						// Use the process umask
						//
						SWEXTDEF_DEBUG("setting umask with process value");

						//
						// This causes a regression test failure
						// if used unconditionally
						//
						if (non_existing_sourceM) {
							//
							// Apply the process umask only for
							// things like 'file -t d foobar'
							// and where 'foobar' does not exist
							//
							umask = my_get_umask();
						} else {

						}
					}
					SWEXTDEF_DEBUG2("umask (now finally) is [%o]", umask);
	
					mode = swlib_apply_mode_umask(type, umask, mode);
					SWEXTDEF_DEBUG2(" mode value after is [%o]", mode);
					if (recursive_context && type == SW_ITYPE_d) {
						//
						// make sure the executable bit is on for any
						// set that has the read bit on.
						// Do this only for file generated by the
						// recursive file extended definition.
						//
						if (mode & S_IRUSR) {
							mode |= S_IXUSR;
						}
						if (mode & S_IRGRP) {
							mode |= S_IXGRP;
						}
						if (mode & S_IROTH) {
							mode |= S_IXOTH;
						}
					}
					snprintf(value, file_permsLength, "%o", (unsigned int)mode);
					value[file_permsLength - 1] = '\0';
					SWEXTDEF_DEBUG2(" mode string value is [%s]", value);
					break;
				case uidE:
				case gidE:
				case ownerE:
				case groupE:
				case sizeE:
				case volaE:
				case compE:
				case cksumE:
				case md5sumE:
				case sha1sumE:
				case sha512sumE:
				case mtimeE:
				case ctimeE:
				case inoE:
				case nlinkE:
					SWEXTDEF_DEBUG("");
					break;
				case majorE:
				case minorE:
					SWEXTDEF_DEBUG("");
					if (type != SW_ITYPE_c && type != SW_ITYPE_b) continue;
					break;
				case linksourceE:
				case swbiE:
					SWEXTDEF_DEBUG("");
					break;
			
				default:
					SWEXTDEF_DEBUG("");
					fprintf(stderr,"swpackage: internal error in attach_ExplicitFileAttributes().\n");
					break;	
			
			}
			//
			// Add the attributes to the definition.
			//
			SWEXTDEF_DEBUG("");
			if (cell != umaskE && *value != '\0' && (fileArrayWasSetM[cell] || file_cell_was_default_set(cell))) {
				swatt = new swAttribute(const_cast<char*>(namesM[cell]), value);
				if (fileArrayWasSetM[cell] >= EXPSET) {
					swatt->set_is_explicit(); 
				}
				swatt->set_level(level + 1);
				if (
					(
						cell == nlinkE ||
						cell == inoE ||
						cell == swbiE 
					)
						||
					(
						(cell == groupE || cell == ownerE) &&
						strcmp(value, TARU_C_DO_NUMERIC) == 0
					)
				) {
					SWEXTDEF_DEBUG("");
					swatt->vremove();
				}
				// if (cell == uidE || cell == gidE) {
				// 	// fprintf(stderr, "cell = %d id = %s\n", cell, value);
				// }
				swdef->list_add(swatt);
			}
		}
		//
		// Set the file back the way it was.
		//
		SWEXTDEF_DEBUG("");
		uxfio_lseek(mem_fd, current_offset, SEEK_SET);
		return 0;
	}
	
	int 
	processExplicitFile_i(char * value, swDefinition * parent_swdef, struct stat *st,
				int check_one_active, int recursive_context, int * do_no_stat)
	{
		int ret;
		swDefinition * swdef;
		char * source_path;
		char * source;
		char * path;
		char * o_source;
		char * o_path;
		char * old_follow_state;


		SWEXTDEF_DEBUG2("Begin: [%s]", value);
		strob_strcpy(file_pathM, "");
		strob_strcpy(file_linksource_pathM, "");

		swdefFormedM = static_cast<swDefinition*>(NULL);
		init_file_opts();
		path_open_if(&file_sourceM);
		
		//
		// Parse file opts and source/path.
		//
		source_path = parse_FileOpts(value, fileArrayM, fileArrayWasSetM, 0);
		if (source_path == static_cast<char*>(NULL)) {
			strob_strcpy(file_pathM, value);
			return -1;
		}
		SWEXTDEF_DEBUG2("source_path=[%s]", source_path);
		SWEXTDEF_DEBUG("");
		parse_SourcePath(source_path, &source, &path);
		SWEXTDEF_DEBUG("");
		SWEXTDEF_DEBUG2("source=[%s]", source);
		SWEXTDEF_DEBUG2("path=[%s]", path);
	
		unprotect_space(source);
		unprotect_space(path);
	
		o_source = source;
		o_path = path;
		if (o_path) strob_strcpy(file_pathM, o_path);

		if (verbose_levelM > SWC_VERBOSE_3) {
			fprintf(stderr, "swpackage: processing %s\n", o_source);
		}

		//
		// Source is req'd according to the synopsis on line 788.
		// Do required Sanity checks.
		//
		if (source == static_cast<char*>(NULL)) {
			fprintf(stderr,"swpackage: psf error: source must be specified: %s.\n", value);
			return -1;
		}

		//
		// Certain uses of this function do not want the following
		// tests done.
		//
		if (check_one_active) {
			if (directoryDestActive() == 0 && directoryActive() == 0 ) {
				//
				// 5.2.13.3: Lines 795-797.
				//
				SWEXTDEF_DEBUG("");
				if (source == static_cast<char*>(NULL) || path == static_cast<char*>(NULL)) {
					fprintf(stderr,
					"swpackage: error: path must be specified if directory dest is not active.\n");
					return -1;
				}
			}
			if (directoryDestActive() == 0) {
				//
				// 5.2.13.3: Lines 823-826.
				//
				SWEXTDEF_DEBUG("");
				if (path && *path != '/') {
					fprintf(stderr,
					"swpackage: psf error: path must be absolute if directory dest is not active.\n");
					return -1;
				}	
			}
		} else {
			;
			SWEXTDEF_DEBUG("");
		}

		if (fileArrayWasSetM[typeE] == 0) {
			SWEXTDEF_DEBUG("");
			;
		} else if (fileArrayM[typeE][0] == SW_ITYPE_d) {	
			SWEXTDEF_DEBUG("");
			;
		} else if (fileArrayM[typeE][0] == SW_ITYPE_h) {	
			SWEXTDEF_DEBUG("");
			fileArrayWasSetM[linksourceE] = 1;
			;
		} else if (fileArrayM[typeE][0] == SW_ITYPE_s) {	
			SWEXTDEF_DEBUG("");
			fileArrayWasSetM[linksourceE] = 1;
			;	
		} else if (fileArrayM[typeE][0] == SW_ITYPE_x) {	
			//
			// C701 extension. not supported.
			//
			SWEXTDEF_DEBUG("");
			fprintf(stderr, "swpackage: type x file type (deletion) is not supported.\n");
			return -1;
		} else {	
			SWEXTDEF_DEBUG("");
			fprintf(stderr, "swpackage: bad file type in PSF file : '%s'.\n", fileArrayM[typeE]);
			return -1;
		}
		SWEXTDEF_DEBUG3("type o_path=[%s] o_source=[%s]\n", o_path, o_source);

		//
		// Process Source Attribute
		//
		SWEXTDEF_DEBUG("");
		if (get_QualifiedAbsoluteSourcePath(file_sourceM, directory_sourceM, source) == static_cast<char*>(NULL)) {
			fprintf(stderr,"swpackage: internal error in  get_QualifiedAbsoluteSourcePath().\n");
			return -3;
		}

		//
		// Process the path attribute.
		//
		SWEXTDEF_DEBUG("");
		if (make_ActivePath(path, source, file_pathM) == static_cast<char*>(NULL)) {
			fprintf(stderr,"swpackage: internal error in  make_ActivePath().\n");
			return -4;
		}

		swdef = swDefinition::make_newDefinition(parent_swdef, "file");
		assert(swdef);

		path = strob_str(file_pathM);
		source = strob_str(file_sourceM);

		swlib_squash_embedded_dot_slash(path);
		swlib_squash_embedded_dot_slash(source);
		SWEXTDEF_DEBUG3("type path=[%s] source=[%s]", path, source);
		SWEXTDEF_DEBUG3("type o_path=[%s] o_source=[%s]", o_path, o_source);
		SWEXTDEF_DEBUG("HERE before main else ladder");
		if (fileArrayWasSetM[typeE] == 0) {
			SWEXTDEF_DEBUG("fileArrayWasSetM[typeE] == 0)");
			old_follow_state = swvarfs_get_stat_syscall(swvarfsM);
			if (follow_symlinksM) {
				swvarfs_set_stat_syscall(swvarfsM, "stat" /* stat or lstat */);
			}
			attach_filePaths(swdef, SW_A_source, source, path);
			SWEXTDEF_DEBUG("Entering generate_ExplicitFileAttributes");
			ret = generate_ExplicitFileAttributes(source, source, st, 1);
			swvarfs_set_stat_syscall(swvarfsM, old_follow_state);
			if (ret < 0) {
				return -1;
			}
			if (ret > 0) {
				// ignore file, such as a socket
				delete swdef;
				swdefFormedM = NULL;
				return 1;
			}
			SWEXTDEF_DEBUG("end of fileArrayWasSetM[typeE] == 0");
		} else if (fileArrayM[typeE][0] == SW_ITYPE_d) {
			//
			// 5.2.13.3: Lines 831-836.
			//
			SWEXTDEF_DEBUG("In type d");
			if (is_path_true(o_source) && !is_path_true(o_path)) {
				if (directoryDestActive()) {
					attach_filePaths(swdef, SW_A_source, source, path);
				} else {
					attach_filePaths(swdef, SW_A_source, source, source);
				}
				SWEXTDEF_DEBUG("type d: case 1");
				SWEXTDEF_DEBUG2("format type %d", swvarfs_get_format(swvarfsM));
				if (
					(
					swvarfs_get_format(swvarfsM) != UINFILE_FILESYSTEM &&
					(strcmp(source, ".") == 0 || strcmp(source, "./") == 0 || strcmp(source, "/") == 0)
					) ||
					local_access(source, F_OK)
				) {
					//
					// not found or no access.
					// Don't stat the source
					//
					if (do_no_stat) *do_no_stat = 1;
					non_existing_sourceM = 1;
					SWEXTDEF_DEBUG2("%s has no access", source);
	
					//
					// apply implementation defined values
					// per Section 5.2.13.3 Lines 831-836
					//
				} else {
					//
					// file exists
					//
					SWEXTDEF_DEBUG2("%s does have access",source);
					non_existing_sourceM = 0;
					if (do_no_stat) *do_no_stat = 0;
				}
			} else if (is_path_true(o_source) && is_path_true(o_path)) {
				//
				// 5.2.13.3: Lines 832-833.
				// Do stat the source.
				//
				attach_filePaths(swdef, SW_A_source, source, path);

				if (local_access(source, F_OK)) {
					SWEXTDEF_DEBUG2("%s has no access", source);
					non_existing_sourceM = 1;
					if (do_no_stat) *do_no_stat = 1;
				} else {
					SWEXTDEF_DEBUG2("%s does have access", source);
					non_existing_sourceM = 0;
					if (do_no_stat) *do_no_stat = 0;
				}
				SWEXTDEF_DEBUG("type d: case 2");
			} else {
				SWEXTDEF_DEBUG("type d: case 3");
				fprintf(stderr, "swpackage: invalid explicit file definition for type 'd'\n");
				return -1;
			}

			is_file_attribute_sufficient(do_no_stat, fileArrayWasSetM, fileDefPermsArrayWasSetM);
			//
			// *do_no_stat has now been modifed by the above function.
			//

			if (*do_no_stat) {
				//
				// no file to stat, therefore 
				// apply implementation defined values
				// per Section 5.2.13.3 Lines 831-836
				//
				;
				if (apply_default_values(fileArrayM[typeE][0]))
					return -1;
			}
			SWEXTDEF_DEBUG("Entering generate_ExplicitFileAttributes");
			if (generate_ExplicitFileAttributes(source, source, st, !(*do_no_stat)) < 0) return -1;
		} else if (fileArrayM[typeE][0] == SW_ITYPE_h) {
			//
			// Hard Link: 5.2.13.3: Lines 837-840.
			//
			SWEXTDEF_DEBUG("In type h");
			if (local_access(source, F_OK)) {
				//
				// not found or no access.
				// Don't stat the source
				//
				if (do_no_stat) *do_no_stat = 1;
				non_existing_sourceM = 1;
				SWEXTDEF_DEBUG2("%s has no access", source);

				//
				// apply implementation defined values
				// per Section 5.2.13.3 Lines 831-836
				//
			} else {
				//
				// file exists
				//
				SWEXTDEF_DEBUG2("%s does have access",source);
				non_existing_sourceM = 0;
				if (do_no_stat) *do_no_stat = 0;
			}

			if (!is_path_true(o_source) || !is_path_true(o_path)) {
				fprintf(stderr, "swpackage: invalid explicit file definition for type 'h'\n");
				return -1;
			}

			if (directoryDestActive()) {
				//
				// apply directory mapping to the link_source.
				//
				if (make_ActivePath((char*)NULL, source, file_linksource_pathM) == static_cast<char*>(NULL)) {
					fprintf(stderr,"swpackage: internal error in  make_ActivePath(). loc=linksource\n");
					return -4;
				}
			} else {
				strob_strcpy(file_linksource_pathM, o_path);
			}

			//
			// There are three ways to handle this:
			//  1) Add a line in the "as now theoretical-non-existing" _install script
			//     and do not include in the storage section.
			//         ln source path 
			//         # The "ln' command uses the file attributes of the source
			//	   # which is what we want to happen (presumably, absent other information).
			//  2) Specify and require all the file attributes are specified
			//     (because there is no safe reasonable way to get default values??
			//  3) Guess at defaults, assume the hard link source is a regular file.
			//
			// For now we will use 3)

			if (*do_no_stat) {
				// No file to stat
				// Assume regular file
				if (apply_default_values((int)(SW_ITYPE_f)))
					return -1;
			}

			attach_filePaths(swdef, "link_source", strob_str(file_linksource_pathM), (char*)(NULL));
			attach_filePaths(swdef, SW_A_source, o_source, strob_str(file_pathM));
			SWEXTDEF_DEBUG("Entering generate_ExplicitFileAttributes");
			if (generate_ExplicitFileAttributes(source, (char *)NULL, st, !(*do_no_stat)) < 0) return -1;

		} else if (fileArrayM[typeE][0] == SW_ITYPE_s) {
			//
			// Symbolic link: 5.2.13.3: Lines 841-844
			//
			SWEXTDEF_DEBUG("In type s");
			if (!is_path_true(source) || !is_path_true(path)) {
				fprintf(stderr, "swpackage: invalid explicit file definition for type 's'\n");
				return -1;
			}
			if (directoryDestActive()) {
				//
				// apply directory mapping to the link_source.
				//
				if (make_ActivePath((char*)NULL, source, file_linksource_pathM) == static_cast<char*>(NULL)) {
					fprintf(stderr,"swpackage: internal error in  make_ActivePath(). loc=sym_linksource\n");
					return -4;
				}
			} else {
				strob_strcpy(file_linksource_pathM, o_path);
			}
			attach_filePaths(swdef, "link_source", strob_str(file_linksource_pathM), (char*)(NULL));
			attach_filePaths(swdef, SW_A_source, o_source, strob_str(file_pathM));
			if (apply_default_values(fileArrayM[typeE][0]))
				return -1;
			copy_file_cell(fileArrayM, fileArrayWasSetM, modeE, "0777");
			SWEXTDEF_DEBUG("Entering generate_ExplicitFileAttributes");
			if (generate_ExplicitFileAttributes(source, (char*)NULL, st, 0) < 0) return -1;
			if (do_no_stat) *do_no_stat = 1;
		} else {
			fprintf(stderr, "swpackage: bad file type in PSF file : '%s'.\n", fileArrayM[typeE]);
			return -1;
		}

		SWEXTDEF_DEBUG("");
		if (attach_ExplicitFileAttributes(swdef, source, recursive_context)) return -2;

		SWEXTDEF_DEBUG("");
		if (recursive_context) {
			//
			// set storage_status of "./" directory
			// to zero.
			//
			if (*fileArrayM[typeE] == SW_ITYPE_d && (strcmp(path, "./") == 0 || strcmp(path, ".") == 0)) {
				swdef->set_storage_status(SWDEF_STATUS_IGN);
			}
		}

		SWEXTDEF_DEBUG("");
		swdefFormedM = swdef;
		return 0;
	}
};
#endif
