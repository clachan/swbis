/* uinfile.c: Open a package and fill in a UINFORMAT structure.
 */

/*
 * Copyright (C) 2003,2007 James H. Lowe, Jr.  <jhlowe@acm.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "ahs.h"
#include "uinfile.h"
#include "fnmatch_u.h"
#include "swlib.h"
#include "swutilname.h"
#include "swgp.h"
#include "swfork.h"
#include "u_ar.h"
#include "ls_list.h"
#include "swgpg.h"

#include "debug_config.h"
#define UINFILENEEDDEBUG 1
#undef UINFILENEEDDEBUG 

#ifdef UINFILENEEDDEBUG
#define UINFILE_E_DEBUG(format) SWBISERROR("UINFILE DEBUG: ", format)
#define UINFILE_E_DEBUG2(format, arg) \
				SWBISERROR2("UINFILE DEBUG: ", format, arg)
#define UINFILE_E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("UINFILE DEBUG: ", format, arg, arg1)
#else
#define UINFILE_E_DEBUG(arg)
#define UINFILE_E_DEBUG2(arg, arg1)
#define UINFILE_E_DEBUG3(arg, arg1, arg2)
#endif /* UINFILENEEDDEBUG */

#define UINFILE_I_INIT_READ_SIZE 124 /* Must be greater then SARHDR which is 60 from ar.h  */
#define UINFILE_I_TAR_BLOCK_SIZE 512

#define MIN_PEEK_LEN	3
#define LENGTH_OF_MAGIC	3  

STROB *
run_gpg_list_packets_command(unsigned char * xbuffer, int buffer_length)
{
	SHCMD * detect_gpg[2];
	STROB * detect_text;
	SHCMD * detect_command;
	SHCMD * recompress_command;
	int fd;
	int ret;
	
	detect_text = strob_open(32);
	/* Run this command
	gpg --list-only --list-packets  --status-fd 1
	*/
	detect_command = shcmd_open();
	shcmd_add_arg(detect_command, "gpg");
	shcmd_add_arg(detect_command, "--list-only");
	shcmd_add_arg(detect_command, "--list-packets");
	shcmd_add_arg(detect_command, "--status-fd");
	shcmd_add_arg(detect_command, "1");
	shcmd_set_errfile(detect_command, "/dev/null");

	detect_gpg[0] = detect_command;
	detect_gpg[1] = (SHCMD*)NULL;
	fd = swlib_open_memfd();
	if (fd < 0) return NULL;
	ret = uxfio_write(fd, (void*)xbuffer, (size_t)buffer_length);
	if (ret <= 0) return NULL;
	swlib_exec_filter(detect_gpg, fd, detect_text);
	if (strob_strlen(detect_text) < 2) return NULL;
	uxfio_close(fd);
	shcmd_close(detect_command);
	return detect_text;
}

static
int
is_gpg_data(void * gp)
{
	if (memcmp(gp, (void*)(UINFILE_MAGIC_gpg_sym), strlen(UINFILE_MAGIC_gpg_sym)) == 0) return 1;
	if (memcmp(gp, (void*)(UINFILE_MAGIC_gpg_enc1), strlen(UINFILE_MAGIC_gpg_enc1)) == 0) return 2;
	if (memcmp(gp, (void*)(UINFILE_MAGIC_gpg_enc2), strlen(UINFILE_MAGIC_gpg_enc2)) == 0) return 3;
	return 0;
}

static
int
is_gpg_packet(unsigned char * gp)
{
/*
	These tests are based on RFC4880
*/
	unsigned char b1;
	unsigned char b2;
	unsigned char packet_tag;

	
	b1 = *(gp+0);
	b2 = *(gp+1);

	if (!(b1 & (1 << 7)))
		return 0;  /* Bit 7 is not set */

	if (b1 & (1 << 6)) {
		/* New Packet */
		packet_tag = \
			(b1 & (1 << 5))*32 + \
			(b1 & (1 << 4))*16 + \
			(b1 & (1 << 3))*8 + \
			(b1 & (1 << 2))*4 + \
			(b1 & (1 << 1))*2 + \
			(b1 & (1 << 0))*1;
	} else {
		/* Old Packet */
		packet_tag = \
			(b1 & (1 << 5))*8 + \
			(b1 & (1 << 4))*4 + \
			(b1 & (1 << 3))*2 + \
			(b1 & (1 << 2))*1;
	}

	if (packet_tag == 0) 
		return 0;  /* Must not be a OpenPGP packet */

	if (
		packet_tag == 1 || /* 1        -- Public-Key Encrypted Session Key Packet */
		packet_tag == 3 || /* 3        -- Symmetric-Key Encrypted Session Key Packet */
		0
	) {
		return 1;
	} else {
		/* Nothing else is supported */
		;
	}	
	return 0;
}


static
int
does_have_name_version(char * buf)
{
	char * s;
	UINFILE_E_DEBUG2("buf=[%s]", buf);
	s = strchr(buf, '-');
	if (s == NULL) return 0;
	s++;
	if (
		isdigit((int)(*s)) &&
		!isdigit((int)(*buf))
	) {
		UINFILE_E_DEBUG("return 1");
		return 1;
	} else {
		UINFILE_E_DEBUG("return 0");
		return 0;
	}
}

static
int
ar_get_size(unsigned char * buf, int * value)
{
	int result;
	struct ar_hdr a;
	/* buf points to a GNU ar header */

	memcpy(&a, buf, sizeof(a));	
	if (memcmp(a.ar_fmag, ARFMAG, 2) != 0)
		return -1;
	a.ar_fmag[0] = '\0';
	*value = swlib_atoi((char*)(a.ar_size), &result);
	if (result != 0)
		return -1;
	return 0;
}

static 
int
determine_if_has_leading_slash(UINFORMAT * uinformat, char * buffer)
{
	int ret = 0;

	if (uinformat->typeM == USTAR_FILEFORMAT) {
		int eoa;
		struct new_cpio_header * file_hdr = taru_make_header();
		taru_read_in_tar_header2(uinformat->taruM, file_hdr,
					-1, buffer, &eoa, 0, TARRECORDSIZE);
		if (*ahsStaticGetTarFilename(file_hdr) == '/') ret = 1;
		taru_free_header(file_hdr);
	} else if (uinformat->typeM == CPIO_POSIX_FILEFORMAT) {
		/*
		* cpio odc format.
		*/
		if (*(buffer+70) == '/') ret = 1;
	} else {
		/*
		* newc, crc format.
		*/
		if (*(buffer+104) == '/') ret = 1;
	}
	return ret;
}

static int
uinfile_del_pid(UINFORMAT * uinformat, int pid)
{
	int i = 0;
	UINFILE_E_DEBUG("");
	while (i < (int)(sizeof(uinformat->pidlistM)/sizeof(int))) {
		if ((uinformat->pidlistM)[i] == pid) {
			(uinformat->pidlistM)[i] = 0;
			return 0;
		}
		i++;
	}
	return -1;
}

static int
uinfile_add_pid(UINFORMAT * uinformat, int pid)
{
	int i = 0;
	UINFILE_E_DEBUG("");
	while (i < (int)(sizeof(uinformat->pidlistM)/sizeof(int))) {
		if ((uinformat->pidlistM)[i] == 0) {
			(uinformat->pidlistM)[i] = pid;
			return 0;
		}
		i++;
	}
	return -1;
}

static
char *
uinfile_i_get_name(UINFORMAT * uinformat,
		struct new_cpio_header *file_hdr, int fd,
			int format, int * retval)
{
        *retval = taru_read_header(uinformat->taruM, file_hdr, fd,
						format, NULL, 0);
	if (*retval < 0)
		return NULL;
        return swlib_strdup(ahsStaticGetTarFilename(file_hdr));
}

static
int
uinfile_check_ieee_fd(int fd) {
	int current_buftype; 
	current_buftype = uxfio_fcntl(fd, UXFIO_F_GET_BUFTYPE, 0);
	if (
		current_buftype != UXFIO_BUFTYPE_DYNAMIC_MEM &&
		current_buftype != UXFIO_BUFTYPE_MEM &&
		current_buftype != UXFIO_BUFTYPE_FILE
		) {
		fprintf(stderr,
		"internal error, incorrect usage of uinfile_check_ieee\n");
		return -1;
	}
	if (current_buftype == UXFIO_BUFTYPE_MEM) {
		if (uxfio_fcntl(fd, UXFIO_F_SET_BUFFER_LENGTH, 3072) != 0)
			return -1;
	}
	return fd;
}

static
int
uinfile_detect_ieee(UINFORMAT * uinformat, int oflags)
{
	int ret;
	int nameretval;
	char * name = NULL;
	int mm=0;
	int lead_bytes = 0;
	int allow_generic_tar = oflags & UINFILE_DETECT_OTARALLOW;
	struct new_cpio_header *file_hdr=uinformat->file_hdrM;

	if (uinformat->swpathM)
		swpath_close(uinformat->swpathM);
	uinformat->swpathM = swpath_open("");

	if (uinfile_check_ieee_fd(uinformat->fdM) < 0) {
		fprintf(stderr, "uinfile: Incorrect uxfio fd settings.\n");
		return -1;
	}

	/* 
	 * Read the package and try to find
	 * the magic "catalog/INDEX" name 
	 */

	UINFILE_E_DEBUG("HERE");
	/* 
	 * read leading directories of package 
	 */
	while (
		mm < UINFILE_IEEE_MAX_LEADING_DIR && 
		((name=uinfile_i_get_name(uinformat, file_hdr,
				uinformat->fdM, uinformat->typeM,
						&nameretval)) != NULL)) {
			if (nameretval <= 0) {
				uinformat->layout_typeM =
					UINFILE_FILELAYOUT_UNKNOWN;
				fprintf (stderr,
					"%s: error: package format read error.\n",
						swlib_utilname_get());
				uxfio_close(uinformat->fdM);
				uinformat->fdM = -1;
				return -1;
			}		
			lead_bytes += nameretval;

			UINFILE_E_DEBUG2("parsing path [%s]", name);
			if (swpath_parse_path(uinformat->swpathM, name) < 0) {
				if (allow_generic_tar == 0) {
					fprintf (stderr,
			"uinfile: swpath_parse_path: error parsing: %s\n",
					name);
					uxfio_close(uinformat->fdM);
					uinformat->fdM = -1;
					return -1;
				} else {
					uinformat->layout_typeM =
						UINFILE_FILELAYOUT_UNKNOWN;
					break;
				}
			}

		if (!fnmatch("*/catalog/INDEX", name, 0) ||
				!fnmatch("catalog/INDEX", name, 0)) {
			UINFILE_E_DEBUG("HERE");
			uinformat->layout_typeM=UINFILE_FILELAYOUT_IEEE;
			break;
		}	
		if ((file_hdr->c_mode & CP_IFMT) != CP_IFDIR) {
			UINFILE_E_DEBUG("HERE");
			if (allow_generic_tar == 0) {
				UINFILE_E_DEBUG("HERE");
				uinformat->layout_typeM = 
						UINFILE_FILELAYOUT_UNKNOWN;
				fprintf (stderr,
			"%s: Package layout_version 1.0 not found\n", swlib_utilname_get());
				uxfio_close(uinformat->fdM);
				uinformat->fdM = -1;
				return -1;
			} else {
				uinformat->layout_typeM = 
						UINFILE_FILELAYOUT_UNKNOWN;
				break;
			}
		}
		mm++;
		swbis_free(name);
	}
	if ((mm >= UINFILE_IEEE_MAX_LEADING_DIR &&
			(allow_generic_tar == 0)) || name == NULL) {
		UINFILE_E_DEBUG("HERE");
		uinformat->layout_typeM = UINFILE_FILELAYOUT_UNKNOWN;
		fprintf (stderr,
		"%s: Package layout_version 1.0 not found.\n", swlib_utilname_get());
		uxfio_close(uinformat->fdM);
		uinformat->fdM = -1;
		return -1;
	}

	/* seek back to the begining of the header. */
	UINFILE_E_DEBUG("HERE");
	if ((ret=uxfio_lseek(uinformat->fdM,
				-lead_bytes, UXFIO_SEEK_VCUR)) != 0) {
		fprintf(stderr,
	"uxfio_lseek error in uinfile_handle_return 0015. off=%d ret=%d\n",
							lead_bytes, ret);
		return -1;
	}
	taruib_unread(lead_bytes);
	UINFILE_E_DEBUG("HERE");
	return uinformat->fdM;
}

static
int
uinfile_handle_return(UINFORMAT * uinformat, int oflags, int uxfio_buffer_type)
{
	struct new_cpio_header *file_hdr=uinformat->file_hdrM;

	UINFILE_E_DEBUG("");
	UINFILE_E_DEBUG2("flags = %d", oflags);
	UINFILE_E_DEBUG2("uxfio_buffer_type = %d", uxfio_buffer_type);

	ahsStaticSetTarFilename(file_hdr, NULL);
	ahsStaticSetPaxLinkname(file_hdr, NULL);

	UINFILE_E_DEBUG2("HERE current_pos_=%d", (int)((uinformat)->current_pos_));
	if (uxfio_lseek(uinformat->fdM, (off_t)(0), SEEK_SET) < 0) {
		UINFILE_E_DEBUG("HERE");
		fprintf(stderr,
		"uxfio_lseek error in uinfile_handle_return 0001. fd=%d\n",
			uinformat->fdM);
		return -1;
	}
	taruib_set_datalen(0);
	/*
	 * Generic TAR or cpio archive.
	 */

	UINFILE_E_DEBUG("HERE");
	if (oflags & UINFILE_DETECT_FORCEUNIXFD && 
				uinformat->fdM >= UXFIO_FD_MIN) {
		UINFILE_E_DEBUG("HERE: forking to make unix fd");
		uxfio_fcntl(uinformat->fdM, UXFIO_F_ARM_AUTO_DISABLE, 1);
		uinformat->fdM = swlib_fork_to_make_unixfd(uinformat->fdM,
					&(uinformat->blockmask_),
					&(uinformat->defaultmask_),
					(int*)NULL);
	} else if ( 
		(oflags & UINFILE_DETECT_FORCEUXFIOFD &&
					uinformat->fdM < UXFIO_FD_MIN)
		)
	{
		UINFILE_E_DEBUG("HERE: opendup with buffer type");
		uinformat->fdM = uxfio_opendup(uinformat->fdM,
							uxfio_buffer_type);
		uxfio_fcntl(uinformat->fdM, UXFIO_F_SET_CANSEEK, 0);
		/*
		UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(uinformat->fdM));
		*/
	} else if ( 
		(oflags & UINFILE_DETECT_FORCE_SEEK &&
					uxfio_espipe(uinformat->fdM) && 
		uinformat->fdM < UXFIO_FD_MIN )
		) 
	{
		UINFILE_E_DEBUG("HERE: 2:opendup with buffer type");
		uinformat->fdM = uxfio_opendup(uinformat->fdM,
							uxfio_buffer_type);
		/*
		UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(uinformat->fdM));
		*/
	} else if ( 
		(oflags & UINFILE_DETECT_FORCE_SEEK &&
					uxfio_espipe(uinformat->fdM) && 
		uinformat->fdM >= UXFIO_FD_MIN )
		) 
	{
		UINFILE_E_DEBUG("HERE: setting to buf type mem");
		uxfio_fcntl(uinformat->fdM, UXFIO_F_SET_BUFTYPE,
							UXFIO_BUFTYPE_MEM);
		/*
		UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(uinformat->fdM));
		*/
	} else {
		UINFILE_E_DEBUG("HERE: non of the above");
		;
	}
	if (uinformat->fdM >= UXFIO_FD_MIN) {
	UINFILE_E_DEBUG2("setting uxfio_buffer_type %d", uxfio_buffer_type);
		uxfio_fcntl(uinformat->fdM, UXFIO_F_SET_BUFTYPE,
							uxfio_buffer_type);
	}
	UINFILE_E_DEBUG2("returning fd=%d", uinformat->fdM);
	return uinformat->fdM;
}

static int
uinfile_i_open(char *filename, int oflag, mode_t mode, UINFORMAT ** uinformat,
		int oflags, int xdupfd, int uxfio_buffer_type, char * slack_name)
{
	int ret;
	int i;
	int fd, pipe_fd[2], dupfd;
	int refd;
	int zpipe[2];
	unsigned long tar_hdr_sum;
	pid_t pid;
	char xbuffer[1025];
	char gzmagic[] = UINFILE_MAGIC_gz;
	char Zmagic[] = UINFILE_MAGIC_Z;
	char rpmmagic[] = UINFILE_MAGIC_rpm;
	char bz2magic[] = UINFILE_MAGIC_bz2;
	char lzmamagic[] = UINFILE_MAGIC_lzma;
	char xzmamagic[] = UINFILE_MAGIC_xz;
	char debmagic[] = UINFILE_MAGIC_deb;
	char *zcat_command = (char *) (NULL);
	pid_t a_pid;
	intmax_t pump_amount = -1;
	int zret = 0;	
	int forcetar;
	int doieee;
	int dodebcontrol;
	int dodebcontext;
	int dodebdata;
	int doarb;
	int dounrpminstall;
	int douncpio;
	int donative;
	int peeklen;
	int deb_gz_size = 0;
	int deb_gz_offset = 0;
	int val1;
	int val2;
	int do_recompress;
	SHCMD * rezip;

	/*
	if (slack_name)
		*-*fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__);
	*/
	UINFILE_E_DEBUG("BEGIN ****************************** ");
	UINFILE_E_DEBUG2("xdupfd = %d", xdupfd);
	UINFILE_E_DEBUG2("slack_name = %s", slack_name);

	dodebcontrol = oflags & UINFILE_DETECT_DEB_CONTROL;
	dodebcontext = oflags & UINFILE_DETECT_DEB_CONTEXT;
	dodebdata = oflags & UINFILE_DETECT_DEB_DATA;
	do_recompress = oflags & UINFILE_DETECT_RECOMPRESS;

	UINFILE_E_DEBUG2("dodebdata=%d", dodebdata);
	UINFILE_E_DEBUG2("dodebcontext=%d", dodebcontext);
	UINFILE_E_DEBUG2("dodebcontrol=%d", dodebcontrol);

	forcetar = oflags & UINFILE_DETECT_OTARFORCE;
	doieee = oflags & UINFILE_DETECT_IEEE;
	doarb = oflags & UINFILE_DETECT_ARBITRARY_DATA;
	dounrpminstall = oflags & UINFILE_DETECT_UNRPM;
	douncpio = oflags & UINFILE_DETECT_UNCPIO;
	donative = oflags & UINFILE_DETECT_NATIVE;
	
	UINFILE_E_DEBUG2("forcetar=%d", forcetar);
	UINFILE_E_DEBUG2("donative=%d", donative);
	UINFILE_E_DEBUG2("dounrpminstall=%d", dounrpminstall);

	if (dodebdata) {
		;
		/* Special case, the uinfile struct is already created */
	} else {
		*uinformat=(UINFORMAT *)malloc(sizeof(UINFORMAT));
		if (!(*uinformat)) {
			return -1;
		}
		(*uinformat)->underlying_fdM = -1;
		(*uinformat)->current_pos_ = 0;
		(*uinformat)->file_hdrM=ahsStaticCreateFilehdr();
		(*uinformat)->verboseM = 0;
		(*uinformat)->has_leading_slashM = 0;
		(*uinformat)->ztypeM = UINFILE_COMPRESSED_NA;
		(*uinformat)->typeM = UNKNOWN_FILEFORMAT;
		memset((*uinformat)->type_revisionM, '\0', sizeof((*uinformat)->type_revisionM));
		(*uinformat)->swpathM = swpath_open("");
		(*uinformat)->taruM = taru_create();
		(*uinformat)->slackheaderM = NULL;
		UINFILE_E_DEBUG("seting deb_file_fd = -1");
		(*uinformat)->deb_file_fd_ = -1;
		(*uinformat)->n_deb_peeked_bytesM = 0;
		(*uinformat)->pathname_prefixM = NULL;
		(*uinformat)->recompress_commandsM = vplob_open();
		if (slack_name)
			(*uinformat)->slack_nameM = strdup(slack_name);
		else
			(*uinformat)->slack_nameM = NULL;
		sigemptyset(&((*uinformat)->blockmask_));
		sigemptyset(&((*uinformat)->defaultmask_));
		sigaddset(&((*uinformat)->blockmask_), SIGALRM);
		sigaddset(&((*uinformat)->defaultmask_), SIGTERM);
		sigaddset(&((*uinformat)->defaultmask_), SIGINT);
		sigaddset(&((*uinformat)->defaultmask_), SIGPIPE);
		for (i=0; i < (int)(sizeof((*uinformat)->pidlistM)/sizeof(int)); i++)
				(*uinformat)->pidlistM[i] = 0;
	}

	/* ------------------------------------------------------------- */
	/*            Open file or pipe                                  */
	/* ------------------------------------------------------------- */

	if (filename == (char *) (NULL) ||
				(!strcmp(filename, "-"))) {	/* is a pipe */
		UINFILE_E_DEBUG("HERE PIPE");

		if (filename == (char *) (NULL)) {
			UINFILE_E_DEBUG("HERE");
			dupfd = xdupfd;
		} else {
			UINFILE_E_DEBUG("HERE");
			dupfd = STDIN_FILENO;
		}
		UINFILE_E_DEBUG2("HERE dupfd = %d", dupfd);
		
		val1 = dupfd < UXFIO_FD_MIN ;
		if (dupfd < UXFIO_FD_MIN) {
			UINFILE_E_DEBUG("HERE:  lseek\'ing on dupfd");
			val2 = lseek(dupfd, 0L, SEEK_CUR);
		} else {
			val2 = -1;
		}
	
		if (val1 && val2 == -1) {
			UINFILE_E_DEBUG3("HERE %d %s", errno, strerror(errno));
			if (errno == ESPIPE){
				/*
				 * unseekable pipe
				 */
				/*
				UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(fd));
				*/
				fd = uxfio_opendup(dupfd, uxfio_buffer_type);
				/*
				UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(fd));
				*/
				if (fd < 0) {
					fprintf(stderr,"uinfile.c: uxfio_opendup failed\n");
					return -1;
				}
			} else {
				perror("uinfile: fatal error");
				return -1;
			}
		} else {
			UINFILE_E_DEBUG("HERE");
			ret = val2; /*lseek(dupfd, 0L, SEEK_CUR); */
			if (ret >= 0) {
				UINFILE_E_DEBUG2("HERE current_pos_=%d", (int)((*uinformat)->current_pos_));
				(*uinformat)->current_pos_ = ret;
				/*
				if ((*uinformat)->current_pos_) {
					fprintf(stderr,"uinfile.c: warning: current_pos_ not equal to zero\n");
				}
				*/
			} else {
				/*
				fprintf(stderr,"uinfile.c: warning: lseek failed\n");
				*/
			}
			fd = dupfd;
		}
		(*uinformat)->did_dupeM=1;
	} else {		/* is a reg file */
		UINFILE_E_DEBUG("HERE REG FILE");
		fd = open(filename, O_RDONLY, 0);
		if (fd < 0) {
			UINFILE_E_DEBUG("HERE error.\n");
			return fd;
		}
		dupfd = fd;
		(*uinformat)->did_dupeM=0;
	}
	refd = fd;
	(*uinformat)->underlying_fdM = refd;
LABEL_REREAD:
	(*uinformat)->fdM = refd;
	if ((*uinformat)->typeM == DEB_FILEFORMAT && dodebdata == 0) {
		/* FIXME, should go through handle_return() */
		return refd;
	}

	/*
	 * Initial read on the serial access archive.
	 * Fixme:
	 */
	if (dodebdata) {
		UINFILE_E_DEBUG("HERE");
		peeklen = SARHDR +
				 1 + /* 1 for possible padding */
				 strlen(UINFILE_MAGIC_gz);  /* for gz magic */
	} else if (doarb == 0) {
		UINFILE_E_DEBUG("HERE");
		peeklen = UINFILE_I_INIT_READ_SIZE;
	} else {
		/*
		 * this is used if DETECT_ABRITRARY_DATA is set
		 */
		UINFILE_E_DEBUG("HERE");
		peeklen = MIN_PEEK_LEN;
	}	

	memset(xbuffer, '\0', sizeof(xbuffer));

	UINFILE_E_DEBUG3("HERE refd=%d  peeklen=%d", refd, peeklen);
	if (doarb) {
		/* arbitray data, make no requirement that it be longer than MIN_PEEK_LEN */
		if ((ret=uxfio_read(refd, xbuffer, peeklen)) < 0) {
			UINFILE_E_DEBUG("ERROR HERE");
			fprintf(stderr, "%s: error in read of arbitrary data: request=%d return=%d\n",
				swlib_utilname_get(), peeklen,  ret);
			return -1;
		}
	}  else {
		if ((ret=taru_tape_buffered_read(refd, xbuffer, peeklen)) != peeklen) {
			UINFILE_E_DEBUG("ERROR HERE");
			if (ret <= 0) fprintf(stderr,
			"%s: error in initial read. request=%d return=%d\n",
				swlib_utilname_get(), peeklen,  ret);
			return -1;
		}
	}
	UINFILE_E_DEBUG("HERE");

	/*
	 * Identify the file type.
	 */
	if (doarb == 0) {
		/* Peeked Length is  UINFILE_I_INIT_READ_SIZE */
		if (0) {
			;
		} else if (dodebdata) {
			/* Peeked length is (SARHDR + 1 + strlen(UINFILE_MAGIC_gz)) */
			UINFILE_E_DEBUG("in dodebdata");
			/* the ar header of data.tar.gz should be in xbuffer */
			UINFILE_E_DEBUG("type: DEB (data)");
			ret = ar_get_size((unsigned char*)xbuffer, &val1);
			if ( ret != 0 ) {
				/* this may be caused by alignment byte padding in the ar format,
				   so try it again, if it fails again only then your screwed. */
				ret = ar_get_size((unsigned char*)xbuffer+1, &val1);
				if ( ret != 0 ) {
					fprintf(stderr,
						"%s: ar archive error %s: at line %d\n", swlib_utilname_get(), __FILE__, __LINE__);
					return -1;
				}
				UINFILE_E_DEBUG("in dodebdata with 1 byte of padding");
				deb_gz_offset = SARHDR + 1;
				/* for example "\n\037\213\b"
				use offset of deb_gz_offset - SARHDR with write size of 3 */

				(*uinformat)->n_deb_peeked_bytesM = strlen(UINFILE_MAGIC_gz);
				
			} else {
				UINFILE_E_DEBUG("in dodebdata with no padding");
				deb_gz_offset = SARHDR;
				/* for example "\037\213\b\xNN"
				use offset of deb_gz_offset - SARHDR with write size of 4 */

				(*uinformat)->n_deb_peeked_bytesM = strlen(UINFILE_MAGIC_gz) + 1;
				;
			}
			memcpy((void*)((*uinformat)->deb_peeked_bytesM), xbuffer+SARHDR, peeklen - SARHDR);
	
			deb_gz_size = val1;
			UINFILE_E_DEBUG2("deb_gz_size is %d", val1);
			ret = uxfio_lseek(refd, (off_t)(0), SEEK_CUR);
			if (ret < 0) {
				fprintf(stderr, "%s: ar archive error %s: at line %d\n", swlib_utilname_get(), __FILE__, __LINE__);
				return -1;
			}
			(*uinformat)->current_pos_ = ret;
			UINFILE_E_DEBUG2("current_pos_ is %d", ret);

		/* End of dodebdata */
		} else if (!memcmp(xbuffer, rpmmagic, 4) && !forcetar) {
			UINFILE_E_DEBUG("type: RPM");
			if (dounrpminstall == 0 && doieee == 0) {
				/*
				 * This is the old historic response to
				 * an RPM file.
				 */
				(*uinformat)->typeM=RPMRHS_FILEFORMAT;
				ret = uinfile_handle_return(*uinformat, oflags,
							uxfio_buffer_type);
				/*
				UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(ret));
				*/
				return ret;
			} else if (dounrpminstall) {
				/*
				 * Model the RPM as a compression type since this
				 * fits with the code below, e.g. instead of calling
				 * ''gzip -d'', call ''swpackage --unrpm''
				 */
				(*uinformat)->ztypeM = UINFILE_COMPRESSED_RPM;
			} else if (doieee && dounrpminstall == 0) {
				fprintf(stderr,
				"%s: try using the --allow-rpm or --any-format options\n",
						swlib_utilname_get());
				return -1;
			} else {
				SWLIB_EXCEPTION("");
				return -1;
			}
		} else if (douncpio && ((strncmp(xbuffer, "0707", 4) == 0) &&
				(1/*strncmp(xbuffer + 257, TMAGIC, 5)*/))) {
			/*
			 * Model the cpio format as a compression type since this
			 * fits with the code below, e.g. instead of calling
			 * ''gzip -d'', call ''arf2arf -H ustar''
			 */
			UINFILE_E_DEBUG("type: 0707");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_CPIO;
		} else if (!strncmp(xbuffer, "0707", 4) &&
				(1/*strncmp(xbuffer + 257, TMAGIC, 5)*/) &&
								!forcetar) {
			if (!strncmp(xbuffer, "070702", 6)) { /* new ascii crc */
				UINFILE_E_DEBUG("type: 070702");
				(*uinformat)->typeM = CPIO_CRC_FILEFORMAT;
			} else if (!strncmp(xbuffer, "070701", 6)) {/*new ascii*/
				UINFILE_E_DEBUG("type: 070701");
				(*uinformat)->typeM = CPIO_NEWC_FILEFORMAT;
			} else if (!strncmp(xbuffer, "070707", 6)) {/* POSIX.1 */
				UINFILE_E_DEBUG("type: 070707");
				(*uinformat)->typeM = CPIO_POSIX_FILEFORMAT;
			} else {
				SWLIB_EXCEPTION("");
				return -1;
			}
			(*uinformat)->has_leading_slashM =
				determine_if_has_leading_slash(*uinformat,
									xbuffer);
			ret =  uinfile_handle_return(*uinformat, oflags,
							uxfio_buffer_type);
			if (ret > 0 && doieee) {
				ret = uinfile_detect_ieee(*uinformat, oflags);
			}
			return ret;
		} else if (!memcmp(xbuffer, (void *) gzmagic, 2)) {
						/* gzipped compression */
			UINFILE_E_DEBUG("type: COMPRESSED_GZ");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GZ;
		} else if (!memcmp(xbuffer, (void *) bz2magic, LENGTH_OF_MAGIC)) {
							/* bz2 compression */
			UINFILE_E_DEBUG("type: COMPRESSED_BZ2");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_BZ2;
		} else if (!memcmp(xbuffer, (void *) lzmamagic, LENGTH_OF_MAGIC)) {
							/* LZMA compression */
			UINFILE_E_DEBUG("type: COMPRESSED_LZMA");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_LZMA;
		} else if (!memcmp(xbuffer, (void *) xzmamagic, LENGTH_OF_MAGIC)) {
							/* XZ compression */
			UINFILE_E_DEBUG("type: COMPRESSED_XZ");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_XZ;
		} else if (!memcmp(xbuffer, (void *) Zmagic, 2)) {
							/* Unix compress */
			UINFILE_E_DEBUG("type: COMPRESSED_Z");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_Z;
		} else if (is_gpg_data((void*)xbuffer)) {
			UINFILE_E_DEBUG("type: GPG Encrypted");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GPG;
		} else if (
			memcmp(xbuffer, (void *) UINFILE_MAGIC_gpg_armor,  strlen(UINFILE_MAGIC_gpg_armor)) == 0 ||
			is_gpg_packet((unsigned char*)xbuffer) ||	
			0
		) {
			STROB * detect_text;
			detect_text = run_gpg_list_packets_command((unsigned char*)xbuffer, peeklen);
			if (detect_text) strob_close(detect_text);
			UINFILE_E_DEBUG("type: GPG Encrypted");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GPG;
		} else if ( donative == 0 &&
			dounrpminstall == 0 &&
			memcmp(xbuffer, (void *)debmagic, strlen(UINFILE_MAGIC_deb)) == 0
		) {
			UINFILE_E_DEBUG("type: DEB");
			/* Great, its a deb package */

			(*uinformat)->ztypeM = UINFILE_COMPRESSED_DEB;
			(*uinformat)->typeM = DEB_FILEFORMAT;
			UINFILE_E_DEBUG2("seting deb_file_fd = %d", dupfd);
			(*uinformat)->deb_file_fd_ = dupfd;

			/* Store the 2.0\n which is the data of the first
			   archive member */
	
			strncpy((*uinformat)->type_revisionM,
				((char*)xbuffer)+68, 4);
			(*uinformat)->type_revisionM[4] = '\0';

			/* Now read some more bytes to include the gzip magic of
			   the control.tar.gz file */

			ret = ar_get_size((unsigned char*)xbuffer+8, &val1);
			if ( ret != 0 || val1 != 4 ) {
				fprintf(stderr, "%s: deb format bad size value\n", swlib_utilname_get());
				return -1;
			}

			/* Now we know for certain where the control.tar.gz file begins,
			   make sure we have peek'ed that far into the file and confirm
			   the gz magic is there */

			deb_gz_offset = SARMAG + SARHDR + val1 + SARHDR;  /* val1 must be 4 */
			if (peeklen < deb_gz_offset + LENGTH_OF_MAGIC) {

				/* read further into the file */
				ret = taru_tape_buffered_read(refd, xbuffer+peeklen, deb_gz_offset+LENGTH_OF_MAGIC - peeklen);
				if (ret != deb_gz_offset+LENGTH_OF_MAGIC - peeklen) {
					fprintf(stderr,
					"%s: error in second initial read. request=%d return=%d\n",
						swlib_utilname_get(), peeklen,  ret);
					return -1;
				}
				peeklen += ((deb_gz_offset+LENGTH_OF_MAGIC) - peeklen);
			}
			
			/* look for gzmagic */

			if (
				memcmp(((unsigned char*)xbuffer)+deb_gz_offset, (void *)gzmagic, LENGTH_OF_MAGIC) != 0 &&
				memcmp(((unsigned char*)xbuffer)+deb_gz_offset, (void *)xzmamagic, LENGTH_OF_MAGIC) != 0
			) {
				fprintf(stderr, "%s: neither gz or xz magic found for control.tar.gz\n", swlib_utilname_get());
				return -1;
			}
		
			/* Now get the size of the compressed control.tar.gz file */
	
			ret = ar_get_size(
					(unsigned char*)xbuffer + SARMAG + SARHDR + val1,  /* point to header beginning */
					&val2);
			if (ret != 0) {
				fprintf(stderr, "%s: bad size conversion\n", swlib_utilname_get());
				return -1;
			}

			if (uxfio_lseek(refd, (off_t)deb_gz_offset, SEEK_SET) != deb_gz_offset) {
				fprintf(stderr, "%s: %s: uxfio_lseek error. 001.3 pos=%d\n",
					swlib_utilname_get(), __FILE__, ((*uinformat)->current_pos_)); 
				return -1;
			}
			/* this is important */
			deb_gz_size = val2;

			(*uinformat)->current_pos_ = deb_gz_offset;

			/* Now continue as a compressed file */
			;
		} else if (doieee == 0 &&
			donative == 0 &&
			dodebcontext == 0 &&
			dodebcontrol == 0 &&
			dounrpminstall == 0 &&
			dodebdata == 0 &&
			does_have_name_version(xbuffer)
		) {
			STROB * tmp;
			STROB * path_prefix;
			SWVARFS * swvarfs;
			struct stat st;
			char * name;
			int found;
			int xeoa;

			UINFILE_E_DEBUG("type: plain source tarball");
			UINFILE_E_DEBUG("");
			tmp = strob_open(32);

			/* read further into the file */
			ret = taru_tape_buffered_read(refd, xbuffer+peeklen, sizeof(xbuffer) - peeklen);
			if (ret != (int)sizeof(xbuffer) - (int)peeklen) {
				return -1;
			}
			peeklen += (sizeof(xbuffer) - peeklen);

			UINFILE_E_DEBUG2("ret=%d", ret);
		
			ret = taru_read_in_tar_header2((*uinformat)->taruM,
							(*uinformat)->file_hdrM,
							-1 /* fd */,
							xbuffer,
							&xeoa, 0, TARRECORDSIZE);
			if (ret != TARRECORDSIZE) {
				return -1;
			}

			/* xbuffer now contains what should be the leading path prefix of the
			   plain tarball */

			taru_print_tar_ls_list(tmp, (*uinformat)->file_hdrM, LS_LIST_VERBOSE_L2);

			(*uinformat)->slackheaderM = malloc(TARRECORDSIZE);
			if ((*uinformat)->slackheaderM == NULL) return -1;
			memcpy((*uinformat)->slackheaderM, xbuffer, TARRECORDSIZE);

			path_prefix = strob_open(32);
			UINFILE_E_DEBUG("");

			/* read the entire package into the buffered file descriptor */
			while ((ret=uxfio_read(refd, xbuffer, TARRECORDSIZE)) > 0); 
			if (ret < 0) {
				SWBIS_E_FAIL("read error");
				return -1;
			}

			/* now seek back to beginning  */
			UINFILE_E_DEBUG("");

			ret = uxfio_lseek(refd, 0, SEEK_SET);
			if (ret != 0) {
				SWBIS_E_FAIL("");
				return -1;
			}

			UINFILE_E_DEBUG("");
			/* Now read the package to determine the path prefix */
			swvarfs = swvarfs_opendup(refd, UINFILE_DETECT_NATIVE, (mode_t)0);
			UINFILE_E_DEBUG("");
			if (swvarfs == NULL) {
				SWBIS_E_FAIL("");
				UINFILE_E_DEBUG("");
				return -1;
			}
			UINFILE_E_DEBUG("");
			found = 0;
			while ((name=swvarfs_get_next_dirent(swvarfs, &st)) != NULL && strlen(name)) {
				/* While we are reading the files, store the leading path prefix
				   and determine if it is the same for every file. */
				if (path_prefix &&
				    strob_strlen(path_prefix) < 4
				) {
					strob_strcpy(path_prefix, name);	
					swlib_unix_dirtrunc(path_prefix);
				} if (path_prefix &&
					strob_strlen(path_prefix) >= 4 &&
					strstr(name, strob_str(path_prefix)) != name
				) {
					strob_close(path_prefix);
					path_prefix = NULL;
				}
				UINFILE_E_DEBUG2("||||||| [%s]", name);
			}

			UINFILE_E_DEBUG("");
			swvarfs_close(swvarfs);
			ret = uxfio_lseek(refd, 0, SEEK_SET);
			if (ret != 0) {
				SWBIS_E_FAIL("");
				return -1;
			}
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_NOT;
			(*uinformat)->typeM = PLAIN_TARBALL_SRC_FILEFORMAT;
			ret =  uinfile_handle_return(*uinformat,
				UINFILE_DETECT_FORCEUXFIOFD,
				UXFIO_BUFTYPE_DYNAMIC_MEM);
			UINFILE_E_DEBUG2("returning fd=%d", ret);
			if (path_prefix && strob_strlen(path_prefix) < 4) {
				strob_close(path_prefix);
				path_prefix = NULL;
			}
			if (path_prefix) {
				/* fprintf(stderr, "path prefix = [%s]\n", strob_str(path_prefix)); */
				(*uinformat)->pathname_prefixM = strdup(strob_str(path_prefix));
				strob_close(path_prefix);
			} else {
				fprintf(stderr, "%s: no constant leading pathname prefix in tarball\n", swlib_utilname_get());
				return -1;
			}
			UINFILE_E_DEBUG2("returning ret=%d", ret);
			strob_close(tmp);
			return ret;

		} else if (slack_name && doieee == 0 && donative == 0 && dodebcontext == 0 &&
			dodebcontrol == 0 && dodebdata == 0 &&
			dounrpminstall == 0 &&
			memcmp(xbuffer, UINFILE_MAGIC_binary_tarball, strlen(UINFILE_MAGIC_binary_tarball)) == 0
		) {
			UINFILE_E_DEBUG2("type: binary_tarball, probably slackware: slack_name=%s", slack_name);

			/* Here we know it could be slackware binary package
			   this code path supports the --slackware-pkg-name=NAME option of swpackage
			   and lxpsf and the swinstall case of ''swinstall --slackware -s:slackage-binary-tarball'' 
			   No other cases take this code path, If we are here we punt to swpackage instead of
			   returning in this process */

			(*uinformat)->ztypeM = UINFILE_COMPRESSED_SLACK_WITH_NAME;
			/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */

		} else if (doieee == 0 && donative == 0 && dodebcontext == 0 &&
			dodebcontrol == 0 && dodebdata == 0 &&
			dounrpminstall == 0 &&
			memcmp(xbuffer, UINFILE_MAGIC_binary_tarball, strlen(UINFILE_MAGIC_binary_tarball)) == 0
		) {
			int xeoa;
			UINFILE_E_DEBUG("type: binary_tarball");
			UINFILE_E_DEBUG("");

			/* read further into the file */
			ret = taru_tape_buffered_read(refd, xbuffer+peeklen, sizeof(xbuffer) - peeklen);
			if (ret != (int)sizeof(xbuffer) - (int)peeklen) {
				return -1;
			}
			UINFILE_E_DEBUG2("ret=%d", ret);
			peeklen += (sizeof(xbuffer) - peeklen);


			/* This will save the first archive member which is "./", we need the
			   permissions on this to create in the sw POSIX archive */
			ret = taru_read_in_tar_header2((*uinformat)->taruM,
							(*uinformat)->file_hdrM,
							-1 /* fd */,
							xbuffer,
							&xeoa, 0, TARRECORDSIZE);
			if (ret != TARRECORDSIZE) {
				return -1;
			}

			if (
				memcmp(xbuffer+TARRECORDSIZE, UINFILE_MAGIC_slack "/", strlen(UINFILE_MAGIC_slack "/")) == 0
			) {

				/* This was too easy,
				   the "install/" directory was a the front of the package */

				UINFILE_E_DEBUG("");
				(*uinformat)->ztypeM = UINFILE_COMPRESSED_NOT;
				(*uinformat)->typeM = SLACK_FILEFORMAT;

				ret =  uinfile_handle_return(*uinformat,
						UINFILE_DETECT_FORCEUXFIOFD,
						UXFIO_BUFTYPE_DYNAMIC_MEM);
				return ret;
			} else {
				SWVARFS * swvarfs;
				struct stat st;
				char * name;
				int found;

				UINFILE_E_DEBUG("Looking for install directory");
				/* The only binary tarball recognized is slackware */

				/* This code path handles the ./install directory placed anywhere
				   in the archive. */

				/* read the entire package into the buffered file descriptor */
				while ((ret=uxfio_read(refd, xbuffer, TARRECORDSIZE)) > 0); 
				if (ret < 0) {
					SWBIS_E_FAIL("read error");
					return -1;
				}

				/* now seek back to beginning  */

				ret = uxfio_lseek(refd, 0, SEEK_SET);
				if (ret != 0) {
					SWBIS_E_FAIL("");
					return -1;
				}

				/* Now read the package and look for the install directory which
				   may be all the way at the end */
				swvarfs = swvarfs_opendup(refd, UINFILE_DETECT_NATIVE, (mode_t)0);
				UINFILE_E_DEBUG("");
				if (swvarfs == NULL) {
					SWBIS_E_FAIL("");
					return -1;
				}
				UINFILE_E_DEBUG("");
				found = 0;
				while ((name=swvarfs_get_next_dirent(swvarfs, &st)) != NULL && strlen(name)) {
					/* While we are reading the files, store the leading path prefix
					   and determine if it is the same for every file. */
					UINFILE_E_DEBUG2("||||||| [%s]", name);
					/* Look for the slackware control directory "install/", if we find
					   one assume it is slackware.
					   Also, need to look for ./install/ since some packagers
					   prepend a ./	
				  	*/
					if (
						strcmp(name, UINFILE_MAGIC_slack "/") == 0 ||
						strcmp(name, "./" UINFILE_MAGIC_slack "/") == 0
					) {
						UINFILE_E_DEBUG("found install directory");
						found = 1;
						break;
					}
				}

				UINFILE_E_DEBUG("");

				ret = uxfio_lseek(refd, 0, SEEK_SET);
				if (ret != 0) {
					SWBIS_E_FAIL("");
					return -1;
				}

				swvarfs_close(swvarfs);
				if (found) {
					UINFILE_E_DEBUG("FOUND SLACK");
					(*uinformat)->ztypeM = UINFILE_COMPRESSED_NOT;
					(*uinformat)->typeM = SLACK_FILEFORMAT;
					ret =  uinfile_handle_return(*uinformat,
						UINFILE_DETECT_FORCEUXFIOFD,
						UXFIO_BUFTYPE_DYNAMIC_MEM);
				} else {
					UINFILE_E_DEBUG("error");
					ret = -1;
				}
				UINFILE_E_DEBUG2("returning fd=%d", ret);
				return ret;
			}
		} else {
			struct tar_header *tar_hdr;
			/*
			 * If nothing special is detected, assume its tar.
			 */

			/*
			if (dounrpminstall) {
				(*uinformat)->ztypeM = UINFILE_COMPRESSED_RPM;
			} else {
			*/
			UINFILE_E_DEBUG("type: arbitrary tar");
			if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_NA) {
				(*uinformat)->ztypeM = UINFILE_COMPRESSED_NOT;
			}

			/* Read further into the file */
			if (taru_tape_buffered_read(refd, xbuffer+peeklen,
					512 - peeklen) != (512 - peeklen))
				return -1;
			peeklen += (512 - peeklen);


			tar_hdr = (struct tar_header *)xbuffer;
			taru_otoul(tar_hdr->chksum, &tar_hdr_sum);

			if (
				strncmp(xbuffer + 257, "ustar" /*TMAGIC*/, 5) == 0 ||
				tar_hdr_sum == taru_tar_checksum(xbuffer)
			) {
				/* uncompressed tar */
				/*
				 * This could be a deb format package
				 */
				if (
					(*uinformat)->ztypeM == UINFILE_COMPRESSED_DEB
				) {
					UINFILE_E_DEBUG("Found Compressed DEB");
					(*uinformat)->typeM=DEB_FILEFORMAT;
					(*uinformat)->has_leading_slashM = 0; /* NO */
					ret = uinfile_handle_return(*uinformat,
							oflags, uxfio_buffer_type);
				} else {
					UINFILE_E_DEBUG("Found USTAR_FILEFORMAT");
					(*uinformat)->typeM=USTAR_FILEFORMAT;
					(*uinformat)->has_leading_slashM =
						determine_if_has_leading_slash(
									*uinformat,
									xbuffer);
					ret = uinfile_handle_return(*uinformat,
							oflags, uxfio_buffer_type);
					if (ret > 0 && doieee) {
						int ret2;
						ret2 = uinfile_detect_ieee(*uinformat,
										oflags);
						if (ret2 < 0 && dounrpminstall) {
							(*uinformat)->ztypeM = UINFILE_COMPRESSED_RPM;
						} else if (ret2 < 0) {
							ret = ret2;
						} else {
							ret = ret2;
						}
					}
				}
				/*
				UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(ret));
				*/
				return ret;
			} else {
				UINFILE_E_DEBUG("TAR");
				if (dounrpminstall) {
					/* Try to let swpackage decode it */
					UINFILE_E_DEBUG("Setting ztypeM to UINFILE_COMPRESSED_RPM");
					(*uinformat)->ztypeM = UINFILE_COMPRESSED_RPM;
				} else {
					uxfio_close(refd);
					swbis_free(*uinformat);
					UINFILE_E_DEBUG("Unrecognized tar layout");
					fprintf(stderr, "%s: error: uinfile: unrecognized tar variant format\n",
							swlib_utilname_get());
					return -1;
				}
			}
			/* } */ /* arbitrary tar */
		}
	} else { /* doarb */
		/* Peeked Length is  MIN_PEEK_LEN */
		UINFILE_E_DEBUG("");
		if (!memcmp(xbuffer, (void *) gzmagic, 2)) {
						/* gzipped compression */
			UINFILE_E_DEBUG("");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GZ;
		} else if (!memcmp(xbuffer, (void *) bz2magic, LENGTH_OF_MAGIC)) {
							/* bz2 compression */
			UINFILE_E_DEBUG("");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_BZ2;
		} else if (!memcmp(xbuffer, (void *) lzmamagic, LENGTH_OF_MAGIC)) {
							/* LZMA compression */
			UINFILE_E_DEBUG("");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_LZMA;
		} else if (!memcmp(xbuffer, (void *) xzmamagic, LENGTH_OF_MAGIC)) {
							/* XZ compression */
			UINFILE_E_DEBUG("");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_XZ;
		} else if (!memcmp(xbuffer, (void *) Zmagic, 2)) {
							/* Unix compress */
			UINFILE_E_DEBUG("");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_Z;
		} else if (is_gpg_data((void*)xbuffer)) {
			UINFILE_E_DEBUG("type: GPG Encrypted");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GPG;
		} else if (
			memcmp(xbuffer, (void *) UINFILE_MAGIC_gpg_armor,  strlen(UINFILE_MAGIC_gpg_armor)) == 0 ||
			is_gpg_packet((unsigned char*)xbuffer) ||	
			0
		) {
			UINFILE_E_DEBUG("type: GPG Encrypted");
			STROB * detect_text;
			detect_text = run_gpg_list_packets_command((unsigned char*)xbuffer, peeklen);
			if (detect_text) strob_close(detect_text);
			UINFILE_E_DEBUG("type: GPG Encrypted");
			(*uinformat)->ztypeM = UINFILE_COMPRESSED_GPG;
		} else {
			UINFILE_E_DEBUG("");
			ret =  uinfile_handle_return(*uinformat,
						oflags, uxfio_buffer_type);
			UINFILE_E_DEBUG2("ret=%d", ret);
			return ret;
		}
	}
	
	/* ------------------------------------------------------------- */
	/*     If we're here then it must be a compressed file.          */
	/*     -or- its an RPM and UINFILE_DETECT_RPM is ON.		 */
	/* ------------------------------------------------------------- */

	UINFILE_E_DEBUG("HERE its compressed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	{ /* Compressed file. */

	SHCMD *unzip[2];

	unzip[0] = (SHCMD*)NULL;
	unzip[1] = (SHCMD*)NULL;

	UINFILE_E_DEBUG2("HERE current_pos_=%d", (int)((*uinformat)->current_pos_));
	if (!dodebdata) {
		if (uxfio_lseek(refd, (off_t)((*uinformat)->current_pos_), SEEK_SET) !=
			(off_t)((*uinformat)->current_pos_)
		) {
			fprintf(stderr, "%s: %s: uxfio_lseek error. 001.1 pos=%d\n",
				swlib_utilname_get(), __FILE__, ((*uinformat)->current_pos_)); 
			return -1;
		}
	}
	if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_Z) {
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], "compress");
		shcmd_add_arg(unzip[0], "-cd");
		if (do_recompress) {
			UINFILE_E_DEBUG("recompress: compress");
			rezip = shcmd_open();
			UINFILE_E_DEBUG("recompress: xz");
			shcmd_add_arg(rezip, "compress");
			shcmd_add_arg(rezip, "-c");
			vplob_add((*uinformat)->recompress_commandsM, rezip);
		}
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_DEB) {

		/* Do not record the recompress_commands for DEB packages */

		UINFILE_E_DEBUG("in deb code");
		if (memcmp(((char*)(xbuffer)) + deb_gz_offset,
				gzmagic, LENGTH_OF_MAGIC) == 0) {
			unzip[0] = shcmd_open();
			shcmd_add_arg(unzip[0], "gzip");
			shcmd_add_arg(unzip[0], "-d");
		} else if (memcmp(((char*)(xbuffer)) + deb_gz_offset,
				bz2magic, LENGTH_OF_MAGIC) == 0) {
			/*
			 * preemptive support for bzip2'ed debian packages
			 */
			unzip[0] = shcmd_open();
			shcmd_add_arg(unzip[0], "bzip2");
			shcmd_add_arg(unzip[0], "-d");
		} else if (memcmp(((char*)(xbuffer)) + deb_gz_offset,
				lzmamagic, LENGTH_OF_MAGIC) == 0) {
			unzip[0] = shcmd_open();
			shcmd_add_arg(unzip[0], "lzma");
			shcmd_add_arg(unzip[0], "-d");
		} else if (memcmp(((char*)(xbuffer)) + deb_gz_offset,
				xzmamagic, LENGTH_OF_MAGIC) == 0) {
			unzip[0] = shcmd_open();
			shcmd_add_arg(unzip[0], "xz");
			shcmd_add_arg(unzip[0], "-d");
		} else {
			/*
			 * error, the compressed magic not found at the expected offset
			 */
			fprintf(stderr, "%s: unsupported compression in a .deb file %s: at line %d\n", swlib_utilname_get(), __FILE__, __LINE__);
			return -1;
		} 
		/*
		 * The file is now positioned at the control.tar.gz file
		 */
		UINFILE_E_DEBUG("leaving deb code");
		; /* continue */
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_GZ) {
		UINFILE_E_DEBUG("in gzip code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], "gzip");
		shcmd_add_arg(unzip[0], "-d");
		if (do_recompress) {
			UINFILE_E_DEBUG("recompress: gzip");
			rezip = shcmd_open();
			shcmd_set_exec_function(rezip, "execvp");
			shcmd_add_arg(rezip, "gzip");
			shcmd_add_arg(rezip, "-c");
			shcmd_add_arg(rezip, "-9");
			vplob_add((*uinformat)->recompress_commandsM, rezip);
		}
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_BZ2) {
		UINFILE_E_DEBUG("in bzip code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], "bzip2");
		shcmd_add_arg(unzip[0], "-d");

		if (do_recompress) {
			UINFILE_E_DEBUG("recompress: bzip2");
			rezip = shcmd_open();
			shcmd_set_exec_function(rezip, "execvp");
			shcmd_add_arg(rezip, "bzip2");
			shcmd_add_arg(rezip, "-c");
			vplob_add((*uinformat)->recompress_commandsM, rezip);
		}
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_LZMA) {
		UINFILE_E_DEBUG("in lzma code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], "lzma");
		shcmd_add_arg(unzip[0], "-d");

		if (do_recompress) {
			UINFILE_E_DEBUG("recompress: lzma");
			rezip = shcmd_open();
			shcmd_set_exec_function(rezip, "execvp");
			shcmd_add_arg(rezip, "lzma");
			shcmd_add_arg(rezip, "-z");
			shcmd_add_arg(rezip, "-c");
			vplob_add((*uinformat)->recompress_commandsM, rezip);
		}
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_XZ) {
		UINFILE_E_DEBUG("in xz code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], "xz");
		shcmd_add_arg(unzip[0], "-d");

		if (do_recompress) {
			rezip = shcmd_open();
			UINFILE_E_DEBUG("recompress: xz");
			shcmd_set_exec_function(rezip, "execvp");
			shcmd_add_arg(rezip, "xz");
			shcmd_add_arg(rezip, "-z");
			shcmd_add_arg(rezip, "-c");
			vplob_add((*uinformat)->recompress_commandsM, rezip);
		}

	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_GPG) {
		UINFILE_E_DEBUG("in gpg code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], SWGPG_GPG_BIN);
		shcmd_add_arg(unzip[0], "--decrypt");
		shcmd_add_arg(unzip[0], "--use-agent");
		shcmd_add_arg(unzip[0], "--no-verbose");
		shcmd_add_arg(unzip[0], "-o");
		shcmd_add_arg(unzip[0], "-");
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_CPIO) {
		UINFILE_E_DEBUG("in unrpm code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], SWBISLIBEXECDIR "/swbis/arf2arf");
		shcmd_add_arg(unzip[0], "-H");
		shcmd_add_arg(unzip[0], "ustar");
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_SLACK_WITH_NAME) {
		UINFILE_E_DEBUG("in UINFILE_COMPRESSED_SLACK_WITH_NAME");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], SWBISBINDIR "/swpackage");
		shcmd_add_arg(unzip[0], "--to-swbis");
		shcmd_add_arg(unzip[0], "-s");
		shcmd_add_arg(unzip[0], "-");
		shcmd_add_arg(unzip[0], "--slackware-pkg-name");
		shcmd_add_arg(unzip[0], slack_name);
		shcmd_add_arg(unzip[0], "--catalog-owner=0");
		shcmd_add_arg(unzip[0], "--catalog-group=0");
		shcmd_add_arg(unzip[0], "@-");
	} else if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_RPM) {
		UINFILE_E_DEBUG("in unrpm code");
		unzip[0] = shcmd_open();
		shcmd_add_arg(unzip[0], SWBISBINDIR "/swpackage");
		shcmd_add_arg(unzip[0], "--to-swbis");
		shcmd_add_arg(unzip[0], "--catalog-owner=0");
		shcmd_add_arg(unzip[0], "--catalog-group=0");
		shcmd_add_arg(unzip[0], "@-");
	} else {
		fprintf(stderr,
			"commpression format not supported by uinfile_open.\n");
		return -1;
	}

	/* -------------------------------------------------------- */
	/*     Uncompress the file                                  */
	/* -------------------------------------------------------- */

	if (pipe(pipe_fd) < 0) {
		fprintf(stderr, "%s", strerror(errno));
		uxfio_close(refd);
		return -1;
	}
	if ((pid = swndfork(&((*uinformat)->blockmask_),
			&((*uinformat)->defaultmask_))) != 0) {
						/* read compressed file */
		/*
		* Parent
		*/
		if (pid < 0) {
			fprintf(stderr,"fork failed.\n");
			uxfio_close(refd);
			return -1;
		}
		uinfile_add_pid(*uinformat, (int)pid);
		close(pipe_fd[1]);
		refd = uxfio_opendup(pipe_fd[0], uxfio_buffer_type);
		/*
		UINFILE_E_DEBUG2("HERE\n%s", uxfio_dump_string(refd));
		*/
		/*
		 * Re-read the alledgedly uncompressed file.
		 */
		if (dodebdata) return refd;
		dounrpminstall = 0;
		UINFILE_E_DEBUG("GOTO'ING !!!!!!!!!!!!!!!!!!!!!!!!!! ");
		goto LABEL_REREAD;
	} else {
		/* 
		 * Child 
		 */
		int jlxx;
		int b_closefd = -1;
		char ** argvector;
	
		close(pipe_fd[0]);
		/*
		 * This is the master child. clear all the pids.
		 */
		for (i=0;
			i < (int)(sizeof((*uinformat)->pidlistM)/sizeof(int));
					i++)
						(*uinformat)->pidlistM[i] = 0;

		if (unzip[0] != NULL) {
			/*
			* Use the command to uncompress the stream.
			*/
			UINFILE_E_DEBUG("HERE ");
			shcmd_set_dstfd(unzip[0], pipe_fd[1]);
			zpipe[0]=-1;
			zpipe[1]=-1;

			/*
			 * If its a uxfio descriptor then we must fork.
			 */
			if (
				1 && (
				refd >= UXFIO_FD_MIN ||
				(*uinformat)->ztypeM == UINFILE_COMPRESSED_DEB
				)
			) {
				UINFILE_E_DEBUG("HERE ");
				if ((*uinformat)->ztypeM == UINFILE_COMPRESSED_DEB) {
					/*
					 * pump the exact size of the control.tar.gz file
					 */
					UINFILE_E_DEBUG("HERE");
					pump_amount = (intmax_t)deb_gz_size;
				} else {
					/*
					 * pump until the end
					 */
					UINFILE_E_DEBUG("HERE");
					pump_amount = -1;
				}

				pipe(zpipe);
				a_pid = swndfork(&((*uinformat)->blockmask_),
						&((*uinformat)->defaultmask_));
				if (a_pid > 0) {	
					UINFILE_E_DEBUG("HERE ");
					uxfio_close(refd);
					close(zpipe[1]);
					uinfile_add_pid(*uinformat,
							(int)a_pid);
				} else if (a_pid == 0) {
					UINFILE_E_DEBUG("HERE ");
					close(zpipe[0]);
					if (
						(1 || (*uinformat)->ztypeM != UINFILE_COMPRESSED_DEB) &&
						dodebdata == 0
					) {
						UINFILE_E_DEBUG("HERE at UXFIO_F_ARM_AUTO_DISABLE");
						uxfio_fcntl(refd, UXFIO_F_ARM_AUTO_DISABLE, 1);
						UINFILE_E_DEBUG2("HERE current_pos_=%d", (int)((*uinformat)->current_pos_));
						if (uxfio_lseek(refd, (off_t)((*uinformat)->current_pos_), SEEK_SET) < 0)
							fprintf(stderr,
						"uxfio_lseek error in child 002.\n");
					}

					if (
						dodebdata &&
						(*uinformat)->n_deb_peeked_bytesM > 0
					) {
						/* Here we are opening the data.tar.gz file and we must
						   write the peek'ed bytes to reconstitute the stream */
						if (
							uxfio_write(zpipe[1],
								(void*)
								((unsigned char*)((*uinformat)->deb_peeked_bytesM) +
										deb_gz_offset - SARHDR
								)
								,
								(*uinformat)->n_deb_peeked_bytesM) !=
								(*uinformat)->n_deb_peeked_bytesM
						   ) {
							fprintf(stderr, "%s: ar archive error %s: at line %d\n",
								swlib_utilname_get(), __FILE__, __LINE__);
							_exit(1);
						    }
					}
					taru_pump_amount2(zpipe[1], refd, pump_amount - ((*uinformat)->n_deb_peeked_bytesM), -1); 
					close(zpipe[1]);
					uxfio_close(refd);
					_exit(0);
				} else {
					UINFILE_E_DEBUG("HERE ");
					fprintf(stderr, "%s\n",
							strerror(errno));	
					exit(29);
				}
				shcmd_set_srcfd(unzip[0], zpipe[0]);
				b_closefd =  zpipe[0];
				UINFILE_E_DEBUG("HERE ");
			} else {	
				UINFILE_E_DEBUG("HERE ");
				shcmd_set_srcfd(unzip[0], refd);
				b_closefd = refd;
			}
			UINFILE_E_DEBUG("HERE ");
			shcmd_set_exec_function(unzip[0], "execvp");
			swgp_signal(SIGPIPE, SIG_DFL);
			shcmd_cmdvec_exec(unzip);
			jlxx = shcmd_cmdvec_wait(unzip);
			UINFILE_E_DEBUG("HERE ");
			zret = shcmd_get_exitval(unzip[0]);
			UINFILE_E_DEBUG("HERE ");
			argvector = shcmd_get_argvector(unzip[0]);
			if (argvector && argvector[0] &&
					 strcmp(argvector[0], "gzip") == 0) {
				if (zret == 2) {
					fprintf(stderr, 
			"%s: Warning: gzip exited with a warning.\n", swlib_utilname_get());
					zret = 63;
				} else if (zret == SHCMD_UNSET_EXITVAL) {
					/*
					* This happens when the whole package
					* is not read. such as when writing
					* out only the catalog section.
					*/
					zret = 0;
				} else if (zret != 0) {
					fprintf(stderr, 
		"%s: Error: gzip exited with an error: pid=%d exit value=%d.\n", swlib_utilname_get(),
						(int)(unzip[0]->pid_), (int)zret);
				}
			} else {	
				if (zret && zret != SHCMD_UNSET_EXITVAL) {
				fprintf(stderr,
		"%s: decompression process exiting with value %d.\n", swlib_utilname_get(),
							zret);
				}
			}
		} else {
			fprintf(stderr, " uinfile: internal error 84.001\n");
		}

		/*
		 * Now wait on all the pids.
		 */
		if (b_closefd >= 0) {
			uxfio_close(b_closefd);
		}
		close(pipe_fd[1]);
		while (uinfile_wait_on_all_pid(*uinformat, WNOHANG) < 0) {
			sleep(1); 
		}
		_exit(zret);
	} /* Child */

	if (unzip[0]) {
		shcmd_close(unzip[0]);
	}
	
	} /* End --  Compressed file. */

	/*
	* The child returns -1, but it never should get here.
	*/
	fprintf(stderr, " uinfile: internal error 85.002\n");
	return -1;
}

/*
 * -------------- Public Functions ------------------
 */

int
uinfile_open(char *filename,  mode_t mode, UINFORMAT ** uinformat, int oflags)
{
	int ret;
	int buftype;
	UINFILE_E_DEBUG("");
	buftype = uinfile_decode_buftype(oflags, UXFIO_BUFTYPE_DYNAMIC_MEM);
	ret = uinfile_i_open(filename, -1, mode, uinformat,
					oflags, -1, buftype, NULL);
	return ret;
}

int
uinfile_open_with_name(char *filename,  mode_t mode, UINFORMAT ** uinformat, int oflags, char * name)
{
	int ret;
	int buftype;
	UINFILE_E_DEBUG("");
	buftype = uinfile_decode_buftype(oflags, UXFIO_BUFTYPE_DYNAMIC_MEM);
	ret = uinfile_i_open(filename, -1, mode, uinformat,
					oflags, -1, buftype, name);
	return ret;
}

int
uinfile_opendup_with_name(int xdupfd, mode_t mode, UINFORMAT ** uinformat, int oflags, char * name)
{
	int ret;
	int buftype;
	UINFILE_E_DEBUG("");

	if (name == NULL)
		return uinfile_opendup(xdupfd, mode, uinformat, oflags);

	buftype = uinfile_decode_buftype(oflags, UXFIO_BUFTYPE_DYNAMIC_MEM);
	ret = uinfile_i_open((char *)NULL, -1, mode,
				uinformat, oflags, xdupfd, buftype, name);
	return ret;
}


int
uinfile_opendup(int xdupfd, mode_t mode, UINFORMAT ** uinformat, int oflags)
{
	int ret;
	int buftype;
	UINFILE_E_DEBUG("");
	buftype = uinfile_decode_buftype(oflags, UXFIO_BUFTYPE_DYNAMIC_MEM);
	ret = uinfile_i_open((char *)NULL, -1, mode,
				uinformat, oflags, xdupfd, buftype, NULL);
	return ret;
}

int
uinfile_wait_on_pid(UINFORMAT * uinformat, int pid, int flag, int * status)
{
	int ret;
	UINFILE_E_DEBUG("ENTERING");
	if (uinformat->verboseM)
			fprintf(stderr,
			"Entering uinfile_wait_on_pid: pid=%d options=%d.\n",
					pid, flag);
	UINFILE_E_DEBUG2("waiting on pid %d", pid);
	ret = waitpid((pid_t)pid, status, flag);
	if (uinformat->verboseM) 
		fprintf(stderr,
	"uinformat: waitpid(pid=%d, options=%d) returned %d, status=%d\n", 
			pid, flag, ret, *status);
	if (ret > 0) {
		if (uinformat->verboseM)
			fprintf(stderr, "uinformat: clearing pid %d\n", pid);
		uinfile_del_pid(uinformat, pid);
	}
	UINFILE_E_DEBUG("LEAVING");
	return ret;
}

int
uinfile_wait_on_all_pid(UINFORMAT * uinformat, int flag)
{
	int retval = 0;
	int ret;
	int i = 0;
	int m;
	int status;
	int gotgzwarn = 0;

	UINFILE_E_DEBUG("");
	if (uinformat->verboseM)
			fprintf(stderr, "Entering uinfile_wait_on_all_pid\n");
	while (i < (int)(sizeof(uinformat->pidlistM)/sizeof(int)))   {
		if ((uinformat->pidlistM)[i] > 0) {
			if (uinformat->verboseM)
				fprintf(stderr, "Processing pid %d.\n",
						(uinformat->pidlistM)[i]);
			m = (uinformat->pidlistM)[i];
			ret = uinfile_wait_on_pid(uinformat,
					(uinformat->pidlistM)[i],
							flag, &status);
			if (uinformat->verboseM)
				fprintf(stderr,
			"uinformat: uinfile_wait_on_pid [%d] returned %d\n",
						m, ret);
			if (ret < 0 && flag == WNOHANG) {
				return -1;
			}
			if (ret > 0) {
				if (WIFEXITED(status)) {
					if (WEXITSTATUS(status)) {
						if (WEXITSTATUS(status) == 63) {
							/*
							* this is gzip warning.
							*/
							gotgzwarn = 63;
						} else {
							retval ++;
						}
					}
				} else {
					retval ++;
				}
			}
		}
		i++;
	}
	if (uinformat->verboseM) fprintf(stderr,
		"uinformat: uinfile_wait_on_all_pid returning 0.\n");
	return gotgzwarn + retval;
}

int
uinfile_decode_buftype(int oflags, int v)
{
	UINFILE_E_DEBUG("");
	if ((oflags & UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM)) {
		v = UXFIO_BUFTYPE_DYNAMIC_MEM;
	} else if ((oflags & UINFILE_UXFIO_BUFTYPE_FILE)) {
		v = UXFIO_BUFTYPE_FILE;
	} else if ((oflags & UINFILE_UXFIO_BUFTYPE_MEM)) {
		v = UXFIO_BUFTYPE_MEM;
	}
	UINFILE_E_DEBUG2("buffer type is %d", v);
	return v;
}

int 
uinfile_get_layout_type(UINFORMAT * uinformat)
{
	return uinformat->layout_typeM;
}

int 
uinfile_get_has_leading_slash(UINFORMAT * uinformat)
{
	return uinformat->has_leading_slashM;
}

int 
uinfile_get_ztype(UINFORMAT * uinformat)
{
	return uinformat->ztypeM;
}

int 
uinfile_get_type(UINFORMAT * uinformat)
{
	return uinformat->typeM;
}

SWPATH *
uinfile_get_swpath(UINFORMAT * uinformat)
{
	return uinformat->swpathM;
}

void
uinfile_set_type(UINFORMAT * uinformat, int type)
{
	uinformat->typeM=type;
}

int
uinfile_close(UINFORMAT * uinformat)
{
	int ret;
	UINFILE_E_DEBUG("entering");
	ahsStaticDeleteFilehdr(uinformat->file_hdrM);
	/* uxfio_close(uinformat->fdM); */
	ret = uinfile_wait_on_all_pid(uinformat, 0);
	if (uinformat->verboseM)	
		fprintf(stderr,
			"Leaving uinfile_close with status %d\n", ret);
	if (uinformat->taruM)
		taru_delete(uinformat->taruM);
	if (uinformat->swpathM)
		swpath_close(uinformat->swpathM);
	if (uinformat->slack_nameM)
		free(uinformat->slack_nameM);
	swbis_free(uinformat);
	UINFILE_E_DEBUG("");
	return ret;
}

SHCMD **
uinfile_get_recompress_vector(UINFORMAT * uinformat)
{
	VPLOB * vplob = uinformat->recompress_commandsM;

	if (vplob_get_nstore(vplob) == 0) {
		/* Not recompressing */
		return NULL;
	} else {
		SHCMD ** ret;
		ret = (SHCMD**)vplob_get_list(vplob);
		return ret;
	}
	return NULL;
}
