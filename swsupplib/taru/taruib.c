/* taruib.c - write the catalog and storage byte streams.

   Copyright (C) 2003 Jim Lowe

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
#include <ctype.h>
#include "taruib.h"
#include "taru.h"
#include "swfork.h"
#include "swlib.h"
#include "uxfio.h"
#include "swpath.h"
#include "swgp.h"
#include "swi.h"
#include "xformat.h"
#include "fnmatch_u.h"

#include "debug_config.h"

static int g_taruib_gst_overflow_releaseM;
static int g_taruib_gst_fdM;
static int g_taruib_gst_lenM;
static char g_taruib_gst_bufferM[TARU_BUFSIZ_RES];
	
static char * md5sum_adjunct_pattern[3] = {
	"catalog/*/adjunct_md5sum",
	"*/catalog/*/adjunct_md5sum",
	(char*)NULL };

static char * md5sum_full_pattern[3] = {
	"catalog/*/md5sum",
	"*/catalog/*/md5sum",
	(char*)NULL };

static char * sha1sum_pattern[3] = {
	"catalog/*/sha1sum",
	"*/catalog/*/sha1sum",
	(char*)NULL };

static char * sha512sum_pattern[3] = {
	"catalog/*/sha512sum",
	"*/catalog/*/sha512sum",
	(char*)NULL };

static char * signature_pattern[3] = {
	"catalog/*/" SWBN_SIGNATURE,
	"*/catalog/*/" SWBN_SIGNATURE,
	(char*)NULL };

static char * signature_header_pattern[3] = {
	"catalog/*/" SWBN_SIG_HEADER,
	"*/catalog/*/" SWBN_SIG_HEADER,
	(char*)NULL };

static
char **
determine_pattern_array(int * detpat, char * name) {

	E_DEBUG("ENTERING");
	if (strstr(name, "/md5sum")) {
		*detpat = TARUIB_N_MD5;
		return md5sum_full_pattern;
	} else if (strstr(name, "/adjunct_md5sum")) {
		*detpat = TARUIB_N_ADJUNCT_MD5;
		return md5sum_adjunct_pattern;
	} else if (strstr(name, "/sha1sum")) {
		*detpat = TARUIB_N_SHA1;
		return sha1sum_pattern;
	} else if (strstr(name, "/sha512sum")) {
		*detpat = TARUIB_N_SHA512;
		return sha512sum_pattern;
	} else if (
		strstr(name, "/" SWBN_SIGNATURE) &&
		strlen(strstr(name, "/" SWBN_SIGNATURE)) ==
			     strlen("/" SWBN_SIGNATURE)
	) {
		*detpat = TARUIB_N_SIG;
		return signature_pattern;
	} else if (
		strstr(name, "/" SWBN_SIG_HEADER) &&
		strlen(strstr(name, "/" SWBN_SIG_HEADER)) ==
			     strlen("/" SWBN_SIG_HEADER)
	) {
		*detpat = TARUIB_N_SIG_HDR;
		return signature_header_pattern;
	} else {
		*detpat = TARUIB_N_OTHER;
		return (char**)NULL;
	}
}

static
int 
check_useArray(int * useArray, int digest_type, int dfd)
{
	int ret = -1;
	E_DEBUG("ENTERING");
	if (useArray) {
		if (digest_type > 4) return -1;
		if (useArray[digest_type] && digest_type != TARUIB_N_SIG) {
			/*
			* More than one sig or digest file in the catalog.
			* This is sanity security check.
			*/
			ret = -1;
			fprintf(stderr,
				"swbis: catalog file error : type = %d\n",
				digest_type);
		} else {
			useArray[digest_type] ++;
			ret = dfd;
		}
	} else {
		ret = dfd;
	}
	return ret;
}

static
int
verbose_decode(int * useArray, int * detpat,
				char * prepath,
				char * name,
				int nullfd,
				int digest_type, 
				int verbose,
				int md5digestfd,
				int sha1digestfd,
				int sigfd,
				int sha512digestfd)
{
	int i = 0;
	int ret;
	int passfd = nullfd;
	char * s;
	char ** pattern_array;
	STROB * tmp;

	E_DEBUG("ENTERING");
	if (verbose == 0) return nullfd;

	if (prepath && strlen(prepath)) {
		/*
		* Normalize the name (exclude the prepath).
		*/
		s = strstr(name, prepath);
		if (s == (char*)NULL) {
			/* 
			* error 
			*/
			return -1;
		} else if (s == name) {
			/*
			* Normalize name
			*/
			name += (strlen(prepath));
			if (*name == '/') name++;
		} else {
			/* 
			* error 
			*/
			return -1;
		}
	}

	if (detpat == NULL) {
		if(digest_type == TARUIB_N_ADJUNCT_MD5) {
			pattern_array = md5sum_adjunct_pattern;
		} else if(digest_type == TARUIB_N_SIG) {
			pattern_array = signature_pattern;
		} else if(digest_type == TARUIB_N_MD5) {
			pattern_array = md5sum_full_pattern;
		} else if(digest_type == TARUIB_N_SHA1) {
			pattern_array = sha1sum_pattern;
		} else if(digest_type == TARUIB_N_SHA512) {
			pattern_array = sha512sum_pattern;
		} else if(digest_type == TARUIB_N_SIG_HDR) {
			pattern_array = signature_header_pattern;
		} else {
			pattern_array = NULL;
			fprintf(stderr, "internal error in verbose_decode\n");
			return -1;
		}
	} else {
		pattern_array = determine_pattern_array(detpat, name);
		digest_type = *detpat;
		if (pattern_array == NULL) return nullfd;
	}

	tmp = strob_open(10);
	while(pattern_array[i]) {
		if (!name || !strlen(name)) {
			fprintf(stderr,
				"internal error in taruib::verbose_decode\n");
			strob_close(tmp);
			return nullfd;
		}
		ret = fnmatch(pattern_array[i], name, 1);
		if (ret == 0) {
			if (digest_type == TARUIB_N_MD5 && md5digestfd >= 0) {
				passfd = check_useArray(useArray,
							digest_type,
							md5digestfd);
			} else if (digest_type == TARUIB_N_SIG && sigfd >= 0) {
				passfd = check_useArray(useArray,
							digest_type,
							sigfd);
			} else if (digest_type == TARUIB_N_SHA1 && sha1digestfd >= 0) {
				passfd = check_useArray(useArray,
							digest_type,
							sha1digestfd);
			} else if (digest_type == TARUIB_N_SHA512 && sha512digestfd >= 0) {
				passfd = check_useArray(useArray,
							digest_type,
							sha512digestfd);
			} else if (digest_type == TARUIB_N_SIG_HDR) {
				passfd = nullfd;
			} else {
				if (detpat) {
					passfd = nullfd;
				} else {
					passfd = STDERR_FILENO;
				}
			}
			break;
		}
		i++;
	}
	strob_close(tmp);
	return passfd;
}

static
int
check_signature_name(char * sigfilename, char * prevname) 
{
	int ret;
	char * p;
	char * s;

	E_DEBUG("ENTERING");
	s = strstr(sigfilename, "/" SWBN_SIGNATURE);
	if (!s || *(s + strlen("/" SWBN_SIGNATURE)) != '\0' ) {
		return 1;
	}	
	s++;
	*s = '\0';
	if ((p=strstr(prevname, sigfilename)) == NULL || p != prevname) {
		ret = 1;
	} else {
		ret = 0;
	}
	*s = 's';
	return ret;
}

static
int
do_header_safety_checks(XFORMAT * package,
		char * name,
		char * prevname,
		char * signature_header)
{
	int type;
	mode_t mode;
	char * linkname;
	int sigsize;
	unsigned long trusted_sigsize;
	char * header_bytes;
	int ret = 0;
	
	E_DEBUG("ENTERING");
	if (check_signature_name(name, prevname)) {
		fprintf(stderr, 
		"Warning: *** The file name [%s]\n"
		"         *** indicate strong possibility of tampering.\n"
		"         *** Use of this package is a security risk.\n", 
									name);
		return -1;
	}

	header_bytes = taru_get_recorded_header(package->taruM, (int *)NULL);
	type = xformat_get_tar_typeflag(package);
	mode = xformat_get_perms(package);
	linkname = xformat_get_linkname(package, NULL);
	sigsize = xformat_get_filesize(package);
	if (
			S_ISDIR(mode) == 0 && 	
			(
			(mode & S_ISUID) ||
			(mode & S_ISGID) ||
			(mode & S_ISVTX) 
			)
	) {
		fprintf(stderr, 
		"Warning: *** The file mode permissions on %s\n"
		"         *** indicate strong possibility of tampering.\n"
		"         *** Use of this package is a security risk.\n",
									name);
		return -2;
	}

	if (type != REGTYPE) {
		fprintf(stderr, 
		"Warning: *** The file type flag on %s\n"
		"         *** indicate strong possibility of tampering.\n"
		"         *** Use of this package is a security risk.\n",
									name);
		return -2;
	}
	
	if (linkname && strlen(linkname)) {
		return -3;
	}

	if (signature_header) {
		/*
		* trusted_sigsize is the file data length of the
		* sig_header attribute file.
		*/
		taru_otoul(signature_header+THB_BO_size, &trusted_sigsize);
		if (header_bytes && (int)trusted_sigsize == sigsize) {
			ret = memcmp(signature_header, 	
					header_bytes, TARRECORDSIZE);
			if (ret) ret = -5;
		} else {
			ret = -6;
		}
	} else {

	}

	/*
	* <= 0 is an error
	* > 0 (512 or 1024) is OK
	*/
	if (ret < 0) return ret;
	if (ret > 0) return -ret;
	return sigsize;
}


/* -------------------------------------------------------------- */
/*  Public Routines.                                              */
/* -------------------------------------------------------------- */

void
taruib_initialize_pass_thru_buffer(void)
{
	g_taruib_gst_overflow_releaseM = 0;
	g_taruib_gst_fdM = -1;
	g_taruib_gst_lenM = 0;
}

int
taruib_write_pass_files(void * vp, int ofd, int adjunct_ofd_p)
{
	XFORMAT * package = (XFORMAT *)vp;
	int nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	int ifd = xformat_get_ifd(package);
	int adjunct_ofd;
	int ret;
	int retval = 0;
	int total_sym_bytes = 0;

	E_DEBUG("ENTERING");
	adjunct_ofd  = adjunct_ofd_p;
	if (adjunct_ofd < 0) adjunct_ofd = -1;
	taruib_set_overflow_release(0);
	taruib_set_fd(ofd);
	while ((ret = xformat_read_header(package)) > 0) {
		retval += ret;
		if (xformat_is_end_of_archive(package)){
			swbis_devnull_close(nullfd);
			break;
		}
		
		if (adjunct_ofd >= 0) {
			/*
			* Hack
			*/
			if (xformat_get_tar_typeflag(package) == SYMTYPE) {
				total_sym_bytes += g_taruib_gst_lenM;
				adjunct_ofd = -2;
				;
			} else {
				/*
				* write the header.
				*/
				uxfio_write(adjunct_ofd,
						(void*)g_taruib_gst_bufferM,
						(size_t)g_taruib_gst_lenM);
			}
		}
		/*
		* write the header.
		*/
		taruib_set_fd(ofd);
		taruib_clear_buffer();

		/*
		* Write the data.
		*/
		if (xformat_file_has_data(package)) {
			taruib_set_fd(-1);
			taruib_set_overflow_release(1);
			if (xformat_copy_pass2(package, ofd, ifd,
							adjunct_ofd) < 0) {
				fprintf(stderr, 
					"error from xformat_copy_pass2\n");	
			}
			taruib_set_overflow_release(0);
			taruib_set_datalen(0);
			taruib_set_fd(ofd);
		}
		
		if (adjunct_ofd == -2) {
			adjunct_ofd = adjunct_ofd_p;
		}
	}

	if (adjunct_ofd_p >= 0) {
		uxfio_write(adjunct_ofd_p,
				(void*)g_taruib_gst_bufferM, (size_t)g_taruib_gst_lenM);
	}
	taruib_clear_buffer();
	taruib_set_fd(-1);
	taruib_set_overflow_release(1);

	ret = taru_pump_amount2(ofd, ifd, -1, adjunct_ofd_p); 

	if (ret < 0) {
		fprintf(stderr, "swbis: error in taruib_write_pass_files\n");
		retval = -1;
	} else {
		retval += ret;
	}
	return  retval;
}

int
taruib_get_nominal_reserve(void)
{
	return taruib_get_reserve() - TARRECORDSIZE;
}

int taruib_get_reserve(void)
{
	return sizeof(g_taruib_gst_bufferM);
}

void taruib_set_overflow_release(int i)
{
	g_taruib_gst_overflow_releaseM = i;
}

int taruib_get_overflow_release(void)
{
	return g_taruib_gst_overflow_releaseM;
}

void taruib_set_fd(int fd)
{
	g_taruib_gst_fdM = fd;
}

int taruib_get_fd(void)
{
	return g_taruib_gst_fdM;
}

char * taruib_get_buffer(void)
{
	return g_taruib_gst_bufferM;
}

int taruib_get_datalen(void)
{
	return g_taruib_gst_lenM;
}

int taruib_get_bufferlen(void)
{
	return sizeof(g_taruib_gst_bufferM);
}

void taruib_set_datalen(int n)
{
	g_taruib_gst_lenM = n;
}

void taruib_unread(int n)
{
	if (n > g_taruib_gst_lenM) {
		g_taruib_gst_lenM = 0;
	} else {
		g_taruib_gst_lenM -= n;
	}
}

int taruib_clear_buffer(void)
{
	int ret;
	ret = uxfio_write(g_taruib_gst_fdM, (void*)g_taruib_gst_bufferM, (size_t)g_taruib_gst_lenM);
	if (ret < 0) {
		fprintf(stderr, "swbis: module taruib: write error (fatal)\n");
		exit(14);
	}
	g_taruib_gst_lenM -= ret;
	
	if (g_taruib_gst_lenM > 0) return -ret;
	if (g_taruib_gst_lenM < 0) return ret;
	return ret;
}

/*
*  taruib_write_catalog_stream
*  write out the catalog half of the package.
*
* version 0 is OBSOLETE.
* version 1 is production version.
*/
int
taruib_write_catalog_stream(void * vp, int ofd, int version, int verbose)
{
	XFORMAT * package = (XFORMAT *)vp;
	int nullfd;
	int passfd;
	int check_ret;
	int ifd = xformat_get_ifd(package);
	int copyret;
	int ret;
	int retval = 0;
	int is_catalog;
	int path_ret;
	STROB * prevname = strob_open(100);
	STROB * namebuf = strob_open(100);
	char * prepath;
	char * name;
	int got_sig = 0;
	SWPATH * swpath = swpath_open("");
	TARU * taru = taru_create();

	E_DEBUG("ENTERING");
	nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	if (!swpath) return -21;
	if (nullfd < 0) return -22;		
	if (ifd < 0) return -32;		
	if (!taru) return -20;

	E_DEBUG("");
	taruib_set_fd(ofd);
	while ((ret = xformat_read_header(package)) > 0) {
		E_DEBUG("");
		retval += ret;
		if (xformat_is_end_of_archive(package)){
			/*
			* This is probably a package with no storage files.
			*/
			/*  jhl 2006-09-08  warning eliminated
			fprintf(stderr,
			"taruib_write_catalog_stream: warning: end of archive.\n");
			*/
			break;
		}
		E_DEBUG("");
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		path_ret = swpath_parse_path(swpath, name);
		if (path_ret < 0) {
			fprintf(stderr,
		"taruib_write_catalog_stream: error parsing path [%s].\n",
				name);
			E_DEBUG("LEAVING");
			return -1;
		}
		prepath = swpath_get_prepath(swpath);

		E_DEBUG("");
		/*
		* version 0: include the leading directories.
		* version 1: exclude the leading directories.
		*/
		is_catalog = swpath_get_is_catalog(swpath);
		if (
		   (version == 0 && (is_catalog == SWPATH_CTYPE_CAT || is_catalog == SWPATH_CTYPE_DIR)) ||
		   (version == 1 && is_catalog == SWPATH_CTYPE_CAT)
		   )
		{
			/*
			 * Leading directories and the /catalog/ files.
			 */

			E_DEBUG("");
			if ( strcmp(swpath_get_basename(swpath),
							SWBN_SIGNATURE) == 0 &&
			 	 strlen(swpath_get_dfiles(swpath)) &&
							1 /*got_sig == 0*/) {
				/* 
				 * Skip it 
				 * Its the signature file in the dfiles
				 * directory,
				 */
				E_DEBUG("");
				check_ret = do_header_safety_checks(package,
							strob_str(namebuf),
							strob_str(prevname),
							NULL);
				if (check_ret < 0) {
					E_DEBUG("LEAVING");
					return -1;
				}

				E_DEBUG("");
				got_sig = 1;
				taruib_unread(ret);
				retval -= ret;
				taruib_set_overflow_release(0); /* OFF */
				taruib_set_fd(-1);
				
				E_DEBUG("");
				if (verbose && version == 1) {
					passfd = verbose_decode(NULL, NULL,
							prepath, name, nullfd,
							2, verbose, -1, -1, -1, -1);
				} else {
					passfd = nullfd;
				}
				xformat_copy_pass(package, passfd, ifd);
				taruib_set_fd(ofd);
			 } else {
				/*
				* Copy it.
				*/
				E_DEBUG("");
				taruib_set_overflow_release(0); /* OFF */

				/*
				* With taruib fd set, this has the affect 
				* of writting it out.  The nominal file 
				* descriptor is set to /dev/null.  That is 
				* the taruib_ buffer is maintained 
				* (and flushed) by the read routine in 
				* xformat_copy_pass().
				*/
				E_DEBUG("");
				E_DEBUG3("nullfd=%d  ifd=%d", nullfd, ifd);
				copyret = xformat_copy_pass(package,
							nullfd, ifd);
				if (copyret < 0) {
					E_DEBUG("");
					swpath_close(swpath);
					strob_close(namebuf);
					strob_close(prevname);
					E_DEBUG("LEAVING : xformat_copy_pass error.");
					fprintf(stderr,
					"swbis : xformat_copy_pass() error\n");
					return copyret;
				}

				E_DEBUG("");
				if (copyret == 0) {
					/*
					* If there was nothing to write, then
					* flush the buffer.
					*/
					E_DEBUG("");
					if(taruib_clear_buffer() < 0) {
						fprintf(stderr, 
			"Internal error in taruib_write_catalog_stream : %d\n", 
							__LINE__);
						return -1;
					}
				} else {
					retval += copyret;
				}
				E_DEBUG("");
			}
			E_DEBUG("");
		} else {
			E_DEBUG("");
			if (version == 1) {
				E_DEBUG("");
				if (is_catalog == SWPATH_CTYPE_DIR) {
					/*
					* skip leading dirs.
					* continue;
					*/
					E_DEBUG("");
					taruib_unread(ret);	
					retval -= ret;
					taruib_set_overflow_release(0);/*OFF*/
					taruib_set_fd(-1);
					xformat_copy_pass(package, nullfd, ifd);
					taruib_set_fd(ofd);
				} else if (is_catalog == SWPATH_CTYPE_STORE) {
					/*
					* into storage structure.  Stop
					*/
					E_DEBUG("");
					taruib_unread(ret);	
					retval -= ret;
					break;
				} else {
					fprintf(stderr, 
			"internal error : taruib_write_catalog_stream\n");
					E_DEBUG("LEAVING");
					return -1;
				}
			} else if (version == 0) {  /* version == 0 */
				/* 
				* read past then catalog, unread the header 
				* Assertion: is_catalog == 0.
				*   Stop.
				*/
				E_DEBUG("");
				taruib_unread(ret);	
				retval -= ret;
				break;
			} else {
				E_DEBUG("");
				fprintf(stderr, "internal error: default else \n");
				E_DEBUG("LEAVING");
				return -1;
			}
		}
		E_DEBUG("");
		/*
		* Prepare to read the next header
		*/
		taruib_set_overflow_release(1); /* ON */
		strob_strcpy(prevname, strob_str(namebuf));
	}

	if (ret >= 0) {
		E_DEBUG("");
		if (taruib_clear_buffer() < 0) {
			fprintf(stderr, 
			"Internal error in taruib_write_catalog_stream : %d\n",
				__LINE__);
			return -1;
		}
	}

	E_DEBUG("");
	if (version == 1) {
		/*
		* write two (null) trailer blocks if tar format.
		*/
		E_DEBUG("");
		retval += taru_write_archive_trailer(taru, arf_ustar, 
							ofd, TARRECORDSIZE, 0, 0);
		if (got_sig == 0) {
			fprintf(stderr, "swbis: signature file not found.\n");
		}
	}

	swbis_devnull_close(nullfd);
	E_DEBUG("");
	strob_close(namebuf);
	E_DEBUG("");
	swpath_close(swpath);
	E_DEBUG2("LEAVING retval = %d", retval);
	return  retval;
}

/*
*     swlib_write_storage_stream
*  write out the storage half of the package.
*
* version 0 is OBSOLETE.
* version 1 is production version.
*
*/
int
taruib_write_storage_stream(void * vp, int ofd, int version, int ofd2, 
						int verbose, int digest_type)
{
	XFORMAT * package = (XFORMAT *)vp;
	int nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	int ifd = xformat_get_ifd(package);
	int path_ret;
	int passfd;
	int copyret;
	int ret;
	int retval = 0;
	int is_catalog;
	STROB * namebuf = strob_open(100);
	char * name;
	SWPATH * swpath = swpath_open("");
	int debug = 0;
	int do_adjunct = 0; 
	int header_bytes;
	char * prepath;
	TARU * taru = taru_create();

	/*
	* digest_type is defined above
	*
	*	md5sum    	0
	*	adjunct_md5sum  1
	*	signature 	2
	*	sha1sum   	3
	*/
	E_DEBUG("ENTERING");

	if (ofd2 > 0) {
		do_adjunct = 1; 
		digest_type = 1;
	}

	if (!taru) return -20;
	if (!swpath) return -21;
	if (nullfd < 0) return -22;		
	if (ifd < 0) return -32;		
	if (debug) 
		fprintf(stderr, "ofd = %d version = %d\n", ofd, version); 

	taruib_set_fd(nullfd);
	taruib_set_overflow_release(0);
	while ((ret = xformat_read_header(package)) > 0) {
		header_bytes = ret;
		retval += ret;
		if (xformat_is_end_of_archive(package)){
			/* fprintf(stderr, "taruib: empty payload section\n"); */
			retval = taru_write_archive_trailer(taru, arf_ustar,
							ofd, 512, 0, 0);
			swbis_devnull_close(nullfd);
			swpath_close(swpath);
			strob_close(namebuf);
			return retval;
		}
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		path_ret = swpath_parse_path(swpath, name);
		if (path_ret < 0) {
			fprintf(stderr, 
				"taruib_write_storage_stream:"
					" error parsing path [%s].\n", name);
			E_DEBUG("LEAVING");
			return -1;
		}
		prepath = swpath_get_prepath(swpath);
		is_catalog = swpath_get_is_catalog(swpath);
		if (debug) fprintf(stderr, "%s : %d\n", name, is_catalog); 
		if (version == 0) {
			if (is_catalog != SWPATH_CTYPE_STORE) {
				/*
				* Either -1 leading dir, or 1 catalog.
				*
				* skip it.
				*/
				
				taruib_set_overflow_release(0); /* OFF */
				taruib_set_fd(0); /* Turn OFF */
				copyret = xformat_copy_pass(package,
								nullfd, ifd);
				if (copyret < 0) {
					swpath_close(swpath);
					strob_close(namebuf);
					swbis_devnull_close(nullfd);
					return copyret;
				}
				taruib_set_fd(nullfd);
				taruib_clear_buffer();
			} else {
				/*
				* = 0, the storage structure.
				*/
				taruib_set_fd(ofd); /* Turn ON */
				taruib_clear_buffer();
				retval = ret;
					/* turn pass-thru buffer off */
				taruib_set_fd(0);
				break;
			}
		} else if (version == 1) {  /* version 1 */
			/*
			* Include the the leading directories in the
			* storage file.
			*/
			if (is_catalog == SWPATH_CTYPE_CAT) {
				/*
				* 1 == catalog.
				*
				* skip it.
				*/
				
				taruib_set_overflow_release(0); /* OFF */
				taruib_set_fd(0); /* Turn OFF */

				passfd = verbose_decode(NULL, NULL,
							prepath, name,
							nullfd, digest_type,
							verbose, -1, -1, -1, -1);
				copyret = xformat_copy_pass_file_data(package, 
								passfd, ifd);
				
				if (copyret < 0) {
					swpath_close(swpath);
					strob_close(namebuf);
					swbis_devnull_close(nullfd);
					return copyret;
				}
				xformat_decrement_bytes_written(package,
						copyret + header_bytes);
				taruib_set_fd(nullfd);
				taruib_clear_buffer();

			} else if (is_catalog == SWPATH_CTYPE_DIR) {
				/*
				* leading dir  -- pass it thru.
				*/
				if (ofd2 >= 0) {
					/*
					* Hack
					*/
					uxfio_write(ofd2,
						(void*)taruib_get_buffer(),
						(size_t)taruib_get_datalen());
				}
				if (debug) 
					fprintf(stderr, "in leading dirs\n"); 
				taruib_set_fd(ofd);
				taruib_clear_buffer();
			} else {
				/*
				*  0, the storage structure.
				*/
				if (ofd2 >= 0) {
					/*
					* Hack
					*/
					uxfio_write(ofd2, 
						(void*)g_taruib_gst_bufferM,
						(size_t)g_taruib_gst_lenM);
				}
				taruib_set_fd(ofd); /* Turn ON */
				taruib_clear_buffer();
				retval = ret;
					/* turn pass-thru buffer off */
				taruib_set_fd(0); 
				/*
				* Now break, the rest of the package is 
				* wriiten below.
				*/
				break;
			}
		}

		/*
		 * Prepare to read the next header
		 */
		taruib_set_overflow_release(1); /* ON */
	}

	/*
	* write the storage structure to the end.
	*/
	if (version == 0) {
		ret = taru_pump_amount2(ofd, ifd, -1, -1);
	} else {
		/*
		* ofd2 is adjunct_ofd
		*/
		/*
		* The current file may have data. need to clear it.
		*/
		if (xformat_file_has_data(package)) {
			if (xformat_copy_pass2(package, ofd, ifd, ofd2) < 0) {
				fprintf(stderr, 
				"error from xformat_copy_pass2\n");	
			}
		}
		ret = taruib_write_pass_files(vp, ofd, ofd2);
	}

	swbis_devnull_close(nullfd);
	strob_close(namebuf);
	swpath_close(swpath);
	if (ret < 0) return -100;	
	retval+=ret;
	return retval;
}

int
taruib_arfcopy(void * xpackage, 
		void * xswpath, 
		int xofd, 
		char * leadingpath, 
		int do_preview, 
		uintmax_t * statbytes, 
		int * deadman,
		void (*alarm_handler)(int))
{
	XFORMAT * package = (XFORMAT*)xpackage;
	SWPATH * swpath = (SWPATH*)xswpath;
	int nullfd;
	int format;
	int output_format;
	char * name;
	int ifd;
	int ret;
	int aret;
	int parseret;
	int pathret;
	int retval = -1;
	int depth;
	int ofd;
	STROB * resolved_path;
	STROB * namebuf;
	STROB * newnamebuf;
	STROB * tmp;
	off_t bytes = 0;
	int tarheaderflags;
	int do_gnu_long_link;
	int namelengthret;
	struct new_cpio_header * file_hdr;
        sigset_t pendmask;

	nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	newnamebuf = strob_open(100);
	namebuf = strob_open(100);
	tmp = strob_open(10);
	resolved_path = strob_open(10);
	tarheaderflags = xformat_get_tarheader_flags(package);
	file_hdr = (struct new_cpio_header*)(xformat_vfile_hdr(package));
	
	output_format = xformat_get_output_format(package);
	format = xformat_get_format(package);
	ifd = xformat_get_ifd(package);
	if (ifd < 0) return -32;		

	/*
	* Here's the trick (Uh.. Hack):
	*     ofd is really /dev/null; and the real output
	*     is set into taruib_set_fd() which enables the write
	*     thru cache resulting in the output being the same bytes
	*     as the input.
	*/
	ofd = nullfd;
	xformat_set_ofd(package, ofd);
	taruib_initialize_pass_thru_buffer();
	taruib_set_fd(xofd);
	if (alarm_handler) {
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);
	}
	while ((ret = xformat_read_header(package)) > 0  && 
			(!deadman || (deadman && *deadman == 0))) {
		if (alarm_handler) {
			if (statbytes) {
				(*statbytes) = (unsigned long int)bytes;
			}
			swgp_signal_unblock(SIGALRM, (sigset_t *)NULL);
        		sigpending(&pendmask);
			if (sigismember(&pendmask, SIGALRM)) {
                                (*alarm_handler)(SIGALRM);
			}
		}
		if (xformat_is_end_of_archive(package)){
			break;
		}
		bytes += ret;
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		strob_strcpy(newnamebuf, leadingpath);
		swlib_unix_dircat(newnamebuf, name);

		/*
		* Make sure its a normal looking posix pathname.
		*/
		parseret = swpath_parse_path(swpath, name);
		if (parseret < 0) {
			retval = -3;
			goto error;
		}

		pathret = swlib_vrealpath("", 
					name, 
					&depth, 
					resolved_path);


		if (depth <= 1 && strstr(name, "..")) {
			/*
			* Somebody tried to sneak in too many ".."
			*/
			retval = -4;
			goto error;
		}
		
		namelengthret = taru_is_tar_filename_too_long(
			strob_str(newnamebuf), 
			tarheaderflags, 
			&do_gnu_long_link, 
			1 /* is_dir */);

		if (namelengthret) {
			/*
			* Name too long for the output format.
			*/
			retval = -5;
			goto error;
		}

		/*
		* FIXME: Need to test symlinks for too many ".."
		*/

		/*
		* Set the New name in the archive.
		*/
		ahsStaticSetTarFilename(file_hdr, strob_str(newnamebuf));

		/*
		* Clear the buffer, this has the effect of writing the
		* header.
		*/	
		if (alarm_handler) {
			swgp_signal_block(SIGALRM, (sigset_t *)NULL);
		}
		taruib_clear_buffer();

		/*
		* Now write any data associated with the current
		* archive member.
		*/	
		aret = xformat_copy_pass(package, ofd, ifd);
		if (aret < 0) goto error;
		bytes += aret;
	}
	if (alarm_handler) {
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);
	}
	if (deadman && *deadman) return -1;
	taruib_clear_buffer();
	retval = 0;

	/*
	* Pump out the trailer bytes.
	*/
	ret = taru_pump_amount2(ofd, ifd, -1, -1);
	if (ret < 0) {
		fprintf(stderr, "swbis: error in taruib_arfcopy\n");
		retval = -1;
	}
	bytes += aret;
	/*
	* Now do a final clear.
	*/
	taruib_clear_buffer();
error:
	strob_close(resolved_path);
	strob_close(tmp);
	strob_close(namebuf);
	if (alarm_handler) {
		swgp_signal_unblock(SIGALRM, (sigset_t *)NULL);
	}
	return retval;
}
