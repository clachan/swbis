/* swpackage.cxx -- The swbis tar archive packager
 */

/*
   Copyright (C) 2003-2009,2013,2014 Jim Lowe
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA */

#define OPEN_MAX 28 

#define SWPNEEDDEBUG 1
#undef SWPNEEDDEBUG 

#ifdef SWPNEEDDEBUG
#define SWP_E_DEBUG(format) SWBISERROR("DEBUG: ", format)
#define SWP_E_DEBUG2(format, arg) SWBISERROR2("DEBUG: ", format, arg)
#define SWP_E_DEBUG3(format, arg, arg1) SWBISERROR3("DEBUG: ", format, arg, arg1)
#else
#define SWP_E_DEBUG(arg)
#define SWP_E_DEBUG2(arg, arg1)
#define SWP_E_DEBUG3(arg, arg1, arg2)
#endif 

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <typeinfo>
#include "stream_config.h"

extern "C" {
#include "swparse.h"
#include "usgetopt.h"
#include "ugetopt_help.h"
#include "swlib.h"
#include "swfork.h"
#include "swevents.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swcommon.h"
#include "swgp.h"
#include "fmgetpass.h"
#include "swutilname.h"
#include "swfdio.h"
#include "swextopt.h"
#include "swgpg.h"
#include "swproglib.h"
}

#include "swmain.h"  /* this file should be included by the main program file */
#include "swparser.h"
#include "swdefinitionfile.h"
#include "swpsf.h"
#include "swpackagefile.h"
#include "swexstruct.h"
#include "swexdistribution.h"

#include "swexfileset.h"
#include "swexproduct.h"
#include "swexpsf.h"
#include "swexdistribution.h"
#include "swscollection.h"

#define PASSPHRASE_LENGTH 240
#define DEVNULL			"/dev/null"
#define SWC_VERBOSE_SWPV	SWPACKAGE_VERBOSE_V0  // This the the verbose level that includes additional verboseness
#define SWP_PASS_TTY		"tty"             // Special name of passphrase_fd to indicate use the tty
#define SWP_PASS_AGENT		"agent"             // Special name of passphrase_fd to indicate use the GPG agent
#define SWP_PASS_ENV		"env"               // Special name of passphrase_fd to indicate use the environment
#define PROGNAME		"swpackage"
#define SWPARSE_AT_LEVEL	0
#define SWPROG_VERSION		SWPACKAGE_VERSION

#define WOPT_LAST		230
#define	SWPACKAGE_PIPE_BUF	PIPE_BUF
#define ARMORED_SIGLEN		1024	/* signature length */
#define IST(x)		is_option_true(x)

static char progName[] = PROGNAME;

static int g_nullfd;
static int g_fail_loudly = -1; 	/* Set to 0 to turn off failure messages. */
static int verboseG = 1;  	/* Default posix verbosity. */

static char * CHARTRUE = "True";
static char * CHARFALSE = "False";
static int g_passfd = -1;
static char * g_passphrase = (char*)NULL;
static char * g_wopt_numeric_owner = (char*)NULL;
static char * g_wopt_dir_numeric_owner = (char*)NULL;
static char * g_wopt_catalog_numeric_owner = (char*)NULL;
static char * g_wopt_dummy_sign = (char*)NULL;
static swPSF * g_psf;
static int g_stdin_use_count = 0;

static char * G_backup_name = (char*)NULL;
static char * G_orig_name = (char*)NULL;

#define FATAL() internal_fatal_error("", __LINE__)
#define FATAL2(a) internal_fatal_error(a, __LINE__)

#define set_CATDIR "catdir"
#define set_PREFIX "prefixdir"
					
#define RESIGN_ORIG_SUFFIX "~"

static
int
get_stderr_fd(void)
{
	return 2;
}

static 
void user_fatal_error(char * s)
{
	swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "fatal: %s", s);
        exit(1);
}

static 
void internal_fatal_error(char * reason, int line)
{
	swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
        	"internal fatal error : line %d : %s\n", line, reason);
        exit(1);
}

static
int
is_option_true(char * s)
{
	return swextopt_is_value_true(s);
}

static			
char *
set_numeric(char * useropt, char * target)
{
	char * s;
	int r;
	/*
	* If already true, don't reset to false
	*/
	r = is_option_true(target);
	if (r) return CHARTRUE;
	
	if (!useropt || strlen(useropt) == 0) 
		return (char*)(NULL);

	s = useropt;
	while(*s) {
		/*
		* Is the name all digits, if so then assume it
		* is an id, not a name
		*/
		if (isdigit((int)(*s)) == 0)
			return (char*)(NULL);
		s++;
	}
	return CHARTRUE;
}

static
void
close_passfd(void)
{
	if (g_passfd >= 0) {
		if (close(g_passfd) < 0) {
			//fprintf(stderr, 
			//	"error closing passphrase fd [%d] : %s\n", 
			//		g_passfd, strerror(errno));
			;
		}
	}
}

static
void 
no_feature(char * s)
{
	swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s : feature not implemented\n", s);
}

static 
void 
no_feature_fatal(char * s)
{
	swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s : feature not implemented\n", s);
	exit(1);
}

static 
void 
copyright_info(FILE * fp)
{
	fprintf(fp,  "%s",
"Copyright (C) 2003-2008, 2014 Jim Lowe\n"
"Portions are copyright 1985-2000 Free Software Foundation, Inc.\n"
"This software is distributed under the terms of the GNU General Public License\n"
"and comes with NO WARRANTY to the extent permitted by law.\n"
"See the file named COPYING for details.\n");
}

static
void 
about_info(FILE * fp)
{
	fprintf(fp,  "%s",
"Written by Jim Lowe\n"
"The portable archive packager and signing tool of the swbis project.\n"
"Conforming to IEEE 1387.2-1995 (ISO/IEC 15068-2:1999) with extensions.\n"
"Other related standards are: Open Group CAE C701.\n");
}

static void 
version_string(FILE * fp)
{
	fprintf(fp,  
	"%s (swbis) version " SWPACKAGE_VERSION "\n",
		progName);
}


static void 
version_info(FILE * fp)
{
	version_string(fp);
	copyright_info(fp);
}

static int 
usage(FILE * fp, 
	char * pgm, 
	struct option lop[], 
	struct ugetopt_option_desc helpdesc[])
{
	version_info(fp);
	fprintf(fp, "%s",
"\n"
"swpackage reads a Product Specification File (PSF) and writes a distribution\n"
"in the form of a tar archive to the specified target.  If no options are given\n"
"a PSF is read on stdin.  This implementation only supports writing to standard\n"
"output. To specify standard output use a dash '-' as the target.\n"
"\n");

	fprintf(fp, "%s",
"Usage:\n"
"\n"
"       swpackage [-p] [-f file] [-s psf] [-x option=value] [-X options_file]\\\n"
"                 [-W option[=value]] [options] [software_selections][@target]\n"
"       swpackage\n"
"       swpackage --to-sw -s -[options]\n"
"       swpackage --to-sw -s slackware_pkg.tgz [options]\n"
"       swpackage --to-sw --slackware-pkg-name=slackware_pkg.tgz -s - [options]\n"
"       swpackage --resign -s FILE [options]  # write to stdout, same target  \n"
"                                             # conventions apply \n"
"");
	fprintf(fp , "\nOptions:\n\n");

	ugetopt_print_help(fp, pgm, lop, helpdesc);

	fprintf(fp , "%s", "\n");

fprintf(fp, "%s",
"\n"
"   software_selections   Refer to the software objects (products, filesets)\n"
"                         on which to be operated. (Not yet implemented)\n"
"   target    Refers to the software_collection where the software\n"
"             selections are to be applied.  To specify standard output\n"
"             use a  dash '-', this overrides media_type setting to 'serial'.\n"
"             Target may be a file system directory, or device file or '-'\n"
"\n"
" Examples:\n"
"    Explicity use as a filter:\n"
"        swpackage -s - @ -\n"
"\n"
"    Show the options and locations of options files:\n"
"        swpackage --show-options\n"
"        swpackage --show-options-files\n"
"\n"
"    Read the psf file /usr/src/develop/product_list and write a tar\n"
"    archive to standard output:\n"
"        cat /tmp/foo.psf | swpackage | tar tvf -\n"
"        swpackage -s /tmp/foo.psf @- | tar tvf -\n"
"\n"
"    Translate a rpm, deb or plain source tarball:\n"
"        cat your_package | swpackage --to-swbis -s - @- | tar tvf -\n"
"\n"
"    Preview a package that will have identical listing\n"
"    for subsequent repeated invocations:\n"
"       swpackage -p -x verbose=4  -Wcreate-time=123 -s PSF\n"
"\n"
"    Resign a package in place\n"
"       swpackage -s foo.tar.bz2 --resign --overwrite --recompress\n"
"\n"
" Serial Archive Format:\n"
"\n"
"  By default, the format is POSIX tar.  The format is bit-for-bit identical\n"
"  to GNU tar 1.15.1 using options ''--format=ustar -b1''\n"
"  When the 'pax' format is specified, extended headers will be used as\n"
"  required.  This implementation does not allow (at this time) extended\n"
"  headers in the catalog section of the package. \n"

"\n"
" Posix Extended Options:  Access with -x option\n"
"\n"
"   These options may appear in the  <libdir>/swbis/swdefaults\n"
"   They may be overridden using the -x option.\n"
"        Syntax : -x option=option_argument [-x ...]\n"
"            or : -x \"option=option_argument  ...\"\n"
"   distribution_target_directory = target  # Applies when media_type=directory\n"
"   distribution_target_serial    = target  # Applies when media_type=serial\n"
"   media_type                    = serial  \n"
"   media_capacity                = 0       # 0 indicates infinite capacity.\n"
"   enforce_dsa                   = false   # Enforce Disk space, Not respected.\n"
"   follow_symlinks               = false\n"
"   logfile                       =         # Not respected.\n"
"   psf_source_file               = -       # Not respected.\n"
"   verbose                       = 1\n"
"              LEVEL is 0,1,2,3,..  Level 1 is the default.\n"
"              Level 0 not implemented.\n"
"\n"
" Implementation Extension Extended Options:\n"
"\n"
"   These can be accessed with the -x option.\n"
"   These options may appear in the ~/.swbis/swbisdefaults file.\n"
"   Syntax:\n"
"       swpackage.swbis_<option> = <value>\n"
"\n"
"          where : <option> is the -W option of the same name using\n"
"                  a dash '-' instead of an underscore '_'.\n"
"   swbis_signer_pgm \n"
"   swbis_cksum    \n"
"   swbis_file_digests    \n"
"   swbis_file_digests_sha2    \n"
"   swbis_files    \n"
"   swbis_sign    \n"
"   swbis_archive_digests    \n"
"   swbis_archive_digests_sha2    \n"
"   swbis_gpg_name    \n"
"   swbis_gpg_path    \n"
"   swbis_gzip    \n"
"   swbis_bzip2    \n"
"   swbis_numeric_owner    \n"
"   swbis_absolute_names    \n"
"   swbis_format    \n"
"   swbis_check_duplicates  \n"
"\n");

fprintf(fp , "%s",
" Implementation Extension Options:\n"
"\n"
"       Syntax : -W option[=option_argument] [-W ...]\n"
"           or   -W option[=option_argument],option...\n"
"           or   --option[=option_argument]\n"
"\n"
" Alternate Modes:\n\n"
"   --addsign   Same as --add-signature-first\n"
"   --delsign   Opposite of --addsign,  Same as --remove-signature=1\n"
"   --add-signature-first\n"
"   --add-signature-last \n"
"   --replace-signature=N  (replace signature, 0 is last sig),\n"
"   --remove-signature=N   (remove signature, 0 is last sig),\n"
"   --resign (same as --replace-signature=0)\n"
"          Replace last signature. Read previously signed package and alter the\n"
"          signature. The source file (-s FILE) is the previously signed package.\n"
"          resigned file is written to stdout unless directed otherwise\n"
"   --resign-test, --zfilter\n"
"          Does not generate new signature, decodes stdin and writes to stdout.\n"
"          Uncompressed ''stdin'' should be identical bit-for-bit to ''stdout''\n"
"          (allows access to decompression and recompression machinery)\n"
"   --recompress   Applies to --resign modes, recompress same as original file\n"
"   --overwrite    Applies to --resign modes when the source and target file\n"
"                  are the same file or only a source file is give.\n"
"                  A backup file (~ appended) will be created and removed.\n"
"\n"
"   --undebian     same as --to-swbis --exclude-system-dirs\n"
"   --unrpm        same as --to-swbis\n"
"   --to-swbis     Automatically detect RPM, deb, slackware, or plain tarball\n"
"                  package and translate to equivalent swbis archive.\n"
"                  This is internally similar to:\n"
"                    SWBISLIBEXECDIR/swbis/lxpsf --psf-form3 -H ustar |\n"
"                        swpackage -Wsource=- -s@PSF \n"
"           The configured SWBISLIBEXECDIR is:\n"
"                 " SWBISLIBEXECDIR "\n"
"\n"
"   --show-options   Show extended options to stdout and exit.\n"
"   --show-options-files  Show swdefaults files that will be parsed and exit.\n"
"   --show-signer-pgm  Show the signer pgm and options and then exit.\n"
"   --write-signed-file-only  Do all the signing steps but don't actually\n"
"                       sign it. The output *is* the byte stream that is signed.\n"
"                       (This is really only a testing option, never used.)\n"
"\n"
" Archive and File Security Options:\n\n"
"   --dereference    don't dump symlinks; dump the files they point to\n"
"                    same as -x follow_symlinks=True\n"
"   --cksum          compute posix cksum of the individual files\n"
"   --sha1           same as --file-digests and --archive-digests\n"
"   --sha2           same as --file-sha512 and --archive-sha512\n"
"   --files          store the distribution file list in .../dfiles/files\n"
"   --archive-digests compute the digests (md5, sha1) of the archive\n"
"   --archive-digests-sha2 compute the digests (sha512) of the archive\n"
"   --digests        same as file-digests (Deprecated)\n"
"   --file-digests   compute digests (md5, sha1) of the individual files\n"
"   --file-digests-sha2 compute digests (sha512) of the individual files\n"
"\n"
" Package Signature Options:\n\n"
"   --gpg-name=NAME  The gpg name, i.e the gpg --local-user\n"
"   --local-user=NAME Same as gpg-name.\n"
"   --homedir=PATH   Same as gpg-path\n"
"   --gpg-homedir=PATH  path to the gpg keys, e.g. ~/.gnupg \n"
"   --nosign         override defaults file setting or turn of sign option.\n"
"   --sign           compute the digests of the archive and create an embedded\n"
"                    digital signature of the catalog portion of the package\n"
"   --dummy-sign     same as --sign except use a dummy signature\n"
"   --signer-pgm={PGP2.6|PGP5|GPG|GPG2}\n"
"                  Specify either PGP2.6, PGP5 or GPG (default)\n"
"   --no-sha1         do not compute the archive sha1 digest\n"
"   --passfile=FILE  Read passphrase from file named FILE.\n"
"                  Setting FILE to a dash '-' (stdin) is supported but not\n"
"                  recommended.  Setting FILE to /dev/tty (re)sets the default\n"
"                  action, which is reading from the terminal with echo off.\n"
"   --passphrase-fd=N  Read passphrase from file descriptor N and not /dev/tty.\n"
"                    May be set to \"agent\" to use the GPG agent.\n"
"   --use-agent      Same as --passphrase-fd=agent\n"
"\n"
" Output Layout Options:\n\n"
"   --dir=NAME       Use NAME as the path name prefix of a distribution and also\n"
"                  as the value of the <distribution>.control_directory and\n"
"                  <distribution>.tag attribute (if not set)\n"
"   --no-front-dir   do not write the leading directory archive member.\n"
"   --no-catalog     do not write the catalog\n"
"   --catalog-only   only write out the exported catalog section\n"
"   --storage-only   only write out the storage section and leading dir\n"
"   --regfiles-only  include only regular files in storage section\n"

"\n"
" Output Compression Options:\n\n"
"    Multiple directives may be given to produce layers of\n"
"    compression and/or encryption in the order given on command line.\n"
"\n"
"   --xz               compress using xz as found in PATH\n"
"   --lzma             compress using lzma as found in PATH\n"
"   --bzip2            compress using bzip2 as found in PATH\n"
"   --gzip             compress using gzip as found in PATH\n"
"   --symmetric        encrypt using gpg as found in PATH\n"
"   --encrypt-for-recipient=NAME  encrypt for NAME using gpg as found in PATH\n"
"\n"
" Archive Format Options:\n\n"
"   --no-overflow-headers  fail if any field overflows, not just file names\n"
"   --numeric-owner  Same as GNU tar option.\n"
"   --absolute-names Same as GNU tar option, allow absolute paths in archive\n"
"   --dir-owner=OWNER    owner of the leading package directory\n"
"   --dir-group=GROUP    group of the leading package directory\n"
"   --dir-mode=MODE      modeof the leading package directory\n"
"             If MODE is '.' then use the permissions of current directory\n"
"   --catalog-owner=OWNER  archive attributes owner of the catalog section\n"
"   --catalog-group=GROUP  archive attributes group of the catalog section\n"
"   --pax-header-pid=NUMBER    Set pax header pid to NUMBER\n"
"   --uuid=UUID      Use UUID as the uuid attribute value stored in INDEX.\n" 
"   --create-time=TIME  applies to catalog files and the create_time attribute.\n" 
"                       TIME is the seconds since the Unix Epoch.\n" 
"   -H,--format=FORMAT  FORMAT is one of ustar, pax, gnutar, newc, crc, odc\n"
"          ustar   is the Posix.1 tar format capable of storing\n"
"                  pathnames up to 255 characters in length.\n"
"                  Identical to GNU tar 1.15.1 --format=ustar\n"
"          ustar0  (deprecated) is same as ustar except the devminor/demajor\n"
"                  fields are different for non device files.\n"
"                  (Same as GNU tar 1.13.25 --posix -b1  for names <99 chars)\n"
"          oldgnu  same GNU tar version 1.13.25 default compilation with block\n"
"                  size set to 1 (i.e. -b1)\n"
"                  Supports pathnames up to 511 characters long\n"
"          gnu     same as GNU tar 1.15.1 default compilation\n"
"          gnutar  same as 'gnu'\n"
"          bsdpax3 is another personality of the Posix.1 tar format\n"
"                  Identical to OpenBSD pax v1.5 with Suse GNU/Linux fixes\n"
"                  e.g.  c6813e180cf914839e1b21973a5462b5  pax-3.0.tar.bz2.\n"
"          pax     (default) ustar format with extended headers\n"
"          posix   same as 'pax'\n"
"          odc     cpio format (magic 070707), as specified in POSIX.1\n"
"          newc    cpio format (magic 070701).\n"
"          crc     cpio format (magic 070702).\n"
"\n"
" Special Format-to-swbis Translation Options:\n\n"
"   --slackware-pkg-name=NAME Name of the tarball file.  Allows swpackage\n"
"                        to read from stdin and still have the name info\n"
"   --checkdigeset-file=FILE checkdigest script, applies to plain tarball\n"
"                        translation\n"
"   --source=FILE    Package files are sourced from serial archive FILE,\n"
"                  the file system contents are ignored.\n"
"                  Note:  Posix option '-s' may have option_arg prefixed\n"
"                  with '@' to indicate the PSF is sourced from within the\n"
"                  archive. (This is an implementation extension.)\n"
"   --construct-missing-files   Add zero length files for files in the RPM\n"
"                  Header but not found in the RPM Archive\n"
"   --exclude-system-dirs  adds --exclude-system-dirs option to lxpsf invocation.\n"
"                  Excludes system directories identified by hier(7) and FHS 2.2\n"
"\n"
" Other  Options:\n\n"
"   --files-from=LIST Read LIST containing a list of files.  Directories are\n"
"                   not descended recursively.\n"
"   --check-duplicates={Yes|No} handle duplicate definitions in a PSF correctly.\n"
"                              Saying No speeds up processing of large packages.\n"
"   --no-defaults    do not read any defaults files\n"
"   --debug=LEVEL    LEVEL=1: Writes debug info, does not write the package.\n"
"   --noop           no operation. A legal option with no effect.\n"
"\n");

fprintf(fp , "%s",
"\n"
"Product Specification Files (PSF) :\n"
"\n"
"  Hewlett-Packard SD-UX Compatibility:\n"
"    The SD-UX swpackage supports a ''end'' keyword in the PSF file which is\n"
"    not specified in the ISO standard.  The swbis (this) swpackage will\n"
"    read and process PSFs with the ''end'' keyword, but it will not use it to\n" 
"    disambiguate the PSF grammar.  This ambiguity most likely involves\n"
"    misplacement of ''control_file'' objects by removal from the product\n"
"    object and placement in the fileset object.\n"
"\n"
"Example PSF:\n"
"\n"
"    # PSF.source -- An example PSF suitable for packaging a\n"
"    #               source distribution.\n"
"    distribution\n"
"     description \"package description\"\n"
"     title \"package title\"\n"
"     package_name swbis     # Unrecognized attributes are allowed.\n"
"     package_version 0.1    # Unrecognized attributes are allowed.\n"
"     vendor\n"
"      the_term_vendor_is_misleading true # true or false\n"
"      tag my_version1\n"
"     product\n"
"      tag somepackage # the package name\n"
"      revision 1.0 # the revision\n"
"      vendor_tag my_version1 # a new release \n"
"      control_directory \"\"    # Empty control directory\n"
"      fileset\n"
"       tag source_files\n"
"       control_directory \"\"   # Empty control directory.\n"
"       file_permissions -o root -g root   # Optional\n"
"       directory .        # Set the directory mapping.\n"
"       file *             # Package all the files.\n"
"       exclude catalog    # Exclude the catalog directory.\n"
"\n"
"Environment:\n"
"\n"
"  GNUPGHOME    Sets the --gpg-home option\n"
"  GNUPGNAME    Sets the --gpg-name option\n"
"  SWPACKAGEPASSFD   Sets the --passphrase-fd option.  Set this variable\n"
"                    to a file descriptor, or \"agent\", or \"env\"\n"
"  SWPACKAGEPASSPHRASE  If --passphrase-fd=\"env\", take passphrase from this\n"
"                       variable.  (Storing your passphrase in the environment\n"
"                       is not secure, however, it can be useful for testing\n"
"                       with dummy GPG keys.)\n"
"\n"
	);
	
	// version_string(fp);
	fprintf(fp , "%s", 
        "Report bugs to " REPORT_BUGS "\n"
	);
	return 0;
}

SHCMD **
get_compression_vector_from_vplob(VPLOB * vplob)
{
	if (vplob == NULL) return NULL;
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

static
void
restore_overwrite_and_exit(char * backup_name, char * orig_name, int exit_val)
{
	int ret;
	if (backup_name && orig_name) {
		if (strlen(backup_name)) {
			swgp_close_all_fd(3);
			ret = rename(backup_name, orig_name);
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"restoring %s to %s\n", backup_name, orig_name);
			if (ret < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
					"restoration of original file failed for %s : reason: %s\n", orig_name, strerror(errno));
			}
		}
	}
	exit(exit_val);
}


static
void
resign_sig_handler(int signum)
{
	swgp_signal_block(SIGTERM, (sigset_t *)NULL);
	swgp_signal_block(SIGINT, (sigset_t *)NULL);
	swgp_signal_block(SIGPIPE, (sigset_t *)NULL);
	swgp_close_all_fd(3);
	restore_overwrite_and_exit(G_backup_name, G_orig_name, signum);
	//
	// Never gets here, function above exits
	//
	exit(127);
}

static
int
set_stdio_output_fd(swExCat * swexdist, char * cl_target, int flags)
{
	int ofd_retval;
	//
	// Set default use of stdio.
	//
	ofd_retval = -1;
	SWP_E_DEBUG("");
	if (verboseG == 0) {
		SWP_E_DEBUG("verbose==0");
		if (swexdist) swexdist->set_preview_fd(g_nullfd);
		if (swexdist) swexdist->set_ofd(g_nullfd);
		ofd_retval = g_nullfd;
	} else {
		if (strcmp(cl_target, "/dev/null") == 0) {
			//
			// /dev/null
			//
			SWP_E_DEBUG("/dev/null");
			if (swexdist) swexdist->set_ofd(g_nullfd);
			ofd_retval = g_nullfd;
		} else if (strcmp(cl_target, "-") == 0) {
			//
			// STDOUT
			//
			SWP_E_DEBUG("stdout");
			ofd_retval = STDOUT_FILENO;
			if (swexdist) swexdist->set_ofd(STDOUT_FILENO);
			if (swexdist) swexdist->set_preview_fd(STDERR_FILENO);
		} else {
			//
			// Open output file
			//
			SWP_E_DEBUG2("open file: [%s]", cl_target);
			ofd_retval = open(cl_target, flags /*O_RDWR|O_TRUNC|O_CREAT */, 0664);
			if (ofd_retval < 0) {
				if (swexdist) swexdist->set_ofd(-1);
				return -1;
			}	
			
		}
	}
	return ofd_retval; 
}

static
char *
check_name_to_convert(char * name, int (*uf)(uid_t,  char *), int * status)
{
	unsigned char * s;
	uintmax_t n;
	char * end;
	int ret;
	char dbname[TARU_SYSDBNAME_LEN];

	*status = 0;
	if (name == NULL) return name;
	if (strlen((char*)name) == 0) return name;
	
	s = (unsigned char*)name;
	while (*s) {
		if (!isdigit((int)(*s)))
			return name;
		s++;
	}

	/* Its all digits, convert it to a name */

	*dbname = '\0';
	n = strtoumax(name, &end, 10);
	/* FIXME check value of end */	

	if (uf == taru_get_tar_user_by_uid) {
		uid_t uid;
		uid = (uid_t)n;
		ret = taru_get_tar_user_by_uid(uid, dbname);
		if (ret) {
			*status = 1;		
			return name;
		}
	}  else {
		gid_t gid;
		gid = (gid_t)n;
		ret = taru_get_tar_group_by_gid(gid, dbname);
		if (ret) {
			*status = 1;		
			return name;
		}
	}
	return strdup(dbname);
}


static
int
set_user_name_from_psf(swExCat * swexdistribution, char * which_one, char * att, char ** wopt_name, char ** g_numeric)
{
	char * tmpname;

	tmpname = swexdistribution->getReferer()->find(att);
	if (*wopt_name != NULL) {
		SWP_E_DEBUG("");
		//
		// Set the value given on the command line. Ignore the value in the PSF
		//
		if (strcmp(att, SW_A_owner) == 0) {
			if (strcmp(which_one, set_CATDIR) == 0) {
				swexdistribution->setCatalogOwner(*wopt_name);
			} else {
				swexdistribution->setLeadingDirOwner(*wopt_name);
			}
		} else if (strcmp(att, SW_A_group) == 0) {
			if (strcmp(which_one, set_CATDIR) == 0) {
				swexdistribution->setCatalogGroup(*wopt_name);
			} else {
				swexdistribution->setLeadingDirGroup(*wopt_name);
			}
		} else {
			FATAL();
		}

		*g_numeric = set_numeric(*wopt_name, *g_numeric);
		return 1;
	}
	
	if (tmpname) {
		SWP_E_DEBUG("");
		//
		//
		*g_numeric = set_numeric(tmpname, *g_numeric);
		*wopt_name = strdup(tmpname);
		if (strcmp(att, SW_A_owner) == 0) {
			if (strcmp(which_one, set_CATDIR) == 0)
				swexdistribution->setCatalogOwner(tmpname);
			else
				swexdistribution->setLeadingDirOwner(tmpname);
		} else if (strcmp(att, SW_A_group) == 0) {
			if (strcmp(which_one, set_CATDIR) == 0)
				swexdistribution->setCatalogGroup(tmpname);
			else
				swexdistribution->setLeadingDirGroup(tmpname);
		} else {
			FATAL();
		}
		return 1;
	}

	// Nothing set, report this as zero
	return 0;
}


static
int
do_unrpm(char * listpsf, char * create_time_is_set,
		unsigned long create_time,
		char * pkgfilename,
		char * checkdigestname,
		char * owner, char * group, char * slack_name, int construct_missing, int exclude_system_dirs)
{
	int lfd[2];
	STROB * tmpx = strob_open(32);
	SHCMD * cmd[2];
	pid_t pid[3];
	int status[3];
	char * lxpsfpath;
	char * lxpsfPathArray[2];
	char * pa0 = SWBISLIBEXECDIR "/swbis/lxpsf";

	char ** lxpsfPathArrayP = lxpsfPathArray;

	lxpsfPathArray[0] = pa0;
	lxpsfPathArray[1] = (char*)NULL;

	lxpsfpath = *lxpsfPathArrayP;
	while(lxpsfpath) {
		if (access(lxpsfpath, X_OK|R_OK) == 0) break;
		lxpsfpath = *(++lxpsfPathArrayP);
	}

	if (!lxpsfpath) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"The libexec program swbis/lxpsf was not found\n"
			"in " SWBISLIBEXECDIR " .\n"
             		"Maybe you need to compile and install swbis using the\n"
			"configure option \"--with-rpm\".\n");
			exit(1);
	}
	
	cmd[0]=shcmd_open();
	cmd[1]=(SHCMD*)NULL;

	if (pipe(lfd)) FATAL();
	pid[0] = swfork((sigset_t*)(NULL));
	if (pid[0] < 0) exit(1);
	if (pid[0] == 0) {
		close_passfd();
		close(lfd[0]);	
		swlib_drop_root_privilege();
		shcmd_add_arg(cmd[0], lxpsfpath);
		if (listpsf == NULL) {
			shcmd_add_arg(cmd[0], "--psf-form3");
			shcmd_add_arg(cmd[0], "--util-name=swpackage: (lxpsf)");
			if (construct_missing == 1)
				shcmd_add_arg(cmd[0], "--construct-missing-files");
			if (exclude_system_dirs)
				shcmd_add_arg(cmd[0], "--exclude-system-dirs");
			shcmd_add_arg(cmd[0], "--drop-privilege");
			shcmd_add_arg(cmd[0], "-H");
			shcmd_add_arg(cmd[0], SWBIS_A_pax);  /* Was ustar, changed 6/14/2014 */
			if (slack_name) {
				/* fprintf(stderr, "JL you're a SLACKER: %s:%s at line %d\n", __FILE__, __FUNCTION__, __LINE__); */
				strob_sprintf(tmpx, 0, "--slackware-pkg-name=%s", slack_name);
				shcmd_add_arg(cmd[0], strob_str(tmpx));
			}
			if (owner) {
				strob_sprintf(tmpx, 0, "--owner=%s", owner);
				shcmd_add_arg(cmd[0], strob_str(tmpx));
			}
			if (group) {
				strob_sprintf(tmpx, 0, "--group=%s", group);
				shcmd_add_arg(cmd[0], strob_str(tmpx));
			}
			if (checkdigestname) {
				shcmd_add_arg(cmd[0], "-D");
				shcmd_add_arg(cmd[0], checkdigestname);
			}
			if (create_time_is_set) {
				strob_sprintf(tmpx, 0, "--create-time=%lu", create_time);
				shcmd_add_arg(cmd[0], strob_str(tmpx));
			}
		} else {
			shcmd_add_arg(cmd[0], "-p");
			shcmd_add_arg(cmd[0], "--drop-privilege");
		}
		if (pkgfilename && strcmp(pkgfilename, "-") == 0) {
			// do nothing, read from stdin
			;
		} else if (pkgfilename) {
			shcmd_set_srcfile(cmd[0], DEVNULL);
			shcmd_add_arg(cmd[0], pkgfilename);
		}
		shcmd_set_dstfd(cmd[0], lfd[1]); /* stdout */
	
		shcmd_apply_redirection(cmd[0]);
		swgp_close_all_fd(3);
		shcmd_unix_exec(cmd[0]);
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"%s not run.  errno=%d %s\n", cmd[0]->argv_[0],
				errno, strerror(errno));
		_exit(2); 
	}
	close(lfd[1]);
	
	pid[1] = swfork((sigset_t*)(NULL));
	if (pid[1] < 0) exit(1);
	if (pid[1] == 0) {
		/* child */

		//
		// return into the main() routine as a child.
		//
		return lfd[0];
	} else {
		;
		/* parent */
	}
	close_passfd();
	close(lfd[0]);

	if (swlib_wait_on_all_pids(pid, 2, status, 0 /*WNOHANG*/, verboseG - 2) < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"error waiting on childs in do_unrpm.\n");
		exit(11);
	}
	
	shcmd_close(cmd[0]);

	/*
	 * This is the exit of the main() parent
	 */

	exit(WEXITSTATUS(status[1]) || WEXITSTATUS(status[0]));
}

static
char *
get_tag(swDefinition * md)
{
	char * tag;
	tag = md->find("tag");
	if (tag == NULL) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"fatal error: missing tag attribute\n");
		exit(1);
	}
	return tag;
}

static
char *
get_control_directory(swDefinition * md)
{
	char * tag;
	char * object_keyword;

	if (md->find("control_directory"))
		return md->find("control_directory");

	tag = md->find("tag");
	if (tag == NULL) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"fatal error: missing tag and control_directory attribute\n");
		exit(1);
	}

	object_keyword =  md->get_keyword();
	if (
		::strcmp(SW_A_product, object_keyword) == 0 ||
		::strcmp(SW_A_fileset, object_keyword) == 0 ||
		::strcmp(SW_A_distribution, object_keyword) == 0 ||
		0
	)  {
		md->add("control_directory", tag);
	}
	return tag;
}

static
void 
init_serial_package(swExCat * swexdist)
{
	char * open_path = "";

	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"init_serial_package() BEGIN\n");
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"running task: performInitializationPass1\n");
	//
	// Misc.  
	// FIXME this should use the taskDispatcher() framework.
	//
	swexdist->performInitializationPass1();
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	
	//
	// Init the pfiles and dfiles objects.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"running task: init1E\n");
	swexdist->taskDispatcher(swExStruct::init1E);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	//
	// Fill in the leading paths at each level in the package.
	// FIXME this should use the taskDispatcher() framework.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"running task: setLeadingPaths\n");
	swexdist->setLeadingPackagePath(open_path);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	//
	// Fix up the "path" and "source" attributes as req'd
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"running task: initPass2: ...\n");
	swexdist->performInfoPass2();
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	
	//
	// Add the control_directory keyword to filesets and products.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: addControlDirectoryE\n");
	swexdist->taskDispatcher(swExStruct::addControlDirectoryE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	
	//
	// Add a control_file definition for the INFO file in each 
	// INFO file.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: addInfoDefinitionE\n");
	swexdist->taskDispatcher(swExStruct::addInfoDefinitionE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	//
	// Update the file stats for all the files in the INFO files.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: addFileStatsE\n");
	swexdist->taskDispatcher(swExStruct::addFileStatsE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		
	//
	// create the control script package files.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: createControlScriptFilesE\n");
	swexdist->taskDispatcher(swExStruct::createControlScriptFilesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		
	//
	// Register the attribute files with the INFO files.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: setupAttributeFilesE\n");
	swexdist->taskDispatcher(swExStruct::setupAttributeFilesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		
	//
	// Set the size attribute in the INFO file.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: setInfoSizesE\n");
	swexdist->taskDispatcher(swExStruct::setInfoSizesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		
	//
	// Set the fileset size.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: generateFileSetSizeE ...\n");
	swexdist->taskDispatcher(swExStruct::generateFileSetSizeE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"task: generateFileSetSizeE ... Done.\n");
		
	swexdist->taskDispatcher(swExStruct::setAllfilesetsE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"running task: initPass2: Finished\n");
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
		"init_serial_package() END\n");
}

static
int
i_add_sig_precursors(swExCat * swexdist, swExCat * swexdistribution, 
				char * attname, int filesize) 
{
	char tmpname[64];
	char * buffer;
	int bsize = filesize + 20;
	swDefinition * swdef;

	SWP_E_DEBUG("");
	buffer = (char*)malloc((size_t)bsize);
	swdef = swexdistribution->getReferer();

	SWP_E_DEBUG("");
	memset(buffer, '\n', bsize);
	buffer[filesize - 1] = '\0';  // because swdef_write_value adds a \n
	memmove(buffer+2, buffer, strlen(buffer)+1);
	SWP_E_DEBUG("");

	*buffer = '<';
	*(buffer + 1) = ':';
	swdef->add(attname, buffer);

	//
	// The "<:" string indicates a special hack to the
	// setupAttributeFilesE method.
	//

	SWP_E_DEBUG("");
	swexdist->taskDispatcher(swExStruct::setupAttributeFilesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	SWP_E_DEBUG("");
	swdef->vremove(attname);
	strncpy(tmpname, "<", 2);
	strncat(tmpname, attname, sizeof(tmpname)-3);
	tmpname[sizeof(tmpname)-1] = '\0';
	swdef->add(attname, tmpname);

	SWP_E_DEBUG("");
	return 0;
}

static
void 
write_serial_package(
		swPSF * psf,
		swExCat * swexdist, 
		int ofd, 
		int opt_catalog_only,  
		int storage_only,
		char * dir_numeric_owner,
		char * catalog_numeric_owner,
		char * numeric_owner,
		int no_front_dir,
		int no_catalog
		)
{
	int nr;
	swexdist->set_ofd(ofd);
	swexdist->getArchiver()->xFormat_reset_bytes_written();

	if (opt_catalog_only == 0 && no_front_dir == 0) {
		//
		// Write the Leading <path>/.
		//
		nr = is_option_true(dir_numeric_owner);
		psf->xFormat_set_numeric_uids(nr);
		nr = is_option_true(numeric_owner);
		if (nr) psf->xFormat_set_numeric_uids(nr);
		SWP_E_DEBUG("");
		swexdist->taskDispatcher(swExStruct::emitLeadingPathE);
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	}
		
	if ((opt_catalog_only == 1 || storage_only == 0) && no_catalog == 0) {
		//
		// Write the Catalog Section.
		//
		swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
				"Writing the catalog.\n");
		nr = is_option_true(catalog_numeric_owner);
		psf->xFormat_set_numeric_uids(nr);
		nr = is_option_true(numeric_owner);
		if (nr) psf->xFormat_set_numeric_uids(nr);
		SWP_E_DEBUG("");
		swexdist->taskDispatcher(swExStruct::emitExportedCatalogE);
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	}

	if (opt_catalog_only == 0) {
		//
		// Write the Storage Section.
		//
		swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
				"Writing the storage files.\n");
		if (
			is_option_true(numeric_owner) ||
			is_option_true(dir_numeric_owner)
		) {
			psf->xFormat_set_numeric_uids(1);
		} else {
			psf->xFormat_set_numeric_uids(0);
		}
		SWP_E_DEBUG("");
		swexdist->taskDispatcher(swExStruct::emitStorageStructureE);
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	}

	//
	// Write the trailer.
	SWP_E_DEBUG("");
	swexdist->getArchiver()->xFormat_write_trailer();
}

static
int 
get_package_digests(
		swExCat * swexdist, 
		int archive_format, 
		char * md5sum_buffer, 
		char * adjunct_md5sum_buffer, 
		char * sha1_buffer, 
		char * sha512_buffer, 
		char * size_buffer, 
		int filesfd, 
		int header_flags,
		int no_front_dir,
		int no_catalog)
{
	int ret = 0;
	char digbuf[512];
	char rbuf2_o[512];
	const int digbuf_offset_md5 = 0;
	const int digbuf_offset_sha1 = 100;
	const int digbuf_offset_size = 200;
	const int digbuf_offset_sha2 = 300;
	const int fbuflen = 512;
	char * fbuf = NULL;
	char * rbuf1;
	char * rbuf2;
	pid_t pid[6];
	int ofd1;
	int status[6];
	int md1[2];
	int md2[2];
	int archive[2];
	int md5_1[2];
	int md5_2[2];
	int files[2];
	int ret1;
	int ret2;
	int ret3;
	int errno1;
	int errno2;
	int errno3;
	int remains1;
	int remains2;

	memset(digbuf, '\0', sizeof(digbuf));

	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
			"Generating archive digests ....\n");
	//
	// Generate the whole archive.
	//
	// If files_fd != 0 then generate the file list.
	//

	files[0] = -1;
	files[1] = -1;

	if (filesfd > 0) {
		if (pipe(files)) FATAL();
		fbuf = (char*)malloc(fbuflen);
		if (!fbuf) return -100;
	} else {
		fbuf = (char*)NULL;
	}

	if (pipe(archive)) FATAL();
	pid[0] = swfork((sigset_t*)(NULL));
	if (pid[0] < 0) return -101;
	if (pid[0] == 0) {
		close_passfd();
		close(archive[0]);
		if (files[0] >= 0) close(files[0]);

		if (files[1] >= 0) {
			swexdist->set_preview_level(TARU_PV_1);
			swexdist->set_preview_fd(files[1]);
		} else {
			swexdist->set_preview_level(TARU_PV_0);
			swexdist->set_preview_fd(STDERR_FILENO);
		}
		ofd1 =  uxfio_opendup(archive[1], UXFIO_BUFTYPE_NOBUF);
		uxfio_fcntl(ofd1, 
			UXFIO_F_SET_OUTPUT_BLOCK_SIZE, 512 /* SWPACKAGE_PIPE_BUF */);
		::write_serial_package(g_psf,
				swexdist, 
				ofd1, 
				0, /*opt_catalog_only*/  
				0, /*opt_storage_only*/
				g_wopt_dir_numeric_owner,
				g_wopt_catalog_numeric_owner,
				g_wopt_numeric_owner, no_front_dir,
				0 /*no_catalog*/);
		if (files[1] >= 0) close(files[1]);
		uxfio_close(ofd1);
		_exit(0);
	}
	close(archive[1]);
	if (files[1] >= 0) close(files[1]);

	//
	// Split the archive into 2 streams to be digested.
	//
	if (pipe(md1)) FATAL();
	if (pipe(md2)) FATAL();
	pid[1] = swfork((sigset_t*)(NULL));
	if (pid[1] < 0) return -202;
	if (pid[1] == 0) {
		XFORMAT * xformat;
		int flags;
		close_passfd();
		xformat = xformat_open(
				STDIN_FILENO, 
				STDOUT_FILENO, 
				archive_format);
		flags = UINFILE_DETECT_FORCEUXFIOFD | 
				UINFILE_UXFIO_BUFTYPE_MEM |
				UINFILE_DETECT_NATIVE;
		::xformat_set_tarheader_flags(xformat, header_flags);
		close(md1[0]);
		close(md2[0]);
		if (files[0] >= 0) close(files[0]);

		if (xformat_open_archive_by_fd(xformat, 
						archive[0], flags, 0)) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"internal error: error opening archive in get_package_digests.\n");
			_exit(4);
		}

		::taruib_write_storage_stream(
					xformat,
					md1[1],
					1 /* 2nd version */, 
					md2[1], 
					0 /* verbose */, 
					0);

		close(md1[1]);
		close(md2[1]);
		close(archive[0]);
		_exit(0);
	}

	close(archive[0]);
	close(md1[1]);
	close(md2[1]);

	//
	// Digest the two streams in parallel.
	//

	if (pipe(md5_1)) FATAL();
	if (pipe(md5_2)) FATAL();

	pid[2] = swfork((sigset_t*)(NULL));
	if (pid[2] < 0) return -203;
	if (pid[2] == 0) {
		int ret;
		char * buf;
		close_passfd();
		buf = (char *)malloc(512);
		if (!buf) _exit(3);
		close(md2[0]);
		close(md5_2[0]);
		close(md5_2[1]);
		close(md5_1[0]);
		if (files[0] >= 0) close(files[0]);
		memset(buf, '\0', 512);
		if (sha1_buffer || sha512_buffer) {
			/* FIXME */
			char * v_sha1;
			char * v_sha2;
			if (sha1_buffer) {
				SWP_E_DEBUG("");
				v_sha1 = buf + digbuf_offset_sha1;
			} else {
				SWP_E_DEBUG("sha2 null");
				v_sha1 = NULL;
			}
			if (sha512_buffer) {
				SWP_E_DEBUG("");
				v_sha2 = buf + digbuf_offset_sha2;
			} else {
				SWP_E_DEBUG("sha512 null");
				v_sha2 = NULL;
			}
			ret = swlib_digests(md1[0], buf, v_sha1, buf + digbuf_offset_size, v_sha2);
			if (ret > 0) ret = 0;
			if (ret < 0) ret = 1;
		} else {
			/* this is the old routine from when sha1 was not supported */
			ret = swlib_md5(md1[0], (unsigned char *)buf, 1);
			if (ret > 0) ret = 0;
			if (ret < 0) ret = 1;
		}
		if (write(md5_1[1], buf, 512) != 512) {
			SWBIS_ERROR_IMPL();
			_exit(2);
		}
		close(md5_1[1]);
		close(md1[0]);
		_exit(ret);
	}
	
	pid[3] = swfork((sigset_t*)(NULL));
	if (pid[3] < 0) return -211;
	if (pid[3] == 0) {
		int ret;
		unsigned char * buf;
		buf = (unsigned char *)malloc(512);
		close_passfd();
		if (!buf) _exit(3);
		close(md1[0]);
		close(md5_1[0]);
		close(md5_1[1]);
		close(md5_2[0]);
		if (files[0] >= 0) close(files[0]);
		memset(buf, '\0', 512);
		ret = swlib_md5(md2[0], 
			(unsigned char *)buf, 
			1 /*int do_ascii*/);
		if (write(md5_2[1], buf, 512) != 512) {
			SWBIS_ERROR_IMPL();
			_exit(2);
		}
		close(md5_2[1]);
		close(md2[0]);
		_exit(ret);
	}

	close(md5_1[1]);
	close(md5_2[1]);

	if (fcntl(md5_1[0], F_SETFL, O_NONBLOCK) < 0) return -2;
	if (fcntl(md5_2[0], F_SETFL, O_NONBLOCK) < 0) return -3;
	if (filesfd >= 0) {
		if (fcntl(files[0], F_SETFL, O_NONBLOCK) < 0) return -4;
	}

	//
	// Read 512 bytes from each pipe which contain the ascii digests.
	// md5sum_buffer:      (same as: arf2arf -S <package.tar.gz | md5sum )
	// adjunct_md5sum_buffer: (same as: arf2arf -s<package.tar.gz| md5sum )
	//
	errno1 = EIO;
	errno2 = EIO;
	errno3 = EIO;
	if (filesfd < 0) errno3 = EEXIST;

	rbuf1 = digbuf;
	rbuf2 = adjunct_md5sum_buffer;
	if (rbuf2 == NULL) rbuf2 = rbuf2_o;
	remains1 = 512;
	remains2 = 512;

	do {
                /* usleep(100000); */ // This is a busy wait loop, so sleep 1/10 second.
				// This usleep is required on Cygwin/Windows to
				// improve performance.

		/* buffer 3 */
		if (filesfd > 0) {
			if (errno3 != EEXIST) {
				ret3 = read(files[0], fbuf, 512);
				if (ret3 < 0) {
					errno3 = errno;
					if (errno3 != EAGAIN) break;
				} else if (ret3 == 0) {
					errno3 = EEXIST; 	
				} else {
					if (uxfio_write( filesfd, fbuf, 
							ret3) != ret3) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
						"error writing files.\n");
						break;
					}
				}
			}
		}
		
		/* buffer 1 */
		if (errno1 != EEXIST) {
			//
			// This the md5, sha1, and the size
			//
			ret1 = read(md5_1[0], rbuf1, remains1);
			if (ret1 < 0) {
				errno1 = errno;
				if (errno1 != EAGAIN) break;
			}
			if (ret1 > 0 && ret1 != remains1) {
				rbuf1+=ret1;
				remains1-=ret1;
			}
			if (ret1 == 0) {
				if (md5sum_buffer) {
					strncpy(md5sum_buffer, digbuf + digbuf_offset_md5, 40);
					md5sum_buffer[32] = '\0'; /* Make sure its terminated */
				}
				if (sha1_buffer) {
					strncpy(sha1_buffer, digbuf + digbuf_offset_sha1, 42);
					sha1_buffer[40] = '\0'; /* Make sure its terminated */
				}
				
				if (sha512_buffer) {
					strncpy(sha512_buffer, digbuf + digbuf_offset_sha2, 130);
					sha512_buffer[128] = '\0'; /* Make sure its terminated */
				}

				if (size_buffer) {
					strncpy(size_buffer, digbuf + digbuf_offset_size, 32);
					size_buffer[31] = '\0'; /* Make sure its terminated */
				}
				errno1 = EEXIST; 	
			}
		}

		/* buffer 2 */
		if (errno2 != EEXIST) {
			ret2 = read(md5_2[0], rbuf2, remains2);
			if (ret2 < 0) {
				errno2 = errno;
				if (errno2 != EAGAIN) break;
			}
			if (ret2 > 0 && ret2 != remains2) {
				rbuf2+=ret2;
				remains2-=ret2;
			}
			if (ret2 == 0) {
				rbuf2[32] = '\0'; /* Make sure its terminated */
				errno2 = EEXIST; 	
			}
		}
	} while (errno1 != EEXIST || errno2 != EEXIST || errno3 != EEXIST);
	
	if (errno1 != EEXIST || errno2 != EEXIST) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"get_package_digest: piping error.\n");
		ret = -3;
	}

	if (filesfd >= 0) close(files[0]);	
	close(md5_1[0]);
	close(md5_2[0]);
	close(md1[0]);
	close(md2[0]);

	//
	// Now wait on all the pids.
	//
	
	if (swlib_wait_on_all_pids(pid, 4, status, 
				0 /*WNOHANG*/, verboseG - 2) <0) {
		swexdist->setErrorCode(18003, NULL);
	}
	
	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
			"Generating archive digests: Done\n");
	if (fbuf) free(fbuf);
	return ret;
}

static int
make_pgp_env(char * env[], int len) {
	int i;
	char * valu;
	STROB * tmp = strob_open(2);

	for (i=0; i<len; i++) {
		env[i] = (char*)NULL;
	}

	env[0] = strdup("PGPPASSFD=3");
	valu = getenv("HOME");
	if (valu) {
		strob_sprintf(tmp, 0, "HOME=%s", valu);
		env[1] = strdup(strob_str(tmp));
	}
	
	valu = getenv("MYNAME");
	if (valu) {
		strob_sprintf(tmp, 0, "MYNAME=%s", valu);
		env[2] = strdup(strob_str(tmp));
	}
	
	strob_close(tmp);
	return 0;
}

static
int
get_does_use_gpg_agent(SHCMD * cmd)
{
	char ** args;
	char * arg;

	args = shcmd_get_argvector(cmd);
	arg = *args;
	while (arg) {
		if (strstr(arg, "--use-agent")) return 1;
		arg = *(++args);
	}
	return 0;
}

static SHCMD * 
get_package_signature_command(
		char * signer, 
		char * gpg_name, 
		char * gpg_path,
		char * passphrase_fd
) {
	static char * env[10];
	SHCMD * sigcmd;
	char * envpath;
	char * signerpath;

	sigcmd = shcmd_open();
	env[0] = (char *)NULL;
	envpath = getenv("PATH");	
	if (
		strcasecmp(signer, "GPG") == 0 ||
		strcasecmp(signer, "GPG2") == 0
	) {
		if (strcasecmp(signer, "GPG2") == 0) {
			signerpath = shcmd_find_in_path(envpath, SWGPG_GPG2_BIN);
		} else {
			signerpath = shcmd_find_in_path(envpath, SWGPG_GPG_BIN);
		}
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		if (gpg_path && strlen(gpg_path)) {
			shcmd_add_arg(sigcmd, "--homedir");
			shcmd_add_arg(sigcmd, gpg_path);
		}
		shcmd_add_arg(sigcmd, "--no-tty");
		shcmd_add_arg(sigcmd, "--no-secmem-warning");
		shcmd_add_arg(sigcmd, "--armor");
		if (
			passphrase_fd != NULL &&
			strcmp(passphrase_fd, SWP_PASS_AGENT) == 0
		) {
			shcmd_add_arg(sigcmd, "--use-agent");
		} else {	
			shcmd_add_arg(sigcmd, "--passphrase-fd");
			shcmd_add_arg(sigcmd, "3");
		}
		shcmd_add_arg(sigcmd, "-sb");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else if (strcasecmp(signer, "PGP2.6") == 0) {
		make_pgp_env(env, sizeof(env)/sizeof(char*));
		shcmd_set_envp(sigcmd, env);
		signerpath = shcmd_find_in_path(envpath, SWGPG_PGP26_BIN);
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		shcmd_add_arg(sigcmd, "+armor=on");
		shcmd_add_arg(sigcmd, "-sb");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else if (strcasecmp(signer, "PGP5") == 0) {
		make_pgp_env(env, sizeof(env)/sizeof(char*));
		shcmd_set_envp(sigcmd, env);
		signerpath = shcmd_find_in_path(envpath, SWGPG_PGP5_BIN);
		if (signerpath == NULL) return NULL;
		shcmd_add_arg(sigcmd, signerpath);
		if (gpg_name && strlen(gpg_name)) {
			shcmd_add_arg(sigcmd, "-u");
			shcmd_add_arg(sigcmd, gpg_name);
		}
		shcmd_add_arg(sigcmd, "-ab");
		shcmd_add_arg(sigcmd, "-o");
		shcmd_add_arg(sigcmd, "-");
	} else {
		shcmd_close(sigcmd);
		sigcmd = NULL;
	}
	return sigcmd;
}

static
char *
get_package_signature(
		swExCat * swexdist, 
		SHCMD * sigcmd, 
		char * gpg_name, 
		char * gpg_path, 
		int * statusp, 
		int archive_format, 
		int catalog_tarheader_flags, 
		char * wopt_passphrase_fd, 
		char * passfile,
		int no_front_dir,
		int no_catalog,
		int signedbytes_fd)
{
	SHCMD *cmd[2];	
	char * sig;
	int does_use_agent;
	int ret;
	int atoiret;
	int opt_passphrase_fd;
	pid_t pid[6];
	int status[6];
	int passphrase[2];
	int input[2];
	int output[2];
	int filter[2];
	int sigfd;
	char * fdmem;
	char nullbyte[2];

	*statusp = 255;
	cmd[1] = (SHCMD*)(NULL);
	nullbyte[0] = '\0';

	if (is_option_true(g_wopt_dummy_sign)) {
		sig = (char*)malloc(ARMORED_SIGLEN);
		memset(sig, '\0', ARMORED_SIGLEN);
		strcat(sig, 
			"-----BEGIN DUMMY SIGNATURE-----\n"
			"Version: swpackage (swbis) " SWPACKAGE_VERSION "\n"
			"dummy signature made using the --dummy-sign option of\n"
			"the swpackage(8) utility.  Not intended for verification.\n"
			"-----END DUMMY SIGNATURE-----\n");
		return sig;
	}

	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Generating package signature ....\n");

	if (sigcmd == NULL) {
		return (char*)NULL;
	} else {
		cmd[0] = sigcmd;
	}

	if (pipe(input)) FATAL();
	if (pipe(output)) FATAL();
	if (pipe(passphrase)) FATAL();
	pipe(filter);

	does_use_agent = get_does_use_gpg_agent(cmd[0]);

	// shcmd_cmdvec_debug_show_to_file(cmd, stderr); 
	//
	// Exec Signer.
	//
	pid[0] = swfork((sigset_t*)(NULL));
	if (pid[0] < 0) FATAL();
	if (pid[0] == 0) 
	{
		close_passfd();
		close(filter[1]);
		close(output[0]);
		shcmd_set_srcfd(cmd[0], filter[0]);
		shcmd_set_dstfd(cmd[0], output[1]);
		shcmd_set_errfile(cmd[0], DEVNULL);
		shcmd_apply_redirection(cmd[0]);
		if (does_use_agent == 0)
			dup2(passphrase[0], 3);
		close(passphrase[1]);
		close(passphrase[0]);
		if (does_use_agent == 0)
			swgp_close_all_fd(4);
		else
			swgp_close_all_fd(3);
		shcmd_unix_exec(cmd[0]);
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s not run.\n", cmd[0]->argv_[0]);
		_exit(2); 
	}
	close(output[1]);
	close(filter[0]);
	close(passphrase[0]);

	//
	// Ask Passphrase
	//
	pid[1] = swfork((sigset_t*)(NULL));
	if (pid[1] < 0) return (char*)NULL;
	if (pid[1] == 0) {
		close(input[1]);
		close(output[0]);
		if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, "-1") == 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_ENV) != 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_AGENT) != 0 &&
			passfile == (char*)NULL
		) {
			//
			// Not using the GPG agent or file desctiptor
			// of swspackage, therefore, use the getpass
			// routine which gets the passphrase from tty
			//
			char pbuf[PASSPHRASE_LENGTH];
			char * pass;
			pass = fm_getpassphrase(
				"Enter Password: ", pbuf, sizeof(pbuf));
			if (pass) {
				pbuf[sizeof(pbuf) - 1] = '\0';
				write(passphrase[1], pass, strlen(pass));
			} else {
				write(passphrase[1], "\n\n", 2);
			}
			memset(pbuf, '\x00', sizeof(pbuf));
			memset(pbuf, '\xff', sizeof(pbuf));
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"swpackage: close: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_AGENT) != 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_ENV) != 0
		) {
			//
			// Here, passfile may have been given or
			// wopt_passphrase_fd was given.
			//
			int did_open = 0;
			int buf[512];
			if (passfile && 
				strcmp(passfile, "-")) {
				opt_passphrase_fd = open(passfile, 
							O_RDONLY, 0);
				if (opt_passphrase_fd < 0) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"passphrase file not found\n");
				}
				did_open = 1;
			} else if (passfile && 
				strcmp(passfile, "-") == 0) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"warning: unsafe use of stdin\n"
					"warning: use --passphrase-fd=0 instead\n");
				opt_passphrase_fd = STDIN_FILENO;
			} else {
				opt_passphrase_fd = swlib_atoi(wopt_passphrase_fd, &atoiret);
				if (atoiret) return (char*)(NULL);
			}
			if (opt_passphrase_fd >= 0) {
				SWP_E_DEBUG2("reading opt_passphrase_fd=%d", opt_passphrase_fd);
				ret = read(opt_passphrase_fd, (void*)buf, sizeof(buf) - 1);
				SWP_E_DEBUG2("read of passphrase ret=%d", ret);
				if (ret < 0 || ret >= 511) { 
					memset(buf, '\0', sizeof(buf));
					memset(buf, '\xff', sizeof(buf));
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"read (loc=p): %s\n", strerror(errno));
					close(opt_passphrase_fd);
					close(passphrase[1]);
					_exit(1);
				}
				if (did_open) close(opt_passphrase_fd);
				buf[ret] = '\0';  // not needed.
				write(passphrase[1], buf, ret);
				memset(buf, '\0', sizeof(buf));
				memset(buf, '\xff', sizeof(buf));
			}
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 1 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_AGENT) == 0
		) {
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else if (
			does_use_agent == 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_ENV) == 0 &&
			g_passphrase
		) {
			write(passphrase[1], g_passphrase, strlen(g_passphrase));
			write(passphrase[1], "\n", 1);
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		} else {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"internal error get_package_signature\n");
			if (close(passphrase[1]) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"close error: %s\n", strerror(errno));
			}
		}
		_exit(0);
	}
	// 
	// Close the passphrase in the parent.
	//
	close_passfd();
	close(passphrase[1]);
					// filter[1] -->
	//
	// Decode signing stream.
	//
	pid[2] = swfork((sigset_t*)(NULL));
	if (pid[2] < 0) return (char *)NULL;
	if (pid[2] == 0) {
		int flags = 0;
		XFORMAT * xformat;
		close_passfd();

		if (signedbytes_fd < 0) {
			xformat = xformat_open(
					STDIN_FILENO, 
					STDOUT_FILENO, 
					archive_format);
			flags = UINFILE_DETECT_FORCEUXFIOFD |
				UINFILE_DETECT_NATIVE |
				UINFILE_UXFIO_BUFTYPE_MEM;
			xformat_set_tarheader_flags(xformat, catalog_tarheader_flags);	
			close(input[1]);
		
			if (xformat_open_archive_by_fd(xformat, 
						input[0], 
						flags, 0)) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"internal error opening archive in get_package_digests.\n");
				_exit(4);
			}
	
			::taruib_write_catalog_stream(
						xformat,
						filter[1],
						1, /*version*/
						0 /*verbose*/);
		} else {
			close(input[1]);
			/* ''dd' the stream un-filtered */
			if (swlib_pipe_pump(filter[1], input[0]) < 0) {
				close(input[0]);
				close(filter[1]);
				_exit(1);
			}
		}	
		close(input[0]);
		close(filter[1]);
		_exit(0);
	}
	close(input[0]);
	close(filter[1]);
					// <<-- input[0]
					// input[1] -->
	//
	// Write whole package.
	//
	pid[3] = swfork((sigset_t*)(NULL));
	if (pid[3] < 0) return (char *)NULL;
	if (pid[3] == 0) {
		int ofd1;
		close_passfd();
		close(input[0]);
		close(output[0]);
		if (swexdist) swexdist->set_preview_fd(STDERR_FILENO);

		ofd1 =  uxfio_opendup(input[1], UXFIO_BUFTYPE_NOBUF);
		uxfio_fcntl(ofd1, UXFIO_F_SET_OUTPUT_BLOCK_SIZE, 
				SWPACKAGE_PIPE_BUF);
		if (signedbytes_fd < 0) {
			swlib_set_verbose_level(1);
			if (swexdist) swexdist->set_preview_level(TARU_PV_0);
			::write_serial_package(g_psf, swexdist, 
				ofd1, 
				0, /*opt_catalog_only*/  
				0, /*opt_storage_only*/
				g_wopt_dir_numeric_owner,
				g_wopt_catalog_numeric_owner,
				g_wopt_numeric_owner, no_front_dir,
				0 /* no_catalog */
				);
		} else {
			if (swlib_pipe_pump(ofd1, signedbytes_fd) < 0) {
				uxfio_close(ofd1);
				_exit(1);
			}
		}
		uxfio_close(ofd1);
		_exit(0);
	}
	close(input[1]);

	sigfd = swlib_open_memfd();
	
	ret = swlib_pump_amount(sigfd, output[0], 1024);
	close(output[0]);
	if (ret < 0 || ret > 1000) return (char*)NULL;
	uxfio_write(sigfd, (void*)nullbyte, 1);	

	if (swlib_wait_on_all_pids(pid, 4, status, 
				0 /*WNOHANG*/, verboseG - 2) < 0) {
		if (swexdist) swexdist->setErrorCode(18004, NULL);
		swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
			"internal error: code: 18004\n");
	}

	if (pid[0] < 0) {
		ret =  WEXITSTATUS(status[0]);
	} else {
		ret = 100;
	}
	*statusp = ret;
	if (ret) return (char*)NULL;

	uxfio_get_dynamic_buffer(sigfd, &fdmem, (int*)NULL, (int*)NULL);
	sig = strdup(fdmem);
	uxfio_close(sigfd);	
	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Generating package signature .... Done.\n");
	return sig;
}

static int
delete_precursor(
	swExCat * swexdist, 
	swPtrList<swPackageFile> * archiveMemberList, 
	char * filename)
{
	int index = 0;
	swPackageFile * swfile;

	SWLIB_ASSERT(archiveMemberList != NULL);
	swfile = archiveMemberList->get_pointer_from_index(index);
	while(swfile) {
		if (::strcmp(filename, 
				swfile->swfile_get_filename()) == 0) {
			//
			// delete from list.
			//
			archiveMemberList->list_del(index);
			return 0;
		}
		swfile = archiveMemberList->get_pointer_from_index(++index);
	}
	// swexdist->setErrorCode(18002, NULL);
	return 1;
}

static
int
delete_files_precursor(swExCat * swexdist, swExCat * swexdistribution) 
{
	int index = 0;
	swPackageFile * swfile;
	swPtrList<swPackageFile> * archiveMemberList;
	
	archiveMemberList = swexdistribution->getAttributeFileList();
	SWLIB_ASSERT(archiveMemberList != NULL);

	swfile = archiveMemberList->get_pointer_from_index(index);
	while(swfile) {
		if ( ::strcmp("files", 
				swfile->swfile_get_filename()) == 0) {
			//
			// delete from list.
			//
			archiveMemberList->list_del(index);
			return 0;
		}
		swfile = archiveMemberList->get_pointer_from_index(++index);
	}
	swexdist->setErrorCode(18008, NULL);
	return 1;
}

static
int
add_files_precursor(swExCat * swexdist, swExCat * swexdistribution) 
{
	swPtrList<swPackageFile> * archiveMemberList;
	swAttributeFile * swattfile;

	archiveMemberList = swexdistribution->getAttributeFileList();
	SWLIB_ASSERT(archiveMemberList != NULL);

	swattfile = new swAttributeFile("files");
	archiveMemberList->list_add(swattfile);
	return 0;
}

static
int
add_archive_digests_precursor(swExCat * swexdist, 
			swExCat * swexdistribution, int opt_no_sha1,
			int opt_size,
			int opt_archive_digests2,
			int opt_archive_digests) 
{
	swPtrList<swPackageFile> * archiveMemberList;
	swAttributeFile * swattfile;

	archiveMemberList = swexdistribution->getAttributeFileList();
	SWLIB_ASSERT(archiveMemberList != NULL);

	if (opt_archive_digests) {
		swattfile = new swAttributeFile(SW_A_md5sum);
		archiveMemberList->list_add(swattfile);
	}

	if (opt_no_sha1 == 0 && opt_archive_digests) {
		swattfile = new swAttributeFile(SW_A_sha1sum);
		archiveMemberList->list_add(swattfile);
	}

	if (opt_archive_digests2) {
		swattfile = new swAttributeFile(SW_A_sha512sum);
		archiveMemberList->list_add(swattfile);
	}

	if (opt_archive_digests) {
		swattfile = new swAttributeFile(SW_A_adjunct_md5sum);
		archiveMemberList->list_add(swattfile);
	}
	
	if (opt_size) {
		swattfile = new swAttributeFile(SW_A_size);
		archiveMemberList->list_add(swattfile);
	}

	return 0;
}

static
int
delete_signature_header_list_precursor(swExCat * swexdist, 
				swExCat * swexdistribution) 
{
	int ret;
	swPtrList<swPackageFile> * archiveMemberList;
	archiveMemberList = swexdistribution->getAttributeFileList();
	ret = delete_precursor(swexdist, 
			archiveMemberList, 
			"sig_header");
	return ret;
}

static
int
delete_signature_list_precursor(swExCat * swexdist, 
				swExCat * swexdistribution) 
{
	int ret;
	swPtrList<swPackageFile> * archiveMemberList;
	archiveMemberList = swexdistribution->getAttributeFileList();
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_signature);
	return ret;
}

static
int
add_signature_list_precursor(swExCat * swexdist, 
				swExCat * swexdistribution) 
{
	swPtrList<swPackageFile> * archiveMemberList;
	swAttributeFile * swattfile;

	archiveMemberList = swexdistribution->getAttributeFileList();
	SWLIB_ASSERT(archiveMemberList != NULL);

	swattfile = new swAttributeFile(SW_A_signature);
	archiveMemberList->list_add(swattfile);
	return 0;
}

static
int
add_signature_header_list_precursor(swExCat * swexdist, 
				swExCat * swexdistribution) 
{
	swPtrList<swPackageFile> * archiveMemberList;
	swAttributeFile * swattfile;

	archiveMemberList = swexdistribution->getAttributeFileList();
	SWLIB_ASSERT(archiveMemberList != NULL);

	swattfile = new swAttributeFile("sig_header");
	archiveMemberList->list_add(swattfile);
	return 0;
}

static
void
delete_archive_digests_precursor(swExCat * swexdist, 
				swExCat * swexdistribution) 
{
	int ret;
	swPtrList<swPackageFile> * archiveMemberList;
	archiveMemberList = swexdistribution->getAttributeFileList();
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_md5sum);
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_sha1sum);
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_sha512sum);
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_adjunct_md5sum);
	ret = delete_precursor(swexdist, archiveMemberList, SW_A_size);
	return;
}

static
int
add_files(swExCat * swexdist, swExCat * swexdistribution, int filesfd) 
{
	char * filesbuf;
	char nc;
	swDefinition * swdef;
	
	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Adding files, the filelist control file ....\n");
	swdef = swexdistribution->getReferer();

	//
	// Null terminate the file list.
	//
	nc = '\0';
	if (uxfio_write(filesfd, (void*)(&nc), 1) != 1) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"uxfio_write error in main::add_files. \n");
		return -1;
	}

	if (uxfio_lseek(filesfd, 0, SEEK_SET) != 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"uxfio_lseek error in main::add_files. \n");
		return -1;
	}

	uxfio_get_dynamic_buffer(filesfd, &filesbuf, (int*)NULL, (int*)NULL);
	
	swdef->add("files", "<:");  // Zero length data file.
	
	//
	// Now reprocess the attribute files to instantiate the zero 
	// length file.
	//
	swexdist->taskDispatcher(swExStruct::setupAttributeFilesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	//
	// Now setup and add the real file list.
	//
	swexdist->setFilesFd(filesfd);
	swexdist->taskDispatcher(swExStruct::tuneFilesFileE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	swdef->vremove("files");
	swdef->add("files", "<files");
	uxfio_close(filesfd);
	swlib_doif_writef(verboseG, SWPACKAGE_VERBOSE_V1, NULL, get_stderr_fd(),
		"Adding files, the filelist control file .... Done.\n");
	return 0;
}

static
int
add_signature_header_precursor(swExCat * swexdist, 
			swExCat * swexdistribution) 
{
	SWP_E_DEBUG("");
	i_add_sig_precursors(swexdist, 
				swexdistribution, 
				"sig_header", 512 /* tar hdr size */); 
	SWP_E_DEBUG("");
	return 0;
}

static
int
add_signature_precursor(swExCat * swexdist, swExCat * swexdistribution) 
{
	i_add_sig_precursors(swexdist, 
				swexdistribution, 
				SW_A_signature, ARMORED_SIGLEN); 
	return 0;
}

static
int add_signature(swExCat * swexdist, char * sig)
{
	if (strlen(sig) > ARMORED_SIGLEN) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"the signature is too long, this can be fixed, but it requires a source code change.\n");
		return -1;
	}

	//
	// Write now there are already 1024 bytes 
	// in the attribute file.  Just overwrite 
	// the file with (char*)sig.
	//

	//
	// Communicate to the swexport objects that we have a sigfile.
	//
	swexdist->setSigFileBuffer(sig);

	//
	// The tuneSignatureFileE task will see
	// the sigfile and install it.
	//
	swexdist->taskDispatcher(swExStruct::tuneSignatureFileE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	return 0;
}

static
int generate_security_files(
		swExCat * swexdist,
		swExCat * swexdistribution, 
		int archive_format, 
		char * md5sum_buffer, 
		char * adjunct_md5sum_buffer, 
		char * sha1_buffer, 
		char * size_buffer, 
		char * sha512_buffer, 
		int do_files,
		int header_flags,
		int no_front_dir,
		int no_catalog)
{
	int ret;
	swDefinition * swdef;
	int filesfd = -1;
	
	swdef = swexdistribution->getReferer();

	if (do_files > 0) {
		filesfd = do_files;
	}

	//
	// Generate the md5sums
	//
	ret = ::get_package_digests(swexdist, 
				(int)archive_format, 
				md5sum_buffer, 
				adjunct_md5sum_buffer,
				sha1_buffer,
				sha512_buffer,
				size_buffer,
				filesfd,
				header_flags,
				no_front_dir, no_catalog);

	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		
	return ret;
}

static
int add_archive_digests(
		swExCat * swexdist, 
		swExCat * swexdistribution, 
		int archive_format, 
		char * md5sum_buffer, 
		char * adjunct_md5sum_buffer, 
		char * sha1_buffer,
		char * size_buffer,
		char * sha512_buffer)
{
	int ret = 0;
	swDefinition * swdef;
	
	swdef = swexdistribution->getReferer();
	
	//
	// Prepend a "<:" to the digest string. 
	// This causes special processing to occur.
	//
	
	if (md5sum_buffer) {	
		memmove(md5sum_buffer+2, md5sum_buffer, strlen(md5sum_buffer)+1);
		*md5sum_buffer = '<'; 
		*(md5sum_buffer + 1) = ':';
	}

	if (adjunct_md5sum_buffer) {	
		memmove(adjunct_md5sum_buffer+2, 
			adjunct_md5sum_buffer, 
				strlen(adjunct_md5sum_buffer) + 1);
		*adjunct_md5sum_buffer = '<';
		*(adjunct_md5sum_buffer + 1)= ':';
	}

	if (sha1_buffer) {
		memmove(sha1_buffer+2, sha1_buffer, strlen(sha1_buffer)+1);
		*sha1_buffer = '<';
		*(sha1_buffer + 1)= ':';
	}		
	
	if (sha512_buffer) {
		memmove(sha512_buffer+2, sha512_buffer, strlen(sha512_buffer)+1);
		*sha512_buffer = '<';
		*(sha512_buffer + 1)= ':';
	}		

	if (size_buffer) {
		memmove(size_buffer+2, size_buffer, strlen(size_buffer)+1);
		*size_buffer = '<';
		*(size_buffer + 1)= ':';
	}
	
	//
	// Add INDEX file entries.
	//	
	if (md5sum_buffer)
		swdef->add(SW_A_md5sum, md5sum_buffer);

	if (sha1_buffer)
		swdef->add(SW_A_sha1sum, sha1_buffer);
	
	if (sha512_buffer)
		swdef->add(SW_A_sha512sum, sha512_buffer);

	if (adjunct_md5sum_buffer)
		swdef->add(SW_A_adjunct_md5sum, adjunct_md5sum_buffer);

	if (size_buffer)
		swdef->add(SW_A_size, size_buffer);

	//
	// Now reprocess the attribute files.
	//
	swexdist->taskDispatcher(swExStruct::setupAttributeFilesE);
	swexdist->assertNoErrorCondition(SW_EXIT_ONE);

	//
	// Now add the correct form to the definition.
	//
	if (md5sum_buffer)
		swdef->vremove(SW_A_md5sum);

	if (sha1_buffer)
		swdef->vremove(SW_A_sha1sum);

	if (adjunct_md5sum_buffer)
		swdef->vremove(SW_A_adjunct_md5sum);

	if (size_buffer)
		swdef->vremove(SW_A_size);

	if (md5sum_buffer)
		swdef->add(SW_A_md5sum, "<" SW_A_md5sum);

	if (sha1_buffer)
		swdef->add(SW_A_sha1sum, "<" SW_A_sha1sum);

	if (sha512_buffer)
		swdef->add(SW_A_sha512sum, "<" SW_A_sha512sum);

	if (adjunct_md5sum_buffer)
		swdef->add(SW_A_adjunct_md5sum, "<" SW_A_adjunct_md5sum);

	if (size_buffer)
		swdef->add(SW_A_size, "<" SW_A_size);

	return ret;
}
		
static
void
make_uuid(STROB * uuid, char * wopt_uuid)
{
	int uid = getuid();

	if (wopt_uuid) {
		//
		// Use the uuid given on the command line.
		//
		strob_strcpy(uuid, wopt_uuid);
		return;
	} else {
		int err = 0;
		SHCMD * vec[4];
		vec[0] = shcmd_open();
		vec[1] = (SHCMD*)NULL;
		vec[2] = (SHCMD*)NULL;
		vec[3] = (SHCMD*)NULL;

		if (access("/usr/bin/uuidgen", X_OK) == 0) {
			shcmd_add_arg(vec[0], "/usr/bin/uuidgen");
			if (g_passfd >= 0) {
				//
				// Close the passphrase fd.
				//
				shcmd_add_close_fd(vec[0], g_passfd);
			}
			vec[1] = (SHCMD*)NULL;
		} else {
			vec[1] = shcmd_open();
			shcmd_add_arg(vec[0], "/bin/sh");
			shcmd_add_arg(vec[0], "-c");
			shcmd_add_arg(vec[0], "uname -a;date");
			shcmd_add_arg(vec[1], "cksum");
			shcmd_set_lowest_close_fd(vec[1], 3);
			shcmd_set_lowest_close_fd(vec[0], 3);
			shcmd_set_exec_function(vec[1], "execvp");
			if (g_passfd >= 0) {
				//
				// Close the passphrase fd.
				//
				shcmd_add_close_fd(vec[0], g_passfd);
				shcmd_add_close_fd(vec[1], g_passfd);
			}
			vec[2] = (SHCMD*)NULL;
		}
		if (uid == 0) {
			//
			// Protect the root user.
			//
			shcmd_set_user(vec[0], AHS_USERNAME_NOBODY);
			if (vec[1]) shcmd_set_user(vec[1], AHS_USERNAME_NOBODY);
		}
		if (swlib_shcmd_output_strob(uuid, vec)) {
			//
			// error
			//
			err = 1;
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"error generating uuid\n");
		} else {
			//
			// Put a null at the first white space char
			//
			char * ws = strpbrk(strob_str(uuid), " 	\n\r");
			if (ws) {
				*ws = '\0';
			} else {
				err = 1;
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error generating uuid (loc=1).\n");
			}
		}

		if (err) {
			strob_set_length(uuid, 100);
			snprintf(strob_str(uuid), 99, "%lu", (unsigned long)(time(NULL)));
			strob_str(uuid)[99] = '\0';
		}
		shcmd_close(vec[0]);
		if (vec[1]) shcmd_close(vec[1]);
		return;
	}
}

static void
add_signer_attributes(swDefinition * swdef, STROB * tmp, 
				char * signer_string)
{
	//
	// Add "signer" and "signer_version" attributes
	//
	char n = '\0';
	char * s;
	char * s0;
	char * a;

	strob_strcpy(tmp, signer_string);
	s = strob_str(tmp);
	s0 = s;
	while(*s && !isdigit(*s)) {
		s++;	
	}
	if (*s) n = *s;
	*s = '\0';

	a = s0;
	while (*a) {
		*a = (char)tolower((int)(*a));
		a++;
	}

	swdef->add("signer_pgm", s0);
	if (n) {
		*s = n;	
		swdef->add("signer_pgm_version", s);
	} else {
		swdef->add("signer_pgm_version", "1");
	}
}

static
char * 
find_is_locatable_attribute(swExStruct * pex, char * where)
{
	char * tmp = NULL;
	char * ret = NULL;
	int index = 0;
	swExStruct * ex;

	ex = pex->containedByIndex(index++);
	while(ex) {
		if (strcmp(where, ex->getObjectName()) == 0) {
			tmp = ex->getReferer()->find("is_locatable");
			if (!tmp) return NULL;
			if (tmp && strcasecmp(tmp, CHARFALSE) == 0) return NULL;
			if (tmp && strcasecmp(tmp, "true") == 0) ret = tmp; 
		}
		ex = pex->containedByIndex(index++);
	}
	return ret;
}

static
void
add_control_directory_instance_part_bh(swPtrList<swDefinition> * refererlist)
{
	STROB * tmp;
	swDefinition * swmd;
	swDefinition * swmd2;
	char * control_directory;
	char * instance_id;
	int i;
	int j;
	int fixed_first = 0;

	tmp = strob_open(10);

	i = 0;
	while((swmd = refererlist->get_pointer_from_index(i++))) {
		control_directory = get_control_directory(swmd);
		if (
			1 && (
			::strcmp(SW_A_bundle, swmd->get_keyword()) == 0 ||
			::strcmp(SW_A_subproduct, swmd->get_keyword()) == 0
			)
		) {
			//
			// Set value to a never used value
			// and illegal value so the comparison
			// below never matches.
			//
			control_directory = ".";
		}
		SWLIB_ASSERT(control_directory != NULL);

		j = i;
		while((swmd2 = refererlist->get_pointer_from_index(j++))) {
			SWLIB_ASSERT(get_control_directory(swmd2) != NULL);

			if (
				strcmp(control_directory, get_control_directory(swmd2)) == 0  &&
				strlen(control_directory)
			) {
				//
				// Got a control_directory name collision
				//
				if (fixed_first == 0) {
					//
					// Fix the first definition's control directory which
					// now has to be renamed to include the ".<instance_id>"
					// part.
					//
					fixed_first = 1;
					if (
						::strcmp(SW_A_product, swmd->get_keyword()) == 0 ||
						::strcmp(SW_A_fileset, swmd->get_keyword()) == 0
					) { 
						instance_id = swmd->find("instance_id");
						SWLIB_ASSERT(instance_id != NULL);
						strob_sprintf(tmp, 0, "%s.%s", control_directory, instance_id);
						swmd->vremove("control_directory");
						swmd->add("control_directory", strob_str(tmp));
					}
				}	
				if (
					::strcmp(SW_A_product, swmd2->get_keyword()) == 0 ||
					::strcmp(SW_A_fileset, swmd2->get_keyword()) == 0
				) { 
					instance_id = swmd2->find("instance_id");
					SWLIB_ASSERT(instance_id != NULL);
					strob_sprintf(tmp, 0, "%s.%s", control_directory, instance_id);
					swmd2->vremove("control_directory");
					swmd2->add("control_directory", strob_str(tmp));
				}
			}
		}	
	}
	strob_close(tmp);
	return;
}

static
void
add_instance_id_bh(swPtrList<swDefinition> * refererlist)
{
	STROB * tmp;
	swDefinition * swmd;
	swDefinition * swmd2;
	char * tag;
	int instance_id;
	char * str_instance_id;
	int i;
	int j;

	tmp = strob_open(10);

	instance_id = 1;  // Instance Id starts at 1
	i = 0;
	while((swmd = refererlist->get_pointer_from_index(i++))) {
		strob_sprintf(tmp, 0, "%d", instance_id); 
		swmd->vremove("instance_id");
		swmd->add("instance_id", strob_str(tmp));
	}

	//
	// Now look for tag collisions
	// and add the instance_id part
	//

	instance_id = 1;
	i = 0;
	while((swmd = refererlist->get_pointer_from_index(i++))) {
		tag = get_tag(swmd);
		SWLIB_ASSERT(tag != NULL);
		str_instance_id = swmd->find("instance_id");
		SWLIB_ASSERT(str_instance_id != NULL);
		if (::strcmp(str_instance_id, "1") != 0) {
			//
			// Already corrected this one.
			// skip it
			//
			continue;
		}
	
		j = i;
		while((swmd2 = refererlist->get_pointer_from_index(j++))) {
			SWLIB_ASSERT(get_tag(swmd2) != NULL);

			if (
				strcmp(tag, get_tag(swmd2)) == 0  &&
				strlen(tag)
			) {
				str_instance_id = swmd2->find("instance_id");
				if (::strcmp(str_instance_id, "1") != 0) {
					//
					// sanity check
					// the instance_id is already something other
					// than "1", this should not happen.
					//
					SWLIB_ASSERT(0);
				}
				instance_id++;
				strob_sprintf(tmp, 0, "%d", instance_id);
				swmd2->vremove("instance_id");
				swmd2->add("instance_id", strob_str(tmp));
			}
		}	
	}
	strob_close(tmp);
	return;
}
	
static
int
check_for_invalid_layout(swExStruct * swexdistribution)
{
	swExStruct * prod_ex;
	swExStruct * file_ex;
	int prod_index = 0;
	int prod_len = 0;
	int prod_obj = 0;
	int file_index = 0;
	int file_len = 0;
	int file_obj = 0;
	int n_prod = 0;
	int n_file = 0;
	int retval;

	prod_ex = swexdistribution->containedByIndex(prod_index++);
	while(prod_ex) {
		if (::strcmp(SW_A_product, prod_ex->getObjectName()) == 0) {
			n_prod ++;  // Number of products
			if (strlen(get_control_directory(prod_ex->getReferer()))) {
				prod_len ++; 
			}
			file_ex = prod_ex->containedByIndex(file_index++);
			while(file_ex) {
				n_file ++;   // Number of filesets
				if (strlen(get_control_directory(file_ex->getReferer()))) {
					file_len ++; 
				}
				file_ex = prod_ex->containedByIndex(file_index++);
			}
		}
		prod_ex = swexdistribution->containedByIndex(prod_index++);
	}

	if ( 
		(n_prod > 1 || n_file > 1) || 
		0
	) {
		//
		// enforce that there are no empty string control
		// directories.
		//
		if (
			(n_prod != prod_len) ||
			(n_file != file_len) ||
			0
		) {
			return -1;
		}  else {
			return 0;	
		}
		
	}

	if ( 
		(n_prod == 1 && n_file == 1) || 
		0
	) {
		//
		// Single fileset and single product
		//
		if (
			(n_prod != prod_len) ||
			(n_file != file_len) ||
			0
		) {
			//
			// If either has empty control directory
			// enforce that both do.
			//
			if (prod_len || file_len)
				//
				// One has a non-empty control dir.
				//
				return -1;
			else
				return 0;
		} else {
			//
			// Both have non empty control directory
			//
		}
	}
	return 0;
}

static
void
add_instance_id_attributes(swExStruct * swexdistribution, char * olevelname)
{
	int index = 0;
	swExStruct * ex;
	swPtrList<swDefinition> * refererlist;

	refererlist = new swPtrList<swDefinition>();

	ex = swexdistribution->containedByIndex(index++);
	while(ex) {
		if (swlib_altfnmatch(olevelname, ex->getObjectName()) == 0) {
			//
			// make a list of products and bundles
			//
			refererlist->list_add(ex->getReferer());
			if (strcmp(ex->getObjectName(), SW_A_product) == 0) {
				//
				// recursively do the same for filesets
				//
				add_instance_id_attributes(ex, SW_A_fileset);	
			}
		}
		ex = swexdistribution->containedByIndex(index++);
	}
	add_instance_id_bh(refererlist);
	add_control_directory_instance_part_bh(refererlist);
	delete refererlist;
	return;
}

static
void
add_is_locatable_attributes(swExStruct * swexdistribution)
{
	int index = 0;
	char * value;
	swExStruct * ex;
	swExStruct * ex2;

	ex = swexdistribution->containedByIndex(index++);
	while(ex) {
		if (strcmp(SW_A_product, ex->getObjectName()) == 0) {
			//
			// See if is_locatable is "true" in any of the 
			// filesets.
			//
			value = find_is_locatable_attribute(ex, SW_A_fileset);
			if (value && strcasecmp("true", value) == 0) {
				ex->getReferer()->add("is_locatable", "true");
			} else {
				ex->getReferer()->add("is_locatable", CHARFALSE);
			}
		}
		ex = swexdistribution->containedByIndex(index++);
	}
	return;
}

static
int 
write_psf_from_list(
		int list_fd, 
		int ofd, 
		char * cwd, 
		char * catalog_owner, 
		char * catalog_group)
{
	FILE * ofp;
	FILE * ifp;
	STROB * tobj;
	char line[1024];
	char * t;
	char *o;
	int newbuflen;

	newbuflen = sizeof(line) * 2;
	tobj = strob_open(10);
	strob_set_length(tobj, newbuflen);

	ofp = fdopen(ofd, "w");
	if (ofp == NULL) return -1;
	ifp = fdopen(list_fd, "r");
	if (ifp == NULL) return -1;

	fprintf(ofp, "distribution\n");
	fprintf(ofp, "title \"Ad hoc package\" \n");
	if (catalog_owner) 
		fprintf(ofp, SWBIS_CATALOG_OWNER_ATT " %s\n",
			catalog_owner);
	if (catalog_group) 
		fprintf(ofp, SWBIS_CATALOG_GROUP_ATT " %s\n",
			catalog_group);
	fprintf(ofp, "product\n");
	fprintf(ofp, "tag product_tag\n");
	fprintf(ofp, "control_directory \"\"\n");
	fprintf(ofp, "fileset\n");
	fprintf(ofp, "tag fileset_tag\n");
	fprintf(ofp, "control_directory \"\"\n");
	fprintf(ofp, "directory %s /\n", cwd);

	while (fgets (line, sizeof(line) - 1, ifp) != (char *) (NULL)) {
		strob_strcpy(tobj,  ""); 
		if (strlen(line) >= sizeof(line) - 2) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"file pathname too long : %s\n", line);
			exit(1);
		}
		if ((t=strpbrk (line,"\n\r"))) {
			*t = '\0';
		}
		if (!strlen(line)) {
			continue;
		}

		//
		// escape spaces in filenames
		//
		o = line;
		while (*o) {
			if (*o == '\x20') {
				strob_charcat(tobj,  '\\'); 
			} 
			strob_charcat(tobj,  *o);
			o++;
		}
		fprintf(ofp, "file %s %s\n",
			strob_str(tobj),
			strob_str(tobj));
	}
	fclose(ofp);
	fclose(ifp);
	strob_close(tobj);
	return 0;
}

int
do_files_from(int ofd, char * cwd, char * files_from,
			char * catalog_owner, char * catalog_group)
{
	int ret;
	int ifd;
	
	if (strcmp(files_from, "-") == 0) {
		ifd = STDIN_FILENO;
	} else {
		ifd = open(files_from, O_RDONLY, 0);
	}
	if (ifd < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"open failed on (%s)\n", files_from);
		exit(1);
	}	
	ret = write_psf_from_list(ifd, ofd, cwd,
			catalog_owner, catalog_group);
	return ret;
}

static
int
is_nominal_dirname(char * dir)
{
	if (
		dir == NULL ||
		strlen(dir) == 0 ||
		strcmp(dir , ".") == 0 ||
		strcmp(dir , "./") == 0 ||
		strstr(dir , "..") == 0 ||
		strcmp(dir , "/") == 0 
	) return 0;
	return 1;
}

static
int
set_format_attributes(
		int format_arf,
		char * opt_format,
		int do_oldgnutar, 
		int do_bsdpax3, 
		int do_oldgnuposix, 
		char ** taremu_attr, 
		char ** taremu_value)
{
	static char * taremulist[6] = {
				(char*)0,
				"", 
				"tar,r=1.13.25",  
				"bsdpax3", 
				"tar,r=1.13.25",
				"tar,r=1.15.*,r=1.14.*"
				};
	static char * options[7] = {
				(char*)0,
				"", 
				"--posix -b1", 
				"-b1", 
				"-b 512",
				"-b1 --format=oldgnu",
				"-b1 --format=ustar"
				};

	*taremu_attr = taremulist[0];
	*taremu_value = options[0];

	if (format_arf == arf_ustar) {
		if (do_oldgnutar) {
			if (strcmp(opt_format, "oldgnu") == 0) {
				*taremu_attr = taremulist[2];
				*taremu_value = options[5];
			} else {
				*taremu_attr = taremulist[2];
				*taremu_value = options[3];
			} 
		} else if (do_oldgnuposix) {
			*taremu_attr = taremulist[2];
			*taremu_value = options[2];
		} else if (do_bsdpax3) {
			*taremu_attr = taremulist[3];
			*taremu_value = options[4];
		} else {
			if (strcmp(opt_format, "ustar0") == 0) {
				*taremu_attr = taremulist[2];
				*taremu_value = options[2];
			} else if (strcmp(opt_format, "ustar") == 0) {
				*taremu_attr = taremulist[5];
				*taremu_value = options[6];
			} else { 
				*taremu_attr = taremulist[2];
				*taremu_value = options[2];
			}
		}
	}
	return 0;
}

static
int
fix_passfd(int fd)
{
	int nullfd;
	int newpassfd;
	/*
	* transfer the stdin file to another
	* descriptor. Not only might this be
	* safer but there is some "bustedness" in
	* swpackage which requires us to do this.
	*/
	if (fd != 0) exit(2);

	newpassfd = dup(fd);
	if (newpassfd < 3 || newpassfd > (SWBIS_MIN_FD_AVAIL - 4)) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s: fix_passfd: internal error, dup error\n",
				swlib_utilname_get());
		exit(1);
	}
	close(fd);
	nullfd = open(DEVNULL, O_RDWR, 0);
	if (nullfd < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"error opening /dev/null\n");
		exit(1);
	}
	if (nullfd > 0) {
		if (dup2(nullfd, fd) < 0) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"dup2 error\n");
			exit(1);
		}
		close(nullfd);	
	} else if (nullfd == 0 && fd == 0) {
		/*
		* nothing to do
		*/
		;
	} else {
		;
		/* 
		* opps, maybe fd is not zero.
		*/
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"internal error: fd not zero\n");
		exit(1);
	}
	return newpassfd;
}

static
void	
set_preview_level(swExCat * swexdist, int opt_preview, int verbose_level)
{
	int adder = 0;
	if (opt_preview) {
		adder = 0;
	}
	if (verbose_level == SWC_VERBOSE_0) {
		swexdist->set_preview_fd(-1);
		swexdist->set_preview_level(TARU_PV_0);
	} else if (verbose_level == SWC_VERBOSE_1) {
		swexdist->set_preview_level(TARU_PV_0+adder);
	} else if (verbose_level == SWC_VERBOSE_2) {
		swexdist->set_preview_level(TARU_PV_1+adder);
	} else if (verbose_level == SWC_VERBOSE_3) {
		swexdist->set_preview_level(TARU_PV_2+adder);
	} else if (verbose_level >= SWC_VERBOSE_4) {
		swexdist->set_preview_level(TARU_PV_3+adder);
	} else {
		swexdist->set_preview_level(TARU_PV_0);
	}
}

int
resign_write_pass_files(XFORMAT * package, int fp_ofd, unsigned char * sig_block_image, int sig_block_length)
{
	int nullfd = swbis_devnull_open("/dev/null", O_RDWR, 0);
	int ifd = xformat_get_ifd(package);
	int ofd;
	int ret;
	int retval = 0;
	int total_sym_bytes = 0;
	STROB * namebuf;
	char *name;
	int did_sigs;
	int package_filetype;
	SWPATH * swpath;

	if (nullfd < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "error opening /dev/null\n");
		return -1;
	}
	swpath = swpath_open("");
	did_sigs = 0;
	ofd = fp_ofd;
	namebuf = strob_open(32);
	SWP_E_DEBUG("ENTERING");
	SWP_E_DEBUG2("sig_block_image=%p", sig_block_image);
	SWP_E_DEBUG2("sig_block_length=%d", sig_block_length);
	taruib_set_overflow_release(0);
	taruib_set_fd(ofd);
	while ((ret = xformat_read_header(package)) > 0) {
		retval += ret;
		if (xformat_is_end_of_archive(package)){
			swbis_devnull_close(nullfd);
			break;
		}
		
		xformat_get_name(package, namebuf);
                name = strob_str(namebuf);

		if (swpath_parse_path(swpath, name) < 0) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "error parsing pathname: %s\n", name);
		}
		
		package_filetype = swpath_get_is_catalog(swpath);

		if (package_filetype == SWPATH_CTYPE_CAT) {
			;	
			SWP_E_DEBUG3("did_sigs=[%d]  NAME=[%s]\n", did_sigs, name);
		}

		if (
			did_sigs < 2 && 
			(
				(
				fnmatch("*/catalog/*/signature", name, 0) == 0 ||
				fnmatch("catalog/*/signature", name, 0) == 0
				)
			) && package_filetype == SWPATH_CTYPE_CAT 
		) {
			SWP_E_DEBUG("/signature match");
			taruib_set_fd(nullfd);
			ofd = nullfd;
			did_sigs = 1;
		} else {
			if (did_sigs == 1) {
				//
				// write out the sig_block_image
				//
				SWP_E_DEBUG("IN did_sigs");
				SWP_E_DEBUG2("writing new sig_block [%d] bytes", sig_block_length);
				SWP_E_DEBUG2("writing new sig_block to fd [%d]", fp_ofd);
				taruib_set_fd(fp_ofd);
				ofd = fp_ofd;
				ret = uxfio_write(ofd, (void*)sig_block_image, (size_t)sig_block_length);
				if (ret != sig_block_length) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "write error\n");
					return -1;
				}
				did_sigs = 2;  // Disable from this point on.
			}
		}

		/*
		* Here is where the header is actually written
		* Note we omit the "/signature" archive member by setting the output fd to /dev/null
		*/

		taruib_clear_buffer();

		/*
		* Here is where the file data (if any) is actually written
		*/
		if (xformat_file_has_data(package)) {
			taruib_set_fd(-1);
			taruib_set_overflow_release(1);
			if (xformat_copy_pass2(package, ofd, ifd, -1) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "write (xformat_copy_pass2) error\n");
				return -2;
			}
			taruib_set_overflow_release(0);
			taruib_set_datalen(0);
		}
		taruib_set_fd(fp_ofd);
		ofd = fp_ofd;
	}

					SWP_E_DEBUG("");
	taruib_clear_buffer();
					SWP_E_DEBUG("");
	taruib_set_fd(-1);
	taruib_set_overflow_release(1);
					SWP_E_DEBUG("before pump_amount");
	ret = taru_pump_amount2(ofd, ifd, -1, -1); 
					SWP_E_DEBUG("after pump_amount");
	if (ret < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "write (taru_pump_amount2) error\n");
		retval = -1;
	} else {
		retval += ret;
	}
	swpath_close(swpath);
	strob_close(namebuf);
	close(nullfd);
	return  retval;
}

static
char *
format_armored_sig_with_newlines(char * sig, int data_length)
{
	size_t LEN = data_length+1;
	char * xx = (char*)malloc(LEN);	

	if ((int)strlen(sig) >= (int)(LEN-1)) {
		// sanity check
		// error
		return NULL;
	}
	memset(	xx,  '\n', LEN);
	xx[LEN] = '\0';
	memcpy((void*)((char*)xx+0), (void*)sig, strlen(sig));
	return xx;
}

static int
resign_modify_sig_block(char * sig, char * sig_tarheader,
			int sig_block_fd,
			int new_sig_block_fd,
			int sig_length,
			int sig_add_num,
			int sig_replace_num,
			int sig_remove_num)
{
	char * p;
	int ret;
	int retval = -1;
	int sig_total_size = sig_length + TARRECORDSIZE;
	int n_existing_sigs;
	
	ret = -1;
	if (sig_add_num >= 0) {
		if (!sig) return -1;
		switch (sig_add_num) {
			case 0:
				//add last signature
				SWP_E_DEBUG("");
				p = format_armored_sig_with_newlines(sig, sig_length);
				SWLIB_ALLOC_ASSERT(p != NULL);
				uxfio_ftruncate(new_sig_block_fd, 0);
				uxfio_lseek(sig_block_fd, 0, SEEK_SET);
				swlib_pipe_pump(new_sig_block_fd, sig_block_fd);

				// now append the new signature
				ret = uxfio_write(new_sig_block_fd, (void*)sig_tarheader, TARRECORDSIZE);
				SWLIB_ALLOC_ASSERT(ret ==TARRECORDSIZE);
				ret = uxfio_write(new_sig_block_fd, (void*)p, sig_length);
				SWLIB_ALLOC_ASSERT(ret == sig_length);
				free(p);
				retval = 0;
				break;
			case 1:
				// add first signature
				SWP_E_DEBUG("");
				p = format_armored_sig_with_newlines(sig, sig_length);
				SWLIB_ALLOC_ASSERT(p != NULL);
			
				uxfio_ftruncate(new_sig_block_fd, 0);
				ret = uxfio_write(new_sig_block_fd, (void*)sig_tarheader, TARRECORDSIZE);
				SWLIB_ALLOC_ASSERT(ret ==TARRECORDSIZE);
				ret = uxfio_write(new_sig_block_fd, (void*)p, sig_length);
				SWLIB_ALLOC_ASSERT(ret == sig_length);
		
				uxfio_lseek(sig_block_fd, 0, SEEK_SET);
				swlib_pipe_pump(new_sig_block_fd, sig_block_fd);
				free(p);
				retval = 0;
				break;
			default:
				// error
				break;
		}
	} else if (sig_replace_num >= 0) {
		SWP_E_DEBUG("sig_replace_num");
		if (!sig) return -1;
		p = format_armored_sig_with_newlines(sig, sig_length);
		SWLIB_ALLOC_ASSERT(p != NULL);
		ret = uxfio_lseek(sig_block_fd, 0, SEEK_END);
		SWLIB_ALLOC_ASSERT(ret > 0);
		n_existing_sigs =  ret / sig_total_size;
		SWLIB_ALLOC_ASSERT(n_existing_sigs >= 1);
		SWP_E_DEBUG("");
		if (sig_replace_num > n_existing_sigs ) {
			return -1;
		}
		SWP_E_DEBUG("");
		if (sig_replace_num == 0) {
			sig_replace_num = n_existing_sigs;
			SWP_E_DEBUG2("sig_replace_num now is %d", sig_replace_num);
		}
		ret = uxfio_lseek(sig_block_fd, 0, SEEK_SET);
		if (ret < 0) return -1;
		ret = swlib_pipe_pump(new_sig_block_fd, sig_block_fd);
		if (ret < 0) return -1;
		ret = uxfio_lseek(new_sig_block_fd, (sig_replace_num-1)*sig_total_size + TARRECORDSIZE, SEEK_SET);
		if (ret < 0) return -1;
		SWP_E_DEBUG("");
		ret = uxfio_write(new_sig_block_fd, (void*)p, sig_length);
		if (ret != sig_length) return -1;
		free(p);
		retval = 0;
	} else if (sig_remove_num >= 0) {
		int sgcount;
		SWP_E_DEBUG2("sig_remove_num=%d", sig_remove_num);
		ret = uxfio_lseek(sig_block_fd, 0, SEEK_END);
		n_existing_sigs =  ret / sig_total_size;
		if (n_existing_sigs < 1) {
			return -1;	
		}
		if (sig_remove_num > n_existing_sigs ) {
			return -1;
		}
		if (sig_remove_num == 0) {
			sig_remove_num = n_existing_sigs;
		}
		uxfio_lseek(sig_block_fd, 0, SEEK_SET);
		uxfio_lseek(new_sig_block_fd, 0, SEEK_SET);
		sgcount = 1;	
		while (sgcount <= n_existing_sigs) {
			SWP_E_DEBUG3("LOOPING: sig_remove_num=%d sgcount=%d", sig_remove_num, sgcount);
			if (sgcount != sig_remove_num) {
				ret = swlib_pump_amount(new_sig_block_fd, sig_block_fd, sig_total_size);
				if (ret != sig_total_size) {
					return -1;				
				}
			} else {
				ret = uxfio_lseek(sig_block_fd, (size_t)sig_total_size, SEEK_CUR);
				if (ret < 0) {
					return -2;				
				}
			}
			sgcount++;
		}
		uxfio_lseek(new_sig_block_fd, 0, SEEK_SET);
		retval = 0;
	} else {
		// error
		// why are we here.
		retval = -1;
	}

	return retval;
}

static
int
add_state_attribute_to_filesets(swExStruct * swexdistribution)
{
	swExStruct * prod_ex = NULL;
	swExStruct * file_ex = NULL;
	int prod_index = 0;
	int file_index = 0;

	prod_ex = swexdistribution->containedByIndex(prod_index++);
	while(prod_ex) {
		if (::strcmp(SW_A_product, prod_ex->getObjectName()) == 0) {
			file_index = 0;
			file_ex = prod_ex->containedByIndex(file_index++);
			while(file_ex) {
				if (::strcmp(SW_A_fileset, file_ex->getObjectName()) == 0) {
					file_ex->getReferer()->vremove(SW_A_state);
				}
				file_ex = prod_ex->containedByIndex(file_index++);
			}
		}
		prod_ex = swexdistribution->containedByIndex(prod_index++);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	int c;
	int optret = 0;
	int psf_source_ifd = STDIN_FILENO;
	int e_argc = 1;
	int w_argc = 1;
	int ret;
	int tmpret;
	int direct_to_dev_null = 0;
	int error_code;
	int main_optind = 0;
	struct extendedOptions * opta = ::optionsArray;
	int wint_catalog_only;
	int wint_storage_only;
	int wopt_format_arf = arf_ustar; // This is a value of the enumeration enum archive_format
	pid_t pid_array[4];
	int  status_array[4];
	int do_extension_options_via_goto = 0;
	int std_option_index = 0;
	int final_ofd;
	int pv_ofd = -100;
	int target_ofd = -1;
	int pv[2];
	int force_tty_passfd = 0;
	int wopt_no_front_dir = 0;
	int wopt_no_catalog = 0;
	int wopt_no_defaults = 0;
	int l_status = 1;
	int exit_retval = 0;
	int id_status;
	int status;
	int unrpmfd;
	int conversion_result;
	int paxhdr_pid = 0;
	int w_option_index = 0;
	int sig_num = -1;
	int sig_replace_num = -1;
	int sig_remove_num = -1;
	int sig_add_num = -1;
	char sysa_owner[TARU_SYSDBNAME_LEN];
	char sysa_group[TARU_SYSDBNAME_LEN];
	char sysa_tmp[TARU_SYSDBNAME_LEN];
	uid_t sysa_uid = getuid();
	gid_t sysa_gid = getgid();
	pid_t pid;

	char *taremu_value = (char*)NULL;
	char *taremu_attr = (char*)NULL;

	char * optionname;
	int  optionEnum;
	char * env_gpg_name = (char*)NULL;
	char * env_gpg_homedir = (char*)NULL;
	char * env_passphrase_fd = (char*)NULL;
	char * wopt_passphrase_fd = "-1";
	char * wopt_passfile = (char*)NULL;
	char * wopt_checkdigestname = (char*)NULL;
	char * wopt_sign;
	char * wopt_dummy_sign = (char*)NULL;
	char * wopt_write_signed_file = (char*)NULL;
	char * wopt_gzip = (char*)NULL;
	char * wopt_bzip2 = (char*)NULL;
	char * wopt_cksum = (char*)NULL;
	char * wopt_check_duplicates = (char*)NULL;
	char * wopt_dir = (char*)NULL;
	char * wopt_file_digests = (char*)NULL;
	char * wopt_file_digests2 = (char*)NULL;
	char * wopt_archive_digests = (char*)NULL;
	char * wopt_archive_digests2 = (char*)NULL;
	char * wopt_files = (char*)NULL;
	char * wopt_signer_bin = (char*)NULL;
	char * wopt_format = (char*)NULL;	// This is the user specified string which may 
						// include "pax" or "posix"  NOTE: wopt_format_arf
						// does not include "pax" or "posix".  PAX format
						// is specified by wopt_format_arf="ustar" and the
						// the appropriate tarheaderflags indicating extended headers.
	char * wopt_do_list = (char*)NULL;
	char * wopt_absolute_names = (char*)NULL;
	char * wopt_do_debug = (char*)NULL;
	char * wopt_catalog_only = (char*)NULL;
	char * wopt_storage_only = (char*)NULL;
	char * wopt_create_time = (char*)NULL;
	char * wopt_uuid = (char*)NULL;
	char * wopt_nosign = (char*)NULL;
	char * wopt_mode_do_unrpm = (char*)NULL;
	char * wopt_mode_do_resign = (char*)NULL;
	char * wopt_mode_do_resign_test = (char*)NULL;
	char * wopt_mode_do_recompress = (char*)NULL;
	char * wopt_mode_do_overwrite = (char*)NULL;
	char * wopt_no_sha1 = (char*)NULL;
	char * wopt_source_filename = (char*)NULL;
	char * wopt_gpg_name = (char*)NULL;
	char * wopt_gpg_path = (char*)NULL;
	char * wopt_show_options = (char*)NULL;
	char * wopt_show_options_files = (char*)NULL;
	char * wopt_show_signer_pgm = (char*)NULL;
	char * wopt_catalog_owner = (char*)NULL;
	char * wopt_catalog_group = (char*)NULL;
	char * wopt_dir_owner = (char*)NULL;
	char * wopt_dir_group = (char*)NULL;
	char * wopt_dir_mode = (char*)NULL; 
	char * wopt_files_from = (char*)NULL; 

	char * eopt_media_capacity;  			// Default set below.
	char * eopt_enforce_dsa;  			// Default set below.
	char * eopt_verbose;  				// Default set below.
	char * eopt_distribution_target_directory;  	// Default set below.
	char * eopt_distribution_target_serial;  	// Default set below.
	char * eopt_media_type;  			// Default set below.
	char * eopt_follow_symlinks;  			// Default set below.
	swDefinition * swdef;
	
	char * tmpname;
	char * gnu_tar_format_options = (char*)NULL;
	int tarheader_flags = 0;
	int catalog_tarheader_flags = 0;
	int do_bsdpax3 = 0;
	int do_paxexthdr = 0;
	int do_oldgnutar = 0;
	int do_gnutar = 0;
	int do_oldgnuposix = 0;
	int blocksize = 10240;
	int opt_preview = 0;
	int filesfd = -1;
	pid_t do_files_from_pid = 0;
	int do_files_pipe[2];
	int regfiles_only = 0;
	int no_header_overflow = 0;
	int construct_missing = 0;
	int unrpm_exclude_system_dirs = 0;
	unsigned long create_time = 0;
	char md5sum_buffer_o[512];
	char * md5sum_buffer = md5sum_buffer_o;
	char adjunct_md5sum_buffer_o[512];
	char * adjunct_md5sum_buffer = adjunct_md5sum_buffer_o;
	char sha1_buffer_o[512];
	char sha512_buffer_o[512];
	char size_buffer_o[512];
	char * sha1_buffer = sha1_buffer_o; /* on by default, unless --no-sha1 is used */
	char * sha512_buffer = NULL;        /* off by default */
	char * size_buffer = size_buffer_o;
	char * opt_option_files = NULL;
	char * system_defaults_files = NULL;
	char * psffilename = NULL;
	char * pkgfilename = NULL;
	char * selections_filename = NULL;
	char * exoption_arg;
	char * exoption_value;
	char * cl_target;
	char * cl_selections;
	char * psf_source_file;
	char * slack_name = NULL;
	char *arg, *p;
	struct stat cwdst;
	char cwd[512];
	STROB * tmp;
	STROB * tmp2;
	CPLOB * w_arglist;
	CPLOB * e_arglist;
	SHCMD * signer_command;
	swPSF * psf;
	swExFileset fset;
	swExProduct prod;
	swINFO iii("");;
	swExCat * swexdist;
	swExCat * swexdistribution;
	SWVARFS * swvarfs;
	VPLOB * compression_layers;  /* array of SHCMD objects */
	SHCMD * compressor;

	/*
	 * Here is the first executing line of the main() routine
	 */
	umask((mode_t)0);
	memset(md5sum_buffer_o, '\0', sizeof(md5sum_buffer_o));
	memset(adjunct_md5sum_buffer_o, '\0', sizeof(adjunct_md5sum_buffer_o));
	memset(sha1_buffer_o, '\0', sizeof(sha1_buffer_o));
	memset(sha512_buffer_o, '\0', sizeof(sha512_buffer_o));
	memset(size_buffer_o, '\0', sizeof(size_buffer_o));

	id_status = 0;
	unrpmfd = -1;
	swfdio_init();
	swlib_utilname_set(progName);
	swgp_check_fds(); /* check for sufficient fd's */
	swc0_create_parser_buffer();
	tmp = strob_open(10);		// General use String object.
	tmp2 = strob_open(10);		// General use String object.
	w_arglist = cplob_open(1);	// Pointer list object.
	e_arglist = cplob_open(1);	// Pointer list object.
	compression_layers = NULL;

	initExtendedOption();
	psffilename = strdup("-");
	SWLIB_ALLOC_ASSERT(psffilename != NULL);

	paxhdr_pid = (int)getpid();
	swlib_set_pax_header_pid((pid_t)paxhdr_pid);

	g_nullfd = open(DEVNULL, O_RDWR, 0);
	if (g_nullfd < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"error opening " DEVNULL "\n");
		exit(1);
	}

	if (getcwd(cwd, sizeof(cwd) - 1) == NULL) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"getcwd error : %s\n", strerror(errno));
		exit(1);
	}
	cwd[sizeof(cwd) - 1] = '\0';
	
	if (lstat(cwd, &cwdst) < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"lstat error : %s : %s\n", cwd, strerror(errno));
		exit(1);
	}

	sysa_owner[0] = '\0';
	sysa_group[0] = '\0';

	//
	// Set the Posix extended option defaults.
	// These are the extended option defaults if no 
	// defaults file is read.
	//
	psf_source_file = "-";
	eopt_media_capacity = "0";
	eopt_enforce_dsa = "true";
	eopt_verbose = "1";
	eopt_distribution_target_directory = "/var/spool/sw";
	eopt_distribution_target_serial = "-";
	eopt_media_type = "serial";
	eopt_follow_symlinks = "false";
	::set_opta_initial(opta, SW_E_verbose, eopt_verbose);
	::set_opta_initial(opta, SW_E_enforce_dsa, eopt_enforce_dsa);
	::set_opta_initial(opta, SW_E_media_capacity, eopt_media_capacity);
	::set_opta_initial(opta, SW_E_distribution_target_directory,
				eopt_distribution_target_directory);
	::set_opta_initial(opta, SW_E_distribution_target_serial,
				eopt_distribution_target_serial);
	::set_opta_initial(opta, SW_E_media_type, eopt_media_type);
	::set_opta_initial(opta, SW_E_follow_symlinks, eopt_follow_symlinks);

	//
	// Set the Implementation extended option defaults.
	// These are the extended option defaults if no defaults file is read.
	//
	wopt_cksum	=		CHARFALSE;
	wopt_file_digests	=	CHARFALSE;
	wopt_file_digests2	=	CHARFALSE;
	wopt_files	=		CHARFALSE;
	wopt_sign	=		CHARFALSE;
	wopt_archive_digests	=	CHARFALSE;
	wopt_archive_digests2	=	CHARFALSE;
	wopt_gpg_name	=		"";
	wopt_gpg_path	=		"~/.gnupg";
	wopt_gzip	=		CHARFALSE;
	wopt_bzip2	=		CHARFALSE;
	g_wopt_numeric_owner	     =	CHARFALSE;
	g_wopt_dir_numeric_owner     = 	CHARFALSE;
	g_wopt_catalog_numeric_owner = 	CHARFALSE;
	g_wopt_dummy_sign            =	CHARFALSE;
	wopt_absolute_names	=	CHARFALSE;
	wopt_format		=	SWBIS_A_pax;
	wopt_signer_bin		=	"GPG";
	wopt_check_duplicates	=	CHARTRUE;

	::set_opta_initial(opta, SW_E_swbis_cksum, wopt_cksum);
	::set_opta_initial(opta, SW_E_swbis_file_digests, wopt_file_digests);
	::set_opta_initial(opta, SW_E_swbis_file_digests_sha2, wopt_file_digests2);
	::set_opta_initial(opta, SW_E_swbis_files, wopt_files);
	::set_opta_initial(opta, SW_E_swbis_sign, wopt_sign);
	::set_opta_initial(opta, SW_E_swbis_archive_digests, wopt_archive_digests);
	::set_opta_initial(opta, SW_E_swbis_archive_digests_sha2, wopt_archive_digests2);
	::set_opta_initial(opta, SW_E_swbis_gpg_name, wopt_gpg_name);
	::set_opta_initial(opta, SW_E_swbis_gpg_path, wopt_gpg_path);
	::set_opta_initial(opta, SW_E_swbis_gzip, wopt_gzip);
	::set_opta_initial(opta, SW_E_swbis_bzip2, wopt_bzip2);
	::set_opta_initial(opta, SW_E_swbis_numeric_owner, g_wopt_numeric_owner);
	::set_opta_initial(opta, SW_E_swbis_absolute_names, wopt_absolute_names);
	::set_opta_initial(opta, SW_E_swbis_format, wopt_format);
	::set_opta_initial(opta, SW_E_swbis_signer_pgm, wopt_signer_bin);
	::set_opta_initial(opta, SW_E_psf_source_file, psf_source_file);
	::set_opta_initial(opta, SW_E_swbis_check_duplicates, wopt_check_duplicates);

	static struct ugetopt_option_desc main_help_desc[] =
{
{"selections-file", "FILE", "Take software selections from this file."},
{"preview", "", "Preview only by writing information to stdout.\n"
"      The package archive is not written. Verbose level 1\n"
"      is silent except for warnings and errors. Apply -v to increase\n"
"      information content on stdout.\n"
"          'swpackage -p -vvv'   is useful for PSF development"
},
{"psf-file", "PSF", "Specify PSF file."},
{"options-file", "FILE[ FILE2 ...]", "Specify files that override \n"
"        system option defaults. Specify empty string to disable \n"
"        option file reading."
},
{"verbose", "",  "same as '-x verbose=2' (-v is implementation extension).\n"
"      -v  is level 2, -vv is level 3,... etc.\n"
"         level 0: silent on stdout and stderr (not implemented).\n"
"         level 1: (default) fatal and warning messages to stderr.\n"
"    -v   level 2: level 1 plus file list.\n"
"    -vv  level 3: level 1 plus in verbose tar listing format.\n"
"    -vvv level 4: level 1 plus in extended verbose tar listing format.\n"
"    -vvv... levels 5 and higher are supported."
},
{"", "option[=value]", "Specify implementation extension option."},
{"", "option=value", "Specify posix extended option."},
{"block-size", "BYTES", "Specify blocksize as number of bytes(octets).\n"
"      0 means no blocksize\'ing, (-b is an implementation extension)."
},
{"format", "FORMAT", "specify archive format"},
{"version", "", "Show version information to stdout."},
{"copyright", "", "Show copyright information to stdout."},
{"about", "", "Show some identifying info."},
{"help", "", "Show this help to stdout."},
{0, 0, 0}
};
	cplob_add_nta(w_arglist, strdup(argv[0]));
	cplob_add_nta(e_arglist, strdup(argv[0]));
	
	static struct option std_long_options[] = {
               {"selections-file", 1, 0, 'f'},
               {"preview", 0, 0, 'p'},
               {"psf-file", 1, 0, 's'},
               {"defaults-file", 1, 0, 'X'},
               {"verbose", 0, 0, 'v'},
               {"extension-option", 1, 0, 'W'},
               {"extended-option", 1, 0, 'x'},
               {"block-size", 1, 0, 'b'},
               {"format", 1, 0, 'H'},
               {"version", 0, 0, 'V'},
               {"copyright", 0, 0, '\010'},
               {"about", 0, 0, '\011'},
               {"help", 0, 0, '\012'},
               {0, 0, 0, 0}
	};

	//
	// This is used internally and do not indicate these
	// are user options.
	//

	static struct option posix_extended_long_options[] =
             {
		{"distribution_target_directory", 1, 0, 200},
		{"distribution_target_serial", 1, 0, 201},
		{"media_type", 1, 0, 202},
		{"media_capacity", 1, 0, 203},
		{"enforce_dsa", 1, 0, 204},
		{"verbose", 1, 0, 205},
		{"follow_symlinks", 1, 0, 199},
		{0, 0, 0, 0}
             };
       
		
	static struct option long_options[] = {
               {"selections-file", 1, 0, 'f'},
               {"preview", 0, 0, 'p'},
               {"block-size", 1, 0, 'b'},
               {"psf-file", 1, 0, 's'},
               {"defaults-file", 1, 0, 'X'},
               {"extension-option", 1, 0, 'W'},
               {"extended-option", 1, 0, 'x'},
               {"version", 0, 0, 'V'},
               {"copyright", 0, 0, '\010'},
               {"about", 0, 0, '\011'},
               {"help", 0, 0, '\012'},
               {"digests", 0, 0, 129},
               {"cksum", 0, 0, 130},
               {"file-digests", 0, 0, 131},
               {"list-psf", 0, 0, 132},
               {"source", 1, 0, 133},
               {"format", 1, 0, 'H'},
               {"numeric-owner", 0, 0, 135},
               {"gzip", 0, 0, 136},
               {"bzip2", 0, 0, 137},
               {"bz2", 0, 0, 137},
               {"sign", 0, 0, 138},
               {"dir", 1, 0, 139},
               {"debug", 1, 0, 140},
               {"catalog-only", 0, 0, 141},
               {"storage-only", 0, 0, 142},
               {"create-time", 1, 0, 143},
               {"gpg-name", 1, 0, 144},
               {"gpg-path", 1, 0, 145}, /* for back compatibility */
               {"gpg-homedir", 1, 0, 145},
               {"write-signed-file-only", 0, 0, 146},
               {"files", 0, 0, 147},
               {"archive-digests", 0, 0, 148},
               {"absolute-names", 0, 0, 149},
               {"2posixformat", 0, 0, 150},
               {"unrpm", 0, 0, 151},
               {"to-swbis", 0, 0, 151},
               {"undebian", 0, 0, 151},
               {"no-sha1", 0, 0, 152},
               {"dereference", 0, 0, 153},
               {"noop", 0, 0, 154},
               {"local-user", 1, 0, 155},
               {"homedir", 1, 0, 156},
               {"passphrase-fd", 1, 0, 157},
               {"show-options", 0, 0, 158},
               {"show-options-files", 0, 0, 159},
               {"uuid", 1, 0, 160},
               {"nosign", 0, 0, 161},
               {"passfile", 1, 0, 162},
               {"signer-pgm", 1, 0, 163},
               {"show-signer-pgm", 0, 0, 164},
               {"catalog-owner", 1, 0, 165},
               {"catalog-group", 1, 0, 166},
               {"dir-group", 1, 0, 167},
               {"dir-owner", 1, 0, 168},
               {"dir-mode", 1, 0, 169},
               {"files-from", 1, 0, 170},
               {"no-front-dir", 0, 0, 171},
               {"no-catalog", 0, 0, 172},
               {"no-defaults", 0, 0, 173},
               {"file-sha2", 0, 0, 174},
               {"file-sha512", 0, 0, 174},
               {"file-digests-sha2", 0, 0, 174},
               {"archive-sha2", 0, 0, 175},
               {"archive-sha512", 0, 0, 175},
               {"archive-digests-sha2", 0, 0, 175},
               {"sha2", 0, 0, 176},
               {"sha1", 0, 0, 177},
               {"dummy-sign", 0, 0, 178},
               {"checkdigest-file", 1, 0, 179},
               {"check-duplicates", 1, 0, 180},
               {"lzma", 0, 0, 181},
               {"symmetric", 0, 0, 182},
               {"encrypt-for-recipient", 1, 0, 183},
               {"use-agent", 0, 0, 184},
               {"regfiles-only", 0, 0, 185},
               {"xz", 0, 0, 186},
               {"slackware-pkg-name", 1, 0, 187},
               {"add-signature-first", 0, 0, 188},
               {"add-signature-last", 0, 0, 189},
               {"replace-signature", 1, 0, 190},
               {"resign", 0, 0, 191},
               {"zfilter", 0, 0, 192},
               {"resign-test", 0, 0, 192},
               {"recompress", 0, 0, 194},
               {"overwrite", 0, 0, 195},
               {"addsign", 0, 0, 196},
               {"delsign", 0, 0, 197},
               {"pax-header-pid", 1, 0, 198},
               {"remove-signature", 1, 0, 193},
 	       {"swbis-signer-pgm", 1, 0, 206},
               {"swbis_signer_pgm", 1, 0, 206},
               {"swbis-cksum", 1, 0, 207},
               {"swbis_cksum", 1, 0, 207},
               {"swbis-file-digests", 1, 0, 208},
               {"swbis_file_digests", 1, 0, 208},
               {"swbis-files", 1, 0, 209},
               {"swbis_files", 1, 0, 209},
               {"swbis-sign", 1, 0, 210},
               {"swbis_sign", 1, 0, 210},
               {"swbis-check-duplicates", 1, 0, 211},
               {"swbis_check_duplicates", 1, 0, 211},
               {"swbis-archive-digests", 1, 0, 212},
               {"swbis_archive_digests", 1, 0, 212},
               {"swbis-gpg-name", 1, 0, 213},
               {"swbis_gpg_name", 1, 0, 213},
               {"swbis-gpg-path", 1, 0, 214},
               {"swbis_gpg_path", 1, 0, 214},
               {"swbis-numeric-owner", 1, 0, 216},
               {"swbis_numeric_owner", 1, 0, 216},
               {"swbis-absolute-names", 1, 0, 217},
               {"swbis_absolute_names", 1, 0, 217},
               {"swbis-file-digests-sha2", 1, 0, 218},
               {"swbis_file_digests_sha2", 1, 0, 218},
               {"swbis-archive-digests-sha2", 1, 0, 219},
               {"swbis_archive_digests_sha2", 1, 0, 219},
               {"swbis-gzip", 1, 0, 220},
               {"swbis_gzip", 1, 0, 220},
               {"swbis-bzip2", 1, 0, 221},
               {"swbis_bzip2", 1, 0, 221},
               {"construct-missing-files", 0, 0, 222},
               {"no-overflow", 0, 0, 223},
	       {"exclude-system-dirs", 0, 0, 230},
               {0, 0, 0, 0}
	};

	env_passphrase_fd = getenv("SWPACKAGEPASSFD");
	if (env_passphrase_fd) {
		wopt_passphrase_fd = strdup(env_passphrase_fd);
		if (strcmp(env_passphrase_fd, SWP_PASS_AGENT) == 0) {
			g_passfd = -1;
			wopt_passphrase_fd = strdup(SWP_PASS_AGENT);
		} else if (strcmp(env_passphrase_fd, SWP_PASS_TTY) == 0) {
			g_passfd = -1;
			g_passphrase = (char*)NULL;
			wopt_passphrase_fd = "-1";
			wopt_passfile = (char*)NULL;
		} else if (strcmp(env_passphrase_fd, SWP_PASS_ENV) == 0) {
			g_passphrase = getenv("SWPACKAGEPASSPHRASE");
			if (!g_passphrase)
				g_passphrase = strdup("");
		} else {
			g_passfd = swlib_atoi(env_passphrase_fd, &tmpret);
			if (    tmpret == 0 &&
				((g_passfd > 2 && g_passfd < OPEN_MAX) || 
				(g_passfd == STDIN_FILENO))
		   	   )
			{
				if (g_passfd == STDIN_FILENO) g_stdin_use_count++;
				if (g_passfd == STDIN_FILENO) {	
					if ((g_passfd = fix_passfd(STDIN_FILENO)) < 0) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
							"invalid passphrase fd\n");
						exit(1);
					}
					strob_sprintf(tmp, 0, "%d", g_passfd);
					wopt_passphrase_fd = strdup(strob_str(tmp));
				}
			} else {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"invalid value for SWPACKAGEPASSFD environment variable\n");
				exit(1);
			}
		}
	}	
	
	env_gpg_name = getenv("GNUPGNAME");
	if (env_gpg_name) {
		wopt_gpg_name = strdup(env_gpg_name);
		::set_opta(opta, SW_E_swbis_gpg_name, wopt_gpg_name);
	}

	env_gpg_homedir = getenv("GNUPGHOME");
	if (env_gpg_homedir) {
		wopt_gpg_path = strdup(env_gpg_homedir);
		::set_opta(opta, SW_E_swbis_gpg_path, wopt_gpg_path);
	}
	
	while (1) {
		c = ugetopt_long(argc, argv, "b:f:pvVs:X:x:W:H:", 
					long_options, &std_option_index);
		if (c == -1) break;

		SWP_E_DEBUG2("c=%d", c);
		switch (c) {
	 	case 'b':
			blocksize = swlib_atoi(optarg, NULL);
			if (blocksize < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"illegal blocksize\n");
				exit(1);
			}
			break;
	 	case 'p':
			opt_preview = 1;
			break;
		case 'v':
			verboseG++;
			eopt_verbose = (char*)malloc(12);
			snprintf(eopt_verbose, 11, "%d", verboseG);
			eopt_verbose[11] = '\0';
			swlib_set_verbose_level(verboseG);
			::set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		case 'f':
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"option -f not implemented... exiting\n");
			exit(1);
			selections_filename = strdup(optarg);
			SWLIB_ALLOC_ASSERT(selections_filename != NULL);
			break;

		case 's':
			psffilename = strdup(optarg);
			pkgfilename = psffilename;
			::set_opta(opta, SW_E_psf_source_file, psffilename);
			SWLIB_ALLOC_ASSERT(psffilename != NULL);
			break;
		case 'H':
			SWP_E_DEBUG("");
			wopt_format = strdup(optarg);
			::set_opta(opta, SW_E_swbis_format, wopt_format);
			swc0_set_arf_format(optarg, 
				&wopt_format_arf,
				&do_oldgnutar,
				&do_bsdpax3,
				&do_oldgnuposix,
				&do_gnutar,
				&do_paxexthdr);
			break;

		case 'W':
			SWP_E_DEBUG("");
			swc0_process_w_option(tmp, w_arglist, optarg, &w_argc);
			break;

		case 'x':
			exoption_arg = strdup(optarg);
			SWLIB_ALLOC_ASSERT(exoption_arg != NULL);
			//
			// Parse the extended option and add to pointer list
			// for later processing.
			//
			{
				char * np;
				char * t;
				STROB * etmp = strob_open(10);
				
				t = strob_strtok(etmp, exoption_arg, " ");
				while (t) {
					exoption_value = strchr(t,'=');
					if (!exoption_value) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
							"invalid extended arg : %s\n",
							optarg);
						exit(1);
					}
					np = exoption_value;
					*exoption_value = '\0';
					exoption_value++;
					//
					// Now add the option and value to 
					// the arg list.
					//
					strob_strcpy(tmp, "--");
					strob_strcat(tmp, t);
					strob_strcat(tmp, "=");
					strob_strcat(tmp, exoption_value);
					cplob_add_nta(e_arglist, 
						strdup(strob_str(tmp)));
					e_argc ++;

					*np = '=';
					t = strob_strtok(etmp, NULL, " ");
				}
				strob_close(etmp);
			}
			break;

		case 'X':
			if (opt_option_files) {
				opt_option_files = (char*)realloc(
						(void*)opt_option_files, 
						strlen(opt_option_files) + 
							strlen(optarg) + 2);
				strcat(opt_option_files, " ");
				strcat(opt_option_files, optarg);
			} else {
				opt_option_files = strdup(optarg);
			}
			break;
               case 'V':
			version_info(stdout);
			exit(0);
		 	break;
               case '\010':
			copyright_info(stdout);
			exit(0);
		 	break;
               case '\011':
			about_info(stdout);
			exit(0);
		 	break;
		case '?':
			swlib_doif_writef(verboseG, g_fail_loudly, 0, get_stderr_fd(),
				"Try `swpackage --help' for more information.\n");
			exit(1);
               case '\012':
			usage(stdout, argv[0], std_long_options, 
						main_help_desc);
			exit(1);
		 	break;
               default:
			SWP_E_DEBUG2("in default case with c = %d", c);
			if (c >= 129 && c <= WOPT_LAST) { 
				//
				// This provides the ablility to specify 
				// extension options by using the 
				// --long-option syntax (i.e. without using 
				// the -Woption syntax) .
				//
				SWP_E_DEBUG2("doing goto with c=%d", c);
				do_extension_options_via_goto = 1;
				goto gotoExtensionOptions;
gotoStandardOptions:
				;
			} else {
				SWP_E_DEBUG("invalid args");
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
						"invalid args.\n");
		 		exit(1);
			}
               break;
               }
	}

	main_optind = optind;
	argv += optind;
	argc -= optind;

	optind = 1;
	optarg =  NULL;

	//
	// Now run the posix extended options (-x) through getopt.
	//
	while (1) {
		int e_option_index = 0;
		c = ugetopt_long (e_argc, cplob_get_list(e_arglist), 
				"", posix_extended_long_options, 
							&e_option_index);
		if (c == -1) break;
	
		SWP_E_DEBUG2("Looking for extended option c=%d", c);
		switch (c) {
		
		case 200:
			SWP_E_DEBUG2("in option case %d", c);
			SWP_E_DEBUG2("optarg is [%s]", optarg);
			eopt_distribution_target_directory = strdup(optarg);
			::set_opta(opta, SW_E_distribution_target_directory,
					eopt_distribution_target_directory);
			SWLIB_ALLOC_ASSERT(eopt_distribution_target_directory 
								!= NULL);
			break;
		case 201:
			SWP_E_DEBUG2("in option case %d", c);
			SWP_E_DEBUG2("optarg is [%s]", optarg);
			eopt_distribution_target_serial = strdup(optarg);
			::set_opta(opta, SW_E_distribution_target_serial,
					eopt_distribution_target_serial);
			SWLIB_ALLOC_ASSERT(eopt_distribution_target_serial
								!= NULL);
			break;
		case 205:
			SWP_E_DEBUG2("in option case %d", c);
			eopt_verbose = strdup(optarg);
			verboseG = swlib_atoi(eopt_verbose, NULL);
			swlib_set_verbose_level(verboseG);
			::set_opta(opta, SW_E_verbose, eopt_verbose);
			break;
		case 199:
		case 204:
			SWP_E_DEBUG("running with getLongOptionNameFromValue (boolean)");
			optionname = getLongOptionNameFromValue(posix_extended_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta_boolean(opta, static_cast<enum eOpts>(optionEnum), optarg);
			break;
		case 202:
		case 203:
			SWP_E_DEBUG("running with getLongOptionNameFromValue");
			optionname = getLongOptionNameFromValue(posix_extended_long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, static_cast<enum eOpts>(optionEnum), optarg);
			break;
		default:
			swlib_doif_writef(verboseG, g_fail_loudly, 0, get_stderr_fd(),
				"%s: error processing extended option\n", swlib_utilname_get());
			swlib_doif_writef(verboseG, g_fail_loudly, 0, get_stderr_fd(),
				"Try `swpackage --help' for more information.\n");
		 	exit(1);
               break;
               }
	}

	optind = 1;
	optarg =  NULL;
	//
	// Now run the Implementation extension options (-W) through getopt.
	//


	w_option_index = 1;
	SWP_E_DEBUG("");
	SWP_E_DEBUG2("w_argc=%d", w_argc);
	SWP_E_DEBUG2("w_option_index=%d", w_option_index);
	while (1) {
		// SWP_E_DEBUG2("by pointer: arg=[%s]", *(cplob_get_list(w_arglist)+w_option_index));
		// SWP_E_DEBUG2("w_arglist option=[%s]", cplob_val(w_arglist, w_option_index));

		c = ugetopt_long(w_argc, cplob_get_list(w_arglist), "", 
					long_options, &w_option_index);

		SWP_E_DEBUG2("c=%d", c);
		SWP_E_DEBUG("");
		if (c == -1) break;

	SWP_E_DEBUG("before label");
gotoExtensionOptions:    // Goto, Uhhg...
	SWP_E_DEBUG("after label");
	
		SWP_E_DEBUG2("Looking for option c=%d", c);
		switch (c) {
		
		case 130:
			SWP_E_DEBUG("");
			wopt_cksum = CHARTRUE;
			::set_opta(opta, SW_E_swbis_cksum, CHARTRUE);
			break;
		case 129:
			SWP_E_DEBUG("");
			wopt_file_digests = CHARTRUE;
			::set_opta(opta, SW_E_swbis_file_digests, CHARTRUE);
			break;
		case 131:
			SWP_E_DEBUG("");
			wopt_file_digests = CHARTRUE;
			::set_opta(opta, SW_E_swbis_file_digests, CHARTRUE);
			break;
		case 132:
			SWP_E_DEBUG("");
			fprintf(stderr, "%s: --list-psf option not supported at this time, exiting\n", swlib_utilname_get()); exit(1);
			wopt_do_list = CHARTRUE;
			break;
		case 133:
			SWP_E_DEBUG("");
			wopt_source_filename = strdup(optarg);
			SWLIB_ALLOC_ASSERT(wopt_source_filename != NULL);
			break;
		case 135:
			SWP_E_DEBUG("");
			g_wopt_numeric_owner = CHARTRUE;
			::set_opta(opta, SW_E_swbis_numeric_owner, CHARTRUE);
			break;
		case 136:
			SWP_E_DEBUG("");
			::set_opta(opta, SW_E_swbis_gzip, CHARTRUE);
			wopt_gzip = CHARTRUE;
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gzip");
			shcmd_add_arg(compressor, "-n");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-9");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 137:
			SWP_E_DEBUG("");
			::set_opta(opta, SW_E_swbis_bzip2, CHARTRUE);
			wopt_bzip2 = CHARTRUE;
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "bzip2");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 181:
			SWP_E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "lzma");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 182: /* symmetric */
			SWP_E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gpg");
			shcmd_add_arg(compressor, "--use-agent");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-o");
			shcmd_add_arg(compressor, "-");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 183: /* encrypt for recipient */
			SWP_E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "gpg");
			shcmd_add_arg(compressor, "--use-agent");
			shcmd_add_arg(compressor, "-e");
			if (strlen(optarg)) {
				shcmd_add_arg(compressor, "-r");
				shcmd_add_arg(compressor, optarg);
			}
			shcmd_add_arg(compressor, "-o");
			shcmd_add_arg(compressor, "-");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 186:
			SWP_E_DEBUG("");
			compressor = shcmd_open();
			shcmd_add_arg(compressor, "xz");
			shcmd_add_arg(compressor, "-c");
			shcmd_add_arg(compressor, "-1");
			shcmd_set_exec_function(compressor, "execvp");
			if (!compression_layers) compression_layers = vplob_open();
			vplob_add(compression_layers, compressor);
		 	break;
		case 187:
			/* slackware package name */	
			slack_name = strdup(optarg);
			break;
		case 196:
		case 188:
			/* add-signnature-first */
			sig_add_num = 1;
			wopt_mode_do_resign = CHARTRUE;
			break;

		case 189:
			/* add-signnature-last */
			sig_add_num = 0;
			wopt_mode_do_resign = CHARTRUE;
			break;

		case 193:
		case 197:
			/* --remove-signature=N */
			/* fall thru */
		case 190:
			/* --replace-signature=N */
			if (
				c == 190 || c == 193
			) {
				sig_num = swlib_atoi(optarg, NULL);
				if (sig_num < 0) {
					/* error */
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
						"invalid arg to option: %s=%d\n",
							getLongOptionNameFromValue(long_options, c), sig_num);
					exit(1);
				}
			}
			switch (c) {
				case 197:
					sig_remove_num = 1;
					break;
				case 190:
					sig_replace_num = sig_num;
					break;
				case 193:
					sig_remove_num = sig_num;
					break;
			}
			wopt_mode_do_resign = CHARTRUE;
			break;
		case 198:
			paxhdr_pid = swlib_atoi(optarg, &conversion_result);
			if (paxhdr_pid < 0 || conversion_result) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"invalid arg to option: %s=%d\n",
						getLongOptionNameFromValue(long_options, c), sig_num);
				exit(1);
			}
			break;
		case 192:
			/* resign-test */
			wopt_mode_do_resign_test = CHARTRUE;
			wopt_mode_do_resign = CHARTRUE;
			break;
		case 191:
			/* --resign */
			/* also same as    --replace_signature=0 */
			sig_replace_num = 0; /* Last Sig is Zero*/
			wopt_mode_do_resign = CHARTRUE;
			break;
		case 194:
			/* --recompress */
			wopt_mode_do_recompress = CHARTRUE;
			break;
		case 195:
			/* --overwrite */
			wopt_mode_do_overwrite = CHARTRUE;
			break;

		case 138:
			wopt_archive_digests = CHARTRUE;
			wopt_sign = CHARTRUE;
			wopt_dummy_sign = CHARFALSE;
			g_wopt_dummy_sign = CHARFALSE;
			::set_opta(opta, SW_E_swbis_archive_digests, CHARTRUE);
			::set_opta(opta, SW_E_swbis_sign, CHARTRUE);
		 	break;
		case 139:
			wopt_dir = strdup(optarg);
			SWLIB_ALLOC_ASSERT(wopt_dir != NULL);
			break;
		case 140:
			wopt_do_debug = strdup(optarg);
			break;
		case 141:
			wopt_catalog_only = CHARTRUE;
			break;
		case 142:
			wopt_storage_only = CHARTRUE;
			break;
		case 143:
			wopt_create_time = CHARTRUE;
			sscanf(optarg, "%lu", &create_time);
			break;
		case 155:
		case 144:
			wopt_gpg_name = strdup(optarg);
			::set_opta(opta, SW_E_swbis_gpg_name, wopt_gpg_name);
			break;
		case 156:
		case 145:
			wopt_gpg_path = strdup(optarg);
			::set_opta(opta, SW_E_swbis_gpg_path, wopt_gpg_path);
			break;
		case 146:
			wopt_write_signed_file = CHARTRUE;
			break;
		case 147:
			SWP_E_DEBUG2("IN option --files c=%d",c);
			wopt_files = CHARTRUE;
			::set_opta(opta, SW_E_swbis_files, CHARTRUE);
			break;
		case 148:
			wopt_archive_digests = CHARTRUE;
			::set_opta(opta, SW_E_swbis_archive_digests, CHARTRUE);
			break;
		case 149:
			wopt_absolute_names = CHARTRUE;
			::set_opta(opta, SW_E_swbis_absolute_names, CHARTRUE);
			break;
		case 150:
		case 151:
			SWP_E_DEBUG("");
			if (
				strcmp(argv[u_optind-1], "--undebian") == 0 ||
				strstr(argv[u_optind-1], "--und")
			)
			{
				//
				// Add the --exclude-system-dir option
				//
				unrpm_exclude_system_dirs = 1;
			}
			wopt_mode_do_unrpm = CHARTRUE;
			wopt_source_filename = strdup("-");
			psffilename = strdup("@PSF");
			::set_opta(opta, SW_E_psf_source_file, psffilename);
			break;
		case 152:
			wopt_no_sha1 = CHARTRUE;
			sha1_buffer = (char*)NULL;
			break;
		case 153:
			// same as  -x follow_symlinks=true
			optionname = getLongOptionNameFromValue(posix_extended_long_options, 199);
			SWLIB_ASSERT(optionname != NULL);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta_boolean(opta, static_cast<enum eOpts>(optionEnum), CHARTRUE);
			break;
		case 154:
			break;
		case 184:
			optarg = strdup(SWP_PASS_AGENT);
			goto LABELcase_157; // fall through to case 157
		case 185:
			regfiles_only = 1;
			break;
		case 157:
LABELcase_157:
			wopt_passfile = (char*)NULL;
			wopt_passphrase_fd = strdup(optarg);
			if (strcmp(wopt_passphrase_fd, SWP_PASS_AGENT) == 0) {
				g_passfd = -1;
			} else if (strcmp(wopt_passphrase_fd, SWP_PASS_ENV) == 0) {
				g_passphrase = getenv("SWPACKAGEPASSPHRASE");
				if (!g_passphrase)
					g_passphrase = strdup("");
			} else if (strcmp(wopt_passphrase_fd, SWP_PASS_TTY) == 0) {
				g_passfd = -1;
				g_passphrase = (char*)NULL;
				wopt_passphrase_fd = "-1";
				wopt_passfile = (char*)NULL;
			} else {
				SWP_E_DEBUG("");
				g_passfd = swlib_atoi(wopt_passphrase_fd, &tmpret);
				if (tmpret) g_passfd = -1;
				if (g_passfd == STDIN_FILENO) {
					int newpassfd;
					/*
					* transfer the stdin file to another
					* descriptor. Not only might this be
					* safer but there is some "bustedness" in
					* swpackage which requires us to do this.
					*/
					g_stdin_use_count++;
					SWP_E_DEBUG("");
					if ((newpassfd = fix_passfd(STDIN_FILENO)) < 0) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
							"invalid passphrase fd\n");
						exit(1);
					}
					g_passfd = newpassfd;
					strob_sprintf(tmp, 0, "%d", g_passfd);
					wopt_passphrase_fd = strdup(strob_str(tmp));
					SWP_E_DEBUG("");
				} else if (g_passfd <= 2) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
						"invalid fd\n");
					exit(1);
				} else {
					;
				}
			}
			break;
		case 158:
			wopt_show_options = CHARTRUE;
			break;
		case 159:
			wopt_show_options_files = CHARTRUE;
			break;
		case 160:
			wopt_uuid = strdup(optarg);
			break;
		case 161:
			wopt_nosign = CHARTRUE;
			wopt_archive_digests = CHARFALSE;
			wopt_sign = CHARFALSE;
			::set_opta(opta, SW_E_swbis_archive_digests, (char*)NULL);
			::set_opta(opta, SW_E_swbis_sign, (char*)NULL);
			break;
		case 162:
			wopt_passfile = strdup(optarg);
			if (strcmp(optarg, "-") == 0) g_stdin_use_count++;
			if (strcmp(wopt_passfile, "/dev/tty") == 0) {
				force_tty_passfd = 1;
				//
				// Here, try to unset the stdin use count
				//
				if (
					env_passphrase_fd &&
					strcmp(env_passphrase_fd, SWP_PASS_AGENT) &&
					strcmp(env_passphrase_fd, SWP_PASS_ENV)
				) {
					if (swlib_atoi(env_passphrase_fd, NULL) == 0) {
						g_stdin_use_count--;
						dup2(g_passfd, 0);
					}
					close(g_passfd);
				}
				g_passfd = -1;
				g_passphrase = (char*)NULL;
				wopt_passphrase_fd = "-1";
				wopt_passfile = (char*)NULL;
			} 
			wopt_passphrase_fd = "-1";
			g_passfd = -1;
			break;
		case 163:
			wopt_signer_bin = strdup(optarg);
			::set_opta(opta, SW_E_swbis_signer_pgm, wopt_signer_bin);
			break;
		case 164:
			wopt_show_signer_pgm = CHARTRUE;
			break;
		case 165:
			wopt_catalog_owner = strdup(optarg);
			wopt_catalog_owner = check_name_to_convert(wopt_catalog_owner, taru_get_tar_user_by_uid, &id_status);
			break;
		case 166:
			wopt_catalog_group = strdup(optarg);
			wopt_catalog_group = check_name_to_convert(wopt_catalog_group, taru_get_tar_group_by_gid, &id_status);
			break;
		case 167:
			SWP_E_DEBUG2("IN option c=%d",c);
			wopt_dir_group = strdup(optarg);
			wopt_dir_group = check_name_to_convert(wopt_dir_group, taru_get_tar_group_by_gid, &id_status);
			break;
		case 168:
			SWP_E_DEBUG2("IN option c=%d",c);
			wopt_dir_owner = strdup(optarg);
			wopt_dir_owner = check_name_to_convert(wopt_dir_owner, taru_get_tar_user_by_uid, &id_status);
			break;
		case 169:
			wopt_dir_mode = strdup(optarg);
			break;
		case 'b':
			blocksize = swlib_atoi(optarg, NULL);
			if (blocksize < 1) exit(1);
			break;
		case 170:
			wopt_files_from = strdup(optarg);
			if (strcmp(optarg, "-") == 0) g_stdin_use_count++;
			break;
		case 171:
			wopt_no_front_dir = 1;
			break;
		case 172:
			wopt_no_catalog = 1;
			break;
		case 173:
			wopt_no_defaults = 1;
			break;
		case 174:
			wopt_file_digests2 = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_file_digests_sha2, CHARTRUE);
			break;
		case 175:
			wopt_archive_digests2 = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_archive_digests_sha2, CHARTRUE);
			break;
		case 176: /* --sha2 */
			wopt_file_digests2 = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_file_digests_sha2, CHARTRUE);
			wopt_archive_digests2 = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_archive_digests_sha2, CHARTRUE);
			break;
		case 177: /* --sha1 */
			wopt_file_digests = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_file_digests, CHARTRUE);
			wopt_archive_digests = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_archive_digests, CHARTRUE);
			break;
		case 178: /* 178 */
			wopt_archive_digests = CHARTRUE;
			wopt_sign = CHARTRUE;
			wopt_dummy_sign = CHARTRUE;
			g_wopt_dummy_sign = CHARTRUE;
			::set_opta_boolean(opta, SW_E_swbis_archive_digests, CHARTRUE);
			::set_opta_boolean(opta, SW_E_swbis_sign, CHARTRUE);
		 	break;
		case 179:
			wopt_checkdigestname=strdup(optarg);
			break;
		case 180:
			::swc_set_boolean_x_option(opta,
				SW_E_swbis_check_duplicates,
				optarg, 
				&wopt_check_duplicates);
			break;
		case 202:
		case 203:
		case 206:
		case 213:
		case 214:
		case 220:
		case 221:
			SWP_E_DEBUG("running with getLongOptionNameFromValue");
			optionname = getLongOptionNameFromValue(long_options , c);
			SWP_E_DEBUG2("option name from getLongOptionNameFromValue is [%s]", optionname);
			SWLIB_ASSERT(optionname != NULL);
			SWP_E_DEBUG2("running getEnumFromName with name [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta(opta, static_cast<enum eOpts>(optionEnum), optarg);
			break;
		case 204:
		case 207:
		case 208:
		case 209:
		case 210:
		case 211:
		case 212:
		case 216:
		case 217:
		case 218:
		case 219:
			optionname = getLongOptionNameFromValue(long_options , c);
			SWLIB_ASSERT(optionname != NULL);
			SWP_E_DEBUG2("option name is [%s]", optionname);
			optionEnum = getEnumFromName(optionname, opta);
			SWLIB_ASSERT(optionEnum > 0);
			set_opta_boolean(opta, static_cast<enum eOpts>(optionEnum), optarg);
			break;

		case 222:
			construct_missing = 1;
			break;

		case 223:
			no_header_overflow = 1;
			break;
		case 230:
			unrpm_exclude_system_dirs = 1;
			break;

		default:
			SWP_E_DEBUG("default case is an error");
			SWP_E_DEBUG2("w_option_index = %d", w_option_index);
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"error processing implementation extension option : %s\n",
				cplob_get_list(w_arglist)[w_option_index]);
		 	exit(1);
		break;
		}
		if (do_extension_options_via_goto == 1) {
			do_extension_options_via_goto = 0;
			goto gotoStandardOptions;
		}
	}
	verboseG = swlib_atoi(eopt_verbose, NULL);
	
	swlib_set_pax_header_pid((pid_t)paxhdr_pid);
	
	optind = main_optind;
	
	if (wopt_mode_do_unrpm) {
		psffilename = strdup("@PSF");
		::set_opta(opta, SW_E_psf_source_file, psffilename);
		wopt_file_digests = CHARTRUE;
		::set_opta_boolean(opta, SW_E_swbis_file_digests, CHARTRUE);
		SWP_E_DEBUG2("g_stdin_use_count=%d", g_stdin_use_count);
		if (g_stdin_use_count > 1) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error: multiple use of stdin\n");
			_exit(1);
		}
		
		unrpmfd = do_unrpm(wopt_do_list,
			wopt_create_time, create_time,
			pkgfilename, wopt_checkdigestname,
			wopt_catalog_owner, wopt_catalog_group, slack_name, construct_missing, unrpm_exclude_system_dirs); 
		SWP_E_DEBUG("");
		if (unrpmfd < 0) {
			_exit(1);	
		}

		//
		// A child will traverse this point.
		// The parent never gets here.
		//

		//
		// the child must now have its -Wsource set to <unrpmfd>, by doing
		// this we have free'd up stdin.
		//


		//
		// restrict usage of tty passphrase entry when translating
		// other formats because a child swpackage is asking and
		// this seemed not to work.  Hence don't even try
		//

		if (
			(is_option_true(wopt_sign) && !is_option_true(wopt_dummy_sign)) && 
			g_passfd == -1 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_AGENT) != 0 &&
			strcmp(wopt_passphrase_fd, SWP_PASS_ENV) != 0 
		)
		{
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error: reading a passphrase from the terminal is not supported\n");
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error: in this mode. Use GnuPG gpg-agent, see option --use-agent\n");
			_exit(1);
		}

		strob_sprintf(tmp, 0, "%d", unrpmfd);
		wopt_source_filename = strdup(strob_str(tmp));
	}
	

	do_files_pipe[0] = -1;
	if (wopt_files_from) {
		pipe(do_files_pipe);
		do_files_from_pid = swfork((sigset_t*)(NULL));
		if (do_files_from_pid == 0) {
			close_passfd();
			close(do_files_pipe[0]);
			ret = do_files_from(do_files_pipe[1], cwd, 
						wopt_files_from, 
						wopt_catalog_owner, 
						wopt_catalog_group);
			close(do_files_pipe[1]);
			_exit(ret);
		} else if (do_files_from_pid < 0) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"fork error : %s \n", strerror(errno));
			exit(1);
		}
		close(do_files_pipe[1]);
	}	

	SWP_E_DEBUG("");
	if (taru_get_tar_user_by_uid(sysa_uid, sysa_owner) != 0) {
		//
		// User not in database, make archive with numeric id's
		//
		SWP_E_DEBUG("");
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"swpackage: Warning: user name for uid = %d not found\n",
				 (int)sysa_uid);
	}

	SWP_E_DEBUG("");
	if (taru_get_tar_group_by_gid(sysa_gid, sysa_group) != 0) {
		//
		// User not in database, make archive with numeric id's
		//
		SWP_E_DEBUG("");
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"swpackage: Warning: group name for gid = %d not found\n",
				(int)sysa_gid);
	}

	//
	// Parse the options files.
	//
	system_defaults_files = initialize_options_files_list(NULL);
	if (wopt_show_options_files) { 
		::swextopt_parse_options_files(opta, 
					system_defaults_files, 
					"swpackage", 
					1, /* not req'd */  
					1 /* show_only */
					);
		::swextopt_parse_options_files(opta, 
					opt_option_files, 
					"swpackage",
					1, /* not req'd */  
					1 /* show_only */
					);
		exit(0);
	}

	//
	// Parse the options files.
	//
	if (wopt_no_defaults == 0) {
		optret += ::swextopt_parse_options_files(opta, 
				system_defaults_files, 
				"swpackage", 
				0, /* not req'd */
				0 /* not show_only */);
		optret += ::swextopt_parse_options_files(opta, 
				opt_option_files, 
				"swpackage",
				1, /* req'd */
				0 /* not show_only */
				);
	} else {
		optret = 0;
	}	
	if (optret) {
		exit(1);
	}

	//
	// Apply the Extension options from the options file.
	//
	wopt_cksum 	  = ::swbisoption_get_opta(opta, SW_E_swbis_cksum);
	wopt_file_digests = ::swbisoption_get_opta(opta, SW_E_swbis_file_digests);
	wopt_file_digests2 = ::swbisoption_get_opta(opta, SW_E_swbis_file_digests_sha2);
	wopt_files 	  = ::swbisoption_get_opta(opta, SW_E_swbis_files);
	wopt_sign 	  = ::swbisoption_get_opta(opta, SW_E_swbis_sign);
	wopt_check_duplicates = ::swbisoption_get_opta(opta, SW_E_swbis_check_duplicates);
	wopt_archive_digests = ::swbisoption_get_opta(opta, SW_E_swbis_archive_digests);
	wopt_archive_digests2 = ::swbisoption_get_opta(opta, SW_E_swbis_archive_digests_sha2);

	if (is_option_true(wopt_sign)) {
		wopt_archive_digests = CHARTRUE;
		wopt_sign = CHARTRUE;
	}
	wopt_gpg_name 		= ::swbisoption_get_opta(opta, SW_E_swbis_gpg_name);
	if (wopt_gpg_name && strlen(wopt_gpg_name) == 0) wopt_gpg_name = 0;

	wopt_gpg_path 		= ::swbisoption_get_opta(opta, SW_E_swbis_gpg_path);
	wopt_gzip 		= ::swbisoption_get_opta(opta, SW_E_swbis_gzip);
	wopt_bzip2 		= ::swbisoption_get_opta(opta, SW_E_swbis_bzip2);
	g_wopt_numeric_owner 	= ::swbisoption_get_opta(opta, SW_E_swbis_numeric_owner);
	wopt_absolute_names 	= ::swbisoption_get_opta(opta, SW_E_swbis_absolute_names);
	wopt_format 		= ::swbisoption_get_opta(opta, SW_E_swbis_format);
	wopt_signer_bin		= ::swbisoption_get_opta(opta, SW_E_swbis_signer_pgm);

	swc0_set_arf_format(wopt_format, 
			&wopt_format_arf, 
			&do_oldgnutar, 
			&do_bsdpax3,
			&do_oldgnuposix,
			&do_gnutar, &do_paxexthdr);
	psffilename = ::get_opta(opta, SW_E_psf_source_file);

	if (wopt_nosign) {
		wopt_sign = CHARFALSE;
	}

	//
	// Apply the Posix Extended options from the options file.
	//
	eopt_distribution_target_directory = ::get_opta(opta, 
					SW_E_distribution_target_directory);
	eopt_distribution_target_serial = ::get_opta(opta, 
					SW_E_distribution_target_serial);
	eopt_media_type 	= ::get_opta(opta, SW_E_media_type);
	eopt_media_capacity 	= ::get_opta(opta, SW_E_media_capacity);
	eopt_enforce_dsa 	= ::get_opta(opta, SW_E_enforce_dsa);
	eopt_verbose 		= ::get_opta(opta, SW_E_verbose);
	eopt_follow_symlinks	= ::get_opta(opta, SW_E_follow_symlinks);

	if (id_status) {
		fprintf(stderr, "swpackage: bad uid to username conversion\n");
		exit(1);
	}

	if (swextopt_get_status()) {
		fprintf(stderr, "swpackage: bad value detected for extended option, probably a boolean option\n");
		fprintf(stderr, "swpackage: has something other than ''true'' or ''false''\n");
		exit(1);
	}
	
	if (wopt_show_options) { 
		swextopt_writeExtendedOptions(STDOUT_FILENO, opta , SWC_U_P);
		exit(0);
	}
	
	if (wopt_show_signer_pgm) {
		signer_command = get_package_signature_command(
					wopt_signer_bin, 
					wopt_gpg_name, 
					wopt_gpg_path,
					wopt_passphrase_fd
					);
		if (signer_command == (SHCMD*)NULL) {
			FATAL2("invalid signer command specification");
			exit(1);
		} else {
			shcmd_debug_show_command(signer_command, 
			STDOUT_FILENO);
			exit(0);
		}
	}
	
	if (is_option_true(wopt_archive_digests)) {
		sha1_buffer = sha1_buffer_o;
		md5sum_buffer = md5sum_buffer_o;
		adjunct_md5sum_buffer = adjunct_md5sum_buffer_o;
	} else {
		sha1_buffer = (char*)NULL;
		md5sum_buffer = (char*)NULL;
		adjunct_md5sum_buffer = (char*)NULL;
	}

	if (is_option_true(wopt_archive_digests2)) {
		sha512_buffer = sha512_buffer_o;
	} else {
		sha512_buffer = (char*)NULL;
	}

	//
	// Parse the sofware_selections and targets.
	//

	c = argc;
	cl_target = NULL;
	cl_selections = NULL;
	while ((c--) > 0) {
		SWP_E_DEBUG2("c=[%d]", c);
		arg = *(argv++);
		SWP_E_DEBUG2("arg=[%s]", arg);
		//cout << arg << endl; 

		if (cl_target == NULL && arg[0] != '@') {
			SWP_E_DEBUG("B1");
			cl_selections = arg;
			if ((p = strrchr(cl_selections, (int) '@')) != NULL) {
				*p = '\0';
				cl_target = p + 1;
				break;
			}
		} else if (cl_target == NULL && arg[0] == '@') {
			SWP_E_DEBUG("B2");
			cl_target = arg + 1;
			if (strlen(cl_target))
				break;
		} else if (cl_target && !strlen(cl_target)) {
			SWP_E_DEBUG("B3");
			cl_target = arg;
			break;
		} else if (cl_target && strlen(cl_target)) {
			// Never happens
			FATAL2("internal error parsing target");
		} else {
			;
			// continue
		}
	}


	//
	// Apply some implementation defined restrictions.
	//
	if (c >= 1) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"multiple targets invalid for swpackage\n");
		exit(1);
	}

	if (cl_target && strlen(cl_target) == 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"invalid target\n");
		exit(1);
	}
	if (cl_selections) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"software selection feature not implemented.\n");
		exit(1);
	}

	if (eopt_media_type && strcmp(eopt_media_type, "serial") != 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"this implementation currently only supports serial media type.\n");
		exit(1);
	}

	if (cl_target == NULL) {
		cl_target = eopt_distribution_target_serial;
	}
	
	if (cl_target && strcmp(cl_target, ".") == 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"target not supported: .\n");
		exit(1);
	}

	if (cl_target && *cl_target != ':' && *cl_target != '/' && *cl_target != '-') {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"warning: continuing with assumption that name \"%s\" is a file name\n", cl_target);
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"warning: in the future, prefix with ':' to avoid this warning\n");
	}

	if (cl_target && *cl_target == ':') {
		cl_target = cl_target + 1;
	}

	SWP_E_DEBUG2("cl_target is [%s]", cl_target);

	//
	// Open a swDefinitionFile object.
	//
	psf = new swPSF("");
	g_psf = psf;
	SWLIB_ALLOC_ASSERT(psf != NULL);

	psf->set_cksum_creation(IST(wopt_cksum) ? 1:0);
	psf->set_digests1_creation(IST(wopt_file_digests) ? 1:0);
	psf->set_digests2_creation(IST(wopt_file_digests2) ? 1:0);

	if (is_option_true(g_wopt_numeric_owner)) {
		psf->xFormat_set_numeric_uids(1);
	}

	if (is_option_true(eopt_follow_symlinks)) {
		psf->set_follow_symlinks(1);
	}

	/*
	 * Set the tarheader personality.
	 */
	if (taru_set_tar_header_policy(psf->xFormat_get_xformat()->taruM,
		wopt_format, (int*)(NULL)))
			exit(1);

	tarheader_flags = psf->xFormat_get_tarheader_flags();


	//
	// Subtract GNU LongLinks and Pax Extended Headers
	// from the flags.  The catalog section never should have
	// these.

	catalog_tarheader_flags = tarheader_flags;
	catalog_tarheader_flags &= ~TARU_TAR_GNU_LONG_LINKS;
	catalog_tarheader_flags &= ~TARU_TAR_PAXEXTHDR;
	catalog_tarheader_flags &= ~TARU_TAR_PAXEXTHDR_MD;

	if (no_header_overflow) {
		taru_set_tarheader_flag(psf->xFormat_get_xformat()->taruM, TARU_TAR_NO_OVERFLOW, 1);
	}

	//
	// Open the PSF.
	//
	if (do_files_pipe[0] >= 0) {
		SWP_E_DEBUG("");
		psf_source_ifd = do_files_pipe[0];
	} else if (strcmp(psffilename, "-") == 0) {
		//
		// Stdin
		//
		SWP_E_DEBUG("");
		if (!wopt_files_from)
			g_stdin_use_count++;
		SWP_E_DEBUG("");
		psf_source_ifd = STDIN_FILENO;
	} else {
		SWP_E_DEBUG("");
		g_stdin_use_count--;
		if (*psffilename != '@') {
			//
			// Normal case, the PSF is a regular file, open it
			//
			SWP_E_DEBUG2("open file: %s", psffilename);
			psf_source_ifd = open(psffilename, O_RDONLY, 0);
			if (psf_source_ifd < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error opening psf file: %s\n", psffilename);
				exit(1);
			}
		} else {
			//
			// Open it later.
			//
			SWP_E_DEBUG("psf_source_ifd will be opened later");
			psf_source_ifd = -1;
		}
	}

	SWP_E_DEBUG2("wopt_source_filename is [%s]", wopt_source_filename);
	if (wopt_source_filename && wopt_mode_do_resign == NULL) {
		SWP_E_DEBUG("wopt_source_filename is true");
		//
		// Source the package from a archive.
		//
		if (strcmp(wopt_source_filename, "-") == 0) {
			SWP_E_DEBUG("wopt_source_filename == -");
			if (STDIN_FILENO == psf_source_ifd) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"invalid usage.\n");
				exit(1);
			}
			ret = psf->open_serial_archive(STDIN_FILENO, 
					UINFILE_DETECT_FORCEUXFIOFD |
					UINFILE_DETECT_NATIVE |
					UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM);
			if (ret) {
				SWBIS_ERROR_IMPL();
				exit(1);
			}
		} else if (isdigit(*wopt_source_filename)) {
			SWP_E_DEBUG("is digit is true");
			ret = atoi(wopt_source_filename);
			SWP_E_DEBUG2("archive fd is %d", ret);
			ret = psf->open_serial_archive(ret,
					UINFILE_DETECT_FORCEUXFIOFD |
					UINFILE_DETECT_NATIVE |
					UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM);
			if (ret) {
				SWBIS_ERROR_IMPL();
				exit(1);
			}
		} else {
			SWP_E_DEBUG("");
			ret = psf->open_serial_archive(
					wopt_source_filename,
					UINFILE_DETECT_NATIVE);
			if (ret) {
				SWBIS_ERROR_IMPL();
				exit(1);
			}
		}
	} else if (wopt_source_filename && wopt_mode_do_resign) {
		/* Invalid Usage */
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"swpackage: invalid options given: conflicting modes.\n");
		exit(1);
	} else {
		//
		// Normal usage.
		//
		if (psf->open_directory_archive(".")) {
			exit(1);
		}
	}

	//
	// Make sure stdin is not being overused.
	//
	if (g_stdin_use_count > 1) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"swpackage: too many uses of standard input\n");
		exit(1);
	}

	//
	// NASTY, there seem to be two (2) HLLIST objects
	// fprintf(stderr, "JL psf xFormat hllist=%p\n", psf->xFormat_get_xformat()->link_recordM);
	// fprintf(stderr, "JL psf swvarfs hllist=%p\n", psf->xFormat_get_xformat()->swvarfsM->link_recordM);
	//

	//
	// These lines are required.
	//
	swvarfs = static_cast<SWVARFS*>(psf->xFormat_get_swvarfs());
	SWLIB_ALLOC_ASSERT(swvarfs != NULL);
	psf->get_swextdef()->set_swvarfs(swvarfs);

	//
	// Open the PSF file if not already open
	//
	SWP_E_DEBUG("");
	if (psf_source_ifd < 0) {	
		SWP_E_DEBUG("");
		if (*psffilename == '@') {
			//
			// Implementation Extension.
			// Source the PSF from the archive.
			//
			if (wopt_source_filename == NULL) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"invalid usage.\n");
				exit(1);
			}
			psffilename++;
			SWP_E_DEBUG("");
			psf_source_ifd = swvarfs_u_open(swvarfs, psffilename);
			if (psf_source_ifd < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"error opening psf file: @%s\n", psffilename);
				exit(1);
			}
		} else {
			SWP_E_DEBUG("");
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"internal invalid usage.\n");
			exit(1);
		}
	}

	//
	// Handle Package Re-signing
	//
	// --------------------------------------------------------------------****
	//
	if (wopt_mode_do_resign) {
		int	xx_fd;
		int 	xx_ofd;
		int 	xx_ifd = psf_source_ifd;
		int 	xx_ret;
		int 	xx_write_ret;
		int 	xx_ext_ret;
		int 	xx_ret2;
		int 	xx_flags;
		int 	xx_sig_block_start;
		int 	xx_sig_block_end;
		int 	xx_signed_bytes_fd;
		int 	xx_sig_block_fd;
		int 	xx_new_sig_block_fd;
		int 	xx_sig_header_fd;
		int 	xx_l_status = 1;
		int	xx_len;
		int	xx_sig_length;
		int	xx_sigheader_fd = -1;
		int	xx_num_existing_sigs;
		char	*xx_sig;
		char	*xx_name;
		char 	*xx_signature_tarheader;
		char 	*xx_sig_block_buffer;
		int	xx_sig_block_len;
		SHCMD ** xx_recompress_commands;
		SHCMD ** xx_cmdline_compression_commands;
		XFORMAT *xx_xformat;
		SWLOG 	*xx_swlog;
		SWI 	*xx_swi;
		SHCMD 	*xx_signer_command;
		SWPATH * xx_swpath = swpath_open("");
		STROB *  xx_cl_target_orig;
		struct new_cpio_header * xx_file_hdr = taru_make_header();
		struct stat xx_st;
		struct stat xx_target_st;
		struct stat xx_source_st;
		struct sw_logspec xx_logspec;

		swgp_signal(SIGINT, resign_sig_handler);
		swgp_signal(SIGTERM, resign_sig_handler);
		swgp_signal_block(SIGALRM, (sigset_t *)NULL);

		swc_initialize_logspec(&xx_logspec, (char*)NULL, 0);
	
		xx_cl_target_orig = strob_open(32);
	
		SWP_E_DEBUG("RESIGNING");
		if (
			(sig_replace_num >= 0 && sig_add_num >=0) ||
			((sig_replace_num >= 0 || sig_add_num >=0) && sig_remove_num >= 0)
		) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"signature modification request invalid: conflicting requests\n");
			exit(1);
		}

		xx_ofd = -1;
		if (strlen(cl_target) == 0 || strcmp(cl_target, "-") == 0 || strcmp(cl_target, ".") == 0) {
			//
			// Output not specified or "-" or "."
			//
			// This supports the following special command line invocation.
			//      swpackage -s foo.tar --overwrite --resign
			//
			if (wopt_mode_do_overwrite) {
				//
				// Overwrite is specified and no target name given, assume the
				// target name is the same as the source name
				//
				// Check to the psffilename is normal name, i.e. not "-"
				//

				if (	psffilename == NULL ||
					strlen(psffilename) == 0 ||
					strcmp(psffilename, "-") == 0 ||
					psf_source_ifd == STDIN_FILENO
				) {
					//
					// Error, invalid use of --overwrite, there is no target name known
					// to the program.
					//
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
						"invalid options, you must supply a source name\n");
					exit(1);
				}
				
				//
				// OK, now dup the source fd onto a new xx_ofd
				// and proceed.  The code below will detect they are
				// the same file and take the normal actions for this case.
				//
				xx_ofd = dup(psf_source_ifd);
				if (xx_ofd < 0) {
					exit(11);
				}
				cl_target = strdup(psffilename);
			}
		} 
		if (xx_ofd < 0) {
			//
			// cl_target is a name, open it
			//
			xx_ofd = set_stdio_output_fd((swExCat *)NULL, cl_target, O_RDWR|O_CREAT /*|O_EXCL */);
			if (xx_ofd < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
					"error opening source archive file: %s\n", cl_target);
				exit(1);
			}
		}

		//
		// Check if the source and target are the same file
		//

		if (
			fstat(xx_ofd, &xx_target_st) == 0 &&
			fstat(psf_source_ifd, &xx_source_st) == 0
		) {
			if (
				xx_target_st.st_dev == xx_source_st.st_dev &&
				xx_target_st.st_ino == xx_source_st.st_ino &&
				S_ISREG(xx_target_st.st_mode)
			) {
				//
				// same file
				//
				SWP_E_DEBUG("");
				if (wopt_mode_do_overwrite == NULL) {
					SWP_E_DEBUG("");
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
						"cowardly refusing to overwrite package file with itself: %s\n", cl_target);
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
						"try option --overwrite if using --resign\n");
					exit(1);
				} else {
					SWP_E_DEBUG("");
					strob_sprintf(xx_cl_target_orig,  0, "%s" RESIGN_ORIG_SUFFIX, cl_target); 
					swlib_doif_writef(verboseG, SWC_VERBOSE_2, NULL, get_stderr_fd(), 
						"original file renamed to: %s\n", strob_str(xx_cl_target_orig));

					G_backup_name = strob_str(xx_cl_target_orig);
					G_orig_name = cl_target;

					xx_ret = rename(cl_target,  strob_str(xx_cl_target_orig));
					if (xx_ret) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
							"rename failed for: %s\n", cl_target);
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
							"abandoning overwrite attempt\n");
						exit(1);
					}
					close(xx_ofd);
					xx_ofd = set_stdio_output_fd((swExCat *)NULL, cl_target, O_RDWR|O_CREAT|O_EXCL);
					if (xx_ofd < 0) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
							"error exclusively opening name: %s\n", cl_target);
						exit(1);
					}
				}
			} else {
				//
				// not the same file, need to tructate xx_ofd
				//
				SWP_E_DEBUG("");
				if (xx_ofd > STDOUT_FILENO  && uxfio_espipe(xx_ofd) == 0) {
					//
					// is a regular file, i.e. not a pipe
					//
					SWP_E_DEBUG("");
					xx_ret = ftruncate(xx_ofd, 0);
					if (xx_ret < 0) {
						swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
								"error truncating file: %s\n", strerror(errno));
						exit(1);
					}
				}
			}
		} else {
			SWP_E_DEBUG("");
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"error using fstat on one or both files: %s and %s\n", cl_target, psffilename);
			exit(1);
		}

		SWP_E_DEBUG("");
		xx_swlog = swutil_open();
		xx_flags = UINFILE_DETECT_FORCEUXFIOFD | 
			UINFILE_DETECT_IEEE | 
			UINFILE_DETECT_UNCPIO |
                        UINFILE_DETECT_UNRPM |
			UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
	
		if (wopt_mode_do_recompress) {
			xx_flags |= UINFILE_DETECT_RECOMPRESS;
		}

		xx_new_sig_block_fd = swlib_open_memfd();	
		xx_sig_block_fd = swlib_open_memfd();	
		xx_sig_header_fd = swlib_open_memfd();	

		SWP_E_DEBUG("");

		//
		// Decode the existing package, this is requireed to make sure
		// it is already signed.
		//

		xx_swi = swi_create();
		SWP_E_DEBUG("");
		SWP_E_DEBUG2("xx_ifd=%d", xx_ifd);
		xx_ret = swi_do_decode(xx_swi, xx_swlog,
			xx_ofd,		/* int target_fd1, */
			xx_ifd,		/* int source_fd0, */
			NULL,		/* char * target_path, */
			NULL,		/* char * source_path, */
			NULL, 		/* VPLOB * swspecs, */
			NULL, 		/* char * target_host, */
			NULL,		/* struct extendedOptions * opta, */
			0,		/* int is_seekable, */
			0,		/* int do_debug_events, */
			2,		/* int verboseG, */
			&xx_logspec, 		/* struct sw_logspec * g_logspec, */
			xx_flags);
	
		if (xx_ret) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"error opening source archive\n");
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		xx_ifd = xformat_get_ifd(xx_swi->xformatM);

		SWP_E_DEBUG("before exam");
		swi_examine_signature_blocks(xx_swi->swi_pkgM->dfilesM, &xx_sig_block_start, &xx_sig_block_end);
		SWP_E_DEBUG2("xx_sig_block_start=%d", xx_sig_block_start);
		SWP_E_DEBUG2("xx_sig_block_end=%d", xx_sig_block_end);
		SWP_E_DEBUG("after exam");

		//
		// enforce that the package must be signed already
		//
		if (xx_sig_block_start < 0 || xx_sig_block_end < 0) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"package is not signed\n");
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		SWP_E_DEBUG("HERE");

		//
		// write the signed bytes to a buffer fd
		//	
		xx_signed_bytes_fd = swlib_open_memfd();
		xx_ret2 = swpl_write_catalog_data(xx_swi, xx_signed_bytes_fd, xx_sig_block_start, xx_sig_block_end);

		SWP_E_DEBUG2("HERE swpl_write_catalog_data xx_ret=[%d]", xx_ret2);

		//
		// This turns off buffering past the catalog, just to save memory and to improve
		// performance.
		//
		if (xx_ifd >= UXFIO_FD_MIN)
		if (uxfio_fcntl(xx_ifd, UXFIO_F_ARM_AUTO_DISABLE, 1) < 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			FATAL2("uxfio_fcntl");
		}
		SWP_E_DEBUG("HERE");

		//
		// Seek back to the beginning of the package
		//
		if (uxfio_lseek(xx_ifd,  0, SEEK_SET) < 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			FATAL2("uxfio_lseek");
		}	

		//
		// Now grab the existing signature block
		// beginning at xx_sig_block_start and ending at xx_sig_block_end
		//

		if (uxfio_lseek(xx_ifd, xx_sig_block_start, SEEK_SET) < 0) FATAL();
		
		SWP_E_DEBUG("");
		ret = swlib_pump_amount(xx_sig_block_fd, xx_ifd, xx_sig_block_end - xx_sig_block_start);
		if (ret < 1024) {
			/* fatal */
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}
		SWP_E_DEBUG("");
		if (uxfio_lseek(xx_ifd, 0, SEEK_SET) != 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		SWP_E_DEBUG("");
		if (uxfio_lseek(xx_signed_bytes_fd, 0L, SEEK_SET) != 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		//
		// Set up the archive reading machinery.
		//

		SWP_E_DEBUG("HERE");
		xx_xformat = xx_swi->xformatM;
		if (!xx_xformat) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}
		SWP_E_DEBUG("");
		if (uxfio_lseek(xx_ifd,  0, SEEK_SET) != 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}
		xformat_set_tarheader_flag(xx_xformat, TARU_TAR_FRAGILE_FORMAT, 1 /*on*/ );
		SWP_E_DEBUG("HERE");

		//
		// Find the sig_header archive member
		//
		SWP_E_DEBUG("HERE");

		xx_sigheader_fd = -1;
		while ((xx_name = xformat_get_next_dirent(xx_xformat, &xx_st)) != NULL) {
			SWP_E_DEBUG2("File name: [%s]", xx_name);
			if (swpath_parse_path(xx_swpath, xx_name) < 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "error parsing pathname: %s\n", xx_name);
				break;
			}

			if (fnmatch("*/sig_header", xx_name, 0) == 0) {
				SWP_E_DEBUG("found sig_header");
				xx_sigheader_fd =  xformat_u_open_file(xx_xformat, xx_name);
				break;
			}
			if (swpath_get_is_catalog(xx_swpath) == SWPATH_CTYPE_STORE) {
				//
				// This should never happen for a signed package.
				//
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "error reading package\n");
				restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
				exit(1);
			}
		}

		SWP_E_DEBUG2("xx_sigheader_fd = %d", xx_sigheader_fd);
		if (xx_sigheader_fd < 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		//
		// Decode the header to determine the size of the signature
		// It should be either 512 or 1024
		//
		if (uxfio_lseek(xx_sigheader_fd,  0, SEEK_SET) < 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		// The file desc returned by xformat_u_open_file()
		// does not support having its buffer length queried.
		// So, Copy to a new file descriptor (that is type DYNAMICMEM)

		SWP_E_DEBUG("");
		swlib_pipe_pump(xx_sig_header_fd, xx_sigheader_fd);

		// Now get the address of this file data
		xx_len = -1;
		uxfio_get_dynamic_buffer(xx_sig_header_fd, &xx_signature_tarheader, NULL, &xx_len);

		SWP_E_DEBUG2("xx_len=%d", xx_len);
		if (xx_len < TARRECORDSIZE) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		//
		// Now read it into the tar header decoder buffer.
		//
		SWP_E_DEBUG("");
               	taru_read_in_tar_header2(xx_xformat->taruM, xx_file_hdr,
                                       -1, xx_signature_tarheader, NULL, 0, TARRECORDSIZE);

		//
		// Now read the size field of this tar header
		// This is the lenth of the ``.../signature'' file is the catalog section
		// of the package.
		//

		if (uxfio_lseek(xx_ifd,  0, SEEK_SET) != 0)  FATAL();
		SWP_E_DEBUG("HERE");

		//
		// Finally, we have the filesize attribute from the tar archive header
		//
		xx_sig_length = (int)taru_hdr_get_filesize(xx_file_hdr);
		SWP_E_DEBUG2("file size from file header is %d", (int)taru_hdr_get_filesize(xx_file_hdr));

		if (xx_sig_length != 512  && /* Old versions of swpackage */
			xx_sig_length != 1024) {
			/* This should be either 512 or 1024 */
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), "invalid signature size: %s\n", xx_sig_length);
		} 	
	
		SWP_E_DEBUG("");
		if (uxfio_lseek(xx_ifd,  0, SEEK_SET) != 0) {
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}
		xformat_set_ifd(xx_swi->xformatM, xx_ifd);

		//
		// Get the data pointer and length of the existing signatures
		// Exactly has they appear in the tar archive.
		//
		xx_sig_block_buffer = uxfio_get_fd_mem(xx_sig_block_fd, &xx_sig_block_len);
		SWLIB_ASSERT(xx_sig_block_buffer != (char*)NULL);
		SWLIB_ASSERT(xx_sig_block_len > 0);

		//
		// Determine how many signatures there are
		//
		xx_num_existing_sigs = xx_sig_block_len / (TARRECORDSIZE+ xx_sig_length);

		SWP_E_DEBUG2("xx_sig_block_len=[%d]", xx_sig_block_len);
		SWP_E_DEBUG2("sig_remove_num=[%d]", sig_remove_num);
		SWP_E_DEBUG2("xx_num_existing_sigs=[%d]", xx_num_existing_sigs);
		if (
			(sig_remove_num >= 0 && xx_num_existing_sigs == 1)
		) {
			//
			// Can't remove the one and only one signature
			//
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"the only signature may not be removed, total existing signature are %d\n", xx_num_existing_sigs);
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		//
		// Check if sig_replace_num and sig_remove_num are valid for this package 
		//
		if (
			(sig_replace_num >= 0 && sig_replace_num > xx_num_existing_sigs)  ||
			(sig_remove_num >= 0 && sig_remove_num > xx_num_existing_sigs)  ||
			0
		) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"signature modification request invalid, total existing signature are %d\n", xx_num_existing_sigs);
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			exit(1);
		}

		SWP_E_DEBUG2("sig_replace_num=[%d]", sig_replace_num);
		//
		// Now decide if we need to run GPG
		//
		if (
			sig_replace_num >= 0 ||
			sig_add_num >= 0 
		) {
			//
			// Need to run GPG
			//

			SWP_E_DEBUG("RUNNING GPG");
			xx_signer_command = get_package_signature_command(
								wopt_signer_bin, 
								wopt_gpg_name, 
								wopt_gpg_path,
								wopt_passphrase_fd);
			SWLIB_ASSERT(xx_signer_command != NULL);

			//
			// get the package signature
			//
			SWP_E_DEBUG("HERE");
			xx_sig = get_package_signature(NULL, 
							xx_signer_command, 
							wopt_gpg_name,
							wopt_gpg_path, 
							&xx_l_status, 
							(int)wopt_format_arf, 
							catalog_tarheader_flags, 
							wopt_passphrase_fd, 
							wopt_passfile, wopt_no_front_dir,
							wopt_no_catalog, xx_signed_bytes_fd);
	
			SWP_E_DEBUG("HERE");
			if (!xx_sig) {
				//
				// Signature failed.
				//
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
					"Package not signed.  GPG exit status=%d\n", l_status);
				restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
				exit(1);
			}
		} else {
			xx_sig = (char*)NULL;
		}

		SWP_E_DEBUG("");
		if (wopt_mode_do_resign_test) {
			//
			// For the resign-test option simply copy the existing sig_block
			// to the new block, then re-write the package.  The package
			// should be identical bit-for-bit.
			//
			if (uxfio_lseek(xx_sig_block_fd,  0, SEEK_SET) != 0)  FATAL();
			swlib_pipe_pump(xx_new_sig_block_fd, xx_sig_block_fd);
		} else {
			SWP_E_DEBUG("");
			xx_ret = resign_modify_sig_block(xx_sig,
					xx_signature_tarheader,
					xx_sig_block_fd,
					xx_new_sig_block_fd,
					xx_sig_length,
					sig_add_num,
					sig_replace_num,
					sig_remove_num);

			if (xx_ret != 0) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
					"Signature modification failed\n");
				restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
				exit(1);
			}
		}


		//
		// The new sig block is in the ``xx_new_sig_block_fd'' descriptor
		// Get the new image constructed by modify_sig_block()
		//
		SWP_E_DEBUG("");
		xx_sig_block_buffer = uxfio_get_fd_mem(xx_new_sig_block_fd, &xx_sig_block_len);
			
		xx_recompress_commands = uinfile_get_recompress_vector(xx_xformat->swvarfsM->format_descM);
		xx_cmdline_compression_commands = get_compression_vector_from_vplob(compression_layers);

		SWP_E_DEBUG("");
		if (
			(
				wopt_mode_do_recompress &&
				xx_recompress_commands != NULL &&
				xx_recompress_commands[0] != NULL // has at least one recompressor
			) ||
			(
				xx_cmdline_compression_commands	
			)
		) {
			pid_t xx_pid;
			int xx_recfd[2];
			int xx_status;
			sigset_t xx_fork_defaultmask;
			sigset_t xx_fork_blockmask;
				
			if (xx_cmdline_compression_commands) {
				xx_recompress_commands = xx_cmdline_compression_commands;
			}

			sigemptyset(&xx_fork_blockmask);
  			sigaddset(&xx_fork_blockmask, SIGALRM);
			sigemptyset(&xx_fork_defaultmask);
			sigaddset(&xx_fork_defaultmask, SIGINT);
			sigaddset(&xx_fork_defaultmask, SIGPIPE);
			sigaddset(&xx_fork_defaultmask, SIGTERM);
			
			pipe(xx_recfd);
			xx_pid = swndfork(&xx_fork_blockmask, &xx_fork_defaultmask);

			if (xx_pid == 0) {
				//
				// Child
				// Writes the Package
				//
				close(xx_recfd[0]);
				uxfio_close(xx_ofd);
				SWP_E_DEBUG("CHILD HERE");
				xformat_set_ofd(xx_swi->xformatM, xx_recfd[1]);
				xx_ret = resign_write_pass_files(xx_swi->xformatM,
							xx_recfd[1],
							(unsigned char*)xx_sig_block_buffer,
							xx_sig_block_len);
				SWP_E_DEBUG("CHILD HERE after resign_write_pass_files");
				close(xx_recfd[1]);
				_exit (xx_ret <= 0 ? 1 : 0);
			} else if (xx_pid > 0) {
				//
				// Parent
				// Runs the Compressors
				//
				int xx_cmdret;
				SHCMD ** xx_cmdv;
				SHCMD * xx_last;
				SHCMD * xx_first;

				close(xx_recfd[1]);
				uxfio_close(xx_ifd);
				xx_last = shcmd_get_last_command(xx_recompress_commands);
				xx_first = xx_recompress_commands[0];
				shcmd_set_dstfd(xx_last, xx_ofd);
				shcmd_set_srcfd(xx_first, xx_recfd[0]);

				shcmd_cmdvec_exec(xx_recompress_commands);
				close(xx_recfd[0]);
	                        xx_cmdret = shcmd_cmdvec_wait2(xx_recompress_commands);
	                        //
				// Now check the exit status of every command
				// to avoid a false indication of success.
				//
				xx_cmdv = xx_recompress_commands;
				while (*xx_cmdv && xx_cmdret == 0) {
					xx_cmdret = shcmd_get_exitval(*xx_cmdv);
					if (xx_cmdret == SHCMD_UNSET_EXITVAL) xx_cmdret = 127;
					xx_cmdv++;
				}
				SWP_E_DEBUG("HERE");
				xx_ext_ret = waitpid(xx_pid, &xx_status, 0);
				SWP_E_DEBUG2("HERE after wait: waitpid return value = [%d]", xx_ext_ret);
				if (WIFEXITED(xx_status)) {
					xx_ext_ret = WEXITSTATUS(xx_status);
					SWP_E_DEBUG2("WEXITSTATUS(xx_status) = [%d]", xx_ext_ret);
				} else {
					xx_ext_ret = 127;
				}
				SWP_E_DEBUG2("xx_cmdret = [%d]", xx_cmdret);
				if (xx_cmdret && xx_ext_ret == 0)
					xx_ext_ret = 199;
				SWP_E_DEBUG2("xx_ext_ret=[%d]", xx_ext_ret);
			} else {
				// error
				return -1;
			}
			xx_write_ret = 0;
		} else {
			//
			// Write the package with the new signature block.
			//
			SWP_E_DEBUG("running resign_write_pass_files");
			xx_ext_ret = 0;
			xx_write_ret = resign_write_pass_files(xx_swi->xformatM,
							xx_ofd,
							(unsigned char*)xx_sig_block_buffer,
							xx_sig_block_len);
		}

		SWP_E_DEBUG("closing now");
                taru_free_header(xx_file_hdr);
		uxfio_close(xx_new_sig_block_fd);
		uxfio_close(xx_sigheader_fd);
		swpath_close(xx_swpath);
		uxfio_close(xx_signed_bytes_fd);
		uxfio_close(xx_sig_block_fd);
		uxfio_close(xx_sig_header_fd);
		swi_delete(xx_swi);
		swutil_close(xx_swlog);
		
		// EXIT

		if (xx_write_ret < 0 || xx_ext_ret != 0) {
			//
			// Error
			//
			SWP_E_DEBUG("error, running restore_overwrite_and_exit");
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"resign failed, restoring original file\n");
			restore_overwrite_and_exit(strob_str(xx_cl_target_orig), cl_target, 1);
			//
			// Never gets here
			//
			exit(99);
	 	} else {
			//
			// No error, unlink the backup file
			//
			SWP_E_DEBUG("no error, unlink the backup file");
			if (strob_strlen(xx_cl_target_orig) > 0) {
				xx_ret2 = unlink(strob_str(xx_cl_target_orig));
				if (xx_ret2 < 0) {
					swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
						"unlink failed (not removed) %s : reason: %s\n", strob_str(xx_cl_target_orig), strerror(errno));
				}
			}
		}

		exit(0);

		//
		//
		// end of the --resign code
		//
		//
	  //
	} // --------------------------------------------------------------------****
	  //

	//
	// Open the parser.
	//
	if (psf->open_parser(psf_source_ifd)){
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"Error opening parser.\n");
	}

	//
	// Parse the PSF.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), "parsing PSF: ...\n");
	if (psf->run_parser(SWPARSE_AT_LEVEL, 
				SWPARSE_FORM_MKUP_LEN |
				SWPARSE_FORM_POLICY_POSIX_IGNORES) < 0) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"error parsing psf file: %s\n", psffilename);
		exit(SW_EXIT_ONE);
	}
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), "parsing PSF: Finished\n");


	//
	// when sourcing from an archive (tranlating .deb and .rpm)
	// close the PSF so the read of the archive padding is forced.
	//

	if (wopt_source_filename) {
		swvarfs_u_close(swvarfs, psf_source_ifd);
	}

	//
	// Expand extended definitions in the PSF file.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(),
		"Resolving Extended Definitions in PSF: ...\n");
	if (psf->generateDefinitions()) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"%s: error processing psf file: %s\n", swlib_utilname_get(), psffilename);
		exit(SW_EXIT_ONE);
	}
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(),
		"Resolving Extended Definitions in PSF: Finished\n");

	//
	// Hard Link Processing.
	// The psf->xFormat_get_xformat()->swvarfsM->link_recordM was used during the 
	// Extended defintion processing, therefore it is the object that must
	// be used during the package generate phase.
	// NOTE: 
	// 	(fset.getPackageDirObject()->xFormat_get_xformat()->link_recordM) ==
	//	(psf->xFormat_get_xformat()->link_recordM)
	//
	// psf->xFormat_get_xformat()->link_recordM = psf->xFormat_get_xformat()->swvarfsM->link_recordM;

	if (wopt_source_filename) {
		/*
		* This is a work around for when the source "file system" is
		* a tar archive, i.e. when transalating an RPM.  In this case
		* the hard link objects in the HLLIST objects are not correct,
		* so don't use them.
		*/
		hllist_clear_entries_and_disable(psf->xFormat_get_xformat()->link_recordM);
		hllist_clear_entries_and_disable(psf->xFormat_get_xformat()->swvarfsM->link_recordM);
		hllist_disable_find(psf->xFormat_get_xformat()->link_recordM);
		hllist_disable_find(psf->xFormat_get_xformat()->swvarfsM->link_recordM);
	}

	if (verboseG > 9) {
		hllist_show_to_file(psf->xFormat_get_xformat()->link_recordM, stderr);
		hllist_show_to_file(psf->xFormat_get_xformat()->swvarfsM->link_recordM, stderr);
	}
	
	//
	// Process the software selections.
	// (Not yet implemented.)
	//
	// process_selections(swspsf, argc, *argv);
	//

	if (do_files_from_pid > 0) {
		//
		// This code supports the "read a file list" mode.
		//
		pid_array[0] = do_files_from_pid;
		if (psf_source_ifd != STDIN_FILENO && psf_source_ifd >= 0) {
			close(psf_source_ifd);
			psf_source_ifd = -1;
		}	
		if (swlib_wait_on_all_pids(pid_array, 1, 
			status_array, 0 /*WNOHANG*/, verboseG - 2) < 0) {
			exit(1);
		}
		if (WEXITSTATUS(status_array[0]) != 0) {
			exit(1);
		}
	}

	//
	// Construct the Collection representing the exported package form.
	//
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(),
		"swExCat package structures: Construction BEGIN\n");
	swexdist = swExDistribution::constructSwExDist(psf, &error_code, opta);
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(),
		"swExCat package structures: Construction END\n");
	
	if (is_option_true(eopt_follow_symlinks)) {
		swvarfs_set_stat_syscall(swvarfs, "stat");
	} else {
		swvarfs_set_stat_syscall(swvarfs, "lstat");
	}
	swexdist->setSwvarfs(swvarfs);
	swexdist->setVerboseLevel(verboseG);
	swexdist->setStoreRegTypeOnly(regfiles_only);

	if (wopt_source_filename) {
		// HACK.
		// set a flag that allows files to be missing, that
		// is they have meta-data in the catalog PSF but are
		// not in the file system.
		//
		// This code path assumes RPM translation mode, this is
		// about the only reason to set set_allow_missing_file(1).
		// Some RPMSs have meta-data but no file in the payload.
		//
		swexdist->set_allow_missing_files(1);
	}

	//
	// The psf is a swPackageFile<> and therefore it has all the archiver
	// methods.  It is also already configured to read the source.
	//
	psf->xFormat_set_output_format(wopt_format_arf);
	swexdist->setArchiver(psf);

	//
	// Check for errors.
	// Any error here is probably an implementation error.
	//
	if (swexdist == NULL || error_code) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
		"internal implementation error: constructSwExDist error:"
			" error code = %d.\n", error_code);
		exit(SW_EXIT_ONE);
	}

	//
	// Set the Output FD
	//
	ret = set_stdio_output_fd(swexdist, cl_target, O_RDWR|O_TRUNC|O_CREAT);
	SWLIB_ASSERT(ret >= 0);
	swexdist->set_ofd(ret);

	//
	// Set the preview level
	//
	set_preview_level(swexdist, opt_preview, verboseG);

	//
	// Set the blocksize.
	//
	final_ofd = swexdist->get_ofd();
	SWP_E_DEBUG2("final_ofd=%d", final_ofd);
	if (compression_layers == NULL && uxfio_espipe(final_ofd)) {
		// It is a pipe and swpackage is writing it.
		//
		SWP_E_DEBUG("is pipe");
		if (blocksize) {
			SWP_E_DEBUG2("setting blocksize final_ofd=%d", final_ofd);
			final_ofd = uxfio_opendup(final_ofd, UXFIO_BUFTYPE_NOBUF);
			SWLIB_ASSERT(final_ofd > 0);	
			uxfio_fcntl(final_ofd, UXFIO_F_SET_OUTPUT_BLOCK_SIZE, blocksize);
			SWP_E_DEBUG2("setting blocksize NEW final_ofd=%d", final_ofd);
			swexdist->set_ofd(final_ofd);
		}
	} else {
		//
		// Regular file or using compression.
		// do not set block size
		SWP_E_DEBUG("is REG");
		;
	}

	//
	// Set the cksum/md5sum policy.
	//
	swexdist->set_cksum_creation(IST(wopt_cksum)?1:0);
	swexdist->set_digests1_creation(IST(wopt_file_digests) ? 1:0);
	swexdist->set_digests2_creation(IST(wopt_file_digests2) ? 1:0);

	//
	// Set the absolute path disposition.
	//
	swexdist->allowAbsolutePaths() = IST(wopt_absolute_names) ? 1 : 0;

	if (opt_preview) {
		//
		// Posix option '-p'
		//
		// Turn off package generation and write the file list
		// to stdout.
		//
		swexdist->set_ofd(g_nullfd);
		swexdist->set_preview_fd(STDOUT_FILENO);
	}

	if (verboseG >= SWC_VERBOSE_3 && opt_preview == 0) {
		//
		// show a file list on stderr.
		//
		swexdist->set_preview_fd(STDERR_FILENO);
	}

	if (wopt_do_list) {
		//
		// List PSF
		// Implementation extension option to list the expanded PSF.
		// 
		// -W list-psf  Option: List the files.
		// This writes a PSF-like form of the package to stdout.
		//
		swexdist->write_fd(STDOUT_FILENO);
	} else if (wopt_do_debug) {
		//
		// Debug option.
		//
		
		fprintf(stderr, 
			"%d\n", 
	swexdist->getGlobalIndex()->swdeffile_linki_write_fd(STDOUT_FILENO));
		swexdist->printDebugStructure(STDOUT_FILENO);	
	} else {/**/

	//
	// -----------------------------------------------------
	// Really make the package.
	// swexdist may either be a host or distribution object.
	// -----------------------------------------------------
	//

	//
	// initial processing.
	//
	swexdist->registerWithGlobalIndex();
	swexdistribution = static_cast<swExDistribution*>
				(swexdist->getDistribution());
	SWLIB_ASSERT(swexdistribution != NULL);	

	//
	// Set initial value.
	//
	swexdistribution->setCurrentDirMode((mode_t)(0));
	
	//
	// Set the creation time
	//
	if (wopt_create_time) {
		swexdist->setCreateTime((time_t)create_time);
		swexdistribution->setDirMtime((time_t)create_time);
	}

	//
	// Add the attributes for the leading directory.
	//
	if (wopt_dir) {	
		//
		// Add the control_directory attribute to the 
		// distribution object.  This value will become 
		// the path name prefix.
		//
		char * new_opt_dtag;

		if (swexdist->allowAbsolutePaths() == 0) {
			new_opt_dtag = swlib_return_relative_path(
						wopt_dir);
		} else {
			new_opt_dtag = wopt_dir;
		}
		swdef = swexdistribution->getReferer();
		swdef->deleteAttribute("control_directory");
		swdef->add("control_directory", new_opt_dtag);
		if (swdef->find("tag") == NULL && 
					strlen(new_opt_dtag)) {
			swdef->add("tag", new_opt_dtag);
		}
	}

	//
	// Set the  Attributes for the Leading Directory.
	//    only if there is a leading directory.
	//

	/* initialize with some reasonable defaults */
	swexdistribution->setLeadingDirOwner("0");
	swexdistribution->setLeadingDirGroup("0");
	swexdistribution->setLeadingDirMode(SWBIS_DISTRIBUTION_MODE_VAL);

	//
	// Now check the PSF distribution object for owner, group and mtime
	// attributes.
	//

	ret = set_user_name_from_psf(swexdistribution, set_PREFIX,
				SWBIS_DISTRIBUTION_OWNER_ATT,
				&wopt_dir_owner,
				&g_wopt_dir_numeric_owner);
	if (ret == 0) {
		//
		// Nothing set, use the current user and group
		//
		SWP_E_DEBUG("");
		if (*sysa_owner != '\0') {
			SWP_E_DEBUG("");
			swexdistribution->setLeadingDirOwner(sysa_owner);
		} else {
			SWP_E_DEBUG("");
			swexdistribution->setLeadingDirOwner(SWBIS_DEFAULT_CATALOG_USER);
		}
	}


	ret = set_user_name_from_psf(swexdistribution, set_PREFIX,
				SWBIS_DISTRIBUTION_GROUP_ATT,
				&wopt_dir_group,
				&g_wopt_dir_numeric_owner);

	if (ret == 0) {
		//
		// Nothing set, use the current user and group
		//
		SWP_E_DEBUG("");
		if (*sysa_group != '\0') {
			SWP_E_DEBUG("");
			swexdistribution->setLeadingDirGroup(sysa_group);
		} else {
			SWP_E_DEBUG("");
			swexdistribution->setLeadingDirGroup(SWBIS_DEFAULT_CATALOG_GROUP);
		}
	}

	tmpname = swexdistribution->getReferer()->find(SW_A_mode);
	if (tmpname && wopt_dir_mode == NULL) {
		wopt_dir_mode = strdup(tmpname);
		swexdistribution->setLeadingDirMode(tmpname);
	}

	tmpname = swexdistribution->getReferer()->find(SW_A_mtime);
	if (tmpname) {
		unsigned long int tm;
		tm = swlib_atoul(tmpname, (int *)NULL, (char**)NULL);
		swexdistribution->setDirMtime((time_t)tm);
	}

	SWP_E_DEBUG("");
	if (wopt_dir_owner == NULL) {
		SWP_E_DEBUG("");
		if (*sysa_owner == '\0') {
			//
			// User not in database, 
			// make archive with numeric id's
			//
			g_wopt_numeric_owner = CHARTRUE;
			SWP_E_DEBUG("");
		} else {
			swexdistribution->setLeadingDirOwner(sysa_owner);
			SWP_E_DEBUG("");
		}
	} else if (strlen(wopt_dir_owner)) {
		SWP_E_DEBUG("");
		swexdistribution->setLeadingDirOwner(wopt_dir_owner);
		swexdistribution->getReferer()->add(
				SWBIS_DISTRIBUTION_OWNER_ATT, 
					wopt_dir_owner);
		g_wopt_dir_numeric_owner = 
				set_numeric(wopt_dir_owner,
					g_wopt_dir_numeric_owner);
		if (is_option_true(g_wopt_dir_numeric_owner))
			g_wopt_catalog_numeric_owner = CHARTRUE;
	} else {
		//
		// wopt_dir_owner is ""
		// As an implementation extension, use the owner of
		// of "."
		//
		SWP_E_DEBUG("");
		if (taru_get_tar_user_by_uid(cwdst.st_uid, sysa_tmp)) {
			g_wopt_numeric_owner = CHARTRUE;
			strob_sprintf(tmp, 0, "%d", (int)(cwdst.st_uid));
			swexdistribution->setLeadingDirOwner(strob_str(tmp));
		} else {
			swexdistribution->setLeadingDirOwner(sysa_tmp);
		}
	}

	if (wopt_dir_group == NULL) {
		SWP_E_DEBUG("");
		if (*sysa_group == '\0') {
			//
			// User not in database, 
			// make archive with numeric id's
			//
			g_wopt_numeric_owner = CHARTRUE;
		} else {
			swexdistribution->setLeadingDirGroup(sysa_group);
		}
	} else if (strlen(wopt_dir_group)) {
		SWP_E_DEBUG("");
		swexdistribution->setLeadingDirGroup(wopt_dir_group);
		swexdistribution->getReferer()->add(
				SWBIS_DISTRIBUTION_GROUP_ATT,
				wopt_dir_group);
		
		if (set_numeric(wopt_dir_group, NULL))
			g_wopt_catalog_numeric_owner = CHARTRUE;
	} else {
		SWP_E_DEBUG("");
		//
		// wopt_dir_group is ""
		// As an implementation extension, use the group of
		// of "."
		//
		if (taru_get_tar_group_by_gid(cwdst.st_gid, sysa_tmp)) {
			g_wopt_numeric_owner = CHARTRUE;
			strob_sprintf(tmp, 0, "%d", (int)(cwdst.st_gid));
			swexdistribution->setLeadingDirGroup(strob_str(tmp));
		} else {
			swexdistribution->setLeadingDirGroup(sysa_tmp);
		}
	}

	if (wopt_dir_mode == NULL) {
		SWP_E_DEBUG("");
		swexdistribution->setLeadingDirMode(
				SWBIS_DISTRIBUTION_MODE_VAL);
	} else {
		char where[5];
		unsigned int on;
		unsigned int perms;
		int scnret;
		SWP_E_DEBUG("");
		scnret = sscanf(wopt_dir_mode, "%o", &on);
		if (*wopt_dir_mode == '.') {
			taru_mode_to_chars(
				(cwdst.st_mode & 0777), where, 
				sizeof(where), 0);
			wopt_dir_mode = strdup(where);
			scnret = sscanf(wopt_dir_mode, 
					"%o", &on);
			if (scnret != 1) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"Bad mode conversion\n");
				exit(1);
			}
			perms = cwdst.st_mode & 
					(S_IRWXU | 
					S_IRWXG | S_IRWXO | 
					S_ISUID | S_ISGID | 
					S_ISVTX);

			//
			// Sanity check.
			//
			if (on != (unsigned int)(perms)) {
				FATAL2("Bad mode conversion\n");
				exit(1);
			}
		} else {
			SWP_E_DEBUG("");
			scnret = sscanf(wopt_dir_mode, "%o", &on);
			if (scnret != 1) {
				swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
					"Bad mode conversion\n");
				exit(1);
			}
			SWP_E_DEBUG("");
			taru_mode_to_chars(
				(on & 0777), where, 
				sizeof(where), 0);
			wopt_dir_mode = strdup(where);
		}
		SWP_E_DEBUG("");
		swexdistribution->setLeadingDirMode(wopt_dir_mode);
		swexdistribution->getReferer()->add(
		 	SWBIS_DISTRIBUTION_MODE_ATT,
		 	wopt_dir_mode);
	}

	
	//
	// Now form the catalog dir perms and file perms string.
	//
	swexdistribution->formCatalogDirModeString();
	swexdistribution->formCatalogFileModeString();

	//
	// Respect the "owner" and "group" attributes in the 
	// distribution object
	// of the PSF file.
	//

	ret = set_user_name_from_psf(swexdistribution, set_CATDIR,
				SWBIS_DISTRIBUTION_OWNER_ATT,
				&wopt_catalog_owner,
				&g_wopt_catalog_numeric_owner);

	if (ret == 0) {
		//
		// Nothing set, use the current user and group
		//
		if (*sysa_owner != '\0') {
			swexdistribution->setCatalogOwner
						(sysa_owner);
		} else {
			swexdistribution->setCatalogOwner
						(SWBIS_DEFAULT_CATALOG_USER);
		}
	}

	ret = set_user_name_from_psf(swexdistribution, set_CATDIR,
				SWBIS_DISTRIBUTION_GROUP_ATT,
				&wopt_catalog_group,
				&g_wopt_catalog_numeric_owner);


	if (ret == 0) {
		//
		// Nothing set, use the current user and group
		//
		if (*sysa_group != '\0') {
			swexdistribution->setCatalogGroup
						(sysa_group);
		} else {
			swexdistribution->setCatalogGroup
					(SWBIS_DEFAULT_CATALOG_GROUP);
		}
	}

	//
	// Add the create_time attribute.
	//
	{
		STROB * tm = strob_open(100);
		swdef = swexdistribution->getReferer();
		strob_sprintf(tm, 0, "%lu",  swexdist->getCreateTime());
		swdef->add(SW_A_create_time, strob_str(tm));
		strob_close(tm);
	}

	//
	// Generate and add the "is_locatable" attribute to the products.
	//
	add_is_locatable_attributes(swexdistribution);

	//
	// Add the signer attributes.
	//
	if (is_option_true(wopt_sign)) {
		SWP_E_DEBUG("");
		if (opt_preview == 0) {
			swDefinition * swdef;
			swdef = swexdistribution->getReferer();
			add_signer_attributes(swdef, tmp, wopt_signer_bin);
		}
	}

	//
	// Add the tar_format options attributes.
	//
	if (wopt_format_arf == arf_ustar) {
		STROB * opt_tmp = strob_open(16);
		SWP_E_DEBUG("");
		set_format_attributes(wopt_format_arf, wopt_format, 
			do_oldgnutar, do_bsdpax3, do_oldgnuposix,
				&taremu_attr, &taremu_value);
		if (taremu_value) {
			SWP_E_DEBUG("");
			strob_strcpy(opt_tmp, taremu_value);
			if (is_option_true(g_wopt_numeric_owner)) {
				strob_strcat(opt_tmp, " --numeric");
			}
			swexdistribution->\
				getReferer()->\
					add(
				"tar_format_emulation_options", 
				strob_str(opt_tmp));
		}
		SWP_E_DEBUG("");
		if (taremu_attr)
			swexdistribution->\
				getReferer()->\
					add(
				"tar_format_emulation_utility",
				taremu_attr);
		strob_close(opt_tmp);
	}

	//
	// Add the uuid attribute to the INDEX file.
	//
	SWP_E_DEBUG("");
	make_uuid(tmp, wopt_uuid);
	swexdistribution->getReferer()->add("uuid", strob_str(tmp));

	//
	// Generate and add the "instance_id" attribute.
	//
	SWP_E_DEBUG("");
	add_instance_id_attributes(swexdistribution, SW_A_product "|" SW_A_bundle);
	
	//
	// Add the state attribute to each fileset with value
	// of "available"
	//
	add_state_attribute_to_filesets(swexdistribution);

	//
	// enforce that both product and fileset control_directories
	// be empty or both be not empty
	//
	if (check_for_invalid_layout(swexdistribution)) {
		swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
			"invalid package layout\n");
		exit(1);
	}

	::init_serial_package(swexdist);

	if (IST(wopt_files)) {
		SWP_E_DEBUG("");
		filesfd = swlib_open_memfd();
		SWLIB_ASSERT(filesfd > 0);
	}

	if (
		is_option_true(wopt_sign) || 
		is_option_true(wopt_archive_digests) || 
		is_option_true(wopt_archive_digests2) || 
		is_option_true(wopt_write_signed_file) ||
		is_option_true(wopt_files)
	) {
		SWP_E_DEBUG("");
		memset(md5sum_buffer_o, '\0', sizeof(md5sum_buffer_o));
		memset(adjunct_md5sum_buffer_o, '\0', 
					sizeof(adjunct_md5sum_buffer_o));
			
		if (IST(wopt_files) && (IST(wopt_archive_digests) || IST(wopt_archive_digests2))) {
			add_archive_digests_precursor(swexdist, 
				swexdistribution, wopt_no_sha1?1:0,
				(size_buffer==NULL) ? 0 : 1,
				IST(wopt_archive_digests2) ? 1 : 0,
				IST(wopt_archive_digests) ? 1 : 0);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		}	

		if (IST(wopt_files)) {
			add_files_precursor(swexdist, swexdistribution);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		}
			
		if (is_option_true(wopt_files) && (is_option_true(wopt_sign) || is_option_true(wopt_write_signed_file))) {
			add_signature_header_list_precursor(swexdist, 
							swexdistribution);
			add_signature_list_precursor(swexdist, 
							swexdistribution);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		}
			
		ret = ::generate_security_files(swexdist,
				swexdistribution, (int)wopt_format_arf, 
					md5sum_buffer, adjunct_md5sum_buffer,
					sha1_buffer, size_buffer, sha512_buffer, filesfd, 
					tarheader_flags, wopt_no_front_dir,
					wopt_no_catalog);
		
		if (IST(wopt_files)) {
			delete_files_precursor(swexdist, swexdistribution);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		}
			
		if (IST(wopt_files) && (IST(wopt_archive_digests) || IST(wopt_archive_digests2))) {
			delete_archive_digests_precursor(swexdist,
						swexdistribution);
		}	

		if (IST(wopt_files) && (is_option_true(wopt_sign) || is_option_true(wopt_write_signed_file))) {
			delete_signature_list_precursor(swexdist, 
							swexdistribution) ;
			delete_signature_header_list_precursor(swexdist, 
							swexdistribution) ;
			// SWBIS_INTERNAL_ASSERT(swexdist->getErrorCode(), SW_EXIT_ONE);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
		}

		// Make sure they stay null terminated.	
		if (IST(wopt_archive_digests)) {
			md5sum_buffer[sizeof(md5sum_buffer_o) - 3] = '\0';
			adjunct_md5sum_buffer[sizeof(adjunct_md5sum_buffer_o) - 3]='\0';
		}
		if (ret) {
			swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
				"generate_security_files returned %d\n", ret);
		}
		SWLIB_ASSERT(ret == 0);
	}

	SWP_E_DEBUG("");
	if (IST(wopt_archive_digests) || IST(wopt_archive_digests2)) {
		ret = ::add_archive_digests(swexdist, 
				swexdistribution, (int)wopt_format_arf, 
					md5sum_buffer, adjunct_md5sum_buffer,
					sha1_buffer, size_buffer, sha512_buffer);
		SWLIB_ASSERT(ret == 0);
	}
	
	SWP_E_DEBUG("");
	if (IST(wopt_files)) {	
		ret = ::add_files(swexdist, swexdistribution, filesfd);
		SWLIB_ASSERT(ret == 0);
	}

	SWP_E_DEBUG("");
	if (
		is_option_true(wopt_sign) ||
		is_option_true(wopt_write_signed_file)) 
	{
		char * sig;
	
		SWP_E_DEBUG("");
		add_signature_header_precursor(swexdist, swexdistribution);
		SWP_E_DEBUG("");
		add_signature_precursor(swexdist, swexdistribution);
		SWP_E_DEBUG("");
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);

		SWP_E_DEBUG("");
		swexdist->taskDispatcher(swExStruct::resetDfilesInfoSizesE);
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);

		if (is_option_true(wopt_sign)) {
			if (opt_preview == 0) {
				if (!is_option_true(wopt_dummy_sign)) {
					signer_command = 
						get_package_signature_command(
							wopt_signer_bin, 
							wopt_gpg_name, 
							wopt_gpg_path,
							wopt_passphrase_fd);
					SWLIB_ASSERT(signer_command != NULL);

					if (verboseG >= SWC_VERBOSE_2 && opt_preview == 0) {
						strob_strcpy(tmp2, "");
						shcmd_write_command_to_buf(signer_command, tmp2);
						swlib_doif_writef(verboseG, g_fail_loudly, 0, get_stderr_fd(),
							"Running %s\n", strob_str(tmp2));
					}

				} else {
					signer_command = (SHCMD*)NULL;
				}
				SWP_E_DEBUG("");
				sig = get_package_signature(swexdist, 
						signer_command, 
						wopt_gpg_name,
						wopt_gpg_path, 
						&l_status, 
						(int)wopt_format_arf, 
						catalog_tarheader_flags, 
						wopt_passphrase_fd, 
						wopt_passfile, wopt_no_front_dir,
						wopt_no_catalog, -1);
				SWP_E_DEBUG("");
				swexdist->assertNoErrorCondition(SW_EXIT_ONE);
			} else {
				sig = NULL;
			}

			if (sig) {
				SWP_E_DEBUG("");
				ret = add_signature(swexdist, sig);
				SWP_E_DEBUG("");
			}
				
			if ((!sig || ret != 0) && opt_preview == 0) {
				//
				// Signature failed.
				//
				swexdist->setSigFileBuffer(NULL);
				swexdist->taskDispatcher(
					swExStruct::tuneSignatureFileE);
				swexdist->assertNoErrorCondition(SW_EXIT_ONE);
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"Package not signed.  GPG exit status=%d\n", l_status);
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(), 
				"Target medium not modified\n");
				delete swexdist;
				delete psf;
				exit(1);
			}
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
			if (sig) free(sig);
		}

		if (is_option_true(wopt_write_signed_file)) {
			//
			// Add a dummy signature so related metadata files
			// are made to look like it is signed.
			//
			if (!is_option_true(wopt_sign)) {
				ret = add_signature(swexdist, "xxxx");
			}

			//
			// Remove the /catalog/dfiles/signature 
			// attribute file.
			// that was added in the step above.  
			// The signature control_file
			// is in the INFO file with a length of 512 
			// bytes, and this entry
			// stays.
			//
			SWP_E_DEBUG("");
			swexdist->setSigFileBuffer(NULL);
			swexdist->taskDispatcher(
				swExStruct::tuneSignatureFileE);
			swexdist->assertNoErrorCondition(SW_EXIT_ONE);
			wopt_catalog_only = CHARTRUE;
			wopt_storage_only = (char*)0;
		}
	} else {
		//
		// Fix up the dfiles/INFO file size attribute.
		// If signing is done this has
		// to be done before the actual signing.
		//
		swexdist->taskDispatcher(
				swExStruct::resetDfilesInfoSizesE);
		swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	}	

	//
	// ----------------------------
	// Write the final package.
	// ----------------------------
	//

	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
	"Writing the final package: ....\n");

	//
	// From this point on it is incorrect to use any hard link
	// recording, so disable "adds".  It is also incorrect to
	// use any link information already present, so disable "find"
	// as well (I think).  This is because the meta-data from the
	// INFO file [which is already fully constructed] should be used.
	//

	hllist_disable_add(psf->xFormat_get_xformat()->link_recordM);
	hllist_disable_add(psf->xFormat_get_xformat()->swvarfsM->link_recordM);
	hllist_disable_find(psf->xFormat_get_xformat()->link_recordM);
	hllist_disable_find(psf->xFormat_get_xformat()->swvarfsM->link_recordM);

	pid = 0;
	pv[0] = -1;
	pv[1] = -1;

	if (
		compression_layers != NULL &&
		opt_preview == 0
	) {
		pipe(pv);
		pid = fork();
		if (pid == 0) {
			int cmdret;
			SHCMD ** command_vector;
			SHCMD ** cmdv;
			SHCMD * last_command;
			close(pv[1]);
			command_vector = (SHCMD**)vplob_get_list(compression_layers);
			shcmd_set_srcfd(command_vector[0], pv[0]);
			last_command = shcmd_get_last_command(command_vector);
			SWLIB_ASSERT(last_command != NULL);
			if (direct_to_dev_null) {
				shcmd_set_dstfile(last_command, DEVNULL);
			} else {
				shcmd_set_dstfd(last_command, final_ofd);
			}
			shcmd_cmdvec_exec(command_vector);
			cmdret = shcmd_cmdvec_wait2(command_vector);
			//
			// Now check the exit status of every command
			// to avoid a false indication of success.
			//
			cmdv = command_vector;
			while (*cmdv && cmdret == 0) {
				cmdret = shcmd_get_exitval(*cmdv);
				if (cmdret == SHCMD_UNSET_EXITVAL) cmdret = 127;
				cmdv++;
			}
			_exit(cmdret);
		} else if (pid > 0) {
			// parent
			uxfio_close(final_ofd);
			final_ofd = -1;
			close(pv[0]);
			SWP_E_DEBUG2("SETTING pv_ofd=%d", pv[1]);
			pv_ofd = pv[1];
			swexdist->set_ofd(pv_ofd);
			;  // just continue
		} else {
			exit(1); // fork failed;
		}
	} else {
		SWP_E_DEBUG2("SETTING pv_ofd=%d", swexdist->get_ofd());
		pv_ofd = swexdist->get_ofd();	
	}

	wint_catalog_only = wopt_catalog_only ? 1 : 0;
	wint_storage_only = wopt_storage_only ? 1 : 0;

	//
	// Here is where the serial package is written for real
	//

	SWP_E_DEBUG2("pv_ofd=%d", pv_ofd);
	::write_serial_package(psf, swexdist,
			pv_ofd,
			wint_catalog_only,
			wint_storage_only,
			g_wopt_dir_numeric_owner,
			g_wopt_catalog_numeric_owner,
			g_wopt_numeric_owner, wopt_no_front_dir,
			wopt_no_catalog);
	SWP_E_DEBUG("OUT of write_serial_package");

	if (pv[1] >= 0) close(pv[1]);
	exit_retval = 3;
	if (pid > 0) {
		ret = waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			ret = WEXITSTATUS(status);
			if (ret) {
				exit_retval = SW_EXIT_TWO;
			} else {
				exit_retval = SW_EXIT_SUCCESS;
			}
		} else {
			exit_retval = SW_EXIT_TWO;
			ret = 127;
		}
		if (ret != 0)
			swlib_doif_writef(verboseG, SWC_VERBOSE_1, NULL, get_stderr_fd(),
				"a compressor (or encryptor) exited abnormally\n");
	} else {
		exit_retval = SW_EXIT_SUCCESS;
	}

	SWP_E_DEBUG("");
	swlib_doif_writef(verboseG, SWC_VERBOSE_SWPV, NULL, get_stderr_fd(), 
	"Writing the final package: Finished\n");
	
	SWP_E_DEBUG("");
	if (verboseG >= SWC_VERBOSE_2 && opt_preview == 0) {
		xformat_write_archive_stats(
			swexdist->getArchiver()->xFormat_get_xformat(), 
			"swpackage", STDERR_FILENO);
	}

	//
	// finis. 
	//
	}/**/

	//
	// Close and delete.
	//

	if (	
		psf_source_ifd != STDIN_FILENO &&
		psf_source_ifd >= 0
	) {
		close(psf_source_ifd);
	}

	ret = swexdist->get_ofd();
	if (ret >= 0 && ret != STDOUT_FILENO) {
		uxfio_close(ret);
	}

	swexdist->assertNoErrorCondition(SW_EXIT_ONE);
	delete swexdist;
	delete psf;
	exit(exit_retval);
}
