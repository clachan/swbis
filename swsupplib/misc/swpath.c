/* swpath.c: parse a Posix-7.2 package pathname.
 */

/*
   Copyright (C) 1998 Jim Lowe <jhlowe@acm.org>
   All Rights Reserved.
  
   COPYING TERMS AND CONDITIONS
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "swpath.h"
#define SWPATH_PARSE_CA		22

#define SWPATH_DEGEN_N 		4

#include "debug_config.h"
#ifdef SWPATHNEEDDEBUG
#define SWPATH_E_DEBUG(format) SWBISERROR("SWPATH DEBUG: ", format)
#define SWPATH_E_DEBUG2(format, arg) SWBISERROR2("SWPATH DEBUG: ", format, arg)
#define SWPATH_E_DEBUG3(format, arg, arg1) SWBISERROR3("SWPATH DEBUG: ", format, arg, arg1)
#else
#define SWPATH_E_DEBUG(arg)
#define SWPATH_E_DEBUG2(arg, arg1)
#define SWPATH_E_DEBUG3(arg, arg1, arg2)
#endif /* SWPATHNEEDDEBUG */

#define SWPATH_DEBUG 1
#undef SWPATH_DEBUG 

/* // -------------------- Private Functions ------------------------- */

static
int 
check_error(SWPATH * swpath, int ret)
{
	if (
		swpath->dfiles_guardM > 1 ||
		swpath->catalog_guardM > 1 ||
		swpath->errorM
	) 
		return -1;
	else
		return ret;
}

static
void 
set_control_depth(SWPATH * swpath, int nd)
{
	if (swpath->swpath_is_catalog_ == 0) 
		return;
	if (swpath->control_path_nominal_depth_ < nd) {
		swpath->control_path_nominal_depth_ = nd; 
	}
}

/*
FIXME: this should be conditionally compiled.
NEEDED for debugging statements, don't delete
*/
static
char * swpath_parse_write_debug(STROB * buf, int relative_n, char ** ca)
{
	strob_sprintf(buf, 0, 
		"rel_n = %d: ca[0]=[%s] ca[1]=[%s] ca[2]=[%s]"
		" ca[3]=[%s] ca[4]=[%s]", relative_n, ca[0]?ca[0]:"", 
					ca[1]?ca[1]:"", 
					ca[2]?ca[2]:"", 
					ca[3]?ca[3]:"", 
					ca[4]?ca[4]:"");
        return strob_str(buf);
}

static
void set_p_pfiles(SWPATH * swpath, char * name) {
	if (!swpath->swpath_p_pfiles_ && name) {
		strob_strcpy(swpath->p_pfiles_, name);
		swpath->swpath_p_pfiles_ = strob_str(swpath->p_pfiles_);
	}
}

static
void set_p_dfiles(SWPATH * swpath, char * name) {
	if (!swpath->swpath_p_dfiles_) {
		strob_strcpy(swpath->p_dfiles_, name);
		swpath->swpath_p_dfiles_=strob_str(swpath->p_dfiles_);
	} else {
		;
	}
}

static
char * get_p_dfiles(SWPATH * swpath) {
	return swpath->swpath_p_dfiles_;
}

static
char * get_p_pfiles(SWPATH * swpath) {
	return swpath->swpath_p_pfiles_;
}

static
void set_p_fileset_dir(SWPATH * swpath, char * name) {
	if (!name) {
		swpath_set_fileset_control_dir(swpath, "");
		swpath->swpath_p_fileset_dir_ = NULL;
	}
	else {
		swpath_set_fileset_control_dir(swpath, name);
		swpath->swpath_p_fileset_dir_ = 
				strob_str(swpath->fileset_control_dir_);
	}
}

static
void swpath__set__basename(SWPATH * swpath, char *pathname)
{
	char *str;
	str = strrchr(pathname, (int) '/');
	if (!str)
		strob_strcpy(swpath->basename_, pathname);
	else
		strob_strcpy(swpath->basename_, str + 1);
}

static
void swpath__slashclean(SWPATH * swpath, char *madelist)
{
	char *p1;

	/* squash double slashes in madelist */
	while ((p1 = strstr(madelist, "//")) != (char *) (NULL))
		memmove(p1, p1 + 1, strlen(p1));

	/* squash leading ./ */
	if (strlen(madelist) >= 3) {
		if (!strncmp(madelist, "./", 2)) {
			p1 = madelist;
			memmove(p1, p1 + 2, strlen(p1 + 1));
		}
	}
	/* squash leading slash */
	if (*madelist == '/' && strlen(madelist) > 1)
		memmove(madelist, madelist + 1, strlen(madelist));

	/* squash trailing slash */
	if (strlen(madelist) > 1 && 
			(*(madelist + strlen(madelist) - 1)) == '/') {
		(*(madelist + strlen(madelist) - 1)) = '\0';
	}
}

static 
void 
swpath__form_path1(SWPATH * swpath, STROB * buf, char * name)
{
	if (strob_strlen(swpath->product_control_dir_) && strob_strlen(buf))
		strob_strcat(buf, "/");
	strob_strcat(buf, strob_str(swpath->product_control_dir_));

	if (strob_strlen(swpath->fileset_control_dir_))
		strob_strcat(buf, "/");
	strob_strcat(buf, strob_str(swpath->fileset_control_dir_));

	if (strob_strlen(swpath->dfiles_))
		strob_strcat(buf, "/");
	strob_strcat(buf, strob_str(swpath->dfiles_));

	if (strob_strlen(swpath->pfiles_))
		strob_strcat(buf, "/");
	strob_strcat(buf, strob_str(swpath->pfiles_));

	if (!name) {
		if (strob_strlen(swpath->pathname_))
			strob_strcat(buf, "/");
		strob_strcat(buf, strob_str(swpath->pathname_));
	} else {
		strob_strcat(buf, name);
	}
}

static
void 
swpath__unsetup1(SWPATH * swpath)
{
	free(swpath->pfiles_default);
	free(swpath->dfiles_default);
	strob_close(swpath->open_pathname_);
	strob_close(swpath->product_control_dir_);
	strob_close(swpath->fileset_control_dir_);
	strob_close(swpath->pfiles_);
	strob_close(swpath->dfiles_);
	strob_close(swpath->swpackage_pathname_);
	strob_close(swpath->pathname_);
	strob_close(swpath->prepath_);
	strob_close(swpath->buffer_);
	strob_close(swpath->basename_);
	strar_close(swpath->pathlist_);
	swpath->is_minimal_layoutM = 0;
	swpath->dfiles_guardM = 0;
	swpath->catalog_guardM = 0;
}

static
void 
swpath__setup1(SWPATH * swpath)
{
	swpath->errorM = 0;
	swpath->open_pathname_ = strob_open(2);
	swpath->product_control_dir_ = strob_open(2);
	swpath->fileset_control_dir_ = strob_open(2);
	swpath->pfiles_ = strob_open(2);
	swpath->dfiles_ = strob_open(2);
	swpath->swpackage_pathname_ = strob_open(512);
	swpath->pathname_ = strob_open(2);
	swpath->prepath_ = strob_open(2);
	swpath->buffer_ = strob_open(2);
	swpath->basename_ = strob_open(2);
	swpath->p_pfiles_ = strob_open(2);
	swpath->p_dfiles_ = strob_open(2);
	swpath->all_filesets_ = strob_open(2);
	swpath->pathlist_ = strar_open();
	
	swpath->max_rel_n_ = 0;
	swpath->pfiles_default = strdup("pfiles");
	swpath->dfiles_default = strdup("dfiles");

	strob_strcpy(swpath->product_control_dir_, "");
	strob_strcpy(swpath->fileset_control_dir_, "");
	strob_strcpy(swpath->pfiles_, "");
	strob_strcpy(swpath->dfiles_, "");
	strob_strcpy(swpath->pathname_, "");
	strob_strcpy(swpath->basename_, "");
	strob_strcpy(swpath->prepath_, "");
	strob_strcpy(swpath->p_pfiles_, "");
	strob_strcpy(swpath->p_dfiles_, "");
	swpath->swpath_is_catalog_ = 0;
	swpath->control_path_nominal_depth_ = 0;
	swpath->swpath_p_prepath_= (char*)NULL;
	swpath->swpath_p_dfiles_= (char*)NULL;
	swpath->swpath_e_dfiles_= (char*)NULL;
	swpath->swpath_p_fileset_dir_= (char*)NULL;
	swpath->swpath_p_product_dir_= (char*)NULL;
	swpath->swpath_p_pfiles_= (char*)NULL;
	swpath->swpath_e_pfiles_= (char*)NULL;
	swpath->is_minimal_layoutM = 0;
	swpath->dfiles_guardM = 0;
	swpath->catalog_guardM = 0;
}

static 
void 
set_v_dfiles(SWPATH * swpath, char *s)
{
	if (strob_strlen(swpath->dfiles_) == 0 &&
		strlen(s)
	) {
		(swpath->dfiles_guardM)++;
	}	
	strob_strcpy(swpath->dfiles_, s);
}

static 
void 
set_v_pfiles(SWPATH * swpath, char *s)
{
	strob_strcpy(swpath->pfiles_, s);
}

static
int
is_in_pfiles(SWPATH * swpath, char ** ca)
{
	char * ppf = get_p_pfiles(swpath);

	if (ppf) {
		if (strcmp(ca[1], ppf) == 0) {
			return 2;
		}
		if (strcmp(ca[2], ppf) == 0) {
			return 3;
		}
	}	
		
	if (strcmp(ca[1], strob_str(swpath->pfiles_)) == 0) { return 2; }
	if (strcmp(ca[1], swpath->pfiles_default) == 0) { return 2; }

	if (strcmp(ca[2], strob_str(swpath->pfiles_)) == 0) { return 3; }
	if (strcmp(ca[2], swpath->pfiles_default) == 0) { return 3; }

	if (swpath->swpath_p_dfiles_ && 
		swpath->swpath_p_pfiles_ == NULL && 
		swpath->swpath_p_fileset_dir_ == NULL) {
			return 3;
		}
	return 0;	
}

static
void
set_max_rel_n(SWPATH * swpath, int n)
{
	if (n > swpath->max_rel_n_) swpath->max_rel_n_ = n;
}

static void 
set_is_catalog(SWPATH * swpath, int n, char * loc)
{
	int old;
	old = swpath->swpath_is_catalog_;
	if (old == -1 && n == 0) {
		/*
		* Illegal transistion.
		*/
		swpath->errorM = 2;
	}
	if (old == 1 && n == -1) {
		/*
		* Illegal transistion.
		*/
		swpath->errorM = 2;
	}

	if (old != 1 && n == 1) {
		/*
		* transition into the catalog
		*/
		swpath->catalog_guardM ++;
	}
	if (swpath->catalog_guardM > 1) {
		/* 
		* Sanity check.
		* Only allow one (1) transistion into the
		* catalog.
		*/
		swpath->errorM = 2;
	}
	swpath_set_is_catalog(swpath, n);
}

/* // ------------ Public Functions ----------------------------- */

int 
swpath_num_of_components(SWPATH * swpath, char *str)
{
	int i = 0;
	strob_strcpy(swpath->buffer_, str);
	while (strtok(i ? (char *) (NULL) : strob_str(swpath->buffer_), "/"))
		i++;
	return i;
}

int 
swpath_resolve_prepath(SWPATH * swpath, char *name)
{
	int len;
	char *p, *s;
	if (swpath->swpath_p_prepath_)
		return 0;
	p = strstr(name, SW_A_catalog);
	if (!p) {
		/*
		 * If name does not contain 'catalog'
		 * and we are still dealing with a partial or
		 * complete pre-path.
		 */
		s = strstr(name, strob_str(swpath->prepath_));
		if (s) {
			strob_strcat(swpath->prepath_, 
				name + strob_strlen(swpath->prepath_));
		} else {
			strob_strcpy(swpath->prepath_, "");
		}
		swpath->swpath_p_prepath_ = (char *) (NULL);
		return -1;
	} else if (p && !swpath->swpath_p_prepath_ && 
				!strob_strlen(swpath->prepath_)) {
		/*
		 * No directory files preceeded the first file in the catalog.
		 *  i.e.  <pre_path>/catalog was the first file in the archive.
		 */
		len = (int) (p - name);
		strob_set_length(swpath->prepath_, len + 1);
		strob_strncpy(swpath->prepath_, name, len + 20);
		if (len) 
			strob_str(swpath->prepath_)[len - 1] = '\0';
		else
			strob_str(swpath->prepath_)[len] = '\0';
		swpath->swpath_p_prepath_ = strob_str(swpath->prepath_);
	} else {
		/*
		 * Already have the pre-path.
		 * Make sure it matches what was accumalated previously.
		 */
		len = (int) (p - name);
		strob_set_length(swpath->prepath_, len + 1);
		strob_str(swpath->prepath_)[len] = '\0';
		swpath->swpath_p_prepath_ = strob_str(swpath->prepath_);
		if (strncmp(name, swpath->swpath_p_prepath_, 
						len-1 < 0 ? 0 : len-1)) {
			fprintf(stderr,
				"swpath: inconsistent package pre-path"
				" %s not = %s over %d chars.\n", 
					name, swpath->swpath_p_prepath_, len);
			return 1;
		}
		return 0;
	}
	return 0;
}

SWPATH *
swpath_open(char *path)
{
	SWPATH *swpath = malloc(sizeof(SWPATH));
	swpath__setup1(swpath);
	strob_strcpy(swpath->open_pathname_, path);
	if (strlen(path))	
		swpath_parse_path(swpath, strob_str(swpath->open_pathname_));
	return swpath;
}

void
swpath_reset(SWPATH * swpath)
{
	STROB * tmp = strob_open(10);
	
	strob_strcpy(tmp, strob_str(swpath->open_pathname_));
	swpath__unsetup1(swpath);
	swpath__setup1(swpath);
	strob_strcpy(swpath->open_pathname_, strob_str(tmp));
	if (strlen(strob_str(tmp)))	
		swpath_parse_path(swpath, strob_str(swpath->open_pathname_));
	return;
}

char *
swpath_form_path(SWPATH * swpath, STROB * buf)
{
	if(swpath->swpath_is_catalog_)
		return swpath_form_catalog_path(swpath, buf);
	else	
		return swpath_form_storage_path(swpath, buf);
}

void 
swpath_set_dfiles(SWPATH * swpath, char * name) {
	free(swpath->dfiles_default);
	swpath->dfiles_default = strdup(name);
}

void 
swpath_set_pfiles(SWPATH * swpath, char * name) {
	free(swpath->pfiles_default);
	swpath->pfiles_default = strdup(name);
}

int 
swpath_close(SWPATH * swpath)
{
	swpath__unsetup1(swpath);
	free(swpath);
	return 0;
}

void 
swpath_set_product_control_dir(SWPATH * swpath, char *s)
{
	if (s && strlen(s)) set_control_depth(swpath, 2);
	if (swpath->swpath_is_catalog_ == 0) {
		if (swpath->control_path_nominal_depth_ == 0) {
			s = "";
		}
	}
	strob_strcpy(swpath->product_control_dir_, s);
}

void 
swpath_set_fileset_control_dir(SWPATH * swpath, char *s)
{
	if (s && strlen(s)) set_control_depth(swpath, 1);
	if (swpath->swpath_is_catalog_ == 0) {
		if (swpath->control_path_nominal_depth_ == 0) {
			s = "";
		}
	}
	strob_strcpy(swpath->fileset_control_dir_, s);
}

void 
swpath_set_pathname(SWPATH * swpath, char *s)
{
	strob_strcpy(swpath->pathname_, s);
}

void 
swpath_set_pkgpathname(SWPATH * swpath, char *s)
{
	strob_strcpy(swpath->swpackage_pathname_, s);
}

void 
swpath_set_filename(SWPATH * swpath, char *s)
{
	if (!s)
		swpath__set__basename(swpath, strob_str(swpath->pathname_));
	else
		swpath__set__basename(swpath, s);
}

char *
swpath_get_product_control_dir(SWPATH * swpath)
{
	return strob_str(swpath->product_control_dir_);
}

char *
swpath_get_fileset_control_dir(SWPATH * swpath)
{
	return strob_str(swpath->fileset_control_dir_);
}

char *
swpath_get_pfiles(SWPATH * swpath)
{
	return strob_str(swpath->pfiles_);
}

char *
swpath_get_dfiles(SWPATH * swpath)
{
	return strob_str(swpath->dfiles_);
}

char *
swpath_get_prepath(SWPATH * swpath)
{
	return strob_str(swpath->prepath_);
}

char *
swpath_get_pathname(SWPATH * swpath)
{
	return strob_str(swpath->pathname_);
}

char *
swpath_get_pkgpathname(SWPATH * swpath)
{
	return strob_str(swpath->swpackage_pathname_);
}

char *
swpath_get_basename(SWPATH * swpath)
{
	return strob_str(swpath->basename_);
}

char *
swpath_form_catalog_path(SWPATH * swpath, STROB * buf)
{
	strob_strncpy(buf, strob_str(swpath->prepath_), 200);
	if (swpath->swpath_is_catalog_ < 0)
		return strob_str(buf);
	if (strob_strlen(buf))
		strob_strcat(buf, "/");
	strob_strcat(buf, SW_A_catalog);
	swpath__form_path1(swpath, buf, NULL);
	return strob_str(buf);
}

char *
swpath_form_storage_path(SWPATH * swpath, STROB * buf)
{
	strob_strncpy(buf, strob_str(swpath->prepath_), 200);
	swpath__form_path1(swpath, buf, NULL);
	return strob_str(buf);
}

void 
swpath_set_is_catalog(SWPATH * swpath, int n)
{
	swpath->swpath_is_catalog_ = n;
}

int 
swpath_get_is_catalog(SWPATH * swpath)
{
	return swpath->swpath_is_catalog_;
}

void
swpath_set_is_minimal_layout(SWPATH * swpath, int n)
{
	swpath->is_minimal_layoutM = n;
}

int 
swpath_get_is_minimal_layout(SWPATH * swpath)
{
	return swpath->is_minimal_layoutM;
}

/* parse name to fill in the path components. */
int 
swpath_parse_path(SWPATH * swpath, char *name)
{
	int relative_n;
	int n = 0;
	int i;
	int n_prepath;
	int ret = 0;
	int compy = 0;
	int pfiles_index = 0;

	char *s, *p;
	char *ca[SWPATH_PARSE_CA + 1];
	#ifdef SWPATHNEEDDEBUG
	STROB * dbuf = strob_open(100);
	#endif

	SWPATH_E_DEBUG2("ENTER swpath_parse_path [%s]", name); 
	strob_strcpy(swpath->buffer_, name);
	swpath_set_pkgpathname(swpath, strob_str(swpath->buffer_));
	swlib_squash_trailing_slash(strob_str(swpath->buffer_));
	swlib_squash_leading_dot_slash(strob_str(swpath->buffer_));
	swlib_squash_leading_slash(strob_str(swpath->buffer_));

	strar_add(swpath->pathlist_, strob_str(swpath->buffer_));
	/* swpath->swpath_is_catalog_ = 0; */
	swpath_set_product_control_dir(swpath, "");
	swpath_set_fileset_control_dir(swpath, "");
	strob_strcpy(swpath->pathname_, "");
	strob_strcpy(swpath->basename_, "");

	if (!swpath->swpath_p_prepath_) {
		SWPATH_E_DEBUG(""); 
		swpath_resolve_prepath(swpath, strob_str(swpath->buffer_));
	}	
	
	if (!swpath->swpath_p_prepath_) {
		/*
		* Prepath is not fully formed from the leading 
		* pacakge directories,
		* return 0.
		*/
		swpath_set_is_catalog(swpath, -1);
		SWPATH_E_DEBUG(""); 
		SWPATH_E_DEBUG2("%s", swpath_dump_string_s(swpath, ""));
		SWPATH_E_DEBUG("EXIT swpath_parse_path"); 
		return check_error(swpath, 0);
	}
	
	if (strlen(swpath->swpath_p_prepath_) == 0) {
		SWPATH_E_DEBUG(""); 
		/* 2003-12-21 swpath_set_is_catalog(swpath, -1); */
		;
	} else if (
		!(s = strstr(strob_str(swpath->buffer_),
				 strob_str(swpath->prepath_))) || 
		(s != strob_str(swpath->buffer_)) ||
    		(*(strob_str(swpath->buffer_) + 
				strob_strlen(swpath->prepath_)) != '/')
		) 
	{
		/* 
		* error 
		*/
		SWPATH_E_DEBUG(""); 
		swpath_set_is_catalog(swpath, -1);
		swpath->errorM = 1;
		SWPATH_E_DEBUG2("%s", swpath_dump_string_s(swpath, ""));
		SWPATH_E_DEBUG("EXIT swpath_parse_path"); 
		return -1;
	}

	swpath__slashclean(swpath, strob_str(swpath->prepath_));
	n_prepath = swpath_num_of_components(swpath, 
						strob_str(swpath->prepath_));
	
	for (i = 0; i < SWPATH_PARSE_CA; i++) ca[i] = (char *)(NULL);


	/*  Remove legacy restriction 08Jun2014 */
	/*
	if (strlen(name) > 255) {
		SWPATH_E_DEBUG(""); 
		SWPATH_E_DEBUG2("%s", swpath_dump_string_s(swpath, ""));
		SWPATH_E_DEBUG("EXIT swpath_parse_path"); 
		return -1;
	}
	*/


	strob_strcpy(swpath->buffer_, name);
	swpath__slashclean(swpath, strob_str(swpath->buffer_));
	
	if ((s = strob_strstr(swpath->buffer_, 
			strob_str(swpath->prepath_))) != 
					strob_str(swpath->buffer_)) {
		SWPATH_E_DEBUG(""); 
		n_prepath = 0;
	}
	
	/* set s to point at first component past the prepath */	
	
	s = strob_str(swpath->buffer_) + strob_strlen(swpath->prepath_);
	if (*s == '/') s++;

	while ((p = strchr(s, (int) ('/'))) && n < SWPATH_PARSE_CA) {
		ca[n] = s;
		s = p + 1;
		*p = '\0';
		n++;
	}
	if (n >= (SWPATH_PARSE_CA -1)) exit(87);
	ca[n] = s;
	n++;
	relative_n = n;

	if (!relative_n) {
		SWPATH_E_DEBUG(""); 
		SWPATH_E_DEBUG2("%s", swpath_dump_string_s(swpath, ""));
		SWPATH_E_DEBUG("EXIT swpath_parse_path"); 
		if (swpath->errorM) return -1;
		return check_error(swpath, 0);
	} else if (relative_n == 1) {
		SWPATH_E_DEBUG("relative_n == 1"); 
		SWPATH_E_DEBUG2("%s", \
			swpath_parse_write_debug(dbuf, relative_n, ca));
		compy = 1;
		if (strcmp(ca[0], SW_A_catalog) != 0) {
			/*
			* Not in catalog.
			*/
			SWPATH_E_DEBUG(""); 
			set_v_dfiles(swpath, "");
			set_v_pfiles(swpath, "");
			set_is_catalog(swpath, 0, "A");
			if (swpath->max_rel_n_ >= SWPATH_DEGEN_N) {
				ret++;
				swpath_set_product_control_dir(swpath, ca[0]);
				SWPATH_E_DEBUG(""); 
			} else {
				compy = 0;
			}
		} else {
			SWPATH_E_DEBUG(""); 
			/*
			* In catalog
			*/
			set_max_rel_n(swpath, relative_n);
			set_is_catalog(swpath, 1, "B");
		}
	} else if (relative_n == 2) {
		compy = 2;
		SWPATH_E_DEBUG("relative_n == 2"); 
		SWPATH_E_DEBUG2("%s", \
				swpath_parse_write_debug(dbuf, relative_n, ca));
		if (strcmp(ca[0], SW_A_catalog) == 0) {
			/*
			* In catalog
			*/
			SWPATH_E_DEBUG("-<>-"); 
			SWPATH_E_DEBUG("In catalog"); 
			set_max_rel_n(swpath, relative_n);
			set_is_catalog(swpath, 1, "C");
			SWPATH_E_DEBUG("-<>-"); 
				/* If dfiles hasn't been set; and Not INDEX */
			if ((get_p_dfiles(swpath) && 
				strcmp(ca[1], get_p_dfiles(swpath)) == 0) || 
				(!swpath->swpath_p_dfiles_ && 
						strcmp(ca[1], "INDEX") != 0)
			) {
				SWPATH_E_DEBUG("C::-<>-"); 
				set_v_dfiles(swpath, ca[1]);
				set_p_dfiles(swpath, ca[1]);
			} else if (1 && 
				strlen(swpath_get_product_control_dir(swpath))
									== 0 && 
				!swpath->swpath_p_pfiles_ &&
				strcmp(ca[1], swpath->pfiles_default) == 0	
				) {
				/*
				* In pfiles.
				*/
				SWPATH_E_DEBUG("C::-<>-.-<>-.-<>-"); 
				swpath_set_is_minimal_layout(swpath, 1);
				set_v_pfiles(swpath, ca[1]);
				set_p_pfiles(swpath, ca[1]);
				set_v_dfiles(swpath, "");
			} else if (strcmp(ca[1], swpath->pfiles_default) == 0) {
			SWPATH_E_DEBUG("C::-<>-.-<>-.-<>-.-<>-.-<>-.-<>-."); 
				set_v_pfiles(swpath, swpath->pfiles_default);
				set_p_pfiles(swpath, swpath->pfiles_default);
				swpath_set_is_minimal_layout(swpath, 1);
				set_v_dfiles(swpath, "");
			} else if (swpath->swpath_p_dfiles_ && 
				strcmp(ca[1], "INDEX") != 0 && 
				strcmp(ca[1], swpath->swpath_p_dfiles_) != 0) {
			SWPATH_E_DEBUG("C:-<>-.-<>-.-<>-.-<>-.-<>-.-<>-.-<>-."); 
				ret++;
				set_v_dfiles(swpath, "");
				set_v_pfiles(swpath, "");
				set_p_fileset_dir(swpath, NULL);
				set_p_pfiles(swpath, NULL); /* does nothing */
				if (swpath_get_is_minimal_layout(swpath)) {
					compy = 1;
				} else {
					swpath_set_product_control_dir(
								swpath, ca[1]);
				}
			} else if (!strcmp(ca[1], "INDEX")) {
	SWPATH_E_DEBUG("C::-<>-.-<>-.-<>-.-<>-.-<>-.-<>-.-<>-.-<>-.-<>-."); 
				compy = 1;
			}
		} else {
			SWPATH_E_DEBUG(""); 
			set_v_dfiles(swpath, "");
			set_v_pfiles(swpath, "");
			set_is_catalog(swpath, 0, "D");
			if (swpath->max_rel_n_ >= SWPATH_DEGEN_N) {
				SWPATH_E_DEBUG("relative_n other"); 
				SWPATH_E_DEBUG("Not in catalog");
				ret++;
				swpath_set_product_control_dir(swpath, ca[0]);
				ret++;
				swpath_set_fileset_control_dir(swpath, ca[1]);
				set_p_fileset_dir(swpath, ca[1]);
				compy = 2;
			} else {
				/*
				* Empty control directories.
				*/
				SWPATH_E_DEBUG(""); 
				compy = 0;
			}
		}
	} else if (relative_n == 3) {
		SWPATH_E_DEBUG2("%s", \
			 swpath_parse_write_debug(dbuf, relative_n, ca));
		SWPATH_E_DEBUG("-<>-"); 
		if (strcmp(ca[0], SW_A_catalog) == 0) {
			/*
			* In catalog
			*/
			SWPATH_E_DEBUG("-<>-"); 
			set_max_rel_n(swpath, relative_n);
			set_is_catalog(swpath, 1, "E");
			compy = n;
			SWPATH_E_DEBUG("In catalog");
			if ((swpath->swpath_p_dfiles_ && 
				strcmp(ca[1], swpath->swpath_p_dfiles_) != 0)) {
				/*
				* not in dfiles.
				*/
				SWPATH_E_DEBUG("-<>-"); 
				SWPATH_E_DEBUG("NOT In dfiles");
				compy = 2;
				ret++;
				if (strcmp(
					swpath_get_product_control_dir(swpath),
								ca[1])) {
					/* New product
					* This case occurs when the empty 
					* leading dir is missing from
					* the package. 
					*/
					SWPATH_E_DEBUG(""); 
					set_p_fileset_dir(swpath, NULL);
					set_p_pfiles(swpath, NULL);
				}
			
				pfiles_index = is_in_pfiles(swpath, ca);

				if (0 || (get_p_pfiles(swpath) == NULL && 
				     strcmp(ca[1], swpath->pfiles_default))) {
					SWPATH_E_DEBUG(""); 
					set_v_dfiles(swpath, "");
					swpath_set_product_control_dir(
								swpath, ca[1]);
				} else if (1 && 
					swpath->swpath_p_pfiles_ == NULL &&
					strcmp(ca[1],
						swpath->pfiles_default) == 0) {
					SWPATH_E_DEBUG(""); 
					set_v_dfiles(swpath, "");
					set_v_pfiles(swpath, ca[1]);
					set_p_pfiles(swpath, ca[1]);
				}  else {
					SWPATH_E_DEBUG(""); 
					set_v_dfiles(swpath, "");
					swpath_set_product_control_dir(swpath,
									ca[1]);
				}

				if (pfiles_index) {
					SWPATH_E_DEBUG("-<>-"); 
					SWPATH_E_DEBUG("In pfiles"); 
					compy = pfiles_index;	
					if (compy == 2) {
						/*
						* Unset the product directory.
						* Empty Control directories.
						*/
						SWPATH_E_DEBUG(""); 
						swpath_set_product_control_dir
								(swpath, "");
					}
					set_p_fileset_dir(swpath, NULL);
					if (strlen(
						strob_str(swpath->pfiles_)
						)
					) {
						SWPATH_E_DEBUG(""); 
						set_v_pfiles(swpath, 
						  strob_str(swpath->pfiles_));
					} else {
						SWPATH_E_DEBUG(""); 
						set_p_pfiles(swpath, ca[2]);
						set_v_pfiles(swpath, ca[2]);
					}
					set_v_dfiles(swpath, "");
					set_p_pfiles(swpath,
						strob_str(swpath->pfiles_));
				} else if (strcmp(ca[2], 
						strob_str(swpath->pfiles_))) {
			/* // Not in pfiles. */
					SWPATH_E_DEBUG("-<>-"); 
					SWPATH_E_DEBUG("NOT In pfiles"); 
					ret++;
					set_v_pfiles(swpath, "");
					swpath_set_fileset_control_dir(swpath,
									ca[2]);
					set_p_fileset_dir(swpath, 
								ca[2]);
					compy = 3;
				} else {
					SWPATH_E_DEBUG(""); 
					fprintf(stderr, 
						"Internal error: Tripped a"
						" bug in swpath.c,"
						" assumes pfiles = pfiles\n");
				}
			} else {
				/*
				* In dfiles 
				*/
				SWPATH_E_DEBUG("-<>-"); 
				SWPATH_E_DEBUG("In dfiles");
				set_v_dfiles(swpath, ca[1]);
				set_p_dfiles(swpath, ca[1]);
				compy = 2;
			}
		} else {	
			/*
			* NOT in catalog section.
			*/
			SWPATH_E_DEBUG(""); 
			set_is_catalog(swpath, 0, "F");
			set_v_pfiles(swpath, "");
			set_v_dfiles(swpath, "");
			if (swpath->max_rel_n_ >= SWPATH_DEGEN_N) {
				SWPATH_E_DEBUG("-<>-"); 
				SWPATH_E_DEBUG("Not in catalog");
				ret++;
				swpath_set_product_control_dir(swpath, ca[0]);
				ret++;
				swpath_set_fileset_control_dir(swpath, ca[1]);
				set_p_fileset_dir(swpath, ca[1]);
				compy = 2;
			} else {
				/*
				* Empty control directories.
				*/
				SWPATH_E_DEBUG(""); 
				compy = 0;
			}
		}
	} else if (relative_n > 3) {
		SWPATH_E_DEBUG(""); 
		SWPATH_E_DEBUG2("%s", \
			swpath_parse_write_debug(dbuf, relative_n, ca));
		if (strcmp(ca[0], SW_A_catalog) == 0) {
			/*
			* In catalog section.
			*/
			SWPATH_E_DEBUG("-<>-"); 
			SWPATH_E_DEBUG("in catalog");
			set_max_rel_n(swpath, relative_n);
			set_is_catalog(swpath, 1, "G");
			compy = n;
			if (strcmp(ca[1], swpath->dfiles_default)) { 
					/* // Not in dfiles. */
				SWPATH_E_DEBUG(""); 
				SWPATH_E_DEBUG("not in dfiles");
				compy = 2;
				ret++;
				swpath_set_product_control_dir(swpath, ca[1]);
				if ((strcmp(ca[2], swpath->pfiles_default)) &&
					(strcmp(ca[2], 
						strob_str(swpath->pfiles_)))
				) {
					SWPATH_E_DEBUG("-<>-"); 
					SWPATH_E_DEBUG("not in pfiles");
					ret++;
					swpath_set_fileset_control_dir(
								swpath, 
								ca[2]);
					set_v_pfiles(swpath, "");
		 			set_v_dfiles(swpath, "");
					set_p_fileset_dir(swpath, ca[2]);
					compy = 3;
				} else if (
					(
					get_p_pfiles(swpath) && 
					strcmp(ca[2], get_p_pfiles(swpath)) == 0
					) ||
					(
					strcmp(ca[2], 
						swpath->pfiles_default) == 0 ||
					strcmp(ca[2],
						strob_str(swpath->pfiles_)) == 0
					)
				) {	
							/* // In pfiles. */
					SWPATH_E_DEBUG("-<>-"); 
					SWPATH_E_DEBUG("In pfiles"); 
					if (strlen(
						strob_str(swpath->pfiles_))) {
						SWPATH_E_DEBUG(""); 
						set_v_pfiles(swpath, 
						   strob_str(swpath->pfiles_));
					} else {
						SWPATH_E_DEBUG(""); 
						set_v_pfiles(swpath, 
						      swpath->pfiles_default);
					}	
					set_p_pfiles(swpath, 
						  strob_str(swpath->pfiles_));
		 			set_v_dfiles(swpath, "");
					compy = 3;
				}
			} else {							
				/*
				* In dfiles 
				*/
				SWPATH_E_DEBUG(""); 
				compy = 2;
			}
		} else {	
			/* 
			* NOT in catalog section. 
			*/
			set_is_catalog(swpath, 0, "H");
			if (swpath->max_rel_n_ >= SWPATH_DEGEN_N) {
				SWPATH_E_DEBUG("-<>-"); 
				SWPATH_E_DEBUG("Not in catalog"); 
				ret++;
				swpath_set_product_control_dir(swpath, ca[0]);
				ret++;
				swpath_set_fileset_control_dir(swpath, ca[1]);
				compy = 2;
			} else {
				/*
				* Empty control directories.
				*/
				SWPATH_E_DEBUG(""); 
				compy = 0;
			}
		}
	}
	
	if (swpath->control_path_nominal_depth_ == 0 && 
				swpath->swpath_is_catalog_ == 0) {
		/*
		* This supports the degenerate form.
		*/
		SWPATH_E_DEBUG("compy=0");
		compy = 0;
	}

	SWPATH_E_DEBUG2("FINAL compy=%d", compy);
	SWPATH_E_DEBUG2("%s", swpath_parse_write_debug(dbuf, relative_n, ca));
	/* 
	* reconstruct filename  
	*/
	for (i = compy; i < n - 1; i++)
		*(ca[i] + strlen(ca[i])) = '/';


	if (ca[compy]) {
		SWPATH_E_DEBUG3("set_pathname ca[%d] = %s", compy, ca[compy]); 
		swpath_set_pathname(swpath, ca[compy]);
	}
	if (ca[compy]) {
		SWPATH_E_DEBUG3("set_filename ca[%d] = %s", compy, ca[compy]); 
		swpath_set_filename(swpath, ca[compy]);
	}
	SWPATH_E_DEBUG2("%s", swpath_dump_string_s(swpath, ""));
	SWPATH_E_DEBUG("EXIT swpath_parse_path"); 
	return check_error(swpath, ret);
}

void
swpath_shallow_fill_export(SWPATH_EX * sx, SWPATH * sp)
{
	sx->is_minimal_layoutM = swpath_get_is_minimal_layout(sp);
	sx->is_catalog = swpath_get_is_catalog(sp);
	sx->ctl_depth = sp->control_path_nominal_depth_;
	sx->pkgpathname = swpath_get_pkgpathname(sp);
	sx->prepath = swpath_get_prepath(sp);
	sx->dfiles = swpath_get_dfiles(sp);
	sx->pfiles = swpath_get_pfiles(sp);
	sx->product_control_dir = swpath_get_product_control_dir(sp);
	sx->fileset_control_dir = swpath_get_fileset_control_dir(sp);
	sx->pathname = swpath_get_pathname(sp);
	sx->basename = swpath_get_basename(sp);
}

SWPATH_EX * 
swpath_shallow_create_export(void)
{
	return swpath_create_export(NULL);
}

SWPATH_EX * 
swpath_create_export(SWPATH * sp)
{
	char * s;
	SWPATH_EX * sx  = (SWPATH_EX*)malloc(sizeof(SWPATH_EX));
	if (!sx) return sx;
	
	if (!sp) {
		sx->is_minimal_layoutM = 0;
		sx->is_catalog = 0;		
		sx->ctl_depth = 0;
		sx->pkgpathname = strdup("");
		sx->prepath = strdup("");
		sx->dfiles =  strdup("");
		sx->pfiles =  strdup("");
		sx->product_control_dir = strdup("");
		sx->fileset_control_dir = strdup("");
		sx->pathname = strdup("");
		sx->basename = strdup("");
	} else {
		sx->is_minimal_layoutM = swpath_get_is_minimal_layout(sp);
		sx->is_catalog = swpath_get_is_catalog(sp);
		sx->ctl_depth = sp->control_path_nominal_depth_;
		sx->pkgpathname = ((s=swpath_get_pkgpathname(sp)) 
						? strdup(s) : (char *)NULL);
		sx->prepath = ((s=swpath_get_prepath(sp)) 
						? strdup(s) : (char *)NULL);
		sx->dfiles = ((s=swpath_get_dfiles(sp)) 
						? strdup(s) : (char *)NULL);
		sx->pfiles = ((s=swpath_get_pfiles(sp)) 
						? strdup(s) : (char *)NULL);
		sx->product_control_dir = 
				((s=swpath_get_product_control_dir(sp)) 
						? strdup(s) : (char *)NULL);
		sx->fileset_control_dir = 
				((s=swpath_get_fileset_control_dir(sp)) 
						? strdup(s) : (char *)NULL);
		sx->pathname = ((s=swpath_get_pathname(sp)) 
						? strdup(s) : (char *)NULL);
		sx->basename = ((s=swpath_get_basename(sp)) 
						? strdup(s) : (char *)NULL);
	}
	return sx;
}

void
swpath_delete_export(SWPATH_EX * sx)
{
	char * s;
	if ((s=sx->pkgpathname)) free(s);
	if ((s=sx->prepath)) free(s);
	if ((s=sx->dfiles)) free(s);
	if ((s=sx->pfiles)) free(s);
	if ((s=sx->product_control_dir)) free(s);
	if ((s=sx->fileset_control_dir)) free(s);
	if ((s=sx->pathname)) free(s);
	if ((s=sx->basename)) free(s);
	free(sx);
}

void
swpath_shallow_delete_export(SWPATH_EX * sx)
{
	free(sx);
}

char * 
swpath_ex_print(SWPATH_EX * swpath, STROB * buf, char * prefix)
{
	
	strob_sprintf(buf, 0, "%s%p (SWPATH_EX*)\n", prefix,  (void*)swpath);
	if (swpath) {
	strob_sprintf(buf, 1, "%s%p->pkgpathname     = [%s]\n", prefix, (void*)swpath, swpath->pkgpathname);
	strob_sprintf(buf, 1, "%s%p->is_minimal_layoutM = [%d]\n",  prefix, (void*)swpath, swpath->is_minimal_layoutM);
	strob_sprintf(buf, 1, "%s%p->is_catalog      = [%d]\n",  prefix, (void*)swpath, swpath->is_catalog);
	strob_sprintf(buf, 1, "%s%p->ctl_depth       = [%d]\n",  prefix, (void*)swpath, swpath->ctl_depth);
	strob_sprintf(buf, 1, "%s%p->prepath         = [%s]\n", prefix, (void*)swpath, swpath->prepath);
	strob_sprintf(buf, 1, "%s%p->dfiles          = [%s]\n", prefix, (void*)swpath, swpath->dfiles);
	strob_sprintf(buf, 1, "%s%p->pfiles          = [%s]\n", prefix, (void*)swpath, swpath->pfiles);
	strob_sprintf(buf, 1, "%s%p->product_control_dir = [%s]\n", prefix, (void*)swpath, swpath->product_control_dir);
	strob_sprintf(buf, 1, "%s%p->fileset_control_dir = [%s]\n", prefix, (void*)swpath, swpath->fileset_control_dir);
	strob_sprintf(buf, 1, "%s%p->pathname            = [%s]\n", prefix, (void*)swpath, swpath->pathname);
	strob_sprintf(buf, 1, "%s%p->basename            = [%s]\n", prefix, (void*)swpath, swpath->basename);
	}
	return strob_str(buf);
}
