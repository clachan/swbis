/* 
 * arf2arf.c - Specialized Archive reading/copying utility.
 */

/*
   Copyright (C) 2003-2004 James H. Lowe, Jr.
   All rights reserved.
  
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

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include "swprog_versions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "usgetopt.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swevents_array.h"
#include "swevents.h"
#include "swlib.h"
#include "swi.h"
#include "xformat.h"
#include "swutilname.h"
#include "swmain.h"
#include "swgpg.h"

extern YYSTYPE yylval;
static int verboseG;

/*
#define GPG_STATUS_PREFIX 	"[GNUPG:] "
*/

static int
usage(FILE * file, char * name)
{

fprintf(file, "%s", 
"arf2arf : A signature and digest checking tool of the swbis project.\n"
"Usage arf2arf [options] [archive_file]\n"
"Translates an archive to stdout with specified filtering.\n"
"  -H, --format FORMAT   Output format  ustar, gnutar, newc, crc, odc\n"
"  --verbose-level=LEVEL\n"
"  -v, --verbose\n"
"  -N, --null-fd   Set stdout to  a /dev/null fd (Useful for debugging)\n"
"  -n, --sig-number=N  select Nth signature for verification; first is N=1,\n"
"                      last is N=0.\n"
"  -m, --numeric-owner\n"
"       --checksig-mode  fifoname  used by swverify --checksig.\n"
"       --sleep TIME  A configurable delay in seconds.\n"
"       --gpg-prog gpgexecname     used by --checksig-mode.\n"
"       --noop        legal option with no effect.\n"
"       --sha1        applies to -Sd only, show the sha1 digest.\n"
"       --decode      applies to -C and -S and --audit only.\n"
"                     Re-encodes the header with the swbis personality.\n"
"  --audit  Pass the input thru to the output unchanged and audit.\n"
"  -p, --pass-thru   Pass the input thru to the output unchanged.\n"
"  --recompress   Applies to pass thru\n"
"  -C, --catalog-sign-file  Write the catalog signature input file to stdout.\n"
"  -S, --storage-digest-file  Write the storage digest input file to stdout.\n"
"  -s, --adjunct-digest-file  Write the adjunct digest input file to stdout.\n"
"  -d, --show-auth-files    Writes the relevent security file to stderr.\n"
"              i.e  .../catalog/*/md5sum, or, adjunct_md5sum, or signature.\n"
"                   is copied directly from the archive to stderr.\n"
"    --test1  Write digest archive file on stdout and adjunct on stderr.\n"
"  -G --get-sig-if FILE Checks md5sum and if valid writes sigfile to FILE.\n"
"                   and the signed file to stdout.\n"
"  --util-name=NAME  NAME appears in verbose messages\n"
"  --logger-fd n   write authentication info to file descriptor n\n"
"\n"
" Note: The adjunct_md5sum attribute does not cover SYMLNK files in the\n"
"        archive.  It is a concession to an inability to preserve\n"
"        the modification time in the installed directory form of a package.\n"
" Example:  Check the md5sum and the (lesser) adjunct_md5sum:\n"
"      arf2arf -d -S <serial_archive_package | /usr/bin/md5sum\n"
"      arf2arf -d -s <serial_archive_package | /usr/bin/md5sum\n"
"         Note: The pairs of digests should be identical.\n"
" Example:  Check the package signature:\n"
"       arf2arf -G /dev/tty packageName.tgz | gpg --verify /dev/tty -\n"
"       (Copy and paste the sigfile, then hit Ctrl-D.)\n"
);
	fprintf(file, "\n");
	fprintf(file, "Date: " SWBIS_DATE "\n");
	fprintf(file, "Copyright (C) 2003 Jim Lowe\n");
	fprintf(file,
"This program is distributed under the GNU GPL Software License.\n");
	return 0;
}

static
void
enforce_tar_format(int format) 
{
	if (format != USTAR_FILEFORMAT) {
		fprintf(stderr, 
		"operation not supported for this archive format[%d]\n",
		format);
		exit(1);
	}
}


static
void
set_if_fd(char * optarg, int * pfd, char * option_name)
{
	int ret;
	int status;
	if (optarg && strlen(optarg) == 1) {
		*(optarg+1) = '\0';  /* insane precaution, only allow single digit */
		ret = swlib_atoi(optarg, &status);
		if (status == 0) {
			*pfd = ret;
		} else {
			fprintf(stderr, "%s: invalid value for option --%s\n", swlib_utilname_get(), option_name);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s: invalid value for --%s\n", swlib_utilname_get(), option_name);
		exit(1);
	}
}


int
main (int argc, char *argv[])
{
	int fd;
	int c;
	int debugmode = 0;
	int format=arf_ustar;
	int detected_format;
	int numeric_uids = 0;
	int do_decode_pass_thru = 0;
	int ret = 0;
	int ifd;
	int flags;
	int cat_sign = 0;
	int store_sign = 0;
	int opt_gnutar = 0;
	int do_checksig = 0;
	int adjunct_store_sign = 0;
	int do_test1 = 0;
	int verbose_stderr = 0;
	int pass_thru_mode = 0;
	int do_audit = 0;
	int get_sign_if = 0;
	int uverbose = 1;
	int digest_type = 0; /* 0 Means get the md5, 3 means get the sha1 */
	int iret;
	int logger_fd;
	int signed_bytes_fd;
	int do_recompress = 0;
	int which_sig = -1; /* 0 means last, -1 means all, 1 means first */
	int stdoutXfileno;
	uintmax_t statbytes;
	char * gpg_prog = NULL;
	char * filearg;
	char * sleeptime = NULL;
	XFORMAT * xformat;
	char * sigfilename = NULL;
	char * which_sig_arg;

	stdoutXfileno = STDOUT_FILENO;
	flags = UINFILE_DETECT_FORCEUXFIOFD | 
			UINFILE_DETECT_IEEE | 
				UINFILE_UXFIO_BUFTYPE_MEM;
	gpg_prog = strdup(SWGPG_GPG_BIN);
	signed_bytes_fd = stdoutXfileno;
	logger_fd = STDERR_FILENO;
	verboseG = uverbose;
	which_sig_arg = strdup("-1");
	swlib_utilname_set("arf2arf");

	E_DEBUG("");

	/*
	* This line is required by the parser.*/	
	yylval.strb = strob_open (8);

         while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"format", 1, 0, 'H'},
               {"get-sig-if", 1, 0, 'G'},
               {"verbose", 0, 0, 'v'},
               {"numeric-owner", 0, 0, 'x'},
               {"decode", 0, 0, 151},
               {"debug-mode", 0, 0, 'D'},
               {"catalog-sign-file", 0, 0, 'C'},
               {"storage-sign-file", 0, 0, 'S'},
               {"null-ofd", 0, 0, 'N'},
               {"adjunct-storage-sign-file", 0, 0, 's'},
               {"show-auth-files", 0, 0, 'd'},
               {"sig-number", 1, 0, 'n'},
               {"pass-thru", 0, 0, 'p'},
               {"test1", 0, 0, 152},
               {"help", 0, 0, 150},
               {"noop", 0, 0, 158},
               {"checksig-mode", 1, 0, 159},
               {"sha1", 0, 0, 160},
               {"gpg-prog", 1, 0, 161},
               {"sleep", 1, 0, 162},
               {"audit", 0, 0, 163},
               {"verbose-level", 1, 0, 164},
               {"util-name", 1, 0, 165},
               {"logger-fd", 1, 0, 166},
               {"signed-bytes-fd", 1, 0, 167},
               {"recompress", 0, 0, 168},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "n:NpdsSCxDH:G:v",
                        long_options, &option_index);
             if (c == -1) break;

             switch (c)
               {
		case 'x':
			numeric_uids = 1;
			break;
		case 'p':
			pass_thru_mode = 1;
			break;
		case 'v':
			uverbose++;
			verboseG++;
			break;
		case 'd':
			verbose_stderr = 1;
			break;
		case 'D':
			debugmode = 1;
			break;
		case 'N':
			stdoutXfileno =  open("/dev/null", O_RDWR, 0);
			if (stdoutXfileno < 0) {
				fprintf(stderr, "%s : %s\n", argv[0], strerror(errno)); 
				exit(1);
			}
			break;
		case 'n':
			which_sig = swlib_atoi(optarg, NULL);
			which_sig_arg = strdup(optarg);
			break;	
		case 'G':
			get_sign_if = 1;
			sigfilename=strdup(optarg);	
			break;	
		case 'H':
			if (!strcmp(optarg,"ustar")) {
  				format=arf_ustar;
			} else if (!strcmp(optarg,"gnutar")) {
				opt_gnutar = 1;
  				format=arf_ustar;
			} else if (!strcmp(optarg,"newc")) {
  				format=arf_newascii;
			} else if (!strcmp(optarg,"crc")) {
  				format=arf_crcascii;
			} else if (!strcmp(optarg,"odc")) {
  				format=arf_oldascii;
			} else {
				fprintf (stderr,
					"unrecognized format: %s\n",
						optarg);
				exit(2);
			}
			break;
		case 'C':
			cat_sign = 1;
			break;
		case 'S':
			store_sign = 1;
			break;
		case 's':
			adjunct_store_sign = 1;
			break;
		case 160:
			digest_type = 3;
			break;
		case 158:
			break;
		case 150:
			usage(stdout, argv[0]);
			exit(0);
			break;
		case 151:
			do_decode_pass_thru = 1;
			break;
		case 152:
			do_test1 = 1;
			break;
		case 159:
			do_checksig = 1;
			sigfilename=strdup(optarg);	
			break;
		case 161:
			gpg_prog=strdup(optarg);	
			break;
		case 162:
			sleeptime=strdup(optarg);	
			break;
		case 163:
			do_audit = 1;
			flags = UINFILE_DETECT_FORCEUXFIOFD | 
					UINFILE_DETECT_IEEE | 
					UINFILE_DETECT_OTARALLOW | 
					UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
			break;
		case 164:
			uverbose = swlib_atoi(optarg, NULL);	
			verboseG = swlib_atoi(optarg, NULL);	
			break;
		case 165:
			swlib_utilname_set(optarg);
			break;
		case 166:
			set_if_fd(optarg, &logger_fd, "logger-fd");
			break;
		case 167:
			set_if_fd(optarg, &signed_bytes_fd, "signed-bytes-fd");
			break;
		case 168:
			do_recompress = 1;
			break;
		default:
			usage(stderr, argv[0]);
			exit (2);
		break;
               }
	}

	E_DEBUG("");
	if (do_decode_pass_thru && verbose_stderr) {
		fprintf(stderr, "option combination not supported.\n");
		exit(1);
	}

	if (sleeptime && swlib_atoi(sleeptime, NULL)) {
		sleep(swlib_atoi(sleeptime, NULL));
	}

	E_DEBUG("");
	if (optind < argc) {
		if (strcmp(argv[optind], "-")) {
			fd = open(argv[optind], O_RDONLY, 0);
			if (fd <  0) {
				fprintf(stderr,
					"%s : %s\n",
					argv[optind], strerror(errno)); 
				exit(1);
			}
			filearg = strdup(argv[optind]);
		} else {
			filearg = strdup("-");
			fd = STDIN_FILENO;
		}
	} else {
		filearg = strdup("-");
		fd = STDIN_FILENO;
	}
	
	if (do_checksig) {
		E_DEBUG2("which_sig_arg=[%s]", which_sig_arg);
		ret = swgpg_run_checksig2(sigfilename,
				SWBISLIBEXECDIR "/swbis/arf2arf", 
					filearg, gpg_prog, uverbose, which_sig_arg);
		E_DEBUG2("swgpg_run_checksig2 ret=[%d]", ret);
		if (uverbose>5) fprintf(stderr, 
				"%s: do_checksig returned %d\n", swlib_utilname_get(), ret);
		if (ret < 0) ret = 1;
		E_DEBUG2("exiting ret=[%d]", ret);
		exit(ret);
	}

	E_DEBUG("");
	xformat = xformat_open(STDIN_FILENO, stdoutXfileno, format);
	if (!xformat) exit(1);

	if (do_recompress) {
		flags = flags | UINFILE_DETECT_RECOMPRESS;
	}

	if (xformat_open_archive_by_fd(xformat, fd, flags, 0)) {
		if (get_sign_if) {
			int sigfd = open(sigfilename, 
					O_RDWR|O_TRUNC|O_CREAT, 0600);
			if (sigfd < 0) {
				fprintf(stderr,
					"open failed ( %s ) : %s\n",
						sigfilename, strerror(errno));
			} else {
				close(sigfd);
			}
		}
		close(stdoutXfileno);
		exit(1);
	}

	detected_format = xformat_get_format(xformat);	
	xformat_set_numeric_uids(xformat, numeric_uids);

	if (opt_gnutar) {
		xformat_set_tarheader_flag(xformat, 
					TARU_TAR_GNU_OLDGNUTAR, 1 /* ON */);
		xformat_set_tarheader_flag(xformat, 
					TARU_TAR_GNU_LONG_LINKS, 1 /* ON */);
	}

	if (store_sign) {
		/*
		* Write the storage section.
		*/

		/*
		* Set taru flags not to tolerate any hiccups.
		*/
		enforce_tar_format(detected_format);
		xformat_set_tarheader_flag(xformat, 
					TARU_TAR_FRAGILE_FORMAT, 1 /*on*/ );

		if (do_decode_pass_thru) {
			/*
			* decodes tar headers and and re-encodes the tar headers
			* archive therefore possibly altering the headers.
			*/
			ret = swlib_write_signing_files(xformat, stdoutXfileno,
				1 /* 0=catalog section 1=storage section */, 0);
		} else {
			/*
			* Decodes, but writes output that is identical
			* to the input.
			*/
			ret = taruib_write_storage_stream(
					(void *)xformat,
					stdoutXfileno,
					1 		/*version*/, 
					-1		/*adjunct_ofd*/ , 
					verbose_stderr 	/*verbose*/, 
					digest_type);
		}
	} else if (get_sign_if) {
		E_DEBUG("");
		enforce_tar_format(detected_format);
		xformat_set_tarheader_flag(xformat,
					TARU_TAR_FRAGILE_FORMAT, 1 /*on*/ );
		E_DEBUG("");
		E_DEBUG2("which_sig=[%d]",  which_sig);
		ret = taruib_write_signedfile_if(
				(void*)xformat, 
				signed_bytes_fd, 
				sigfilename, 
				uverbose,
				which_sig /*last signature*/, logger_fd); 
		E_DEBUG2("taruib_write_signedfile_if: ret=%d", ret);
		if (uverbose >= 5) {
			fprintf(stderr,
				"%s: taruib_write_signedfile_if returned %d\n",
							swlib_utilname_get(), ret);
		}
	} else if (adjunct_store_sign) {
		enforce_tar_format(detected_format);
		if (do_decode_pass_thru) {
			swlib_write_signing_files(xformat,
					stdoutXfileno, 1, 1);
		} else {
			int nullfd;
			nullfd = open("/dev/null", O_RDWR, 0);
			if (nullfd < 0) {
				fprintf(stderr, "error opening /dev/null \n");
				exit(2);
			}
			taruib_write_storage_stream(
					(void*)xformat, 
					nullfd, 
					1 /* version */, 
					stdoutXfileno, 
					verbose_stderr, 
					digest_type);
		}
	} else if (cat_sign) {
		enforce_tar_format(detected_format);
		if (do_decode_pass_thru) {
			ret = swlib_write_signing_files(
				xformat, 
				stdoutXfileno, 
				0, 
				0);
		} else {
			ret = taruib_write_catalog_stream(
				(void*)xformat, 
				stdoutXfileno, 
				1 /* version */, 
				verbose_stderr);
		}
	} else if (do_test1) {
		xformat_set_tarheader_flag(xformat,
				TARU_TAR_FRAGILE_FORMAT, 1 /*on*/ );
		ret = taruib_write_storage_stream(
			xformat, 
			stdoutXfileno, 
			1 /*version*/, 
			STDERR_FILENO, 
			0 /*verbose*/, 
			digest_type);	

	} else if (pass_thru_mode) {
		/* uncompress write-thru and re-compress per options */
		SHCMD ** cmd_vec;
		SHCMD * cmd;
		int ipipe[2];
		pid_t childi;

		if (do_recompress) {
			E_DEBUG("do_recompress");
			cmd_vec = uinfile_get_recompress_vector(
					swvarfs_get_uinformat(
						xformat_get_swvarfs(xformat)
					)
				);
		} else {
			cmd_vec = NULL;
		}

		if (cmd_vec) {
			/* restore the compression */
			E_DEBUG("do_recompress: have cmd_vec");
			pipe(ipipe);
			childi = swfork((sigset_t*)(NULL));
			if (childi == 0) {
				int ret;
				swgp_signal(SIGPIPE, SIG_DFL);
				swgp_signal(SIGINT, SIG_DFL);
				swgp_signal(SIGTERM, SIG_DFL);
				swgp_signal(SIGUSR1, SIG_DFL);
				swgp_signal(SIGUSR2, SIG_DFL);
				close(ipipe[0]);
				ret = taruib_write_pass_files((void*)xformat, ipipe[1], -1);
				if (ret < 0)
					ret = 1;
				else
					ret = 0;
				_exit(ret);
			}
			E_DEBUG("");
			if (childi < 0) {
				return -1;
			}
			close(ipipe[1]);
			shcmd_set_srcfd(cmd_vec[0], ipipe[0]);
			cmd = shcmd_get_last_command(cmd_vec);
			shcmd_set_dstfd(cmd, stdoutXfileno);
			shcmd_cmdvec_exec(cmd_vec);
			ret = shcmd_cmdvec_wait2(cmd_vec);
			if (ret != 0) ret = -1;
		} else {
			ret = taruib_write_pass_files((void*)xformat, stdoutXfileno, -1); 
		}

	} else if (do_audit) {
		/*
		* Test use of the swlib_arfcopy() and swi_<*> routines.
		*/
		ret = swlib_audit_distribution(xformat, 
					do_decode_pass_thru, 
					stdoutXfileno, 
					&statbytes, 
					(int*)(NULL),
					(void (*)(int))(NULL));
	} else {
		xformat_set_output_format(xformat, (int)format);
		xformat_set_false_inodes(xformat, XFORMAT_OFF);
	
		ifd = xformat_get_ifd(xformat);

		while ((ret = xformat_read_header(xformat)) > 0) {
			if (xformat_is_end_of_archive(xformat)){
				break;
			}
			xformat_write_header(xformat);
			xformat_copy_pass(xformat, stdoutXfileno, ifd);
		}
		
		if (ret >= 0) {
			xformat_write_trailer(xformat);
			if (xformat_get_pass_fd(xformat)) {
				xformat_clear_pass_buffer(xformat);
			}
		}
	}

	iret = xformat_close(xformat);
	if (fd != STDIN_FILENO) close(fd);

	if (uverbose >= 5) {
		fprintf(stderr, "%s : ret=%d\n", swlib_utilname_get(), ret);
		fprintf(stderr, "%s : iret=%d\n", swlib_utilname_get(), iret);
		fprintf(stderr, "%s : exiting with %d\n", swlib_utilname_get(), ret);
	}
	if (ret == 0) ret = iret;

	/* close(STDERR_FILENO); */
	if (ret < 0) ret = 1;
	E_DEBUG2("exiting with [%d]", ret);
	exit(ret);
}
