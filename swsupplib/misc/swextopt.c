/* swextopt.c -- parse options files.

  Copyright (C) 2004 James H. Lowe, Jr.  <jhlowe@acm.org>
  All rights reserved.

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


#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "usgetopt.h"
#include "strob.h"
#include "uxfio.h"
#include "taru.h"
#include "swparse.h"
#include "swheader.h"
#include "swlib.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swheaderline.h"
#include "swssh.h"
#include "swutilname.h"
#include "atomicio.h"
#include "swextopt.h"

extern struct extendedOptions * optionsArray;
extern char * CHARTRUE;

static int swextopt_statusG = 0;

static char * utilnames[] = {
	"swask",       /* SWC_U_A  (1 << 0) */
	"swcopy",      /* SWC_U_C  (1 << 1) */
	"swinstall",   /* SWC_U_I  (1 << 2) */
	"swconfig",    /* SWC_U_G  (1 << 3) */
	"swlist",      /* SWC_U_L  (1 << 4) */
	"swmodify",    /* SWC_U_M  (1 << 5) */
	"swpackage",   /* SWC_U_P  (1 << 6) */
	"swremove",    /* SWC_U_R  (1 << 7) */
	"swverify",    /* SWC_U_V  (1 << 8) */
	"",            /* SWC_U_X  (1 << 9) */
	NULL };

static
int
i_is_value_false(char * s)
{
	if (
		s == NULL ||
		(
			strcasecmp(s, "f") == 0 ||
			strcasecmp(s, "false") == 0 ||
			strcasecmp(s, "no") == 0 ||
			strcasecmp(s, "n") == 0 ||
			0
		)
	) {
		return 1;
	}
	return 0;
}

static
int
i_is_value_true(char * s)
{
	if (
		s != NULL &&
		(
			strcasecmp(s, "t") == 0 ||
			strcasecmp(s, "true") == 0 ||
			strcasecmp(s, "yes") == 0 ||
			strcasecmp(s, "y") == 0 ||
			0
		)
	) {
		return 1;
	}
	return 0;
}

static
int
find_pow(int flag)
{
	int i = 1;
	if (flag <= 1) return 0;

	while (flag/2 > 1) {
		flag = flag >> 1;
		i++;
	}
	return i;
}

static
int 
i_getEnumFromName(char * optionname, struct extendedOptions * peop)
{
	struct extendedOptions * eop;
	eop = peop;
	while (eop->optionNameM && strcmp(eop->optionNameM, optionname)) {
		eop ++;
	}
	if (eop->optionNameM == NULL) return -1;
	return (((char*)eop) - (char*)peop) / sizeof(struct extendedOptions);
}

static
int
check_applicability(struct extendedOptions * opta, enum eOpts nopt)
{
	int bit;
	int index;
	char * cu;
	int ret;
	char ** names = utilnames;
	int ap_mask;	

	E_DEBUG("case");
	ap_mask = opta[nopt].app_flags;

	cu = swlib_utilname_get();
	
	while (*names && strcmp(cu, *names)) {
		names++;
	}
	if (!(*names)) return 1;
	index = (int)(names - utilnames);
	bit = 1 << index;
	ret = ! (ap_mask & bit);
	if (ret)
		fprintf(stderr, "%s: option not valid: %s\n", cu, opta[nopt].optionNameM);
	return ret;
}

static
void
combine1(STROB * result, char * directory, char * soc_spec)
{
	char * x;
	char * t;
	E_DEBUG3("directory=%s soc_spec=%s", directory, soc_spec);
	if (*directory == '@') directory++;
	if (*soc_spec == '@') directory++;
	strob_strcpy(result, "@"); /* FIXME: the utilities prepend a '@' */
	strob_strcat(result, soc_spec);
	E_DEBUG2("result=%s", strob_str(result));
	x = directory;
	if (*directory == ':') x++;
	if (*directory == '@') x++;  /* FIXME: the utilities prepend a '@' */

	if (
		soc_spec[strlen(soc_spec)-1] != ':' &&
		directory[0] != ':' &&
		1
	) {
		strob_strcat(result, ":");
		E_DEBUG2("result=%s", strob_str(result));
	}
	if (*directory == ':') directory ++;
	if ((t=strrchr(strob_str(result), ':')) && *(t+1) == '\0') {
		strob_strcat(result, directory);
		E_DEBUG2("result=%s", strob_str(result));
	} else {
		swlib_unix_dircat(result, directory);
		E_DEBUG2("result=%s", strob_str(result));
	}
}

static
void 
close_stdio(void) {
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

static
void 
i_set_opta(int optsetflag, struct extendedOptions * opta, 
			enum eOpts nopt, char * value)
{
	if (value && strpbrk(value, SWBIS_TAINTED_CHARS)) {
		fprintf(stderr, 
		"%s: error: shell meta-characters detected for %s option\n",
		swlib_utilname_get(), opta[nopt].optionNameM);
		exit(1);
	}

	/* check_applicability(opta, nopt); */

	opta[nopt].option_setM = (char)optsetflag;
	if (value) {
		opta[nopt].valueM = strdup(value);
	} else {
		opta[nopt].valueM = value;
	}
}

void 
debug_writeBooleanExtendedOptions(int ofd, struct extendedOptions * opta)
{
	struct extendedOptions * eop = opta;
	int op = 0;
	STROB * tmp = strob_open(10);
	while (eop->optionNameM) {
		if (eop->is_boolM) 
		swlib_writef(ofd, tmp, "%s=%d\n", eop->optionNameM,
			swextopt_is_value_true(get_opta(opta, op))
			);
		op++;
		eop++;
	}
	strob_close(tmp);
}

void 
swextopt_write_session_options(STROB * buf, struct extendedOptions * opta, int SWC_FLAG)
{
	char * value;
	struct extendedOptions * eop = opta;
	int op = 0;
	int bv;

	strob_strcpy(buf, "");
	while (eop->optionNameM) {
		if ((SWC_FLAG == 0 ) || (SWC_FLAG & (eop->app_flags))) {
			if (eop->is_boolM) {
				bv = swextopt_is_value_true(get_opta(opta, op));
				switch(bv) {
					case 1: 
						strob_sprintf(buf, 1, "%s=\"true\"\n", eop->optionNameM);
						break;
					case 0:
						strob_sprintf(buf, 1, "%s=\"false\"\n", eop->optionNameM);
						break;
				}
			} else {
				value=get_opta(opta, op);
				if (value) {
					/*
					* Check for evil shell meta chars
					*/
					if (strpbrk(value, SWBIS_TAINTED_CHARS)) {
						fprintf(stderr, 
						"%s: error: shell meta-characters detected for %s option\n",
							swlib_utilname_get(), eop->optionNameM);
						value="";
					}
				} else {
					value="";	
				}
				strob_sprintf(buf, 1, "%s=\"%s\"\n", eop->optionNameM, value);
			}
		}
		op++;
		eop++;
	}
}

int
swextopt_is_value_false(char * s)
{
	if (i_is_value_false(s)) {
		return 1;
	} else if (i_is_value_true(s)) {
		return 0;
	} else {
		fprintf(stderr, 
		"%s: warning: extended option boolean value '%s' is improperly formatted, assuming false\n", swlib_utilname_get(), s);
	}
	return 1;
}


int
swextopt_is_value_true(char * s)
{
	if (i_is_value_true(s)) {
		return 1;
	} else if (i_is_value_false(s)) {
		return 0;
	} else {
		fprintf(stderr, 
		"%s: warning: extended option boolean value '%s' is improperly formatted, assuming false\n", swlib_utilname_get(), s);
	}
	return 0;
}

int
swextopt_is_option_false(enum eOpts nopt, struct extendedOptions * options)
{
	char * val;
	val = get_opta(options, nopt);
	return swextopt_is_value_false(val);
}

int
swextopt_is_option_true(enum eOpts nopt, struct extendedOptions * options)
{
	char * val;
	val = get_opta(options, nopt);
	return swextopt_is_value_true(val);
}

int
swextopt_is_option_set(enum eOpts nopt, struct extendedOptions * options)
{
	return (int)(options[nopt].option_setM);
}

int 
parseDefaultsFile(char * utility_name, char * defaults_filename, 
		struct extendedOptions * options, int doPreserveOptions)
{
	int ret = 0;
	int is_option_set;
	int is_util_set;
	char * value;
	char * option;
	char * optionutil;
	char nullb = '\0';
	int fd;
	int uxfio_fd;
	char * buf;
	STROB * tmp;
	char * line;
	char * oldvalue;
	int is_util_option;
	int is_global_option;
	int is_set;

	uxfio_fd = uxfio_open("", O_RDONLY, 0);
	if (uxfio_fd < 0) {
		return 1;
	}
	uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	uxfio_fcntl(uxfio_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

	if (strcmp(defaults_filename, SWBIS_STDIO_FNAME) != 0)
		fd = open(defaults_filename, O_RDONLY, 0);
	else
		fd = STDIN_FILENO;

	if (fd < 0) {
		fprintf(stderr, "%s : %s\n", 
				defaults_filename, strerror(errno));
		uxfio_close(uxfio_fd);
		return 1;
	}

	if (sw_yyparse(fd, uxfio_fd, "OPTION", 0, SWPARSE_FORM_MKUP_LEN)) {
		uxfio_close(uxfio_fd);
		if (fd != STDIN_FILENO) close(fd);
		return 3;
	}
	
	if (fd != STDIN_FILENO) close(fd);

	uxfio_write(uxfio_fd, (void*)(&nullb), 1);

	if (uxfio_lseek(uxfio_fd, 0, SEEK_SET)) {
		uxfio_close(uxfio_fd);
		return 4;
	}

	if (uxfio_get_dynamic_buffer(uxfio_fd, &buf, NULL, NULL) < 0)
		return 5;

	/*
	// the parsed output is now in buf in the form
	//       A01<space>keyword<space>value<NEWLINE>
	*/

	/* step through the lines using strob_strtok
	*/

	tmp = strob_open(10);
	
	line = strob_strtok(tmp, buf, "\r\n");
	while (line) {
		/*
		// A line is in the form of swbiparse -n --options
		//
		// write(1, line, strlen(line));
		// write(1, "\n", 1);
		*/
		value = NULL;
		option = NULL;
		optionutil = NULL;
		is_util_option = 0;
		is_global_option = 0;
		
		option = swheaderline_get_keyword(line);
		value = swheaderline_get_value(line, NULL);
		
		if ((strstr(option, utility_name) == option) && 
				option[strlen(utility_name)] == '.') {
			is_util_option = 1;
			option += (strlen(utility_name) + 1);
		} else if (strchr(option, '.') == NULL) {
			is_global_option = 1;
		} 

		if (is_util_option || is_global_option ) {
			/*
			// put pointer in array.
			*/
			if (
				(oldvalue = getExtendedOption(option, 
							options, 
							&is_set, 
						&is_option_set, &is_util_set))
			) {
				if ( (
					is_util_set == 0 && 
					is_global_option && 
					is_option_set == 0
				      ) ||
				      (
					(is_util_option || is_set == 0) && 
						is_option_set == 0
				      )
				) {
					if (is_set && oldvalue) 
						free(oldvalue);
					if (value) 
						value = strdup(value);
				 	setExtendedOption(option, value, 
						options,
						doPreserveOptions,
						is_util_option);
				}
			} else {
			 	/*
				// error.
				*/
				fprintf(stderr, 
					"%s: option %s not recognized\n",
						utility_name, option);
				ret = 1;
				break;
			}
		}
		line = strob_strtok(tmp, NULL, "\n");
	}
	uxfio_close(uxfio_fd);
	return ret;
}

int 
initExtendedOption(void){
	return 0;
}

char * 
getLongOptionNameFromValue(struct option * arr, int val)
{
	struct option * p = arr;
	while (p->val && p->name) {
		if (p->val == val) 
			return (char*)(p->name);
		p++;
	}
	return (char*)NULL;
}

int 
getEnumFromName(char * fp_optionname, struct extendedOptions * peop)
{
	int ret;
	char * optionname;
	STROB * tmp;
	
	tmp = strob_open(10);
	strob_strcpy(tmp, fp_optionname);
	optionname = strob_str(tmp);
	
	swlib_tr(optionname, (int)'_', (int)'-');
	ret = i_getEnumFromName(optionname, peop);
	if (ret >= 0) {
		return ret;
	}	
	swlib_tr(optionname, (int)'-', (int)'_');
	ret = i_getEnumFromName(optionname, peop);
	strob_close(tmp);
	return ret;
}

int 
setExtendedOption(char * optionname, char * value, 
	struct extendedOptions * peop, int optSet, int is_util_option)
{
	struct extendedOptions * eop;

	if (peop) {
		eop = peop;
	} else {
		eop = optionsArray;
	}
	while (eop->optionNameM && strcmp(eop->optionNameM, optionname)) {
		eop ++;
	}
	if (eop->optionNameM) {
		if (eop->util_setM && is_util_option == 0) {
			return 0;
		}
		if (optSet && eop->option_setM) {
			return 0;
		}
		if (is_util_option) eop->util_setM = (char)1;
		eop->valueM = value;
		return 0;
	} else {
		return -1;
	}
}

int 
swextopt_writeExtendedOptions(int ofd, struct extendedOptions * eop, int SWC_FLAG)
{
	int ret;
	STROB * tmp = strob_open(10);
	swextopt_writeExtendedOptions_strob(tmp, eop, SWC_FLAG, 0);
	ret = atomicio((ssize_t (*)(int, void *, size_t))write, ofd, strob_str(tmp), strob_strlen(tmp));
	if (ret != (int)strob_strlen(tmp)) return -1;
	strob_close(tmp);
	return ret; 
}

void 
swextopt_writeExtendedOptions_strob(STROB * tmp, struct extendedOptions * eop, int SWC_FLAG, int do_shell_protect)
{
	char * s;
	while (eop->optionNameM) {
		if ((SWC_FLAG == 0 ) || (SWC_FLAG & (eop->app_flags))) {
			s = getExtendedOption(eop->optionNameM, eop, NULL, NULL, NULL);
			if (!s) s = "";
			if (do_shell_protect == 0) {
				strob_sprintf(tmp, STROB_DO_APPEND, "%s=%s\n", eop->optionNameM, s);
			} else {
				swlib_is_sh_tainted_string_fatal(s);
				strob_sprintf(tmp, STROB_DO_APPEND, "%s=\"%s\"\n", eop->optionNameM, s);
			}
		}
		eop ++;
	}
}

char * 
getExtendedOption(char * optionname, struct extendedOptions * peop, 
		int * pis_set, int * isOptSet, int *isUtilSet){
	struct extendedOptions * eop;

	if (peop) {
		eop = peop;
	} else {
		eop = optionsArray;
	}	
	
	while (eop->optionNameM && strcmp(eop->optionNameM, optionname)) 
		eop ++;
	if (eop->optionNameM) {
		if (eop->valueM)  {
			if (isOptSet) *isOptSet = (int)(eop->option_setM);
			if (isUtilSet) *isUtilSet = (int)(eop->util_setM);
			if (pis_set) *pis_set = 1;
			return eop->valueM;
		} else {
			if (isOptSet) *isOptSet = (int)(eop->option_setM);
			if (isUtilSet) *isUtilSet = (int)(eop->util_setM);
			if (pis_set) *pis_set = 0;
			return eop->defaultValueM;
		}
	}
	if (isOptSet) *isOptSet = 0;
	if (isUtilSet) *isUtilSet = 0;
	return NULL; /* error, option not found. */
}

void
set_opta_boolean(struct extendedOptions * opta, enum eOpts nopt, char * value)
{
	if (opta[nopt].is_boolM) {
		if (
			value != NULL &&
			strcasecmp(value, "t") &&
			strcasecmp(value, "f") &&
			strcasecmp(value, SW_E_TRUE) &&
			strcasecmp(value, SW_E_FALSE) &&
			strcasecmp(value, "no") &&
			strcasecmp(value, "yes") &&
			strcasecmp(value, "n") &&
			strcasecmp(value, "y") &&
			strcmp(value, "0") &&
			strcmp(value, "1")
		) {
			swextopt_statusG = 1;
		}
	}
	if (value) {
		if (*value == '0') value = SW_E_FALSE;
		if (*value == '1') value = SW_E_TRUE;
	}
	i_set_opta(1, opta, nopt, value);
}

void
set_opta(struct extendedOptions * opta, enum eOpts nopt, char * value)
{
	if (opta[nopt].is_boolM)
		set_opta_boolean(opta, nopt, value);
	else
		i_set_opta(1, opta, nopt, value);
}

void
set_opta_initial(struct extendedOptions * opta, enum eOpts nopt, char * value)
{
	i_set_opta(0, opta, nopt, value);
}

char *
swbisoption_get_opta(struct extendedOptions * opta, enum eOpts nopt)
{
	char * value;
	value = opta[nopt].valueM;
	if (value == NULL) {
		fprintf(stderr, "internal warning: swbis option %d [%s] is null\n", (int)nopt, opta[nopt].optionNameM);
	}
	if (!value) value = opta[nopt].defaultValueM;
	return value;
}

char *
get_opta_isc(struct extendedOptions * opta, enum eOpts nopt)
{
	char * value;
	char * s;
	char * host;
	char * file;
	E_DEBUG("");
	if (nopt != SW_E_installed_software_catalog) {
		return get_opta(opta, nopt); 
	}
	value = opta[nopt].valueM;
	if (value == NULL) {
		fprintf(stderr, "%s: warning: sw option %d [%s] is null, using default [%s]\n",
			swlib_utilname_get(), (int)nopt, opta[nopt].optionNameM, opta[nopt].defaultValueM);
	}
	if (!value) value = opta[nopt].defaultValueM;

	/* now put here support for an absolute path specified by URL syntax
	    such as file://localhost/path  or
                    file:///path

	   if that value is not in URL file syntax even if it is an absolute path
	   the leading slash will be stripped.  */

	s = strstr(value, FILE_URL);
	if (s == NULL) {
		/* Here is the normal path when not using a file URL.
		   In this case, all paths whether absolute or not are treated
		   as a relative path [which becomes relative to the target
		   path] */
		E_DEBUG("processing ordinary path");
		swlib_squash_all_leading_slash(value);
		swlib_squash_all_dot_slash(value);
		swlib_squash_leading_dot_slash(value);
		return value;
	} else {
		/* Here is support for an absolute path specification
		   for the installed software catalog location using the
		   file URL syntax:
			file://host/path
			file:///path  */

		E_DEBUG("processing file URL");
		if (s != value) {
			/* maybe a locahost pathname that has "file://" in it.
			   issue a warning and return it */
			fprintf(stderr, "%s: warning malformed file URL: %s\n",
				swlib_utilname_get(), value);
			return value;
		}

		/* If we are at here then the value must be one of two things
			file:///path
				or
			file://host/path */
	
		s += strlen(FILE_URL);

		if (*s == '/') {
			/* Is the form file:///path */
			host = NULL;
			file = s;
		} else {
			/* Is the form file://host/path */
			host = s;
			file = strchr(s, '/');
			if (file == NULL) {
				/* malformed URL,
					file://name
				i.e. where name is either a host or single name in the '/' root directory */
				fprintf(stderr, "%s: malformed file URL, assuming host part is file part\n",
					swlib_utilname_get());
				file = host;
			} else {
				if (strstr(host, "localhost/") != host) {
					/* error, remote catalogs not supported at this time */
					fprintf(stderr, "%s: error: remote catalog URL not supported: %s\n", swlib_utilname_get(), value);
					fprintf(stderr, "%s: error: Possibly you intend URL: file:///absolute/path\n", swlib_utilname_get());
					fprintf(stderr, "%s: error: exiting with error\n", swlib_utilname_get());
					exit(1);
				}
			}
		}
	}
	return file;
}

char *
get_opta(struct extendedOptions * opta, enum eOpts nopt)
{
	char * value;
	value = opta[nopt].valueM;
	if (value == NULL) {
		fprintf(stderr, "%s: warning: sw option %d [%s] is null, using default [%s]\n",
			swlib_utilname_get(), (int)nopt, opta[nopt].optionNameM, opta[nopt].defaultValueM);
	}
	if (!value) value = opta[nopt].defaultValueM;
	return value;
}

int
parse_options_file(struct extendedOptions * opta, char * filename, 
					char * util_name)
{
	int ret;

	if (access(filename, R_OK) == 0 || strcmp(filename, SWBIS_STDIO_FNAME) == 0) {
		ret = parseDefaultsFile(util_name, 
			filename, opta, 1 /*doPreserveOptions */);
	} else {
		ret = 0;
	}
	return ret;
}

char * 
initialize_options_files_list(char * usethis)
{
	char * ret;
	if (usethis) {
		ret = strdup(usethis);
	} else {
		STROB * tmp = strob_open(100);
		strob_strcpy(tmp, SYSTEM_DEFAULTS_FILE);
		strob_strcat(tmp, " ");
		strob_strcat(tmp, SYSTEM_SWBISDEFAULTS_FILE);
		if (getenv("HOME")) {
			strob_strcat(tmp, " ");
			strob_strcat(tmp, getenv("HOME"));
			strob_strcat(tmp, "/.swbis/swdefaults");
			strob_strcat(tmp, " ");
			strob_strcat(tmp, getenv("HOME"));
			strob_strcat(tmp, "/.swbis/swbisdefaults");
		}
		ret = strdup(strob_str(tmp));
		strob_close(tmp);
	}
	return ret;
}

int
swextopt_parse_options_files(struct extendedOptions * opta, char * option_files, 
				char * util_name, int reqd, int show_only)
{
	int do_check_access = 0;
	char * file;
	int skip = 0;
	int ret = 0;
	STROB * tmp = strob_open(100);
	STROB * ktmp = strob_open(100);

	if (option_files && strlen(option_files) == 0) return 0;
	if (!option_files) return 0;

	if (show_only) {
		do_check_access = 1;
	}

	do_check_access = 1;
	strob_strcpy(tmp, option_files);

	file = strob_strtok(ktmp, strob_str(tmp), " ,\n\r");
	while (file) {
		if (do_check_access && strcmp(file, SWBIS_STDIO_FNAME)) {
			if (access(file, R_OK) != 0) {
				E_DEBUG("case");
				if (reqd) {
					fprintf(stderr, "%s : %s\n", 
						file, strerror(errno));
					if (show_only == 0) {
						close_stdio();
						exit(1);
					}
				} else {
					skip = 1;
				}
			} else {
				skip = 0;
				E_DEBUG("case");
				if (show_only) {
					fprintf(stdout, "%s\n", file);
					skip = 1;
				}
			}
		}

		if (show_only) {
			E_DEBUG("case");
			skip = 1;
		}

		if (!skip) {
			E_DEBUG("case");
			ret = parse_options_file(opta, file, util_name);
			if (ret) {
				E_DEBUG("case");
				fprintf(stderr, 
				"error processing option file : %s\n", file);
				return 1;
			}
		}
		E_DEBUG("case");
		skip = 0;
		file = strob_strtok(ktmp, NULL, " ");
	}
	
	strob_close(tmp);
	strob_close(ktmp);
	return ret;
}


static
int
has_file_part(char * soc)
{
	if (!soc || strlen(soc) == 0) return 0;
	if (strchr(soc, ':')) {
		/* is it the last one */
		if (*(soc+(strlen(soc)-1)) == ':') {
			/* yes, the last one indeed 
			   i.e. form    host:host: */

			/* no file part */
			return 0;
		} else {
			/* has file part */
			return 1;
		}
	}
	return 0;
}


static
int
is_host(char * soc)
{

	if (!soc || !strlen(soc)) return 0;
	if (*soc == '@') soc++;
	if (
		(strcmp(soc, ".") && strcmp(soc, "./")) &&
		(
		(strchr(soc, ':') && *soc != ':') ||
		(strchr(soc, ':') == NULL && *soc != '/') ||
		soc[strlen(soc) -1] == ':' ||
		0
		)
	) {
		E_DEBUG2("is host: %s", soc);
		return 1;
	}
	E_DEBUG2("is not host: %s", soc);
	return 0;
}

static
int
is_fq(char * soc_spec)
{
	char * trailing_colon;

	if (*soc_spec == '/') return 1;
	trailing_colon = strrchr(soc_spec, ':');
	if (trailing_colon && *(trailing_colon+1) == '/') return 1;
	if (
		trailing_colon && 
		*(trailing_colon+1) == '.' &&
		*(trailing_colon+2) == '\0'
	) return 1;
	return 0;
}

/**
 *  swextopt_combine_directory  - merge target specs
 *
 *  merge the exteneded option directory with the source or target
 *  given on the the command line input to form a complete source or
 *  target.  If the command line arg has a directory component then
 *  do nothing.
 */

int
swextopt_combine_directory(STROB * result, char * soc_spec, char * directory)
{
	int retval;
	E_DEBUG("ENTERING");

	E_DEBUG3("soc_spec=%s directory=%s", soc_spec, directory);
	if (soc_spec) {
		strob_strcpy(result, soc_spec);
	}

	if (
		soc_spec == NULL ||
		directory == NULL ||
		strlen(directory) == 0 ||
		strcmp(directory, "-") == 0 ||
		strcmp(directory, ":") == 0 ||  /* not used */
		(*soc_spec == '.' && *directory == '.') ||
		(*soc_spec == '-') ||
		is_fq(soc_spec) ||
		0
	) {
		/*
		 * don't merge these special values
		 */
		E_DEBUG("returning doing nothing");
		E_DEBUG("LEAVING retval=0");
		return 0;
	} 

	E_DEBUG3("soc_spec=%s directory=%s", soc_spec, directory);
	
	/*
	 * Now determine if soc_spec has a directory component
	 */	

	retval = 0;
	if (
		(is_host(directory) && is_host(soc_spec)) ||
		(is_host(directory) == 0 && is_host(soc_spec) == 0) ||
		0
	) {
		;
		/* AMBIG error */
		E_DEBUG3("directory: is_host(%s)=%d", directory, is_host(directory));
		E_DEBUG3("soc_spec:  is_host(%s)=%d", soc_spec, is_host(soc_spec));
		E_DEBUG("AMBIG retval=-1");
		retval = -1;
	}
	else if (
		(strlen(soc_spec) > 1 && *soc_spec == ':') ||
		0
	) {
		/* 
		 * Implementation extension 
		 * soc_spec is ':PATH'
		 */
		/*
		 * soc_spec is a file base name,
		 * directory must be a host:path or host or path
		 */
		E_DEBUG("case");
		if (strchr(directory, ':')) {
			/*
			 * directory = "HOST:PATH"
			 * soc_spec = "BASENAME"
			 */
			strob_strcpy(result, directory);
			if (directory[strlen(directory)-1] != '/') {
				strob_strcat(result, "/");
			}
			strob_strcat(result, soc_spec+1);
		} else {
			/*
			 * directory = "HOST"
			 * soc_spec = ":PATHNAME"
			 */
			E_DEBUG("case");
			strob_strcpy(result, directory);
			strob_strcat(result, soc_spec);
		}
		if (strchr(soc_spec, '/')) {
			/* 
			 * Sanity check
			 */
			E_DEBUG("case");
			E_DEBUG("LEAVING retval=-1");
			return -1;
		}
	}
	else if (
		strlen(soc_spec) >= 1 &&
		*soc_spec == '/' &&
		strchr(directory, ':') == NULL &&
		*directory != '/' &&
		1
	) {
		/*
		 * Implementation extension
		 * assume directory is a host
		 */
		E_DEBUG("case");
		strob_strcpy(result, "@"); /* FIXME: the utilities prepend a '@' */
		strob_strcpy(result, directory);
		strob_strcat(result, ":");
		strob_strcat(result, soc_spec);
	}
	else if (
		*soc_spec != ':' &&
		*soc_spec != '/' &&
		strchr(soc_spec, ':') == NULL &&
		*directory == '/' &&
		strchr(directory, ':') == NULL &&
		strlen(directory) > 0 &&
		1
	) {
		/*
		 * Usage per the POSIX spec.
		 * soc_spec is a host, directory is a directory
		 */
		E_DEBUG("case");
		strob_strcpy(result, soc_spec);
		if (strlen(soc_spec) > 0) strob_strcat(result, ":");
		strob_strcat(result, directory);
	}
	else if (
		!is_host(directory) &&
		is_host(soc_spec) &&
		has_file_part(soc_spec) == 0 &&
		1
	) {
		/*
		 * Implementation extension usage
		 */
		E_DEBUG("case");
		combine1(result, directory, soc_spec);
	} else if (
		is_host(directory) &&
		!is_host(soc_spec) &&
		1
	) {
		/*
		 * Implementation extension usage
		 */
		E_DEBUG("case");
		combine1(result, soc_spec, directory);
	} else if (
		is_host(soc_spec) &&
		has_file_part(soc_spec) &&
		1
	) {
		; /* do nothing */
	} else {
		E_DEBUG("null case");
		retval = -1;
	}

	E_DEBUG2("result=%s", strob_str(result));
	E_DEBUG2("returning with value %d", retval);
	E_DEBUG2("LEAVING retval=%d", retval);
	return retval;
}

int
swextopt_get_status(void)
{
	E_DEBUG("");
	return swextopt_statusG;
}

