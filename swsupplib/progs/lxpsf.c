/* lxpsf.c -- A Package reading and translation tool.
 */

/*
 * Copyright (C) 1999,2007,2008,2014  James H. Lowe, Jr. 
 * 
 * COPYING TERMS AND CONDITIONS:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define FILENEEDDEBUG 1
#undef FILENEEDDEBUG

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "um_rpmlib.h"
#include "um_header.h"
#include "swfork.h"
#include "usgetopt.h"
#include "ugetopt_help.h"

#include "uxfio.h"
#include "rpmpsf.h"
#include "swlib.h"
#include "shcmd.h"
#include "swparse.h"
#include "swlex_supp.h"
#include "swparser_global.h"
#include "swutilname.h"
#include "swevents_array.h"
#include "swcommon_options.h"
#include "etar.h"
#include "debpsf.h"

static void usage (char * progname, FILE*);


static int G_OFD;


extern YYSTYPE yylval;
int yydebug=0;
/* int swlex_definition_file = SW_PSF; */

#define CHARTRUE  "true";

static
void
show_version(FILE * fp)
{
    fprintf (fp, "lxpsf (swbis) version " SWBIS_RELEASE "\n");
}

static
void
copyright_info(FILE * fp)
{
	fprintf(fp,  "%s",
	"Copyright (C) 2003,2004,2005,2007,2014 Jim Lowe\n"
	"This software is distributed under the terms of the GNU General Public License\n"
	"and comes with NO WARRANTY to the extent permitted by law.\n"
	"See the file named COPYING for details.\n");
}

static 
void  usage (char * progname, FILE * fp) {
    fprintf (fp, "\n");
    fprintf (fp, "Usage: lxpsf [options] [file]\n");
    fprintf (fp, "\n");
    fprintf (fp, "Read any format (RPM,DEB,slackware) package and write a tar archive\n");
    fprintf (fp, "to stdout that is able to be converted to a POSIX package by swpackage\n");
    fprintf (fp, "when installed in the file system.  GNU swpackage has a special\n");
    fprintf (fp, "feature to read a tar archive in memory as a file system (See below).\n");
    fprintf (fp, "Currently the following package formats are supported:\n");
    fprintf (fp, "    RPM v3 (source and binary), Debian Package Format (Deb v2.0),\n");
    fprintf (fp, "    Slackware runtime tarballs, and plain vanilla tarballs\n");
    fprintf (fp, "\n");
    fprintf (fp, "   --psf-form1  default form, full control directories\n");
    fprintf (fp, "   --psf-form2  include path name prefix in archive\n");
    fprintf (fp, "   --psf-form3  no control directories, with path name prefix\n");
    fprintf (fp, "   --slackware-pkg-name=NAME  Tarball filename.\n");
    fprintf (fp, "   --deb-control   write uncompressed control.tar.gz tar file to stdout.\n");
    fprintf (fp, "   --deb-data   write uncompressed data.tar.gz tar file to stdout.\n");
    fprintf (fp, "   --create-time=cal_time  Use this time for archive header times\n");
    fprintf (fp, "   --owner=NAME  package owner\n");
    fprintf (fp, "   --group=NAME  package group\n");
    fprintf (fp, "   --exclude=NAME  add exclude directive to the PSF\n");
    fprintf (fp, "   --exclude-system-dirs  exclude all system directories listed in manual page\n"
	         "         hier(7) (conforming to FHS 2.2)\n");
    fprintf (fp, "   -d, --debug-level=TYPE    always use 'link'\n");
    fprintf (fp, "   -D checkdigest  checkdigest script, applies to plain tarball translation.\n");
    fprintf (fp, "   -x, --use-recursive-fileset Use \"file *\" instead of individual file definitions.\n");
    fprintf (fp, "   -r, --no-leading-path   use \".\" as the leading archive path\n");
    fprintf (fp, "   -B buffertype   mem or file  Mem is default.\n");
    fprintf (fp, "   -H format     archive format: ustar, pax, newc, crc, odc (pax is default)\n");
    fprintf (fp, "   -p, --psf-only   Write the PSF to stdout.\n");
    fprintf (fp, "   -o, --info-only   Write the INFO file for the rpm archive.\n");
    fprintf (fp, "   -r, --no-leading-path.\n");
    fprintf (fp, "   -L, --smart-leading-path  Use leading path for srpm and not rpm.\n");
    fprintf (fp, "   --construct-missing-files  Applies to RPM translation. Construct zero length\n");
    fprintf (fp, "                   files for header entries that have no files in the archive.\n");
    fprintf (fp, "   -v verbose messages about the translation.\n");
    fprintf (fp, 
		 "\n"
		 "Howto use with GNU swpackage:\n"
		 "\n"
		 "  cat your.rpm | lxpsf --psf-form3 -H ustar | swpackage -Wsource=- -s@PSF | tar tvf -\n"
		 "  cat your.deb | lxpsf | swpackage -Wsource=- -s@PSF | tar tvf -\n"
		 "    # Note: you may also install the ouput of lxpsf with tar into the file\n"
		 "    #       system, then run from that directory run swpackage normally.\n"
		 "\n"
		   );

	fprintf (fp, "lxpsf is an implementation extension utility of the GNU swbis project\n");
	fprintf (fp, "Report bugs to <bug-swbis@gnu.org>\n");
	fprintf (fp, "\n");
	show_version(fp);
	copyright_info(fp);
}
			
static
void
add_system_dirs(STRAR * exclude_list)
{
	strar_add(exclude_list, ".//");
	strar_add(exclude_list, "/bin//");
	strar_add(exclude_list, "/boot//");
	strar_add(exclude_list, "/dev//");
	strar_add(exclude_list, "/etc//");
	strar_add(exclude_list, "/etc/opt//");
	strar_add(exclude_list, "/etc/sgml//");
	strar_add(exclude_list, "/etc/skel//");
	strar_add(exclude_list, "/etc/X11//");
	strar_add(exclude_list, "/home//");
	strar_add(exclude_list, "/lib//");
	strar_add(exclude_list, "/media//");
	strar_add(exclude_list, "/mnt//");
	strar_add(exclude_list, "/opt//");
	strar_add(exclude_list, "/proc//");
	strar_add(exclude_list, "/root//");
	strar_add(exclude_list, "/sbin//");
	strar_add(exclude_list, "/srv//");
	strar_add(exclude_list, "/tmp//");
	strar_add(exclude_list, "/usr//");
	strar_add(exclude_list, "/usr/X11R6//");
	strar_add(exclude_list, "/usr/X11R6/bin//");
	strar_add(exclude_list, "/usr/X11R6/lib//");
	strar_add(exclude_list, "/usr/X11R6/lib/X11//");
	strar_add(exclude_list, "/usr/X11R6/include/X11//");
	strar_add(exclude_list, "/usr/bin//");
	strar_add(exclude_list, "/usr/bin/X11//");
	strar_add(exclude_list, "/usr/dict//");
	strar_add(exclude_list, "/usr/doc//");
	strar_add(exclude_list, "/usr/etc//");
	strar_add(exclude_list, "/usr/games//");
	strar_add(exclude_list, "/usr/include//");
	strar_add(exclude_list, "/usr/include/X11//");
	strar_add(exclude_list, "/usr/include/asm//");
	strar_add(exclude_list, "/usr/include/linux//");
	strar_add(exclude_list, "/usr/include/g++//");
	strar_add(exclude_list, "/usr/lib//");
	strar_add(exclude_list, "/usr/lib/X11//");
	strar_add(exclude_list, "/usr/lib/gcc-lib//");
	strar_add(exclude_list, "/usr/lib/groff//");
	strar_add(exclude_list, "/usr/lib/uucp//");
	strar_add(exclude_list, "/usr/local//");
	strar_add(exclude_list, "/usr/local/bin//");
	strar_add(exclude_list, "/usr/local/doc//");
	strar_add(exclude_list, "/usr/local/etc//");
	strar_add(exclude_list, "/usr/local/games//");
	strar_add(exclude_list, "/usr/local/lib//");
	strar_add(exclude_list, "/usr/local/include//");
	strar_add(exclude_list, "/usr/local/info//");
	strar_add(exclude_list, "/usr/local/man//");
	strar_add(exclude_list, "/usr/local/sbin//");
	strar_add(exclude_list, "/usr/local/share//");
	strar_add(exclude_list, "/usr/local/src//");
	strar_add(exclude_list, "/usr/man//");
	strar_add(exclude_list, "/usr/sbin//");
	strar_add(exclude_list, "/usr/share//");
	strar_add(exclude_list, "/usr/share/dict//");
	strar_add(exclude_list, "/usr/share/doc//");
	strar_add(exclude_list, "/usr/share/games//");
	strar_add(exclude_list, "/usr/share/info//");
	strar_add(exclude_list, "/usr/share/locale//");
	strar_add(exclude_list, "/usr/share/man//");
	strar_add(exclude_list, "/usr/share/man/man1//");
	strar_add(exclude_list, "/usr/share/man/man2//");
	strar_add(exclude_list, "/usr/share/man/man3//");
	strar_add(exclude_list, "/usr/share/man/man4//");
	strar_add(exclude_list, "/usr/share/man/man5//");
	strar_add(exclude_list, "/usr/share/man/man6//");
	strar_add(exclude_list, "/usr/share/man/man7//");
	strar_add(exclude_list, "/usr/share/man/man8//");
	strar_add(exclude_list, "/usr/share/man/man9//");
	strar_add(exclude_list, "/usr/share/misc//");
	strar_add(exclude_list, "/usr/share/nls//");
	strar_add(exclude_list, "/usr/share/sgml//");
	strar_add(exclude_list, "/usr/share/terminfo//");
	strar_add(exclude_list, "/usr/share/tmac//");
	strar_add(exclude_list, "/usr/share/zoneinfo//");
	strar_add(exclude_list, "/usr/src//");
	strar_add(exclude_list, "/usr/src/linux//");
	strar_add(exclude_list, "/usr/tmp//");
	strar_add(exclude_list, "/var//");
	strar_add(exclude_list, "/var/adm//");
	strar_add(exclude_list, "/var/backups//");
	strar_add(exclude_list, "/var/cache/man/cat1//");
	strar_add(exclude_list, "/var/cache/man/cat2//");
	strar_add(exclude_list, "/var/cache/man/cat3//");
	strar_add(exclude_list, "/var/cache/man/cat4//");
	strar_add(exclude_list, "/var/cache/man/cat5//");
	strar_add(exclude_list, "/var/cache/man/cat6//");
	strar_add(exclude_list, "/var/cache/man/cat7//");
	strar_add(exclude_list, "/var/cache/man/cat8//");
	strar_add(exclude_list, "/var/cache/man/cat9//");
	strar_add(exclude_list, "/var/catman/cat1//");
	strar_add(exclude_list, "/var/catman/cat2//");
	strar_add(exclude_list, "/var/catman/cat3//");
	strar_add(exclude_list, "/var/catman/cat4//");
	strar_add(exclude_list, "/var/catman/cat5//");
	strar_add(exclude_list, "/var/catman/cat6//");
	strar_add(exclude_list, "/var/catman/cat7//");
	strar_add(exclude_list, "/var/catman/cat8//");
	strar_add(exclude_list, "/var/catman/cat9//");
	strar_add(exclude_list, "/var/cron//");
	strar_add(exclude_list, "/var/lib//");
	strar_add(exclude_list, "/var/local//");
	strar_add(exclude_list, "/var/lock//");
	strar_add(exclude_list, "/var/log//");
	strar_add(exclude_list, "/var/opt//");
	strar_add(exclude_list, "/var/mail//");
	strar_add(exclude_list, "/var/msgs//");
	strar_add(exclude_list, "/var/preserve//");
	strar_add(exclude_list, "/var/run//");
	strar_add(exclude_list, "/var/spool//");
	strar_add(exclude_list, "/var/spool/at//");
	strar_add(exclude_list, "/var/spool/cron//");
	strar_add(exclude_list, "/var/spool/lpd//");
	strar_add(exclude_list, "/var/spool/mail//");
	strar_add(exclude_list, "/var/spool/mqueue//");
	strar_add(exclude_list, "/var/spool/news//");
	strar_add(exclude_list, "/var/spool/rwho//");
	strar_add(exclude_list, "/var/spool/smail//");
	strar_add(exclude_list, "/var/spool/uucp//");
	strar_add(exclude_list, "/var/tmp//");
	strar_add(exclude_list, "/var/yp//");
}

static
int
write_slack(TOPSF * topsf)
{
	int ret;
	int psffd;
	int nfd;
	TARU * taru;
	ETAR * etar;
	E_DEBUG("");

	psffd = swlib_open_memfd();
	ret = topsf_write_psf(topsf, psffd, 1 /* not used */);
	uxfio_lseek(psffd, 0, SEEK_SET);
	E_DEBUG("");

	nfd = G_OFD;
	ret = uxfio_fcntl(topsf->fd_, UXFIO_F_ARM_AUTO_DISABLE, 1);
	if (ret < 0) {
		E_DEBUG("error");
	}

	ret = uxfio_lseek(topsf->fd_, 0, SEEK_SET);
	if (ret < 0) {
		E_DEBUG2("%s", strerror(errno));
		return -1;
	}

	/* write the first archive member which should be the "./" */

	swlib_pump_amount(nfd, topsf->fd_, TARRECORDSIZE);

	/* Next, write the PSF file */

	taru = taru_create();
	etar = etar_open(taru->taru_tarheaderflagsM);
	etar_init_hdr(etar);
	etar_set_size_from_fd(etar, psffd, (int*)NULL);
	etar_set_pathname(etar, "PSF");
	etar_set_uname(etar, "root");
	etar_set_gname(etar, "root");
	etar_set_typeflag(etar, REGTYPE);
	etar_set_mode_ul(etar, (unsigned int)(0644));	
	etar_set_chksum(etar);
	etar_emit_header(etar, nfd);
	etar_emit_data_from_fd(etar, nfd, psffd);
	taru_delete(taru);

	/* Now just write the rest of the archive */

	ret = swlib_pipe_pump(nfd, topsf->fd_);
	if (ret < 0)
		return -1;
	E_DEBUG("");
	return 0;
}


static
int
write_plain_source_tarball(TOPSF * topsf)
{
	int ret;
	int psffd;
	int digfd;
	int nfd;
	STROB * namebuf;
	TARU * taru;
	ETAR * etar;
	SWVARFS * swvarfs;
	XFORMAT * xformat;
	struct stat st;
	char * name;
	int nullfd;

	nullfd = open("/dev/null", O_RDWR, 0);
	namebuf = strob_open(32);
	psffd = swlib_open_memfd();
	ret = topsf_write_psf(topsf, psffd, 1 /* not used */);
	uxfio_lseek(psffd, 0, SEEK_SET);
	E_DEBUG("");

	nfd = G_OFD;
	ret = uxfio_fcntl(topsf->fd_, UXFIO_F_ARM_AUTO_DISABLE, 1);
	if (ret < 0) {
		E_DEBUG("error");
	}

	E_DEBUG("");
	ret = uxfio_lseek(topsf->fd_, 0, SEEK_SET);
	if (ret < 0) {
		E_DEBUG2("%s", strerror(errno));
		return -1;
	}

	/* write the first archive member which should be the pathname prefix */
	/* swlib_pump_amount(nfd, topsf->fd_, TARRECORDSIZE); */

	/* Next, write the PSF file */

	E_DEBUG("");
	taru = taru_create();
	etar = etar_open(taru->taru_tarheaderflagsM);

	/* Next, write the PSF file */
	etar_init_hdr(etar);
	etar_set_size_from_fd(etar, psffd, (int*)NULL);
	etar_set_pathname(etar, "PSF");
	etar_set_uname(etar, "root");
	etar_set_gname(etar, "root");
	etar_set_time(etar, time(NULL));
	etar_set_typeflag(etar, REGTYPE);
	etar_set_mode_ul(etar, (unsigned int)(0644));	
	etar_set_chksum(etar);
	etar_emit_header(etar, nfd);
	etar_emit_data_from_fd(etar, nfd, psffd);

	/* Now write the checkdigest file if given. The source for the
	   checkdigest file is topsf->checkdigestnameM  Write it as
	   an archive member catalog/checkdigest */
	if (topsf->checkdigestnameM) {
		digfd = open(topsf->checkdigestnameM, O_RDONLY);
		if (digfd < 0) {
			fprintf(stderr, "%s: %s: %s\n", swlib_utilname_get(), topsf->checkdigestnameM, strerror(errno));
			return -1;
		}
		etar_init_hdr(etar);
		etar_set_size_from_fd(etar, digfd, (int*)NULL);
		etar_set_pathname(etar, "catalog/checkdigest");
		etar_set_uname(etar, "root");
		etar_set_gname(etar, "root");
		etar_set_typeflag(etar, REGTYPE);
		etar_set_mode_ul(etar, (unsigned int)(0644));	
		fstat(digfd, &st);
		etar_set_time(etar, st.st_mtime);
		etar_set_chksum(etar);
		etar_emit_header(etar, nfd);
		etar_emit_data_from_fd(etar, nfd, digfd);
	}

	taru_delete(taru);

	/* Now just write the rest of the archive stripping off the path prefix
	   because the directory keyword does not work for in-memory tar archives */

	xformat = xformat_open(-1, -1, arf_ustar);
	ret = uxfio_lseek(topsf->fd_, 0, SEEK_SET);
	if (ret < 0) {
		E_DEBUG("error");
		return -2;
	}
	
	ret = xformat_open_archive_by_fd(xformat, topsf->fd_,
			UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_NATIVE, (mode_t)(0));
	E_DEBUG("");
	if (ret < 0) {
		E_DEBUG("error");
		return -3;
	}

	E_DEBUG("");
	xformat_set_ofd(xformat, G_OFD);

	E_DEBUG("");
	while (
		xformat_read_header(xformat) > 0
	) {
		E_DEBUG("HERE: Storage");
		if (xformat_is_end_of_archive(xformat)){
			break;
		}
		xformat_get_name(xformat, namebuf);
		name = strob_str(namebuf);
		if (strstr(name, topsf->format_desc_->pathname_prefixM) == name) {
			name += strlen(topsf->format_desc_->pathname_prefixM);
			swlib_squash_leading_slash(name);
		}
		if (strlen(name) == 0) {
			if (xformat_file_has_data(xformat)) { 
				xformat_copy_pass_by_dst(xformat, nullfd);
			}
		} else {
			ahsStaticSetTarFilename(ahs_vfile_hdr(xformat_ahs_object(xformat)), name);
			xformat_write_header(xformat);
			if (xformat_file_has_data(xformat)) { 
				xformat_copy_pass_thru(xformat);
			}
		}
	}
	xformat_close(xformat);
	etar_write_trailer_blocks(NULL, G_OFD, 2);

	/*
	ret = swlib_pipe_pump(nfd, topsf->fd_);
	if (ret < 0)
		return -1;
	*/
	E_DEBUG("");
	close(nullfd);
	return 0;
}

static
int
write_deb(TOPSF * topsf)
{
	int ret;
	int data_fd;
	int control_fd;
	int psffd;
	int srcfd;
	int ofd;
	TARU * taru;
	ETAR * etar;
	DEBPSF * dp;
	XFORMAT * control_xformat;
	XFORMAT * data_xformat;
	STROB * tmpname;
	struct new_cpio_header * file_hdr;   
	AHS * ahs;
	char * name;

	ofd = G_OFD;
	tmpname = strob_open(32);
	psffd = swlib_open_memfd();
	ret = topsf_write_psf(topsf, psffd, 1 /* not used */);
	E_DEBUG2("psf returned %d", ret);
	if (ret < 0)
		return -1;
	uxfio_lseek(psffd, 0, SEEK_SET);

	/* Now write the PSF archive member and the control script
	   archive members, then write the data archive */

	dp = topsf->debpsfM;
	control_fd = dp->control_fdM; 
	data_fd = dp->source_data_fdM;
	control_xformat = xformat_open(-1, -1, arf_ustar);
	E_DEBUG("");
	if (!control_xformat)
		return -2;
	if (uxfio_lseek(control_fd, 0, SEEK_SET) != 0) {
		return -3;
	}
	E_DEBUG("");
	ret = xformat_open_archive_by_fd(control_xformat, control_fd,
		UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTEXT, (mode_t)(0));
	if (ret != 0)
		return -4;
	xformat_set_ofd(control_xformat, ofd);
	E_DEBUG("");

	/* write the PSF file */
	taru = taru_create();
	etar = etar_open(taru->taru_tarheaderflagsM);
	etar_init_hdr(etar);
	etar_set_size_from_fd(etar, psffd, (int*)NULL);
	etar_set_pathname(etar, "PSF");
	etar_set_uname(etar, "root");
	etar_set_gname(etar, "root");
	etar_set_typeflag(etar, REGTYPE);
	etar_set_mode_ul(etar, (unsigned int)(0644));	
	etar_set_chksum(etar);
	etar_emit_header(etar, ofd);
	etar_emit_data_from_fd(etar, ofd, psffd);
	taru_delete(taru);

	/* Loop through the control.tar.gz archive members */
	while ((ret=xformat_read_header(control_xformat)) > 0) {
		E_DEBUG("in loop");
		if (xformat_is_end_of_archive(control_xformat)) {
			E_DEBUG("");
			break;  /* FIXME: is this an error?? */
		}
		ahs = xformat_ahs_object(control_xformat);
		name = ahsStaticGetTarFilename(ahs->file_hdrM);
		E_DEBUG2("name=[%s]", name);
		if (xformat_get_tar_typeflag(control_xformat) == REGTYPE) {
			strob_strcpy(tmpname, DEBPSF_PSF_CONTROL_DIR); 
			swlib_unix_dircat(tmpname, name);
			ahsStaticSetTarFilename(ahs->file_hdrM, strob_str(tmpname));
			xformat_write_header(control_xformat);
			xformat_copy_pass_thru(control_xformat);
		} else {
			/*
			fprintf(stderr, "%s: unexpected archive member [%s]\n", swlib_utilname_get(), name);
			*/
			;
		}
	}
	E_DEBUG("");
	if (ret < 0) {
		fprintf(stderr, "%s: error reading control archive, ret=%d\n", swlib_utilname_get(), ret);
		return -5;
	}
	E_DEBUG("");

	data_xformat = xformat_open(-1, -1, arf_ustar);
	ret = xformat_open_archive_by_fd(data_xformat, data_fd,
		UINFILE_DETECT_OTARFORCE|UINFILE_DETECT_DEB_CONTEXT, (mode_t)(0));
	xformat_set_ofd(data_xformat, G_OFD);
	
	while ((ret=xformat_read_header(data_xformat)) > 0) {
		if (xformat_is_end_of_archive(data_xformat)) {
			break;
		}
		ahs = xformat_ahs_object(data_xformat);
		name = ahsStaticGetTarFilename(ahs->file_hdrM);
		E_DEBUG2("name=[%s]", name);
		/*
		strob_strcpy(tmpname, DEBPSF_PSF_DATA_DIR); 
		swlib_unix_dircat(tmpname, name);
		*/
		if (strcmp(name, "./") == 0) continue;	
		if (
			strlen(name) > 2 &&
			strstr(name, "./") == name
		) {
			name+=2;
		}
		strob_strcpy(tmpname, name);
		ahsStaticSetTarFilename(ahs->file_hdrM, strob_str(tmpname));
	
		xformat_write_header(data_xformat);
		if (xformat_get_tar_typeflag(data_xformat) == REGTYPE) {
			xformat_copy_pass_thru(data_xformat);
		} else {
			;
		}
	}
	etar_write_trailer_blocks(etar, ofd, 2);
	if (ret < 0) {
		fprintf(stderr, "%s: error reading data archive, ret=%d\n", swlib_utilname_get(), ret);
		return -6;
	}

	xformat_close(control_xformat);
	xformat_close(data_xformat);
	strob_close(tmpname);
	etar_close(etar);
	return 0;
}

int 
main(int argc, char **argv)
{
	int  nfd;
	TOPSF *topsf;
	STRAR * exclude_list;
	int no_leading_path = 0;
	int ret=0;
	int debfd;
  	int c=0;
	int info_only = 0;
	int psf_only = 0;
	int tarpipe[2];
	int verbose;
	char * owner;
	char * group;
	char * progname;
	char * checkdigestname = NULL;
	char * wopt_create_time = (char*)NULL;
	char * slack_name = NULL;
	char * path_prefix;
	char format[21];
	pid_t pid;	
	int bufferflag = UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
	int use_recursive_fileset_spec = 0;
	int single_fileset_override = 0;
	int smart_path=0;        
	int rpmform = 3;  /* --psf-form3 is default */
	int do_drop_priv = 0;
	int do_deb_control = 0;
	int do_deb_data = 0;
	char * debug_level = NULL;
	unsigned long create_time = 0;
	int construct_missing_files = 0;

	progname = argv[0];
	G_OFD = STDOUT_FILENO;
	verbose = 1;
	exclude_list = NULL;
	yylval.strb = strob_open (8);
	strcpy(swlex_filename,"none");
	*format='\0';
	owner = NULL;
	group = NULL;

	swlib_utilname_set("lxpsf");

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"psf-only", 0, 0, 'p'},
               {"psf-form1", 0, 0, 'A'},
               {"psf-form2", 0, 0, 'M'},
               {"psf-form3", 0, 0, 136},
               {"format", 1, 0, 'H'},
               {"debug_level", 1, 0, 'd'},
               {"checkdigest", 1, 0, 'D'},
               {"no-leading-path", 0, 0, 'r'},
               {"use-recursive-fileset", 0, 0, 'x'},
               {"buffer-type", 1, 0, 'B'},
               {"help", 0, 0, 132},
               {"version", 0, 0, 133},
               {"single-fileset", 0, 0, 134},
               {"util-name", 1, 0, 135},
               {"info-only", 0, 0, 'o'},
               {"smart-leading-path", 0, 0, 'L'},
               {"drop-privilege", 0, 0, 198},
               {"create-time", 1, 0, 199},
               {"deb-control", 0, 0, 200},
               {"deb-data", 0, 0, 201},
               {"owner", 1, 0, 202},
               {"group", 1, 0, 203},
               {"slackware-pkg-name", 1, 0, 204},
               {"exclude", 1, 0, 205},
               {"exclude-system-dirs", 0, 0, 206},
               {"construct-missing-files", 0, 0, 207},
               {"verbose", 0, 0, 'v'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "ArB:pH:xD:oLMv", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'A':
		 rpmform = 0;
		 break;
               case 136: /* --psf-form3 */
		 rpmform = 3;
		 break;
               case 'M': /* --psf-form2 */
		 rpmform = 2;
		 break;
               case 'r':
                 no_leading_path=1;        
		 break;
               case 'v':
                 verbose++;        
		 break;
               case 'x':
		 /* Use recursive fileset spec for binary packages */
                 use_recursive_fileset_spec=1;
		 break;
               case 'o':
                 info_only=1;        
		 break;
               case 'L':
                 smart_path=1;        
		 break;
               case 'p':
                 psf_only=1;        
		 break;
	       case 'B':
		 if (!optarg) usage(progname, stderr);
		 if (strstr(optarg, "mem")) {
			bufferflag = UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		 } else if (strstr(optarg, "file")) {
			bufferflag = UINFILE_UXFIO_BUFTYPE_FILE;
		 }
                 break;
	       case 'd':
		 if (!optarg) usage(progname, stderr);
		 debug_level = strdup(optarg);
                 break;
	       case 'D':
		 if (!optarg) usage(progname, stderr);
		 checkdigestname = strdup(optarg);
                 break;
	       case 'H':
		 if (!optarg) usage(progname, stderr);
		 strncpy(format,optarg, sizeof(format));
		 format[sizeof(format) - 1]='\0';
                 break;
               case 133:
	      	 show_version(stdout);
		 exit(0);
               case 134:
	      	 single_fileset_override = 1;
	       	 break;
               case 135:
	      	 swlib_utilname_set(optarg);
	       	 break;
	       case 132:
                 usage(progname, stdout);
		 exit(0);
		 break;
               case 198:
	      	 do_drop_priv = 1;
	       	 break;
               case 199:
		wopt_create_time = CHARTRUE;
		sscanf(optarg, "%lu", &create_time);
		break;
               case 200:
	      	 do_deb_control = 1;
		{
			UINFORMAT * uinformat;	
			if (optind < argc) {
				debfd = uinfile_open(argv[optind],  0, &uinformat, bufferflag /*UINFILE_DETECT_FORCEUNIXFD*/);
			} else {
				debfd = uinfile_open("-",  0, &uinformat, bufferflag /*UINFILE_DETECT_FORCEUNIXFD*/);
			}
			if (uinfile_get_type(uinformat) != DEB_FILEFORMAT) exit(1);
			if (debfd < 0) exit(1);
			ret = swlib_pipe_pump(G_OFD, debfd);
			uxfio_close(debfd);
			uinfile_close(uinformat);
			close(G_OFD);
			if (ret < 0)
				exit(1);
			else
				exit(0);	
		}
	      	break;
               case 201:
	      	 do_deb_data = 1;
		{
			int fd;
			UINFORMAT * uinformat;
			if (optind < argc) {
				debfd = uinfile_open(argv[optind],  0, &uinformat, 0 /*UINFILE_DETECT_FORCEUNIXFD*/);
			} else {
				debfd = uinfile_open("-",  0, &uinformat, 0 /*UINFILE_DETECT_FORCEUNIXFD*/);
			}
			if (uinfile_get_type(uinformat) != DEB_FILEFORMAT) exit(1);
			if (debfd < 0) exit(1);
			/* this pumps the control.tar into oblivion */
			ret = swlib_pipe_pump(-1, debfd);
			if (ret < 0)
				exit(1);
			/* fprintf(stderr, "uinformat->deb_file_fd_ is %d\n", uinformat->deb_file_fd_); */
			ret = uinfile_opendup(uinformat->deb_file_fd_, 0, &uinformat, UINFILE_DETECT_DEB_DATA);
			if (ret < 0) exit(1);
			if (uinfile_get_type(uinformat) != DEB_FILEFORMAT) exit(1);
			/* this pumps the data.tar to stdout */
			fd = ret;
			ret = swlib_pipe_pump(G_OFD, fd);
			uxfio_close(fd);
			uinfile_close(uinformat);
			close(G_OFD);
			if (ret < 0)
				exit(1);
			else
				exit(0);
		}
	         break;
               case 202:
			owner=strdup(optarg);
	         	break;
               case 203:
			group=strdup(optarg);
	         	break;
               case 204:
			/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
			slack_name = strdup(optarg);
	         	break;
               case 205:
			if (exclude_list == NULL) exclude_list = strar_open();
			strar_add(exclude_list, optarg);
	         	break;
               case 206:
			if (exclude_list == NULL) exclude_list = strar_open();
			add_system_dirs(exclude_list);
	         	break;
               case 207:
			construct_missing_files = 1;
	         	break;
               default:
                 usage(progname, stderr);
		 exit(1);
                 break;
               }
          }

	E_DEBUG("");
	if (optind < argc) {
		/* Open a regular file by name */
		E_DEBUG("");
		topsf = topsf_open(argv[optind], bufferflag, NULL);
		if (optind+1 < argc) {
			G_OFD = open(argv[optind+1], O_RDWR|O_CREAT|O_TRUNC, 0644);
			if (G_OFD < 0) {
				fprintf (stderr, "lxpsf: error opening %s: %s\n", argv[optind+1], strerror(errno));
				exit(1);
			}
		}


	} else {
		E_DEBUG("");
		topsf = topsf_open("-", bufferflag, NULL);	/* stdin */
	}
	if (!topsf) {
		exit(1);
	}

	topsf->verboseM = verbose;
	topsf->exclude_listM = exclude_list;

	topsf->rpm_construct_missing_filesM = construct_missing_files;

	if (owner)
		topsf->ownerM = strdup(owner);
	if (group)
		topsf->groupM = strdup(group);

	if (wopt_create_time) {
		topsf_set_mtime(topsf, (time_t)(create_time));
	}

	if (checkdigestname) {
		topsf->checkdigestnameM = strdup(checkdigestname);
	}

	E_DEBUG("");
	topsf->smart_path_ = smart_path;
	path_prefix = topsf_make_package_prefix(topsf, "");
	if (smart_path && uinfile_get_type(topsf->format_desc_) == RPMRHS_FILEFORMAT) {
		E_DEBUG("");
		strob_strcpy(topsf->control_directoryM, path_prefix);
		path_prefix = ".";
		topsf_set_cwd_prefix(topsf, path_prefix);
	}

	E_DEBUG("");
	if ((rpmform == 2 || rpmform == 3) && uinfile_get_type(topsf->format_desc_) == RPMRHS_FILEFORMAT) {
		strob_strcpy(topsf->control_directoryM, path_prefix);
		path_prefix = ".";
		topsf_set_cwd_prefix(topsf, path_prefix);
		if (rpmform == 2)
			topsf->form_ = TOPSF_PSF_FORM2;
		else if (rpmform == 3)
			topsf->form_ = TOPSF_PSF_FORM3;
		else 
			topsf->form_ = TOPSF_PSF_FORM3;
	}

	E_DEBUG("");
	if (no_leading_path || path_prefix == NULL) {
		E_DEBUG("");
		path_prefix = ".";
	}

	E_DEBUG("");
	topsf_set_cwd_prefix(topsf, path_prefix);

	topsf->single_fileset_ = single_fileset_override;
	topsf->use_recursive_ = use_recursive_fileset_spec;
	if (debug_level) {
		topsf->debug_link_ = 1;
	}

	if (!strcmp(format, SWBIS_A_ustar) || !strcmp(format, SWBIS_A_pax) ) {
		topsf->reverse_links_ = 1;
	}

	if (do_drop_priv) {
		swlib_drop_root_privilege();
	}

	if (psf_only){
		E_DEBUG("");
		ret = topsf_write_psf(topsf, G_OFD, 1 /* do indent */);
		E_DEBUG2("(topsf_write_psf) ret=%d", ret);
		
		if (uinfile_get_type(topsf->format_desc_) == SLACK_FILEFORMAT) {
			nfd=open("/dev/null", O_RDWR);
			swlib_pipe_pump(nfd, topsf->fd_);
			close(nfd);
		}
	} else if (info_only){
		E_DEBUG("");
		topsf->info_only_ = 1;
		ret=topsf_write_info(topsf, G_OFD, 1 /* do indent */);
	} else {
		if (uinfile_get_type(topsf->format_desc_) == DEB_FILEFORMAT) {
			ret = write_deb(topsf);
			if (ret != 0) {
				fprintf (stderr, "lxpsf: write_deb() returned %d\n", ret);
			}
		} else if (uinfile_get_type(topsf->format_desc_) == SLACK_FILEFORMAT) {
			E_DEBUG("");
			if (topsf->pkgfilenameM == NULL) {
				/* Need info from the name to translate a slackware package */
				if (slack_name == NULL) {
					fprintf (stderr, "%s: lxpsf: slackware package, specify package file as arg()\n",
						swlib_utilname_get());
					fprintf (stderr, "%s: lxpsf: Example: swpackage --to-sw -s name-revision-arch-build.tgz\n",
						swlib_utilname_get());
					exit(1);
				} else {
					/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
					topsf->pkgfilenameM = strdup(slack_name);
				}
			}
			E_DEBUG("");
			ret = write_slack(topsf);
			E_DEBUG("");
			;
		} else if (uinfile_get_type(topsf->format_desc_) == RPMRHS_FILEFORMAT) {

			/* topsf_write_psf must be run here to cause the file list to be generated
		   	 * ahead of calling topsf_copypass_swacfl_list(topsf) 
			 */
			E_DEBUG("");
	
			nfd=open("/dev/null", O_RDWR);
			topsf_write_psf(topsf, nfd, 0);
			close(nfd);		

			/* now the file link lists are generated.  
			 */

			if (strcmp(format, SWBIS_A_ustar) == 0 || strcmp(format, SWBIS_A_pax) == 0) {
				/*
				* Convert output format to ustar.
				*/
				TARU * taru;
				pipe(tarpipe);
				pid = swfork((sigset_t*)(NULL));
				if (pid < 0) {
					fprintf (stderr, "lxpsf: fork failed.\n");
					_exit(1);
				}
				if (pid == 0) {
					close(tarpipe[0]);
					ret=topsf_copypass_swacfl_list(topsf, tarpipe[1]);
					topsf_close(topsf);	
					_exit(ret);
				} else {
					close(tarpipe[1]);
					taru = taru_create();
					taru_set_tar_header_policy(taru, SWBIS_A_pax, NULL);
					ret = taru_process_copy_in(taru, tarpipe[0], G_OFD);	
				}
			} else {
				ret=topsf_copypass_swacfl_list(topsf, G_OFD);
			}
		} else if (uinfile_get_type(topsf->format_desc_) == PLAIN_TARBALL_SRC_FILEFORMAT) {
			ret = write_plain_source_tarball(topsf);
		} else {
			E_DEBUG("");
			fprintf (stderr, "lxpsf: unsupported format.\n");
		}
		E_DEBUG("");
	}
	E_DEBUG("");
	if (G_OFD != STDOUT_FILENO) close(G_OFD);
	topsf_close(topsf);	
	exit(ret ? 1 : 0);
}
