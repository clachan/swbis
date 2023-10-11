#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <tar.h>
#include "usgetopt.h"
#include "to_oct.h"
#include "xformat.h"
#include "taru.h"
#include "swlib.h"
#include "etar.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
#include "ls_list.h"


static int HN = 0; /* Header Count Number for listing */



static
void
usage(char * name)
{
	fprintf(stderr, "%s : A testing and administrative tool of the swbis project.\n", name);
	fprintf(stderr, "Usage: %s [options] [directory [directory..]]\n", name);
	fprintf(stderr, "Writes a tar archive to stdout.\n");
	fprintf(stderr, "Default format is identical to GNU tar cf - -b1 --posix\n");
	fprintf(stderr, "         for filenames <= 99 chars.\n");
	fprintf(stderr, "Option '-L' extends identicalness to filenames >99 chars.\n");
	fprintf(stderr, "Option `-a' produces an archive identical to ''pax -w -d -b 512 | ./swbistar -X''\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --help  Show this help.\n");
	fprintf(stderr, "  -X, --pax-rdev-filter  reads tar archive on stdin and normalizes\n");
	fprintf(stderr, "                         st_rdev values of non-device files to zero.\n");
	fprintf(stderr, "  -I,--tar2tar tar to tar filter with decoding and re-writing.\n");
	fprintf(stderr, "  -G,--tarheaderflags N  See taru/taru.h for values.\n");
	fprintf(stderr, "     TARU_TAR_BE_LIKE_PAX = %d\n", TARU_TAR_BE_LIKE_PAX);
	fprintf(stderr, "     TARU_TAR_NUMERIC_UIDS = %d\n", TARU_TAR_NUMERIC_UIDS);
	fprintf(stderr, "     TARU_TAR_GNU_LONG_LINKS = %d\n", TARU_TAR_GNU_LONG_LINKS);
	fprintf(stderr, "     TARU_TAR_GNU_GNUTAR = %d\n", TARU_TAR_GNU_GNUTAR);
	fprintf(stderr, "     TARU_TAR_GNU_BLOCKSIZE_B1 = %d\n", TARU_TAR_GNU_BLOCKSIZE_B1);
	fprintf(stderr, "     TARU_TAR_BE_LIKE_STAR = %d\n", TARU_TAR_BE_LIKE_STAR);
	fprintf(stderr, "     TARU_TAR_FRAGILE_FORMAT = %d\n", TARU_TAR_FRAGILE_FORMAT);
	fprintf(stderr, "     TARU_TAR_PAXEXTHDR = %d\n", TARU_TAR_PAXEXTHDR);
	fprintf(stderr, "     TARU_TAR_PAXEXTHDR_MD = %d\n", TARU_TAR_PAXEXTHDR_MD);
	fprintf(stderr, "  -D,--debug-dump  An ascii dump of the the tar headers.\n");
	fprintf(stderr, "  -F,--file-list-only  Write the file list to stdout.\n");
	fprintf(stderr, "  -L,--gnu-long-links  Store long file names like GNU tar. (non-POSIX).\n");
	fprintf(stderr, "  -H,--format  {ustar|gnutar|newc|crc|odc|ustar.star} ustar is the default.\n");
	fprintf(stderr, "                ustar  IEEE Std 1003.1  Posix format. same as GNU tar 1.15.1\n");
	fprintf(stderr, "                ustar0  IEEE Std 1003.1  same as GNU tar 1.13.25 --posix -b1\n");
	fprintf(stderr, "                        for short path names.\n");
	fprintf(stderr, "                gnutar GNU tar 1.13.17 default format using -b1 option.\n");
	fprintf(stderr, "                oldgnu   same as GNU tar 1.13.25\n");
	fprintf(stderr, "                ustar.star  Identical to star 1.5a04 using -b=1 -H=ustar.\n");
	fprintf(stderr, "                gnu   same as GNU tar 1.15.1\n");
	fprintf(stderr, "     --owner=NAME  Same as tar GNU tar option.\n");
	fprintf(stderr, "     --group=NAME  Same as tar GNU tar option.\n");
	fprintf(stderr, "  -x,--numeric-owner  Same as tar GNU tar option.\n");
	fprintf(stderr, "  -a,--be-like-pax   Writes a format identical to pax v3.0.\n");
	fprintf(stderr, "--oldgnu-namesize  mimics 1.13.25 name size split.\n");
	fprintf(stderr, "--gnu-namesize  mimics 1.15.1 name size split.\n");
	fprintf(stderr, "Copyright (C) 2003 Jim Lowe\n");
	fprintf(stderr, "Report bugs to jhlowe@acm.org\n");
	fprintf(stderr, "This program is distributed under the GNU GPL Software License.\n");
}

static
unsigned long
mychecksum(void * tar_hdr, int len)
{
  unsigned long sum = 0;
  char *p = (char *) tar_hdr;
  char *q = p + len;
  while (p < q)
    sum += *p++ & 0xff;
  return sum;
}

static
void
tr_char (void * src, int len, int new, int old) 
{
	int i;
	unsigned char * p = (unsigned char*)src;	
	
	for (i=0; i< len; i++) {
		if ((int)(*p) == old) *(p) = (unsigned char)new;
		p++;
	}
}

static
void
write_field(
		struct  new_cpio_header * hdr,
		int verbose, 
		STROB * wf, 
		int ofd, 
		char * paxheader_annotation, 
		char * annotation_prefix, 
		unsigned char * tarheader,
		int thboffset,
		int buflen, 
		char * fieldname
	)
{
	char * unctimeme;
	unsigned int uitmp;
	unsigned char * buf = tarheader + thboffset;
	int ap_len = strlen(annotation_prefix);
	int paxap_len = strlen(paxheader_annotation);

	if (verbose)  tr_char((void*)buf, buflen, (int)'^', (int)'\000'); 

	if (verbose > 1) {
		write(ofd, (void*)paxheader_annotation, (size_t)paxap_len);
		write(ofd, (void*)annotation_prefix, (size_t)ap_len);
		swlib_writef(ofd, wf, " ");
	}

	if (verbose > 0) {
		swlib_writef(ofd, wf, "[%d] %-12s[", HN, fieldname);
	} else {
		swlib_writef(ofd, wf, "[%d] %s:=[", HN, fieldname);
	}	

	write(ofd, (void*)buf, (size_t)buflen);
	write(ofd, "]", 1);

	if (verbose) {
	switch(thboffset) {
		case THB_BO_uid:
		case THB_BO_gid:
		case THB_BO_size:
		case THB_BO_devmajor:
		case THB_BO_devminor:
				uitmp = 0;
				if (sscanf((char*)buf, "%o", &uitmp) == 1) {
					swlib_writef(ofd, wf, " (%d)", (int)uitmp);
				}
				break;
		case THB_BO_mtime:
				uitmp = 0;
				if (sscanf((char*)buf, "%o", &uitmp) == 1) {
					unctimeme = ctime((time_t*)(&uitmp));
					unctimeme[strlen(unctimeme) - 2] = '\0';
					swlib_writef(ofd, wf, " (%d) (%s)", (int)uitmp, unctimeme);
				}
				break;

	}
	}
	write(ofd, "\n", 1);
}

static
int
write_header_decode_format(struct  new_cpio_header * hdr, STROB * wf, int ofd, char * buf, int buflen, int v)
{
	char * p;
	unsigned char * paxheader = (unsigned char*)buf;
	char paxSum[100];
	uintmax_t sum;
	char Mchksum[THB_FL_chksum + 1];
	int termch = ' ';

	sum = mychecksum(paxheader, buflen);
	memset(paxSum, '\0', sizeof(paxSum));
	uintmax_to_chars(sum, paxSum, 7, POSIX_FORMAT, termch);
	
	strncpy(Mchksum, buf + THB_BO_chksum, sizeof(Mchksum));
	/* swlib_writef(ofd, wf, "pax Format Archive Header: (length: %d)\n", buflen); */

	if ((p=strchr(Mchksum, ' '))) *p = '\0';
	Mchksum[THB_FL_chksum] = '\0';

	swlib_writef(ofd, wf, "[%d] Extended name=[%s]\n", HN, ahsStaticGetTarFilename(hdr));
	swlib_writef(ofd, wf, "[%d] Extended linkname=[%s]\n", HN, ahsStaticGetTarLinkname(hdr));
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_name, THB_FL_name, "name");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_mode, THB_FL_mode, "mode");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_uid, THB_FL_uid, "uid");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_gid, THB_FL_gid, "gid");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_size, THB_FL_size, "size");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_mtime, THB_FL_mtime, "mtime");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_chksum, THB_FL_chksum, "chksum");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_typeflag, THB_FL_typeflag, "typeflag");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_linkname, THB_FL_linkname, "linkname");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_magic, THB_FL_magic, "magic");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_version, THB_FL_version, "version");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_uname, THB_FL_uname, "uname");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_gname, THB_FL_gname, "gname");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_devmajor, THB_FL_devmajor, "devmajor");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_devminor, THB_FL_devminor, "devminor");
	write_field(hdr, v, wf, ofd, paxSum, Mchksum, paxheader , THB_BO_prefix, THB_FL_prefix, "prefix");

	return 0;
}

static
int
decode_filter(XFORMAT * package, int ifd, int verbose)
{
	int nullfd;
	char * name;
	int ret;
	int aret;
	int retval = -1;
	int ofd;
	struct new_cpio_header * file_hdr;
	STROB * namebuf;
	STROB * tmp;
	char * extended_header;
	int tarheader_length;
	struct  new_cpio_header * hdr;

	nullfd = open("/dev/null", O_RDWR);
	file_hdr = (struct new_cpio_header*)(xformat_vfile_hdr(package));
	
	namebuf = strob_open(100);
	tmp = strob_open(100);

	if (ifd < 0) ifd = STDIN_FILENO;
	xformat_set_ifd(package, ifd);
	ofd = nullfd;
	xformat_set_ofd(package, ofd);

	package->taruM->do_record_headerM = 1;
	
	taruib_set_fd(nullfd);
	retval = 1;
	hdr = (struct new_cpio_header *)(xformat_vfile_hdr(package));
	while ((ret = xformat_read_header(package)) > 0) {
		if (xformat_is_end_of_archive(package)){
			retval = 0;
			break;
		}
		xformat_get_name(package, namebuf);
		name = strob_str(namebuf);
		extended_header = strob_str(package->taruM->headerM);
		tarheader_length = package->taruM->header_lengthM;

		write_header_decode_format(hdr, tmp, STDOUT_FILENO, extended_header, tarheader_length, verbose);

		taruib_clear_buffer();
		aret = xformat_copy_pass(package, ofd, ifd);
		if (aret < 0) goto error;
		retval = 0;
		HN++;
	}
	taruib_clear_buffer();

	/*
	 * Pump out the trailer bytes.
	 */
	ret = taru_pump_amount2(ofd, ifd, -1, -1);
	if (ret < 0) {
		fprintf(stderr, "swbis: error in taruib_arfcopy\n");
		retval = 2;
	}

	/*
	* Now do a final clear.
	*/
error:
	taruib_clear_buffer();
	strob_close(namebuf);
	close(nullfd);
	return retval;
}

static int
paxfilter(void)
{
	int nullblockcount = 0;
	uintmax_t nblocks;
	uintmax_t i;
	int ret = 0;
	int rret = 0;
	char buf[512];
	char nullblock[512];
	char devstring[] = "0000000";
	uintmax_t filesize;
	unsigned long sum;
	memset(nullblock, '\0', 512);

	while ( ret == 0 && (rret = uxfio_unix_safe_atomic_read(STDIN_FILENO, buf, 512)) == 512 ) {

		if (memcmp(buf, nullblock, 512) == 0) {
			nullblockcount ++;
			if (nullblockcount >= 2) {
				write(STDOUT_FILENO, nullblock, 512);
				swlib_pipe_pump(STDOUT_FILENO, STDIN_FILENO);
				break;
			} else {
				write(STDOUT_FILENO, nullblock, 512);
				continue;
			}
		}

		if (strncmp(buf+257, "ustar", 5) != 0) {
			fprintf(stderr, "tar format not recognized.\n");
			ret = 1;
			break;
		} else if (*(buf+156) != CHRTYPE && *(buf+156) != BLKTYPE) {
			/*
			* Correct rdev to zero.
			*/
			strcpy(buf+329, devstring);
			strcpy(buf+337, devstring);
			memcpy (buf+148, CHKBLANKS, 8);
			sum = taru_tar_checksum((void*)buf);
			uintmax_to_chars(sum, buf+148, 8, POSIX_FORMAT, 0);			
		}

		write(STDOUT_FILENO, buf, 512);

		if (*(buf+156) == REGTYPE) {
			taru_otoumax(buf+124, &filesize);
			i = 0;
			nblocks = 0;
			if (filesize) {
				nblocks = filesize / 512;
				if (filesize % 512) nblocks ++;
			
				while (i < nblocks) {
					ret = uxfio_unix_safe_atomic_read(STDIN_FILENO, buf, 512);
					if (ret != 512) {
						ret = 2;
						break;
					}
					write(STDOUT_FILENO, buf, 512);
					i++;
				}
				ret = 0;
			}
		}
	}

	if (ret == 0 && rret == 512) ret = 0;

	return ret;
}


static char *
getnextpath(FILE * fp)
{
	char * ret;
	static char buf[1024];
	char * s;

	ret = fgets(buf, sizeof(buf), fp);
	if (!ret) return ret;
	buf[sizeof(buf) - 1] = '\0';
	if ((s=strrchr(buf, '\n'))) *s = '\0';
	if ((s=strrchr(buf, '\r'))) *s = '\0';
	return ret;
}

static 
char * get_next_filename(FILE * flp, SWVARFS * swvarfs, struct stat * st, char * filename)
{
	char * path;
	if (filename) {
		path = filename;
	} else if (flp) {
		path = getnextpath(flp);
		if (path) {
			if (lstat(path, st)) {
				fprintf(stderr, "could not stat pathname [%s] \n", path);
				path = "";
			}
		}
	} else {
		path = swvarfs_get_next_dirent(swvarfs, st);
	}
	return path;
}

int 
main (int argc, char ** argv ) {
	char * path;
	char * t;
	char * filename;
	int c;
	int did_list = 0;
	int filelistonly = 0;
	int dumpmode = 0;
	int ret = 0;
	int statret = 0;
  	int arf_format = arf_ustar;
	int verbose = 0;
	int testmode = 0;
	int eoa;
	struct stat st;
	FILE * flp;
	char * opt_owner = NULL;
	char * opt_group = NULL;
	uid_t opt_owner_uid;
	gid_t opt_owner_gid;
	STROB * buf = strob_open(100);
	SWVARFS * swvarfs = NULL;
	XFORMAT * xformat;
	int nullfd = open("/dev/null", O_RDWR, 0);
	struct new_cpio_header * file_hdr;
	int tarheaderflags = 0;
	int mode_tar2tar = 0;	

	
	xformat = xformat_open(-1, STDOUT_FILENO, arf_format);
	
	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"debug-dump", 0, 0, 'D'},
               {"test", 0, 0, 't'},
               {"tar2tar", 0, 0, 'I'},
               {"tarheaderflags", 1, 0, 'G'},
               {"numeric-owner", 0, 0, 'x'},
               {"gnu-long-links", 0, 0, 'L'},
               {"pax-rdev-filter", 0, 0, 'X'},
               {"be-like-pax", 0, 0, 'a'},
               {"verbose", 0, 0, 'v'},
               {"format", 1, 0, 'H'},
               {"file-list-only", 0, 0, 'F'},
               {"owner", 1, 0, 155},
               {"group", 1, 0, 156},
               {"oldgnu-namesize", 0, 0, 157},
               {"gnu-namesize", 0, 0, 158},
               {"scratch-pad", 1, 0, 159},
               {"help", 0, 0, '\005'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "tvaxLIFXDH:G:", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
	       case '\005':
			usage(argv[0]);
			exit(2);
		       	break;
               case 'D':
			dumpmode = 1;
		 	break;
               case 'I':
			mode_tar2tar = 1;
		 	break;
               case 'G':
			tarheaderflags = atoi (optarg);
			xformat->taruM->taru_tarheaderflagsM = tarheaderflags;
		 	break;
               case 't':
			xformat_set_ofd(xformat, nullfd);
			xformat->taruM->do_record_headerM = 1;
			testmode = 1;
			break;
               case 'v':
			verbose++;
			break;
               case 'a':
			xformat_set_tarheader_flag(xformat, TARU_TAR_BE_LIKE_PAX, 1);
			break;
               case 'L':
			xformat_set_tarheader_flag(xformat, TARU_TAR_GNU_LONG_LINKS, 1);
			break;
               case 'X':
			ret = paxfilter();
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			exit(ret);
			break;
               case 'F':
			filelistonly = 1;
			break;
               case 'H':
			if (taru_set_tar_header_policy(xformat->taruM, optarg, &arf_format))
				exit(1);
			break;
               case 'x':
			xformat_set_numeric_uids(xformat, 1);
			break;
	       case 155:
	       		opt_owner = strdup(optarg);
			taru_get_uid_by_name(opt_owner, &opt_owner_uid);
			break;
	       case 156:
	       		opt_group = strdup(optarg);
			taru_get_gid_by_name(opt_group, &opt_owner_gid);
			break;
	       case 157:
			xformat_set_tarheader_flag(xformat, TARU_TAR_NAMESIZE_99, 1);
			break;
	       case 158:
			xformat_set_tarheader_flag(xformat, TARU_TAR_NAMESIZE_99, 0);
			break;
	       case 159:
			{
				#define ETAR__HDR(a)  ((struct tar_header *)((a)->tar_hdrM))
				unsigned long devno;
				TARU * taru = taru_create();
				ETAR * etar;
				etar = etar_open(taru->taru_tarheaderflagsM);
				devno = swlib_atoul(optarg, (int *)NULL, (char**)NULL);	
				etar_set_devmajor(etar, devno);
				fprintf(stdout, "%s\n", (char*)(ETAR__HDR(etar)->devmajor));
			}
			exit(0);
			break;
               default:
			exit(1);
			break;
               }
	}

	if (mode_tar2tar) {
		int tf_fd;
		TARU * tf_taru;
		int tf_ret;
			
		tf_fd = STDIN_FILENO;
		if (optind < argc) {
			if (strcmp(argv[optind], "-") == 0) {
				tf_fd = STDIN_FILENO;
			} else {
				tf_fd = open(argv[optind], O_RDONLY);
				if (tf_fd < 0) {
					fprintf(stderr, "swbistar: %s\n", strerror(errno));
					exit(1);	
				}
			}	
		}
		tf_taru = xformat->taruM;
		tf_ret = taru_process_copy_out(tf_taru, tf_fd, STDOUT_FILENO,
					NULL, NULL, arf_ustar, -1, -1, (intmax_t*)NULL, NULL);
		if (tf_ret > 0) {
			exit(0);
		} else {
			exit(1);
		}
	}

	if (dumpmode) {
			int xfd = -1;
			if (optind < argc) {
				xfd = open(argv[optind], O_RDONLY);
				if (xfd < 0) {
					fprintf(stderr, "swbistar: %s\n", strerror(errno));
					exit(1);	
				}
			}
			HN = 0;
			ret = decode_filter(xformat, xfd, verbose);
			exit(ret);
	}

	xformat_set_output_format(xformat, arf_format);
	
	did_list = 1;
	filename = NULL;
	while (optind < argc || did_list) {
		did_list = 0;
		if (optind < argc) {
			if (filename) free(filename);
			filename = NULL;
			if ((statret=lstat(argv[optind], &st)) == 0 && (S_ISDIR(st.st_mode)) == 0) {
				filename = strdup(argv[optind]);
				flp = NULL;
			} else if (statret == 0 && (S_IFDIR & st.st_mode)) {
				/*
				* Open directory name.
				*/
				swvarfs=swvarfs_open(argv[optind], UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE, (mode_t)(NULL));
				flp = NULL;
			} else {
				fprintf(stderr, "%s not found\n", argv[optind]);
				exit(1);
			}
		} else  {
			/*
			* read list of files from stdin.
			*/
			swvarfs = NULL;
			flp = stdin;
		}
	
		path = get_next_filename(flp, swvarfs, &st, filename);
		while (ret >= 0 && path) {
		
			if ( (t=strpbrk (path,"\n\r"))) {
				*t = '\0';
			}
		
			if (strlen(path)) {
				if (filelistonly || (testmode && verbose == 0)) {
					fprintf(stdout, "%s\n", path);
				} else if (testmode && verbose > 0) {
					if (opt_owner) {
						st.st_uid = opt_owner_uid;
					}
					if (opt_group) {
						st.st_gid = opt_owner_gid;
					}
					ret = xformat_write_by_name(xformat, path, &st);
					if (ret > 0) {
						file_hdr = xformat_vfile_hdr(xformat);
						taru_read_in_tar_header2(xformat->taruM,
							file_hdr,
							-1,
							strob_str(xformat->taruM->headerM),
							&eoa, 
							xformat_get_tarheader_flags(xformat), 512);
						taru_print_tar_ls_list(buf, file_hdr, LS_LIST_VERBOSE_L1);
						write(STDOUT_FILENO, strob_str(buf), strob_strlen(buf));
					} else if (ret == 0) {
						;
						/* 
						* This path occurs for sockets.
						*/
					} else {
						;
						fprintf(stderr, "swbistar: xformat_write_by_name returned %d\n", ret);
						/* 
						* This path occurs for sockets.
						*/
					}
				} else {
					if (opt_owner) {
						st.st_uid = opt_owner_uid;
					}
					if (opt_group) {
						st.st_gid = opt_owner_gid;
					}
					ret = xformat_write_by_name(xformat, path, &st);
				}
			}
			if (filename) {
				path = NULL;
			} else {
				path = get_next_filename(flp, swvarfs, &st, filename);
			}
		}
		optind++;
	}


	if (!filelistonly && ret >= 0) xformat_write_trailer(xformat);

	xformat_close(xformat);
	if (swvarfs) swvarfs_close(swvarfs);
	exit(0);
}

