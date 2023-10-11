/*
 * == This file has been MODIFIED by jhlowe@acm.org essentially beyond
 *    recognition with the original (2008-03-13).
 *
 * Copyright (c) 2001 Thorsten Kukuk.
 *
 * Copyright (c) 1992 Keith Muller.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * 
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <time.h>
#include <utmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <tar.h>
#ifdef HAVE_SYSMACROS_H
#include <sys/sysmacros.h>
#endif
#include "cpiohdr.h"
#include "swlib.h"
#include "ls_list.h"
#include "vis.h"


/*
 * a collection of general purpose subroutines used by pax
 */

/*
 * constants used by ls_list() when printing out archive members
 */
#define MODELEN 20
#define DATELEN 64
#define DAYSPERNYEAR    365
#define SECSPERDAY      (60 * 60 * 24)
#define SIXMONTHS	((DAYSPERNYEAR / 2) * SECSPERDAY)
#define CURFRMT		"%b %e %H:%M"
#define OLDFRMT		"%b %e  %Y"
#define NAME_WIDTH	8

#define PAX_DIR        DIRTYPE
#define PAX_CHR        CHRTYPE
#define PAX_BLK        BLKTYPE     
#define PAX_REG        REGTYPE    
#define PAX_SLK        SYMTYPE   
#define PAX_SCK        REGTYPE  /* Not used */
#define PAX_FIF        FIFOTYPE  
#define PAX_HLK        LNKTYPE 
#define PAX_HRG        LNKTYPE 
#define PAX_CTG        REGTYPE   /* Not used */

#define MAJOR(x)        major(x)
#define MINOR(x)        minor(x)
#define TODEV(x, y)     makedev((x), (y))

#define UGSWIDTH 18
static int ugswidth = UGSWIDTH;

static int encoding_flag;

void
ls_list_set_encoding_flag(int flag)
{
	encoding_flag = flag;
}

int
ls_list_get_encoding_flag(void)
{
	if (encoding_flag == 0) {
		/* default encoding if not set */
		return VIS_NONE;
	}
	return encoding_flag;
}

int
ls_list_set_encoding_by_lang(void)
{
	int flag;
	char * e;

	e = getenv("LANG");
	if (e && strcmp(e, "C") == 0) {
		flag = VIS_OCTAL;
	} else {
		flag = VIS_NONE;
	}
	ls_list_set_encoding_flag(flag);
	return flag;
}

static
char *
prepend_dotslash(char *str, STROB * fp)
{
    if (strcmp(str, ".") == 0) {
	/* . alone */
	E_DEBUG();
       return "./";
    } else if (strncmp(str, "./", 2) == 0) {
	E_DEBUG();
	/* leading ./  do nothing */
       return str;
    } else if (strncmp(str, "/", 1) == 0) {
	/* / alone or not, prepend . */
	E_DEBUG();
       strob_strcat(fp, ".");
    } else {
	E_DEBUG();
       strob_strcat(fp, "./");
    }
    return str;
}

static
char *
tartime(time_t t)
{
  static char buffer[100];
  char *p;

  /* Use ISO 8610 format.  See:
     http://www.cl.cam.ac.uk/~mgk25/iso-time.html  */
  struct tm *tm = localtime (&t);
  if (tm)
    {
      sprintf (buffer, "%04d-%02d-%02d %02d:%02d:%02d",
               tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
               tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
  else
   {
      sprintf (buffer, "%lu", (unsigned long)t);
   }
  return buffer;
}

static
char *
strip_slash(char * pname, int vflag)
{
	char * name;
	name = pname;
	if (vflag & LS_LIST_VERBOSE_STRIPSLASH) {
		while( name && *name == '.' && *(name+1) == '/' /*&& *(name+2) != '\0'*/) name++;
		while( name && *name == '/' && *(name+1) != '\0' ) name++;
	}
	return name;
}

static
void
print_date(STROB * fp, int vflag, int type, time_t mtime)
{
	if (vflag & LS_LIST_VERBOSE_WITH_ALL_DATES) {
		strob_sprintf(fp, 1, "%s", tartime(mtime));
	} else if (vflag & LS_LIST_VERBOSE_WITH_REG_DATES) {
		if (type == REGTYPE) {
			strob_sprintf(fp, 1, "%s", tartime(mtime));
		} else {
			/* no print */
			/* strob_sprintf(fp, 1, " "); */
		}
	} else {
		;
		/* don't print the date */
	}
}

static
void
safe_print (char *str, FILE * fp)
{
  char visbuf[5];
  char *cp;

  /*
   * if printing to a tty, use vis(3) to print special characters.
   */
  if (isatty (fileno (fp)))
    {
      for (cp = str; *cp; cp++)
	{
	  (void) vis (visbuf, cp[0], ls_list_get_encoding_flag(), cp[1]);
	  (void) fputs (visbuf, fp);
	}
    }
  else
    {
      (void) fputs (str, fp);
    }
}

void
ls_list_safe_print_to_strob(char *fp_str, STROB * fp, int do_prepend)
{
  char visbuf[5];
  char *cp;
  char * str;

/*
fprintf(stderr, "JLJLx Before\n");
fprintf(stderr, "JLJLx Before\n");
fprintf(stderr, "JLJLx Before string <%s>\n", fp_str);
*/
  if (do_prepend) {
	str = prepend_dotslash(fp_str, fp);
  } else {
	str = fp_str;
  }

  if (1 || /* isatty (fileno (fp))*/ 0 )
    {
      for (cp = str; *cp; cp++)
	{
	  /* visbuf[0] = *cp; visbuf[1] = '\0'; */

	  (void) vis (visbuf, cp[0], ls_list_get_encoding_flag(), cp[1]);
	  strob_strcat(fp, visbuf);
	}
    }
  else
    {
      strob_strcat(fp, str);
    }
/*
fprintf(stderr, "JLJLx Final string <%s>\n", strob_str(fp));
fprintf(stderr, "JLJLx Final string\n");
fprintf(stderr, "JLJLx Final string\n");
*/

}


static
void
print_size_rdev(STROB * fp, struct new_cpio_header * file_hdr, int type)
{ 
	intmax_t sizeme;
	if ((type == PAX_CHR) || (type == PAX_BLK)) {
#ifdef LONG_OFF_T
		strob_sprintf(fp, 1, "%4u,%4u", (unsigned)(file_hdr->c_rdev_maj), (unsigned)(file_hdr->st_rdev_min));
#else
		strob_sprintf(fp, 1, "%4lu,%4lu", (unsigned long)(file_hdr->c_rdev_maj), (unsigned long)(file_hdr->c_rdev_min));
#endif
	} else {
		if ((type == PAX_SLK) || (type == PAX_HLK)) {
			sizeme = 0;
		} else {
			sizeme = (intmax_t)(file_hdr->c_filesize);
		}
		strob_sprintf(fp, 1, "%s", swlib_imaxtostr(sizeme, NULL));
	}
}

static
void
print_mode_owners(STROB * fp, struct new_cpio_header * file_hdr, int type, int vflag, char * fp_uname, char * fp_gname)
{
	char numid[64];
	char f_mode[MODELEN];
	int pad;
	char * uname;
	char * gname;
	int l_uid;
	int l_gid;
	static STROB * tmpstrob = NULL;
	
	numid[0] = '\0';
	numid[sizeof(numid) - 1] = '\0';
	swbis_strmode ((mode_t)(file_hdr->c_mode), f_mode);
	
	if (vflag & LS_LIST_VERBOSE_WITHOUT_PERMISSIONS) {
		f_mode[0] = '\0';
	}

	E_DEBUG("");

	if (vflag & LS_LIST_VERBOSE_WITHOUT_OWNERS) {
		uname = "";
		gname = "";
		l_uid = 0;
		l_gid = 0;
	} else {
		uname = fp_uname;
		gname = fp_gname;
		l_uid = (int)(file_hdr->c_uid);
		l_gid = (int)(file_hdr->c_gid);
	}

	if ( (vflag & LS_LIST_VERBOSE_WITH_NAMES) ||
		((vflag & LS_LIST_VERBOSE_WITH_SYSTEM_IDS) && (vflag & LS_LIST_VERBOSE_WITH_SYSTEM_NAMES))
	) {
		E_DEBUG("");
		if (LS_LIST_VERBOSE_ALTER_FORM & vflag) {
			/* Form used in representation of file attributes used
			   by swverify */
			E_DEBUG("");
			swlib_squash_trailing_char(f_mode, ' ');
			if (vflag & LS_LIST_VERBOSE_WITHOUT_OWNERS) {
				strob_sprintf(fp, 1, "[%s] [len=0 []]", f_mode);
			} else {
				if (!tmpstrob) tmpstrob = strob_open(24);

				strob_sprintf(tmpstrob, 0, "%d(%s)/%d(%s)",
					l_uid, uname, l_gid, gname);

				strob_sprintf(fp, 1, "[%s] [len=%d [%d(%s)/%d(%s)]]",
					f_mode, strob_strlen(tmpstrob),
					l_uid, uname,
					l_gid, gname);
			}
		} else {
			E_DEBUG("");
			pad = strlen(uname) + 2  +  /* e.g  ``(root)'' */
			strlen(gname) + 2  +  /* e.g. ``(root)'' */
			9 /*strlen(size)*/ +
			1 +
			1 +
			11 + 11 +
			0;
			if (pad > ugswidth) {
       				 ugswidth = pad;
			}
			strob_sprintf(fp, 1, "%s[%d](%s)/[%d](%s)%*s",
				f_mode,
				l_uid, uname,
				l_gid, gname,
				ugswidth - pad, "");
		}
	} else if (vflag & LS_LIST_VERBOSE_WITH_SYSTEM_IDS) {
		E_DEBUG("");
		goto do_print_ids;
	} else if (vflag & LS_LIST_VERBOSE_WITH_SYSTEM_NAMES && strlen(uname) && strlen(gname)) {
		E_DEBUG("");
		goto do_print_names;
	} else {
		E_DEBUG("");
		if (strlen(uname) && strlen(gname)) {
do_print_names:
			E_DEBUG("");
			pad = strlen(uname) + strlen(gname) + 9 /*strlen(size)*/ + 1;
			if (pad > ugswidth) {
				ugswidth = pad;
			}
			if (LS_LIST_VERBOSE_ALTER_FORM & vflag) {
				swlib_squash_trailing_char(f_mode, ' ');
				if (!tmpstrob) tmpstrob = strob_open(24);

				strob_sprintf(tmpstrob, 0, "%s/%s", uname, gname);
				/* strob_sprintf(fp, 1, "[%s] [%s/%s]", f_mode, uname, gname); */
				strob_sprintf(fp, 1, "[%s] [len=%d [%s]]", f_mode, strob_strlen(tmpstrob), strob_str(tmpstrob));

			} else {
				strob_sprintf(fp, 1, "%s%s/%s%*s", f_mode, uname, gname, ugswidth - pad, "");
			}
		} else {
do_print_ids:
			E_DEBUG("");
			snprintf(numid, sizeof(numid)-1, "%d/%d", l_uid, l_gid);
			pad = strlen(numid) + 8 /*strlen(size)*/ + 1;
			numid[sizeof(numid)-1] = '\0';
			if (pad > ugswidth) {
				E_DEBUG("");
				ugswidth = pad;
			}
			if (LS_LIST_VERBOSE_ALTER_FORM & vflag) {
				E_DEBUG("");
				swlib_squash_trailing_char(f_mode, ' ');
				strob_sprintf(fp, 1, "[%s] [len=%d [%s]]", f_mode, strlen(numid), numid);
			} else {
				E_DEBUG("");
				strob_sprintf(fp, 1, "%s%s%*s", f_mode, numid, ugswidth - pad, "");
			}
		}
		E_DEBUG("");
	}
}

static
void
print_linkname(STROB * fp, int type, char * ln_name, int do_prepend)
{
	if ((type == PAX_HLK) || (type == PAX_HRG)) {
		/* strob_strcat(fp, " == "); */
		strob_strcat(fp, " link to ");
		ls_list_safe_print_to_strob(ln_name, fp, do_prepend);
	} else {
		if (type == PAX_SLK) {
			strob_strcat(fp, " -> ");
			ls_list_safe_print_to_strob(ln_name, fp, do_prepend);
    		} else {
			/* print no symbol indicating a link, just the name */
			strob_strcat(fp, "");
			ls_list_safe_print_to_strob(ln_name, fp, do_prepend);
		}
	}
}

static
void
print_dig(STROB * fp, struct new_cpio_header * file_hdr, int do_dig, char * key, char * val)
{
	strob_strcat(fp, key);
	if (file_hdr->digsM->do_poisonM && strlen(val) == 0 ) {
		strob_sprintf(fp, 1, "<not available>");
	} else {
		strob_sprintf(fp, 1, "%s", val);
	}
}

static
void
print_sha512(STROB * fp, struct new_cpio_header * file_hdr)
{
	print_dig(fp, file_hdr, file_hdr->digsM->do_sha512, "SHA512=", file_hdr->digsM->sha512);
}

static
void
print_sha1(STROB * fp, struct new_cpio_header * file_hdr)
{
	print_dig(fp, file_hdr, file_hdr->digsM->do_sha1, "SHA1=", file_hdr->digsM->sha1);
	/*
	if (file_hdr->digsM && file_hdr->digsM->do_sha1 == DIGS_ENABLE_ON) {
		strob_strcat(fp, "SHA1=");
		strob_sprintf(fp, 1, "%s", file_hdr->digsM->sha1);
	}
	*/
}


static
void
print_md5(STROB * fp, struct new_cpio_header * file_hdr)
{
	print_dig(fp, file_hdr, file_hdr->digsM->do_md5, "MD5=", file_hdr->digsM->md5);
	/*
	if (file_hdr->digsM && file_hdr->digsM->do_md5 == DIGS_ENABLE_ON) {
		strob_strcat(fp, "MD5=");
		strob_sprintf(fp, 1, "%s", file_hdr->digsM->md5);
	}
	*/
}

static
void
alt_ls_list_to_string(char * fp_name, char * ln_name, struct new_cpio_header * file_hdr,
	time_t now, STROB * fp, char * uname, char * gname, int type, int vflag)
{
	char * name;
	time_t mtime;
	int did_drop;
	int name_len;
	int linkname_len;

	did_drop = 0;
	name = strip_slash(fp_name, vflag);

	if (type == DIRTYPE)
		swlib_toggle_trailing_slash("drop", fp_name, &did_drop);

	mtime = (time_t)(file_hdr->c_mtime);


	/* Get the lengths of the names */
	strob_strcpy(fp, "");
	ls_list_safe_print_to_strob(name, fp, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);
	name_len = strob_strlen(fp);

	strob_strcpy(fp, "");
	print_linkname(fp, (LS_LIST_VERBOSE_LINKNAME_PLAIN & vflag) ? 0 : type, ln_name, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);
	linkname_len = strob_strlen(fp);

	/* print the name length */
	strob_sprintf(fp, 0 /* do not append */, LS_LIST_NAME_LENGTH "=%d ", name_len);
	
	/* print the name */
	ls_list_safe_print_to_strob(name, fp, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);

	/* print the linkname */
	strob_sprintf(fp, 1 /* do append */, " [" LS_LIST_LINKNAME_LENGTH "=%d " LS_LIST_LINKNAME_MARK, linkname_len);
	print_linkname(fp,  (LS_LIST_VERBOSE_LINKNAME_PLAIN & vflag) ? 0 : type, ln_name, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);
	strob_strcat(fp, "]]");
	
	/* filesize */
	strob_strcat(fp, " [");
	if (vflag & LS_LIST_VERBOSE_WITH_SIZE)
		print_size_rdev(fp, file_hdr, type);
	strob_strcat(fp, "]");

	/* owner names */
	strob_strcat(fp, " ");
	print_mode_owners(fp, file_hdr, type, vflag, uname, gname);

	/* date */
	strob_strcat(fp, " [");
	print_date(fp, vflag, type, mtime);
	strob_strcat(fp, "]");

	strob_strcat(fp, " [");
	if (type == REGTYPE)
		if (vflag & LS_LIST_VERBOSE_WITH_MD5)
			print_md5(fp, file_hdr);
	strob_strcat(fp, "]");
	
	strob_strcat(fp, " [");
	if (type == REGTYPE)
		if (vflag & LS_LIST_VERBOSE_WITH_SHA1)
			print_sha1(fp, file_hdr);
	strob_strcat(fp, "]");
	
	strob_strcat(fp, " [");
	if (type == REGTYPE)
		if (vflag & LS_LIST_VERBOSE_WITH_SHA512)
			print_sha512(fp, file_hdr);
	strob_strcat(fp, "]");
		
	if (type == DIRTYPE)
		swlib_toggle_trailing_slash("restore", fp_name, &did_drop);
	return;
}

void
ls_list (char * name, char * ln_name, struct new_cpio_header * sbp,
	time_t now, FILE * fp, char * uname, char * gname, int type, int vflag)
{
	STROB * buf = strob_open(160);
        ls_list_to_string(name, ln_name, sbp, now, buf, uname, gname, type, vflag);
	fprintf(fp, "%s\n", strob_str(buf));
        (void) fflush (fp);
	strob_close(buf);
}

void
ls_list_to_string(char * fp_name, char * ln_name, struct new_cpio_header * file_hdr,
	time_t now, STROB * fp, char * uname, char * gname, int type, int vflag)
{
	char * name;
	time_t mtime;

	name = strip_slash(fp_name, vflag);

	/*
	 * if not verbose, just print the file name
	 */
	if (vflag & LS_LIST_VERBOSE_OFF) {
		/* just print a list of file names */
		strob_sprintf(fp, 0, "%s", name);
		return;
	} else if (LS_LIST_VERBOSE_ALTER_FORM & vflag) {
		alt_ls_list_to_string(fp_name, ln_name, file_hdr, now, fp, uname, gname, type, vflag);
		return;
	} else {
		/*
		 * user wants traditional long mode
		 */
		;  /* continue */
	}

	mtime = (time_t)(file_hdr->c_mtime);
	strob_strcpy(fp, "");

	/* print file mode, link count, uid, gid and time */
	print_mode_owners(fp, file_hdr, type, vflag, uname, gname);
	
	strob_strcat(fp, " ");
 
	/* print the size or rdevs */
	print_size_rdev(fp, file_hdr, type);

	strob_strcat(fp, " ");
	
	/* Print the date */
	print_date(fp, vflag, type, mtime);
	
	strob_strcat(fp, " ");

	/* Print the name */
	ls_list_safe_print_to_strob(name, fp, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);

	/* Print the linkname for symlinks and hard links */
	print_linkname(fp, (LS_LIST_VERBOSE_LINKNAME_PLAIN & vflag) ? 0 : type, ln_name, vflag & LS_LIST_VERBOSE_PREPEND_DOTSLASH);

	return;
}
