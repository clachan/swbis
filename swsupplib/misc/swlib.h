/* swlib.h - General purpose, misc. routines.

   Copyright 1997-2004 James Lowe, Jr.
   All Rights Reserved.

   This file may be copied under the terms of the GNU GPL.
*/

#ifndef SWLIB_H_200701a
#define SWLIB_H_200701a

#include "swuser_assert_config.h"
#include "swprog_versions.h"
#include "sw.h"
/* #include "swinttypes.h" */
#define SW_TRUE 1
#define SW_FALSE 0
#define SWLIB_PIPE_BUF 512
#define SWLIB_SYNCT_BLOCKSIZE 128  /* blocksize for sync'ed ouput with inband TRAILER */

#define xSTR(s) iSTR(s)
#define iSTR(s) #s
#define CSHID "# Generated in " __FILE__ " at line " xSTR(__LINE__) "\n"

#define SHA_BLOCKSIZE 512 /* this must be the same as BLOCKSIZE and SHA_MEM_BLOCKSIZE
			     in sha.c and sha512.c */

#define swio_read(a, b, c) 	atomicio(read, a, b, c)
#define swio_write(a, b, c) 	atomicio(write, a, b, c)

/* FIXME
#ifndef __FUNCTION__
#define __FUNCTION__ "unknown"
#endif
*/
#define SWBIS_PROGS_USER_ERROR 			20000		/* Error code >= are cockpit errors */
#define SWBIS_PROGS_IMPLEMENTATION_ERROR 	10000		/* Error code >= are implementation errors */

#define SWBIS_DEBUG_PRINT() fprintf(stderr, \
		 "%s: %s:%d\n", \
			swlib_utilname_get(), __FILE__, __LINE__);

#define SWBIS_ERROR_IMPL() fprintf(stderr, \
		 "%s: swbis release %s: unexpected result or internal error at %s:%d\n", \
			swlib_utilname_get(), SWBIS_RELEASE, __FILE__, __LINE__);

#define TARRECORDSIZE		512

#define SWBIS_INTERNAL_ASSERT(code, status)  swlib_swprog_assert(code, status, "", SWBIS_RELEASE, (char*)__FILE__, __LINE__, (char*)__FUNCTION__)

#define SWBIS_IMPL_ERROR_DIE(status)  swlib_swprog_assert(SWBIS_PROGS_IMPLEMENTATION_ERROR, status, "Impl fatal internal error", SWBIS_RELEASE, (char*)__FILE__, __LINE__, (char*)__FUNCTION__)

#define SWI_FILESET_0	0
#define SWI_PRODUCT_0	0

#define SWC_VERBOSE_0	0
#define SWC_VERBOSE_1	1
#define SWC_VERBOSE_2	2
#define SWC_VERBOSE_3	3
#define SWC_VERBOSE_4	4
#define SWC_VERBOSE_5	5
#define SWC_VERBOSE_6	6
#define SWC_VERBOSE_7	7
#define SWC_VERBOSE_8	8
#define SWC_VERBOSE_9	9
#define SWC_VERBOSE_10	10
#define SWC_VERBOSE_11	11
#define SWC_VERBOSE_12	12

#define SWPACKAGE_VERBOSE_V0	SWC_VERBOSE_4
#define SWPACKAGE_VERBOSE_V1	SWC_VERBOSE_5
#define SWPACKAGE_VERBOSE_V2	SWC_VERBOSE_6
#define SWPACKAGE_VERBOSE_V3	SWC_VERBOSE_7
#define SWPACKAGE_VERBOSE_V4	SWC_VERBOSE_8

#define SWC_VERBOSE_SWIDB 	5		/* Level at which scripts `set -vx' */
#define SWC_VERBOSE_IDB 	8		/* Lowest debugging level*/
#define SWC_VERBOSE_IDB2 	9

#define SWLIB_FATAL(A)		swlib_fatal(A, (char*)__FILE__, __LINE__, (char*)__FUNCTION__);
#define SWLIB_RESOURCE(A)	swlib_resource(A, (char*)__FILE__, __LINE__, (char*)__FUNCTION__);
#define SWLIB_INTERNAL(A)	swlib_internal_error(A, (char*)__FILE__, __LINE__, (char*)__FUNCTION__);
#define SWLIB_EXCEPTION(A)	swlib_exception(A, (char*)__FILE__, __LINE__, (char*)__FUNCTION__);

#define SWLIB_INFO(A) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "INFO: " A "\n")
#define SWLIB_INFO2(A,B) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "INFO: " A "\n", B)
#define SWLIB_INFO3(A,B,C) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "INFO: " A "\n", B, C)
#define SWLIB_INFO4(A,B,C,D) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "INFO: " A "\n", B, C, D)

#define SWLIB_WARN(A) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "WARNING: " A "\n")
#define SWLIB_WARN2(A,B) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "WARNING: " A "\n", B)
#define SWLIB_WARN3(A,B,C) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "WARNING: " A "\n", B, C)
#define SWLIB_WARN4(A,B,C,D) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "WARNING: " A "\n", B, C, D)

#define SWLIB_ERROR(A) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "ERROR: " A "\n")
#define SWLIB_ERROR2(A,B) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, (char*)__FILE__, __LINE__, (char*)__FUNCTION__, "ERROR: " A "\n", B)
#define SWLIB_ERROR3(A,B,C) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "ERROR: " A "\n", B, C)
#define SWLIB_ERROR4(A,B,C,D) swutil_cpp_doif_writef(1,1, (void*)NULL, STDERR_FILENO, __FILE__, __LINE__, __FUNCTION__, "ERROR: " A "\n", B, C, D)

#define SWBIS_STDIO_FNAME	"-"

#define SWBIS_PGM_SH		"sh"
#define SWBIS_PGM_BIN_SH	"/bin/sh"
#define SWBIS_PGM_BIN_KSH	"/bin/ksh"
#define SWBIS_PGM_BIN_MKSH	"/bin/mksh"
#define SWBIS_PGM_BIN_BASH	"/bin/bash"
#define SWBIS_PGM_XPG4_SH	"/usr/xpg4/bin/bash"
#define SWBIS_LOGGER_SIGTERM		SIGUSR2
#define SWBIS_TAINTED_CHARS		"'\"|*?;&<>`$[]"
#define SWBIS_WS_TAINTED_CHARS		"\n\r\t "
#define SWBIS_DEFAULT_CATALOG_USER		"root"
#define SWBIS_DEFAULT_CATALOG_GROUP		"root"
#define SWBIS_DEFAULT_DISTRIBUTION_USER		"root"
#define SWBIS_DEFAULT_DISTRIBUTION_GROUP	"root"

#define SWBIS_CATALOG_OWNER_ATT		"catalog_owner"
#define SWBIS_CATALOG_GROUP_ATT		"catalog_group"
#define SWBIS_CATALOG_MODE_ATT		"catalog_mode"

#define SWBIS_DIR_OWNER_ATT		"leading_dir_owner"
#define SWBIS_DIR_GROUP_ATT		"leading_dir_group"
#define SWBIS_DIR_MODE_ATT		"leading_dir_mode"
#define SWBIS_DIR_MODE_VAL		"0755"

#define SWBIS_DISTRIBUTION_OWNER_ATT		"owner"
#define SWBIS_DISTRIBUTION_GROUP_ATT		"group"
#define SWBIS_DISTRIBUTION_MODE_ATT		"mode"
#define SWBIS_DISTRIBUTION_MODE_VAL		"0755"

#define SWBIS_CATALOG_OWNER_VAL		SWBIS_DEFAULT_CATALOG_USER 
#define SWBIS_CATALOG_GROUP_VAL		SWBIS_DEFAULT_CATALOG_GROUP
#define SWBIS_CATALOG_MODE_VAL		"0755"
#define SWBIS_SIGNATURE_ATT		"signature"
#define SWBN_SIGNATURE	"signature"
#define SWBN_SIG_HEADER	"sig_header"

#define SWBIS_A_ustar	"ustar"
#define SWBIS_A_pax	"pax"
#define SWBIS_A_posix	"posix"
#define SWBIS_A_gnu	"gnu"

#define SW_ROOT_DIR "/"
#define SWBIS_SYNCT_EOF			CPIO_INBAND_EOA_FILENAME "\r\n\r\n"

#define SWBIS_PROGS_LAYOUT_VERSION 		"1.0"

#define SW_EXIT_SUCCESS	0
#define SW_EXIT_ONE	1	/* Medium not modified. */
#define SW_EXIT_TWO	2	/* Medium modified. */


#define SWC_FC_NOOP		0 	/* no op */
#define SWC_FC_NOAB		1	/* squash absolute path, then compare */
#define SWC_FC_NORE		2	/* resolve relative path to abs, then compare */


#define SWBIS_SC_OPEN_MAX	24	/* Max No. of open fd's if sysconf can't tell us */
#define SWBIS_MIN_FD_AVAIL	16	/* Minimum number of open fd to operate */

#include "strob.h"
#include "vplob.h"
#include "xformat.h"
#include "shcmd.h"
/* #include "swutillib.h" */
#include	<termios.h>
#include	<sys/ioctl.h>	/* 44BSD requires this too */
#include "swevents.h"

#define SYNCT_EOF_CONDITION_0	 0
#define SYNCT_EOF_CONDITION_1	 1
#define SYNCT_EOF_CONDITION_2	 2   /* mangled trailer */

typedef struct {
	int do_debugM;  /* normally not used, should be -1 */
	int debugfdM;   /* normally not used, should be -1 */ 
	unsigned char * bufM;
	unsigned char * tailM;
	char * trailer_startM;
	int countM;
        char * mtM;   /* TRAILER!!!\r\n\r\n string */
} SYNCT;

/* void 		* swlib_realloc(void * ptr, size_t newsize, size_t oldsize); */
void 		swlib_filemodestring (mode_t  mode, char * str);
int 		swlib_exec_filter(SHCMD ** cmd, int feed_pump_on, STROB * );
int 		swlib_read_amount(int suction_fd, intmax_t amount);
int 		swlib_pump(int suction_fd, int discharge_fd, int suctionPID, int * status, int *childGotStatus);
int 		swlib_pipe_pump(int ofd, int suction_fd);
intmax_t	swlib_pump_amount(int ofd, int suction_fd, intmax_t amount);
intmax_t	swlib_pump_amount6(int ofd, int uxfio_ifd, intmax_t amount, int adjunct_ofd);
intmax_t	swlib_pump_amount7(int ofd, int uxfio_ifd, intmax_t amount, int adjunct_ofd);
intmax_t	swlib_pump_amount8(int ofd, int uxfio_ifd, intmax_t amount, int adjunct_ofd, FILE_DIGS *);
int 		swlib_fork_to_make_unixfd(int uxfio_fd, sigset_t *, sigset_t *, int *);
int 		swlib_pump_impeller(int fd_to, int fd_from, int fd_plug);
intmax_t	swlib_i_pipe_pump(int suction_fd, int discharge_fd,
			intmax_t *amount, int adjunct_ofd, STROB * obuf, 
			ssize_t (*thisfpread)(int, void*, size_t));
int 		swlib_md5(int uxfio_fd, unsigned char *bindigest, int do_ascii);
intmax_t	swlib_md5_copy(int ifd, intmax_t amount, char * md5, int ofd);
int 		swlib_digests(int ifd, char * md5, char * sha1, char * size, char * sha512);
ssize_t 	swlib_safe_read(int fd, void * buf, size_t nbyte);
int 		swlib_md5_from_memblocks(void * thisisa, char *bindigest, unsigned char * iblock, int icount);
unsigned long 	swlib_bsd_sum_from_mem(unsigned char * data, size_t len);
unsigned long 	swlib_cksum(int uxfio_fd);
char * 		swlib_strdup(char * s);
void 		swlib_squash_double_slash(char *path);
void 		swlib_squash_trailing_slash(char *path);
void		swlib_squash_trailing_vnewline(char *path);
void 		swlib_squash_embedded_dot_slash(char *path);
void 		swlib_squash_all_dot_slash(char *path);
void 		swlib_squash_leading_dot_slash(char *path);
void		swlib_squash_leading_slash(char * name);
void		swlib_squash_all_leading_slash(char * name);
char *		swlib_return_relative_path(char * path);
char * 		swlib_return_no_leading(char *path);
void 		swlib_slashclean(char *path);
int 		swlib_resolve_path(char * ppath, int * depth, STROB * resolved_path);
int 		swlib_parse_args(int * argc, char *** argv, char * cmd);
int 		swlib_ansi_escaped_value(int, int * is_valid_escape);
int 		swlib_c701_escaped_value(int, int * is_valid_escape);
int		swlib_is_c701_escape(int);
int		swlib_is_ansi_escape(int);
int		swlib_writef(int fd, STROB * buffer, char * format, ...);
int		swlib_cat_line(STROB * line, int uxfio_fd);
int 		swlib_write_OLDcatalog_stream(XFORMAT * package, int ofd);
int 		swlib_write_OLDstorage_stream(XFORMAT * package, int ofd);
int 		swlib_write_catalog_stream(XFORMAT * package, int ofd);
int 		swlib_write_storage_stream(XFORMAT * package, int ofd);
char * 		swlib_strncpy(char * dst, const char * src, size_t n);
int		swlib_write_stats(char *filename, char * linkname_p,  struct stat * pstatbuf, int terse, \
				char * markup_prefix, int ofd, STROB * pbuffer);
void 		swlib_fatal(char * reason, char * file, int line, char * function);
void 		swlib_exception(char * reason, char * file, int line, char * function);
void 		swlib_assertion_fatal(int assertion_result, char * reason, char * file, int line, char * function);
void		swlib_swprog_assert(int assertion_result, int status, char * version, char * reason, char * file, int line, char * function);
int 		swlib_write_signing_files(XFORMAT * package, int ofd, int which_file /* 0=catalog  1=storage */, int do_adjunct_md5);
int		swlib_atoi(const char *nptr, int * result);
int 		swlib_atoi2(const char *nptr, char ** fp_endptr, int * result);
unsigned long int swlib_atoul(const char *nptr, int * result, char ** endptr);
int 		swlib_unix_dircat(STROB * dest, char * dirname);
int 		swlib_dir_compare(char * s1, char * s2, int compare_flag /* See SWC_FC_* */);
int		swlib_basename_compare(char * s1, char * s2);
void 		swlib_toggle_leading_slash(char * mode, char * name, int *pflag);
void		swlib_toggle_trailing_slash(char * mode, char * name, int *pflag);
void		swlib_toggle_all_trailing_slash(char * mode, char * name, int *pflag);
void 		swlib_squash_all_trailing_vnewline(char *path);
void 		swlib_toggle_leading_dotslash(char * mode, char * name, int *pflag);
void 		swlib_toggle_trailing_slashdot(char * mode, char * name, int *pflag);
char * 		swlib_dirname(STROB * dest, char * source);
char * 		swlib_basename(STROB * dest, char * source);
int		swlib_kill_all_pids(pid_t * pid, int num, int signo, int debug);
int 		swlib_wait_on_all_pids(pid_t * pid, int num, int * status, int flags, int debug);
int 		swlib_wait_on_all_pids_with_timeout(pid_t * pid, int num, int * status, int flags, int debug, int tm);
int		swlib_wait_on_pid_with_timeout(pid_t pid, int * status, int flags, int verbose_level, int fp_tmo);
int 		swlib_update_pid_status(pid_t keypid, int value, pid_t * pid, int * status, int len);
int		swlib_sha1(int uxfio_fd, char *digest);
int		sha_stream (int ifd, void *resblock);
int	 	sha_block(void * thisisa, void *resblock, unsigned char * iblock, int icount);
int	 	swlib_shcmd_output_fd(SHCMD ** cmdvec);
int	 	swlib_shcmd_output_strob(STROB * output, SHCMD ** cmdvec);
int 		swlib_vrealpath(char * virtual_pwd, char * ppath, int * depth, STROB * resolved_path);
int 		swlib_arfcopy(XFORMAT * package, SWPATH * swpath, int ofd, char * path, int preview, int * deadman);
int 		swlib_is_ascii_noaccept(char * s, char * acc, int minlen);
void 		swlib_is_sh_tainted_string_fatal(char * s);
int 		swlib_is_sh_tainted_string(char * s);
int 		swlib_audit_distribution(XFORMAT * xformat, int do_re_encode, int ofd, uintmax_t *, 
			int * deadman, void (*alarm_handler)(int));
mode_t		swlib_apply_mode_umask(char type, mode_t umask, mode_t mode);
int 		swlib_open_memfd(void);
int 		swlib_close_memfd(int fd);
int 		swlib_get_verbose_level(void);
void 		swlib_set_verbose_level(int n);
size_t		swlib__strlcpy(char * dst, const char * src, size_t siz);
int 		swlib_pad_amount(int fd, int amount);
int 		swlib_tr(char * src, int to, int from);
int 		swlib_process_hex_escapes(char * s1);
int 		swlib_compare_8859(char * s1, char * s2);
int 		swlib_test_verbose(struct swEvents *, int verbose_level, int swbis_event,
int		is_swi_event, int event_status, int is_posix_event);
int 		swlib_drop_root_privilege(void);
struct timespec ** swlib_get_io_req_p(void);
struct timespec * swlib_get_io_req(void);
int *		swlib_burst_adjust_p(void);
uintmax_t **	swlib_pump_get_ppstatbytes(void);
int 		swlib_vrelpath_compare(char * s1, char * s2, char * cwd);
void	 	swlib_add_trailing_slash(STROB *path);
int 		swlib_check_clean_relative_path(char * s);
int 		swlib_check_clean_path(char * s);
int 		swlib_check_clean_absolute_path(char * s);
int 		swlib_check_safe_path(char * s);
void		swlib_internal_error(char * reason, char * file, int line, char * function);
int 		swlib_expand_escapes(char **pa, int *newlen, char *src, STROB * ustore);
int 		swlib_unexpand_escapes(STROB * dest, char *src);
int 		swlib_unexpand_escapes2(char *src);
int 		swlib_rpmvercmp(const char * a, const char * b);
int 		swlib_check_legal_tag_value(char * s);
int 		swlib_altfnmatch(char * s1, char * s2);
int 		swlib_doif_writeap(int fd, STROB * buffer, char * format, va_list * pap);
int 		swlib_tee_to_file(char * filename, int source_fd, char * source_buf, int len, int do_append);
int 		swlib_synct_read(SYNCT *, int fd, void * buf);
int 		swlib_synct_suck(int ofd, int ifd);
void 		swlib_synct_delete(SYNCT * synct);
SYNCT * 	swlib_synct_create(void);
char * 		swlib_umaxtostr(uintmax_t i, STROB * buf);
char *		swlib_imaxtostr(intmax_t i, STROB * buf);
mode_t 		swlib_get_umask(void);
int 		swlib_ascii_text_fd_to_buf(STROB * pbuf, int ifd);
void 		swlib_apply_location(STROB * relocated_path, char * path, char * location, char * directory);
char * 		swlib_attribute_check_default(char * object, char * keyword, char * value);
int 		swlib_is_option_true(char * s);
void 		swlib_set_swbis_verbose_threshold(int s);
int 		swlib_squash_trailing_char(char *path, char ch);
int 		swlib_unix_dirtrunc(STROB * buf);
int 		swlib_unix_dirtrunc_n(STROB * buf, int n);
void 		swlib_squash_illegal_tag_chars(char * s);
intmax_t 	swlib_digs_copy(int ofd, int ifd, intmax_t count, FILE_DIGS * digs, int);
void 		std_quicksort (void *const pbase, size_t total_elems, size_t size, int  (*)(const void *, const void *));
mode_t		swlib_filestring_to_mode(char * str, int do_include_type);
void		swlib_append_synct_eof(STROB * buf);
pid_t		swlib_get_pax_header_pid(void);
void		swlib_set_pax_header_pid(pid_t pd);
char *		swlib_printable_safe_ascii(char * sf);

#endif
