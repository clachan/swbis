/* taruib_i.c - verify archive digests

   Copyright (C) 2004,2007 Jim Lowe

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
#include "swutilname.h"
#include "xformat.h"
#include "fnmatch_u.h"
#include "debug_config.h"
#include "atomicio.h"

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

static char * size_pattern[3] = {
	"catalog/*/size",
	"*/catalog/*/size",
	(char*)NULL };

static char * signature_header_pattern[3] = {
	"catalog/*/" SWBN_SIG_HEADER,
	"*/catalog/*/" SWBN_SIG_HEADER,
	(char*)NULL };
	

#define DIG_VERBOSE_LEVEL	SWC_VERBOSE_2
#define SHA1_ASCII_LEN		40
#define MD5_ASCII_LEN		32
#define SHA512_ASCII_LEN	128

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
	} else if (strstr(name, "/size")) {
		*detpat = TARUIB_N_SIZE;
		return size_pattern;
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
		if (digest_type > TARUIB_N_MAX_INDEX) return -1;
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
verbose_decode(XFORMAT * package,
		int * useArray,
		int * detpat,
		char * prepath,
		char * name,
		int nullfd,
		int digest_type, 
		int verbose,
		int md5digestfd,
		int sha1digestfd,
		int sizefd,
		int sigfd,
		int sha512digestfd)
{
	int i = 0;
	int ret;
	int passfd = nullfd;
	char * s;
	char ** pattern_array;
	STROB * tmp;
	intmax_t filesize;

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

	pattern_array = determine_pattern_array(detpat, name);
	digest_type = *detpat;
	if (pattern_array == NULL) return nullfd;

	tmp = strob_open(10);
	while(pattern_array[i]) {
		if (!name || !strlen(name)) {
			fprintf(stderr,
				"internal error in taruib::verbose_decode\n");
			strob_close(tmp);
			return nullfd;
		}
		E_DEBUG3("fnmatch [%s] [%s]", pattern_array[i], name);
		ret = fnmatch(pattern_array[i], name, 1);
		if (ret == 0) {
			filesize = xformat_get_filesize(package);

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
			} else if (digest_type == TARUIB_N_SIZE && sizefd >= 0) {
				passfd = check_useArray(useArray,
							digest_type,
							sizefd);
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
do_sig_safety_checks(int memfd, int sigsize, int whichsig)
{
	char * data;
	int data_len;
	int cnt = 0;
	char * s;	
	int foundnull = 0;
	int foundchar = 0;
	int which_offset = 0;
	int nsig;
	int start_sig;
	int end_sig;

	E_DEBUG("ENTERING");
	if (uxfio_get_dynamic_buffer(memfd, &data, NULL, &data_len) < 0) {
		return -1;
	}

	E_DEBUG2("data_len=[%d]", data_len);


	if (whichsig == 0) {
		/*
		* sanity check
		*/
		if (data_len < sigsize) {
			return -3;
		}
	}

	E_DEBUG("");
	if (whichsig > 0) {
		/*
		* sanity check
		*/
		if (data_len < (whichsig * sigsize)) {
			fprintf(stderr, "%s: invalid signature number specified\n", swlib_utilname_get());
			return -4;
		}
	}

	E_DEBUG("");
	if (data_len % sigsize) {
		/*
		* sanity check
		*/
		E_DEBUG("error");
		return -5;
	}

	nsig = data_len / sigsize;
	E_DEBUG2("number of sigs are [%d]", nsig);

	if (nsig < 1) {
		/*
		* sanity check
		*/
		E_DEBUG("error");
		return -6;
	}



	if (whichsig == 0) {
		E_DEBUG("whichsig = 0");
		which_offset = (nsig-1) * sigsize;
		start_sig = end_sig = nsig;
	} else if (whichsig > 0) {
		E_DEBUG("whichsig > 0");
		which_offset = (whichsig-1) * sigsize;
		start_sig = end_sig = whichsig;
	} else {
		E_DEBUG("whichsig < 0");
		start_sig=1;
		end_sig = nsig;	
	}

	while (start_sig++ <= end_sig) {
		s = data + ((start_sig-2) * sigsize);
		E_DEBUG2("((start_sig-2) * sigsize) = %d", ((start_sig-2) * sigsize));
		while (cnt < sigsize) {
			switch ((int)(*s)) {
				case '\0':
					if (!foundchar) {
						fprintf(stderr,
							"%s: Corrupt char in sigfile\n", swlib_utilname_get());
						return 3;
					}
					foundnull++;
					break;
	
				default:
					if (foundnull) {
						fprintf(stderr,
							"%s: Corrupt char in sigfile\n", swlib_utilname_get());
						return 2;
					}
					if ( !isprint((int)(*s)) && 
							*s != '\x0a' && *s != '\x0d') {
						fprintf(stderr, 
							"%s: Corrupt char in sigfile\n", swlib_utilname_get());
						return 1;
					}
					foundchar++;
					break;
			}
			s++;
			cnt++;
		}
	}


	/*
	 * A trailing NUL is optional
	 */
	return 0;
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
	intmax_t sigsize;
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

static
int
read_catalog_section(void * vp, int ifd)
{
	XFORMAT * package = (XFORMAT *)vp;
	int nullfd;
	int copyret;
	int ret;
	int retval = 0;
        int path_ret;
	int is_catalog;
	STROB * namebuf = strob_open(100);
	char * name;
	SWPATH * swpath = swpath_open("");
	int header_bytes;

	E_DEBUG("ENTERING");
	nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	if (!swpath) return -21;
	if (nullfd < 0) return -22;		
	if (ifd < 0) return -32;		

	taruib_set_fd(nullfd);
	taruib_set_overflow_release(0);
	while ((ret = xformat_read_header(package)) > 0) {
		header_bytes = ret;
		retval += ret;
		if (xformat_is_end_of_archive(package)){
			/* No longer an error in order to support
			   packages with no payload */

			taruib_clear_buffer();
			swpath_close(swpath);
			strob_close(namebuf);
			swbis_devnull_close(nullfd);
			return 0;
		}
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		path_ret = swpath_parse_path(swpath, name);
		if (path_ret < 0) {
			/*
			* Fatal error
			*/
			fprintf(stderr, 
				"read_catalog_section:"
					" error parsing path [%s].\n", name);
			return -2;
		}
		is_catalog = swpath_get_is_catalog(swpath);
		if (is_catalog != SWPATH_CTYPE_STORE) {
			/*
			* Either -1 leading dir, or 1 catalog.
			*
			* skip it.
			*/
			
			taruib_set_overflow_release(0); /* OFF */
			taruib_set_fd(0); /* Turn OFF */
			copyret = xformat_copy_pass(package, nullfd, ifd);
			if (copyret < 0) {
				swpath_close(swpath);
				strob_close(namebuf);
				swbis_devnull_close(nullfd);
				return copyret;
			}
			taruib_set_fd(nullfd);
			taruib_clear_buffer();
		} else {
			taruib_clear_buffer();
			swpath_close(swpath);
			strob_close(namebuf);
			swbis_devnull_close(nullfd);
			return 0;
		}
	}
	taruib_clear_buffer();
	swpath_close(swpath);
	strob_close(namebuf);
	swbis_devnull_close(nullfd);
	return 1;
}

static
int
taruib_write_signedfile_if_i(void * vp, int ofd,
				char * sigfile,
				int md5digestfd,
				int sha1digestfd,
				int sizefd,
				int sigfd, 
				int sha512digestfd, 
				int * p_has_storage_section)
{
	XFORMAT * package = (XFORMAT *)vp;
	int i;
	int nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	int memfd = -1;
	int ifd = xformat_get_ifd(package);
	int passfd;
	int copyret;
	int ret;
	int retval = 0;
	int is_catalog;
	char * name;
	char * prepath;
	int debug = 0;
	int header_bytes;
	int pattern_code = 0;
	int verbose = 1;
	int path_ret;
	int detected_pattern_code;
	STROB * namebuf = strob_open(100);
	STROB * oldnamebuf = strob_open(100);
	SWPATH * swpath = swpath_open("");
	int useArray[TARUIB_N_MAX_INDEX+1];
	int sigsize = 0;
	int sighdr_len = 0;
	int got_md5 = 0;
	int got_sha1 = 0;
	int got_sha512 = 0;
	int got_size = 0;
	int got_sig = 0;
	char * sighdr_addr = NULL;

	E_DEBUG("ENTERING");
	*p_has_storage_section = 1;

	/* The TARUIB_N_* defines are indexes for this array.
		 See taruib.h */
	for(i=0; i<=TARUIB_N_MAX_INDEX; i++) useArray[i] = 0;

	if (!swpath) return -21;
	if (nullfd < 0) return -22;		
	if (ifd < 0) return -32;		

	taruib_set_fd(nullfd);
	taruib_set_overflow_release(0);

	taru_set_header_recording(package->taruM, 1/*ON*/);
	while ((ret = xformat_read_header(package)) > 0) {
		header_bytes = ret;
		retval += ret;
		if (xformat_is_end_of_archive(package)) {
			/* This code path happens for packages with
			   no storage section */

			taruib_set_fd(ofd); /* Turn ON */
			taruib_clear_buffer();
			retval = ret;
			taruib_set_fd(0); /* turn pass-thru buffer off */
			*p_has_storage_section = 0;
			break;

			/*
			swbis_devnull_close(nullfd);
			swpath_close(swpath);
			strob_close(oldnamebuf);
			strob_close(namebuf);
			return -1;
			*/
		}
		strob_strcpy(oldnamebuf, strob_str(namebuf));
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		path_ret = swpath_parse_path(swpath, name);
		if (path_ret < 0) {
			fprintf(stderr,
				"taruib_write_signedfile_if:"
				" error parsing path [%s].\n", name);
			E_DEBUG("LEAVING");
			return -2;
		}

		is_catalog = swpath_get_is_catalog(swpath);
		if (debug) fprintf(stderr, "%s : %d\n", name, is_catalog); 
		prepath = swpath_get_prepath(swpath);
			
		/*
		 * Include the the leading directories in the storage file.
		 */
		if (is_catalog == SWPATH_CTYPE_CAT) {
			/* catalog section */

			taruib_set_overflow_release(0); /* OFF */
			taruib_set_fd(0); /* Turn OFF */

			passfd = verbose_decode(package, useArray, 
						&detected_pattern_code,
						prepath, name, nullfd, 
						pattern_code, verbose,
						md5digestfd,
						sha1digestfd,
						sizefd,
						sigfd, sha512digestfd);
			pattern_code = detected_pattern_code;	

			if (passfd < 0) {
				E_DEBUG2("name=[%s] passfd < 0 error", name);

				/* More than one sig or digest file in the 
				catalog. This is sanity security check.  */

				copyret = -1;
			} else if (pattern_code == TARUIB_N_OTHER) {
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				copyret = xformat_copy_pass(package,
								passfd,
								ifd);
			} else if (pattern_code == TARUIB_N_SIG_HDR) {
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				if (got_sig) {
					/* Sanity check, error.  */
					return -3;
				}
				memfd = swlib_open_memfd();
				copyret = xformat_copy_pass(package,
								memfd,
								ifd);
				sighdr_addr = uxfio_get_fd_mem(memfd,
							&sighdr_len);

				if (sighdr_addr == NULL ||
					(
					sighdr_len != 512 /*  && */
					/* sighdr_len != 1024   BAD CHECK, confused with .../signature size.*/
					)
					) {
					/* Sanity check, error.  */
					return -4;
				}			
			} else if (pattern_code == TARUIB_N_SIG) {
				/* As a sanity and security check,
				 check the header for signs of tampering.
				 since the signature archive header is 
				 not in the signed stream.  */

				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				if ((sigsize=do_header_safety_checks(package,
						strob_str(namebuf),
						strob_str(oldnamebuf),
						sighdr_addr
						)) < 0) {
					/* Error or corruption or tampering.  */

					fprintf(stderr, 
"\nWarning: *** The signature header checks failed (status=%d)\n"
"         *** This indicates possibility of tampering or corruption.\n"
"         *** Use of this package is a security risk.\n", sigsize);
					
					return -5;
				}

				/*
				* Write the sigfile.
				*/
				got_sig++;
				copyret = xformat_copy_pass(package, 
							passfd, ifd);

				if ((sigsize != 512 && sigsize != 1024) || 
					sigsize != copyret) {
					fprintf(stderr, 
"\nWarning: *** The signature file size and data length does not match. (%d %d)\n"
"         *** This indicates possibility of tampering or corruption.\n"
"         *** Use of this package is a security risk.\n",
						sigsize, copyret);
						return -7;
				}
			} else if (pattern_code == TARUIB_N_MD5) {
				/* Got the md5sum  */
				got_md5++;
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				copyret = xformat_copy_pass(package,
							passfd,
							ifd);
			} else if (pattern_code == TARUIB_N_SHA1) {
				/* Got the sha1 */
				got_sha1++;
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				copyret = xformat_copy_pass(package,
							passfd,
							ifd);
			} else if (pattern_code == TARUIB_N_SHA512) {
				/* Got the sha512 */
				got_sha512++;
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				copyret = xformat_copy_pass(package,
							passfd,
							ifd);
			} else if (pattern_code == TARUIB_N_SIZE) {
				/* Got the size attribute */
				got_size++;
				E_DEBUG3("name=[%s] pattern code = %d", name, pattern_code);
				copyret = xformat_copy_pass(package,
							passfd,
							ifd);
			} else if (passfd == nullfd) {
				/* FIXME: This copies the padding and data
				   slack bytes too (which does not matter for
				   tar formats).  */

				copyret = xformat_copy_pass(package,
								passfd,
								ifd);
			} else {
				fprintf(stderr, "internal error loc = 5a\n");
				copyret = -1;
			}

			if (copyret < 0) {
				swpath_close(swpath);
				strob_close(oldnamebuf);
				strob_close(namebuf);
				swbis_devnull_close(nullfd);
				return -8;
			}
			E_DEBUG3("copyret = %d for %s", copyret, name);
			taruib_set_fd(nullfd);
			taruib_clear_buffer();

		} else if (is_catalog == SWPATH_CTYPE_DIR) {
			/* leading dir  -- pass it thru.  */

			if (debug) fprintf(stderr, "in leading dirs\n"); 
			taruib_set_fd(ofd);
			taruib_clear_buffer();
		} else {
			/* the storage structure.  */

			taruib_set_fd(ofd); /* Turn ON */
			taruib_clear_buffer();
			retval = ret;
			taruib_set_fd(0); /* turn pass-thru buffer off */

			/* Now break */
			break;
		}

		/* Prepare to read the next header */

		taruib_set_overflow_release(1); /* ON */
	}

	taru_set_header_recording(package->taruM, 0 /*OFF*/);
	strob_close(oldnamebuf);
	strob_close(namebuf);
	swpath_close(swpath);
	swbis_devnull_close(nullfd);
	if (got_sha1 && !got_md5) {
		/* anomaly, disallow it */
		return -9;
	}
	if (!got_sig && !got_md5 && !got_sha1) {
		return -TARUIB_TEXIT_NOT_SIGNED;
	}
	if (!got_sig || !got_md5) {
		/* Dead code. Here for safety  */
		return -10;
	}
	if (sigsize == 0) {
		return -11;
	}
	if (got_sig && got_md5) {
		/* sha1 is optional to preserve
		   back compatibility. */

		return sigsize;
	}
	return -12;
}

static void
truncate_sigfile(char * sigfile)
{
	int sigfd = open(sigfile, O_RDWR|O_TRUNC|O_CREAT, 0600);
	E_DEBUG("ENTERING");
	if (sigfd < 0) {
		fprintf(stderr, "open failed ( %s ) : %s\n",
				sigfile, strerror(errno));
		return;
	}
	close(sigfd);
	return;
}


static
int
write_selected_sig_i(int sigfd, int memfd, int whichsig, int sigsize)
{
	/* int len; */
	int offset;
	int ret;
	char * s;
	char * sigbuf;
	char * nsig;

	if (whichsig == 0) {
		return -1;
	} else if (whichsig < 0) {
		return -1;
	} else {
		/*
		* 1 is first, 2 is second,...
		*/
		offset = sigsize * (whichsig - 1);
	}

	if (uxfio_lseek(memfd, (off_t)0, SEEK_END) < 0) {
		return -1;
	}
	uxfio_write(memfd, "\0", 1);
	
	sigbuf = uxfio_get_fd_mem(memfd, NULL);
	nsig = sigbuf + offset;	
	if ((s=strstr(nsig, "\n\n\n\n\n\n\n\n\n\n")) != NULL) {
		/* Now strip off trailing empty lines so that when
		the 'swverify -G /dev/tty' is used the signature
		block doesn't go scrolling by */

		*(s+4) = '\0';
		ret = uxfio_write(sigfd, nsig, strlen(nsig));
		if (ret != (int)strlen(nsig))
			return -1;
	} else {
		ret = uxfio_write(sigfd, nsig, sigsize);
		if (ret != sigsize)
			return -1;
	}

	return 0;
}


static
int
write_selected_sig(int sigfd, int memfd, int whichsig, int sigsize)
{
	int len;
	int start_sig, end_sig;
	int ret;
	
	len = uxfio_lseek(memfd, 0, SEEK_END);
	if (len < 0) return -2;

	end_sig = len / sigsize;

	if (whichsig == 0) {
		/* Last sig */
		start_sig = end_sig;
	} else if (whichsig < 0) {
		/* all sigs */
		start_sig = 1;
	} else {
		/*
		* 1 is first, 2 is second,...
		*/
		start_sig = whichsig;
		end_sig = whichsig;
	}
	while (start_sig++ <= end_sig) {
		ret = write_selected_sig_i(sigfd, memfd, start_sig-1, sigsize);
		if (ret < 0) return -1;
	}
	return 0;
}



static
void
report_dig_error(STROB * buf, int msgfd, int efd, char * digname, int dlen, int verbose)
{
	if (verbose >= DIG_VERBOSE_LEVEL)
		swlib_writef(msgfd, buf, "%s FAIL (Bad)\n", digname);
	if (dlen > 0) {
		swlib_writef(efd, buf, "%s: package integrity error: archive %s\n", swlib_utilname_get(), digname);
	}
	if (dlen == 0) {
		fprintf(stderr, "%s: Warning: %s digest not present\n", swlib_utilname_get(), digname);
	}

}

/* -------------------------------------------------------------- */
/*  Public Routines.                                              */
/* -------------------------------------------------------------- */

/*
* write the sigfile to (char*)sigfile and check the md5sum,
* if valid then write the signed file to ofd.
*/
int
taruib_write_signedfile_if(void * vp, int ofd, char * sigfile,
			int verbose, int whichsig, int logger_fd)
{
	pid_t pid[3];
	int status[3];
	int md5dig[2];
	int sha1dig[2];
	int sha512dig[2];
	int filesizefd[2];
	int digfile[2];
	int sigpipefd[2];
	int msgfd;
	char md5digbuf[1024];
	char sha1digbuf[1024];
	char sha512digbuf[1024];
	char sizebuf[1024];
	char calc_md5digbuf[64];
	char calc_sha1digbuf[64];
	char calc_sha512digbuf[129];
	char calc_size[64];
	XFORMAT * package = (XFORMAT *)vp;
	int sigsize;
	int ifd = xformat_get_ifd(package);
	int efd;
	int sigfd;
	int ret;
	int pri_ret;
	int memfd;
	int ifret;
	int has_storage_section;
	unsigned long actual_read_size;
	unsigned long claimed_size;
	STROB * tmp;


	E_DEBUG("ENTERING");
	E_DEBUG2("ENTERING whichsig=[%d]", whichsig);
	msgfd = logger_fd;
	efd = STDERR_FILENO;
	tmp = strob_open(32);
	memset(sizebuf, '\0', sizeof(sizebuf));
	memset(sha1digbuf, '\0', sizeof(sha1digbuf));
	memset(sha512digbuf, '\0', sizeof(sha512digbuf));
	memset(md5digbuf, '\0', sizeof(md5digbuf));
	memset(calc_sha1digbuf, '\0', sizeof(calc_sha1digbuf));
	memset(calc_sha512digbuf, '\0', sizeof(calc_sha512digbuf));
	memset(calc_md5digbuf, '\0', sizeof(calc_md5digbuf));
	memset(calc_size, '\0', sizeof(calc_size));
	
	E_DEBUG("");
	if ((ret=uxfio_lseek(ifd, 0, UXFIO_SEEK_VCUR)) != 0) {
		fprintf(stderr, "uxfio_lseek error fd=%d loc=1 ret=%d\n",
								ifd, ret);
		return 1;
	}

	/* This has the affect of reading the catalog portion into memory.  */

	E_DEBUG("");
	uxfio_fcntl(ifd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);
	if ((ret=read_catalog_section(vp, ifd))) {
		fprintf(stderr, "internal error from read_catalog_section, ret=%d\n", ret);
		return 2;
	}

	/* Now seek back to the beginning.  */

	E_DEBUG("");
	if ((ret = uxfio_lseek(ifd, 0, UXFIO_SEEK_VSET)) != 0) {
		fprintf(stderr, "uxfio_lseek error loc=2 ret=%d\n", ret);
		return 3;
	}


	E_DEBUG("");
	pipe(digfile);
	pipe(sha1dig);
	pipe(sha512dig);
	pipe(filesizefd);
	pipe(md5dig);
	pipe(sigpipefd);
	pid[0] = swfork((sigset_t*)(NULL));
	if (pid[0] < 0) return 1;
	if (pid[0] == 0) {
		int cret;
		close(filesizefd[0]);
		close(sha1dig[0]);
		close(sha512dig[0]);
		close(md5dig[0]);
		close(digfile[0]);
		close(sigpipefd[0]);
		E_DEBUG("entering taruib_write_signedfile_if_i in child");
		cret = taruib_write_signedfile_if_i(vp, digfile[1],
					sigfile, md5dig[1],
					sha1dig[1], filesizefd[1], sigpipefd[1], sha512dig[1],
					&has_storage_section);
		E_DEBUG2("taruib_write_signedfile_if_i cret=%d", cret);
		if (cret <= 0) {
			/*
			* error
			*/
			E_DEBUG2("exiting with value cret=%d", cret);
			truncate_sigfile(sigfile);
			if (cret == 0)
				_exit(TARUIB_TEXIT_ERROR_CONDITION_0);
			else
				_exit(TARUIB_TEXIT_ERROR_CONDITION_1);
		}
		sigsize = cret;
		uxfio_fcntl(ifd, UXFIO_F_SET_BUFACTIVE, UXFIO_OFF);

		if (has_storage_section) {
			if (xformat_file_has_data(package)) {
				E_DEBUG("xformat_file_has_data is true");
				E_DEBUG("into xformat_copy_pass2");
				if (xformat_copy_pass2(package, digfile[1], 
								ifd, -1) < 0) {
					fprintf(stderr, "taruib_write_signedfile_if:"
						" error from xformat_copy_pass2\n");	
					truncate_sigfile(sigfile);
					_exit(TARUIB_TEXIT_ERROR_CONDITION_2);
				}
				E_DEBUG("out of xformat_copy_pass2");
			}
		}
		close(sigpipefd[1]);
		E_DEBUG("entering taruib_write_pass_files");
		cret = taruib_write_pass_files(vp, digfile[1], -1);
		E_DEBUG2("taruib_write_pass_files exited cret=%d", cret);
		if (cret < 0) 
			_exit(TARUIB_TEXIT_ERROR_CONDITION_3);
		E_DEBUG2("sigsize=%d", sigsize);
		if (sigsize == 512) _exit(TARUIB_TEXIT_512);
		if (sigsize == 1024) _exit(TARUIB_TEXIT_1024);
		_exit(TARUIB_TEXIT_ERROR_CONDITION_4);
	}
	close(filesizefd[1]);
	close(sha1dig[1]);
	close(sha512dig[1]);
	close(md5dig[1]);
	close(digfile[1]);
	close(sigpipefd[1]);

	E_DEBUG("");
	calc_sha1digbuf[0] = '\0';
	calc_sha512digbuf[0] = '\0';
	calc_md5digbuf[0] = '\0';
	E_DEBUG("");

	/* Now calculate the digests from the archive */

	ret = swlib_digests(digfile[0], calc_md5digbuf, calc_sha1digbuf, calc_size, calc_sha512digbuf);
	if (ret) {
		fprintf(stderr, "taruib_write_signedfile_if: error reading archive data\n");
		return 15;
	}

	md5digbuf[0] = '\0';
	sha1digbuf[0] = '\0';
	sha512digbuf[0] = '\0';

	/* read the md5 digest from catalog/dfiles/md5sum */
	
	E_DEBUG("");
	if ((ret=safeio(read, md5dig[0], md5digbuf, 513)) < 0) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading ascii digest loc=1\n");
		truncate_sigfile(sigfile);
		return 4;
	}

	E_DEBUG("");
	if (ret < 0 || (ret > 0 && ret < MD5_ASCII_LEN)) {
		fprintf(stderr, "taruib_write_signedfile_if:"
		" internal error reading ascii digest loc=2, ret=%d\n", ret);
		truncate_sigfile(sigfile);
		return 5;
	}

	/* read the sha1 digest from catalog/dfiles/sha1sum */

	E_DEBUG("");
	if ((ret=safeio(read, sha1dig[0], sha1digbuf, 513)) < 0) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading ascii sha1 digest loc=1\n");
		truncate_sigfile(sigfile);
		return 6;
	}
	
	E_DEBUG("");
	if ((ret=safeio(read, sha512dig[0], sha512digbuf, 513)) < 0) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading ascii sha512 digest loc=1\n");
		truncate_sigfile(sigfile);
		return 10;
	}

	E_DEBUG("");
	if (ret < 0 || (ret > 0 && ret < SHA1_ASCII_LEN)) {
		fprintf(stderr, 
			"taruib_write_signedfile_if:"
			" internal error reading ascii sha1 digest loc=2,"
			" ret=%d\n",
				 ret);
		truncate_sigfile(sigfile);
		return 7;
	}

	/* read the size from catalog/dfiles/size */

	E_DEBUG("");
	if ((ret=safeio(read, filesizefd[0], sizebuf, 513)) < 0) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading ascii size loc=1\n");
		truncate_sigfile(sigfile);
		return 16;
	}
	swlib_squash_all_trailing_vnewline(sizebuf);	

	E_DEBUG("");
	if (ret < 0 || (ret > 0 && ret < 30) /* FIXME: arbitrary sanity check */ ) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading ascii size loc=2,"
			" ret=%d\n", ret);
		truncate_sigfile(sigfile);
		return 17;
	}

	/* Read the sigfile from catalog/dfiles/signature into memory.  */
	
	E_DEBUG("");
	memfd = swlib_open_memfd();
	ret = swlib_pipe_pump(memfd, sigpipefd[0]);
	if (ret < 0) {
		fprintf(stderr, "taruib_write_signedfile_if:"
			" internal error reading sigpipdfd,  ret=%d\n", ret);
		return 18;
	}
	E_DEBUG2("ret from sigpipefd is [%d]", ret);

	if (uxfio_lseek(memfd, 0, SEEK_SET) != 0) {
		fprintf(stderr, "taruib_write_signedfile_if: internal error : uxfio_lseek\n");
		truncate_sigfile(sigfile);
		return 8;
	}

	E_DEBUG("");
	close(sigpipefd[0]);
	close(digfile[0]);
	close(md5dig[0]);
	close(filesizefd[0]);
	close(sha1dig[0]);
	close(sha512dig[0]);
	swlib_wait_on_all_pids(pid, 1, status, 0 /*was WNOHANG, oops */, 0);
	if (WIFEXITED(status[0]) == 0) {
		fprintf(stderr, "%s: %d exited abnormally : status = %d\n",
			swlib_utilname_get(), (int)pid[0], status[0]);
		return 9;
	}
	ifret = WEXITSTATUS(status[0]);
	
	if (ifret == TARUIB_TEXIT_NOT_SIGNED) {
		/*
		* Normal error for unsigned package.
		*/
		fprintf(stderr, "%s: Package not signed.\n", swlib_utilname_get());
		return 12;
	}
	
	E_DEBUG2("ifret=%d", ifret);
	if (ifret != TARUIB_TEXIT_512 && ifret != TARUIB_TEXIT_1024) {
		/*
		* abnormal exit.
		*/
		E_DEBUG("");
		fprintf(stderr, "%s: pid[%d] exited with ifret=%d\n",
			swlib_utilname_get(), (int)pid[0], ifret);
		return 13;
	}

	E_DEBUG("");
	if (ifret == TARUIB_TEXIT_512) 
		sigsize = 512;
	else
		sigsize = 1024;

	if (verbose >= DIG_VERBOSE_LEVEL) {
		swlib_writef(msgfd, tmp, "%s: Archive digest: ", swlib_utilname_get());
	}

	pri_ret = 1;
	calc_md5digbuf[MD5_ASCII_LEN] = '\0';
	md5digbuf[MD5_ASCII_LEN] = '\0';
	if (strlen(md5digbuf) == MD5_ASCII_LEN && 
			strncmp(calc_md5digbuf, md5digbuf, MD5_ASCII_LEN) == 0) {
		pri_ret = 0;
		if (verbose >= DIG_VERBOSE_LEVEL) {
			swlib_writef(msgfd, tmp, "md5 OK (Good)\n");
		}
	} else {
		/*
		* md5sum mismatch.
		*/
		pri_ret = 2;
		if (verbose >= 1) {
			report_dig_error(tmp, msgfd, efd, "md5", strlen(md5digbuf), verbose);
		}
	}

	E_DEBUG("");
	if (verbose >= DIG_VERBOSE_LEVEL) {
		swlib_writef(msgfd, tmp, "%s: Archive digest: ", swlib_utilname_get());
	}
	calc_sha1digbuf[SHA1_ASCII_LEN] = '\0';
	sha1digbuf[SHA1_ASCII_LEN] = '\0';
	E_DEBUG("");
	if (strlen(sha1digbuf) == SHA1_ASCII_LEN && 
			strncmp(calc_sha1digbuf, sha1digbuf, SHA1_ASCII_LEN) == 0) {
		if (verbose >= DIG_VERBOSE_LEVEL) {
			swlib_writef(msgfd, tmp, "sha1 OK (Good)\n");
		}
	} else {
		/*
		* sha1 mismatch.
		*/
		report_dig_error(tmp, msgfd, efd, "sha1", strlen(md5digbuf), verbose);
		if (strlen(sha1digbuf)) {
			pri_ret = 1;
		}
	}

	E_DEBUG("");
	/* Compare the lengths of the digested data */
	if (verbose >= DIG_VERBOSE_LEVEL) {
		swlib_writef(msgfd, tmp, "%s: Archive size: ", swlib_utilname_get());
	}
	E_DEBUG("");
	if (strlen(sizebuf)) {
		int size_failed;
		char *e;

		size_failed = 1;
		E_DEBUG2("calc_size is [%s]", calc_size);
		actual_read_size = strtoul(calc_size, &e , 10);
		/* sanity checks */
		if ((int)(e - calc_size) == 0 || actual_read_size == 0) {
			pri_ret = 1;
		}

		E_DEBUG2("file size is [%s]", sizebuf);
		claimed_size = strtoul(sizebuf, &e , 10);
		/* sanity checks */
		if ((int)(e - calc_size) == 0 || claimed_size == 0) {
			pri_ret = 1;
		}

		if (claimed_size != actual_read_size) {
			pri_ret = 1;
			size_failed = 1;
		} else {
			size_failed = 0;
		}

		if (size_failed) {
			if (verbose >= 1) {
				report_dig_error(tmp, msgfd, efd, "size", -1, verbose);
			}
		} else {
			if (verbose >= DIG_VERBOSE_LEVEL) {
				swlib_writef(msgfd, tmp, "OK (Good)\n");
			}
		}
	} else {
		if (verbose >= 2) {
			swlib_writef(efd, tmp, "Warning, the package does not contain an archive size attribute.\n");
		}
	}

	E_DEBUG("");
	sha512digbuf[SHA512_ASCII_LEN] = '\0';
	calc_sha512digbuf[SHA512_ASCII_LEN] = '\0';
	

	if (verbose >= DIG_VERBOSE_LEVEL) {
		swlib_writef(msgfd, tmp, "%s: Archive digest: ", swlib_utilname_get());
	}

	E_DEBUG("");
	if (strlen(sha512digbuf) == SHA512_ASCII_LEN && 
			strncmp(calc_sha512digbuf, sha512digbuf, SHA512_ASCII_LEN) == 0) {
		if (verbose >= DIG_VERBOSE_LEVEL) {
			swlib_writef(msgfd, tmp, "sha512 OK (Good)\n");
		}
	} else {
		/*
		* sha512 mismatch.
		*/
		if (strlen(sha512digbuf)) {
			pri_ret = 1;
		}
		report_dig_error(tmp, msgfd, efd, "sha512", strlen(sha512digbuf), verbose);
	}

	
	E_DEBUG("");
	sigfd = open(sigfile, O_RDWR|O_TRUNC|O_CREAT, 0600);
	if (sigfd < 0) {
		fprintf(stderr, "open failed ( %s ) : %s\n",
						sigfile, strerror(errno));
		return 14;
	}

	E_DEBUG2("whichsig=%d", whichsig);
	if (pri_ret == 0) {
		/*
		* Look for evidence of trojan data hiding in the
		* sigfile.
		*/
		E_DEBUG("");
		if (do_sig_safety_checks(memfd, sigsize, whichsig)) {
			fprintf(stderr, "%s: error reading signature block\n", swlib_utilname_get());
			pri_ret = 10;
		} else {
			/*
			* OK, if were here then the archive md5 matched.
			* and the signature data looked OK. 
			* open and write the sigfile.
			swlib_pipe_pump(sigfd, memfd);
			*/
			write_selected_sig(sigfd, memfd, whichsig, sigsize);
		}
		/*
		* Close the sigfile.
		*/
		E_DEBUG("");
		close(sigfd);
		E_DEBUG("");
		uxfio_close(memfd);
		E_DEBUG("");

		if (verbose >= 4) {
			swlib_writef(logger_fd, tmp, "%s: size: %s from archive.\n",
						swlib_utilname_get(), sizebuf);
			swlib_writef(logger_fd, tmp, "%s: size: %s calculated.\n",
						swlib_utilname_get(), calc_size);
			swlib_writef(logger_fd, tmp, "%s: md5: %s from archive.\n",
						swlib_utilname_get(), md5digbuf);
			swlib_writef(logger_fd, tmp, "%s: md5: %s calculated.\n",
						swlib_utilname_get(), calc_md5digbuf);
			swlib_writef(logger_fd, tmp, "%s: sha1: %s from archive.\n",
						swlib_utilname_get(), sha1digbuf);
			swlib_writef(logger_fd, tmp, "%s: sha1: %s calculated.\n", 
						swlib_utilname_get(), calc_sha1digbuf);
			swlib_writef(logger_fd, tmp, "%s: sha512: %s from archive.\n",
						swlib_utilname_get(), sha512digbuf);
			swlib_writef(logger_fd, tmp, "%s: sha512: %s calculated.\n", 
						swlib_utilname_get(), calc_sha512digbuf);
		}
	
		E_DEBUG("");
		/*
		* Now emit the signed file out to the user.
		*/

		/*
		* Seek back to the start.
		*/
		E_DEBUG("");
		if ((ret = uxfio_lseek(ifd, 0, UXFIO_SEEK_VSET)) != 0) {
			fprintf(stderr, "uxfio_lseek error loc=3 ret=%d\n", 
									ret);
			return 10;
		}
		E_DEBUG("");
		uxfio_fcntl(ifd, UXFIO_F_ARM_AUTO_DISABLE, 1);

		E_DEBUG("");
		ret = taruib_write_catalog_stream(vp, ofd, 1 /*version*/, 
							0 /* verbose */);
		E_DEBUG("");
		if (ret <= 0) {
			pri_ret = 9;
		}
		E_DEBUG("");
	} else {
		/*
		* digest mismatch.
		* Emit empty file to gpg to make it happy
		*/
		E_DEBUG("");
		if (verbose > 2) {
			/* these are real errors */
			fprintf(stderr, 
			"swbis: md5: %s from archive.\n", md5digbuf);
			fprintf(stderr, 
			"swbis: md5: %s calculated.\n", calc_md5digbuf);
			fprintf(stderr, 
			"swbis: sha1: %s from archive.\n", sha1digbuf);
			fprintf(stderr, 
			"swbis: sha1: %s calculated.\n", calc_sha1digbuf);
		}
		E_DEBUG("");
		close(sigfd);
		E_DEBUG("");
		uxfio_close(memfd);
		E_DEBUG("");
	}
	strob_close(tmp);
	E_DEBUG2("ret=%d", pri_ret);
	return pri_ret;
}
