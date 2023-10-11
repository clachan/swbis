/* taru.h
 */

#ifndef TARU_H_INC_20031101
#define TARU_H_INC_20031101

#include "swuser_config.h"
#include <sys/stat.h>

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#include "cpiohdr.h"
#include "tarhdr.h"
#include "filetypes.h"
#include "strob.h"
#include "defer.h"
#include "hllist.h"
#include "porinode.h"
#include "config_remains.h"
#include "etar.h"

#define TARU_AR_HEADER_SIZE  60  /* GNU ar format */
#define TARU_AR_SIZE_OFFSET  48  /* GNU ar format */


/*
	tar Header Block byte offsets
*/
#define THB_BO_name 		0
#define THB_BO_mode		100
#define THB_BO_uid		108
#define THB_BO_gid		116
#define THB_BO_size		124
#define THB_BO_mtime		136
#define THB_BO_chksum		148
#define THB_BO_typeflag		156
#define THB_BO_linkname		157
#define THB_BO_magic		257
#define THB_BO_version		263
#define THB_BO_uname		265
#define THB_BO_gname		297
#define THB_BO_devmajor		329
#define THB_BO_devminor		337
#define THB_BO_prefix		345


/*
	tar Header Block field lengths
*/

#define THB_FL_name 		100
#define THB_FL_mode		8
#define THB_FL_uid		8
#define THB_FL_gid		8
#define THB_FL_size		12
#define THB_FL_mtime		12
#define THB_FL_chksum		8
#define THB_FL_typeflag		1
#define THB_FL_linkname		100
#define THB_FL_magic		6
#define THB_FL_version		2
#define THB_FL_uname		32
#define THB_FL_gname		32
#define THB_FL_devmajor		8
#define THB_FL_devminor		8
#define THB_FL_prefix		155

#define XHDTYPE		'x'            /* Extended header */
#define XGLTYPE		'g'            /* Global extended header */
#define GNUTYPE_LONGNAME 	'L'
#define GNUTYPE_LONGLINK	'K'

#define PREFIX_FIELD_SIZE 155    /* Tar Header field size, do not change */
#define NAME_FIELD_SIZE   100    /* Tar Header field size, do not change */
#define SNAME_LEN         32     /* Tar Header field size, do not change */

#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__ || defined __EMX__ || defined __DJGPP__ 
  /* Win32, Cygwin, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#endif

#ifndef DIRECTORY_SEPARATOR
# define DIRECTORY_SEPARATOR '/'
#endif

#ifndef ISSLASH
# define ISSLASH(C) ((C) == DIRECTORY_SEPARATOR)
#endif

typedef struct {
	char idM;			/* Sanity ID  currently 'A' */
	STROB * headerM;       		/* Contains the archive header */
	int header_lengthM;    		/* Length of (*)->headerM bytes */
	int taru_tarheaderflagsM;	/* tar format personality flags */
	int do_record_headerM;		/* 1 or 0, 1 turns on capture of header bytes */
	STROB * u_name_bufferM;
	STROB * u_ent_bufferM;
	int nullfdM;
	/* int do_md5M; */
	STROB * md5bufM;
	int linkrecord_disableM;
	int preview_fdM;
	int preview_levelM;
	STROB * preview_bufferM;	/* Buffer used by taru_write_preview_line */
	STROB * read_bufferM;		/* Buffer used by taru_read_in_tar_header2 */
	int read_buffer_posM;	/* offset to First byte of invalid data (i.e. free space */
	STROB * exthdr_dataM;
	ETAR * etarM;           /* Easy tar object, have one open for performance sake*/
	STROB * localbufM;      /* utility object, open for performance sake*/
} TARU;


#define TARU_SYSDBNAME_LEN        120

#define TARU_FORMAT_TAR 0
#define TARU_FORMAT_CPIO 1

#define GNU_LONG_LINK "././@LongLink"

#define CPIO_INBAND_EOA_FILENAME "TRAILER!!!"
#define TARRECORDSIZE 512

#define TAR_MAX_7OCTAL 2097151    /* ``7777777 '' */
#define TAR_MAX_UID TAR_MAX_7OCTAL
#define TAR_MAX_GID  TAR_MAX_7OCTAL
#define TAR_MAX_SIZE 8589934591   /* ``77777777777 '' */
#define TAR_MAX_DEVICE TAR_MAX_7OCTAL


#define STAR_VENDOR	"SCHILY"
#define SWBIS_VENDOR	"SWBIS"
#define PAX_KW_path	"path"
#define PAX_KW_linkpath	"linkpath"
#define PAX_KW_gname	"gname"
#define PAX_KW_uname	"uname"
#define PAX_KW_atime	"atime"
#define PAX_KW_mtime	"mtime"
#define PAX_KW_ctime	"ctime"
#define PAX_KW_gid	"gid"
#define PAX_KW_uid	"uid"
#define PAX_KW_size	"size"
#define PAX_KW_devmajor	"devmajor"
#define PAX_KW_devminor	"devminor"


#define ISASCII(Char) 1

#define ISODIGIT(Char) \
  ((unsigned char) (Char) >= '0' && (unsigned char) (Char) <= '7')
#define ISSPACE(Char) (ISASCII (Char) && isspace ((int)Char))

#define CPIO_NEWASCII_MAGIC 070701
#define CPIO_CRCASCII_MAGIC 070702
#define CPIO_OLDASCII_MAGIC 070707

/* 22MAR2013   #define TARU_BUFSIZ BUFSIZ */
#define TARU_BUFSIZ 8192
#define TARU_BUFSIZ_RES (TARU_BUFSIZ + TARRECORDSIZE)

#define TARU_TAR_BE_LIKE_PAX		(1 << 0) /* i.e. utility, not format, off by default */
#define TARU_TAR_NUMERIC_UIDS		(1 << 1) /* off by default taru_tarheaderflagsM */
#define TARU_TAR_GNU_LONG_LINKS		(1 << 2) /* off by default taru_tarheaderflagsM */
#define TARU_TAR_GNU_GNUTAR		(1 << 3) /* Same as GNU tar 1.15.x */
#define TARU_TAR_GNU_BLOCKSIZE_B1	(1 << 4) /*                taru_tarheaderflagsM */
#define TARU_TAR_BE_LIKE_STAR		(1 << 5) /*                taru_tarheaderflagsM */
#define TARU_TAR_FRAGILE_FORMAT		(1 << 6) /* Do not recover from skipped bytes, bad cksum, etc. */
#define TARU_TAR_DO_STRIP_LEADING_SLASH	(1 << 7) /*                taru_tarheaderflagsM */
#define TARU_TAR_RETAIN_HEADER_IDS	(1 << 8) 
#define TARU_TAR_GNU_OLDGNUTAR		(1 << 9) /* Same as 1.13.25 default compilation settings */
#define TARU_TAR_GNU_OLDGNUPOSIX	(1 << 10) /* Same as 1.13.25 --posix for short names only */
#define TARU_TAR_NAMESIZE_99		(1 << 11) /* Use 99 char name size split like oldgnu format */
#define TARU_TAR_PAXEXTHDR		(1 << 12) /* Use Pax Extended Headers where needed */
#define TARU_TAR_PAXEXTHDR_MD		(1 << 13) /* Mandatory Pax Extended Headers */
#define TARU_TAR_ACLEXTHDR		(1 << 14) /* NOT Used in 1.13.1 */
#define TARU_TAR_SELINUXEXTHDR		(1 << 15) /* NOT Used in 1.13.1 */
#define TARU_TAR_NO_OVERFLOW		(1 << 16) /* Do not allow any fields to overflow */


#define TARU_PV_0	0  /* preview level, no preview */
#define TARU_PV_1	1  /* preview level, print filename */
#define TARU_PV_2	2  /* preview level, print tar listing */
#define TARU_PV_3	3  /* preview level, print verbose ad-hoc tar listing */


#define TARU_HE_LINKNAME_4  -4   /* write_header() return value for linkname too long  -4 */

/*
* These control the /etc/passwd /etc/group
* name,id lookup and mapping policy.
*/

#define TARU_C_BY_UNA		4 /* not available, not set */
#define TARU_C_BY_UNAME		3 /* get uid from uname. */
#define TARU_C_BY_UID		2 /* get uname from uid */
#define TARU_C_BY_USYS		1 /* Default existing policy */
#define TARU_C_BY_UNONE		0 /* no database lookups, use id and name as is */

#define TARU_C_BY_GNA		4 /* not available, not set */
#define TARU_C_BY_GNAME		3 /* get gid from gname */
#define TARU_C_BY_GID		2 /* get gname from gid */
#define TARU_C_BY_GSYS		1 /* Default existing policy */
#define TARU_C_BY_GNONE		0 /* no database lookups, use id and name as is */
#define TARU_C_DO_NUMERIC	"__SwbisInternalDoNumeric__"  /* FIXME, inband signalling */

enum archive_format {
	arf_unknown, arf_binary, arf_oldascii, arf_newascii, arf_crcascii,
	arf_tar, arf_ustar, arf_hpoldascii, arf_hpbinary, arf_filesystem
};

TARU * taru_create(void);

void taru_delete(TARU * taru);

void taru_clear_header_buffer(TARU * taru);

void taru_append_to_header_buffer(TARU * taru, char * buf, int len);

void taru_set_tarheader_flag(TARU * taru, int flag, int n);

ssize_t taru_tape_buffered_read(int fd, void * buf, size_t count);

intmax_t taru_pump_amount2(int discharge_fd, int suction_fd, intmax_t amount, int adjunct_ofd);

intmax_t taru_pump_amount3(int discharge_fd, int suction_fd, intmax_t amount, int adjunct_ofd);

intmax_t taru_pump_amount4(int discharge_fd, int suction_fd, intmax_t amount, int adjunct_ofd);

intmax_t taru_read_amount(int suction_fd, intmax_t amount);

void taru_mode_to_chars(mode_t v, char *p, size_t s, int termch);

intmax_t taru_hdr_get_filesize(struct new_cpio_header * file_hdr);

/*
 * Allocate/deallocate the header struct
 */
struct new_cpio_header *	taru_make_header(void);
void 				taru_free_header(struct new_cpio_header *h);
void 				taru_init_header(struct new_cpio_header * file_hdr);
void				taru_init_header_digs(struct new_cpio_header * file_hdr);

/*
 * Translate a format to another format 
 */
int 	taru_process_copy_out(TARU * taru, int input_fd, int output_fd, DEFER * defer,
		PORINODE * porindoe, enum archive_format archive_format, int ls_fd, int ls_verbose, intmax_t*, FILE_DIGS * digs);
int 	taru_format_translate(int ifd, int ofd, enum archive_format output_format);
int 	taru_process_copy_in(TARU * taru, int input_fd, int output_fd);


/*
 * Translate Metadata structures 
 */
int 	taru_filehdr2statbuf(struct stat *statbuf, struct new_cpio_header *file_hdr);
int 	taru_statbuf2filehdr(struct new_cpio_header *file_hdr, struct stat *statbuf, char * sourcefilename, char *filename, char *linkname);
int 	taru_filehdr2filehdr(struct new_cpio_header *file_hdr_dst, struct new_cpio_header *file_hdr_src);


/*
 * Accessor (Get) Functions 
 */

int taru_get_tar_filetype(mode_t mode);

int taru_get_cpio_filetype(mode_t mode);

int taru_get_uid_by_name(char *userkey, uid_t *puid);

int taru_get_tar_user_by_uid(uid_t uid, char * buf);

int taru_get_pax_user_by_uid(uid_t uid, STROB * buf);

int taru_get_gid_by_name(char *userkey, gid_t *guid);

int taru_get_tar_group_by_gid(gid_t gid, char * buf);

int taru_get_pax_group_by_gid(gid_t gid, STROB * buf);

int taru_safewrite(int fd, void *vbuf, size_t amount);

/* =========
 * Primary reading/writing functions.  
 */
intmax_t taru_write_archive_member
			(TARU * taru, char *filename, 
			struct stat *statbuf, 
			struct new_cpio_header *file_hdr0, 
			HLLIST * linkrec, 
			DEFER * defer, 
			PORINODE * porinode, 
			int uxfio_ofd, 
			int in_fd, 
			enum archive_format archive_format_out, 
			int tarheaderflags
			);


int taru_write_archive_trailer
			(
			TARU * taru, enum archive_format archive_format_out,
			int fd, 
			int block_size, 
			uintmax_t bytes_written, 
			int header_flags
			);


/*
 * Read Archive Header 
 */
int taru_read_header(
			TARU * taru, 
			struct new_cpio_header *file_hdr, 
			int in_des, 
			enum archive_format format, 
			int * eoa, 
			int flags);

int taru_read_in_header(
			TARU * taru, 
			struct new_cpio_header *file_hdr, 
			int in_des, 
			enum archive_format archive_format_in, 
			int * eoa, 
			int flags);

int taru_read_in_tar_header2(TARU * taru, struct new_cpio_header *file_hdr, int in_des, char *header_buffer, int * eoa, int flags, int);
int taru_read_in_old_ascii2(TARU * taru, struct new_cpio_header *file_hdr, int in_des, char *buf);
int taru_read_in_new_ascii(TARU * taru, struct new_cpio_header *file_hdr, int in_des, enum archive_format archive_format_in);


/*
 * Write Archive Header 
 */
int 	taru_write_out_header(
			TARU * taru, 
			struct new_cpio_header *file_hdr, int out_des, 
			enum archive_format archive_format_out, 
			char *header_buffer, 
			int tarheaderflags);

int 	taru_write_archive_member_header(
			TARU * taru, 
			struct stat *file_stat, 
			struct new_cpio_header *file_hdr0, 
			HLLIST * linkrecord, 
			DEFER * defer, 
			PORINODE * porinode,
			int out_file_des, 
			enum archive_format archive_format_out, 
			struct new_cpio_header *file_hdr_return, 
			int tarheaderflags);


int 	taru_write_out_tar_header2(
			TARU * taru, 
			struct new_cpio_header *file_hdr, 
			int out_des, 
			char *header_buffer, 
			char * user, 
			char * group, 
			int tarheaderflags);

int     taru_write_long_link_member(
			TARU * taru, 
			int out_file_des, 
			char * filename, 
			int gnu_long_type, /* K or L */
			unsigned long c_type,
			int tarheaderflags);

/*
 * write out member data 
 */
intmax_t
taru_write_archive_member_data(
		TARU * taru, 
		struct new_cpio_header *file_hdr0, 
		int out_file_des, 
		int input_fd, 
		int (*f)(int output_fd),
		enum archive_format archive_format_out, 
		int adjunct_ofd, FILE_DIGS * digs);

intmax_t
taru_write_archive_file_data(TARU * taru, 
				struct new_cpio_header *file_hdr0,
			      	int out_file_des, 
			      	int input_fd,
				int (*fout)(int),
			  	enum archive_format archive_format_out,
				int adjuct_ofd);

/*
 * Archive format padding 
 */
int 	taru_tape_pad_output(int out_file_des, intmax_t offset, enum archive_format archive_format);
int 	taru_tape_skip_padding(int in_file_des, uintmax_t offset, enum archive_format archive_format);



/*
 * Utility Functions 
 */
void taru_strip_leading_slash(char *name);

int taru_set_filetype_from_tartype(char tar_typeflag, mode_t * mode, char * filename);

char *	taru_dup_tar_name(void *tar_hdr);

char *	taru_dup_tar_linkname(void *tar_hdr);

unsigned long taru_tar_checksum(void *hdr);

unsigned long taru_read_for_crc_checksum (int in_file_des, int file_size, char * file_name);

int taru_is_tar_filename_too_long(char * filename, int tarheaderflags, int * do_long_link, int is_dir);
int taru_is_tar_linkname_too_long(char *name, int tarheaderflags, int * p_do_gnu_long_link);

long taru_from_oct(int digs, char *where);

void taru_to_oct(register long value, register int digits, register char *where);

int taru_otoul(char *s, unsigned long *n);
int taru_otoumax (char * s, uintmax_t *n);

int taru_datoul (char * s, unsigned long *n);

int taru_tarheader_check(char *buffer);

int taru_header_dump(struct new_cpio_header * file_hdr, FILE *fp);

char * taru_header_dump_string_s(struct new_cpio_header * file_hdr, char * prefix);

void taru_set_header_recording(TARU * taru, int n);

char * taru_get_recorded_header(TARU * taru, int * len);

int taru_split_name_ustar(struct tar_header *tar_hdr, char * name, int tar_iflags);

int taru_set_new_name(/* TARU * taru,*/ struct tar_header *, int len, char * name, int tarheaderflags);

int taru_set_new_linkname(TARU * taru, struct tar_header * fp_tar_hdr, char * name);

int taru_set_tar_header_sum(struct tar_header * fp_tar_hdr, int tar_header_flags);

int taru_print_tar_ls_list(STROB * buf, struct new_cpio_header * file_hdr, int vflag);

int taru_get_preview_fd(TARU * taru);

void taru_set_preview_fd(TARU * taru, int fd);

void taru_write_preview_line(TARU * taru, struct new_cpio_header * file_hdr);

void taru_set_preview_level(TARU * taru, int);

int taru_get_preview_level(TARU * taru);

int taru_set_tar_header_policy(TARU * taru, char * user_format, int * p_arf_format);

void taru_digs_delete(FILE_DIGS *);

FILE_DIGS * taru_digs_create(void);
void taru_digs_init(FILE_DIGS *, int, int);
void taru_digs_print(FILE_DIGS * digs, STROB * );
int taru_check_devno_for_system(struct new_cpio_header * file_hdr);
int taru_check_devno_for_tar(struct new_cpio_header * file_hdr);

int taru_read_pax_data_blocks(TARU *, int in_fd, char * source, int data_len,  ssize_t (*vfread) (int, void *, size_t));

void
taru_set_sysdb_nameid_by_policy(struct tar_header *tar_hdr, char * vselect, char * sysusername,
		char * tarbuf, STROB *paxbuf,
		struct new_cpio_header *file_hdr,
		int termch, int tar_iflags_numeric_uids);


void taru_set_filehdr_sysdb_nameid_by_policy(char * vselect, struct new_cpio_header *file_hdr, int termch, int tar_iflags_numeric_uids);

void taru_exatt_init(EXATT * hnew);
EXATT * taru_exatt_create(void);
int taru_exatt_parse_fqname(EXATT * exatt, char * attr, int attrlen);
int taru_read_all_ext_header_records (struct new_cpio_header * file_hdr, char * data, int len, int * errorcode);
void taru_exattlist_init(struct new_cpio_header * file_hdr);
EXATT * taru_exatt_get_free(struct new_cpio_header * file_hdr);
int taru_exatt_override_ustar (struct new_cpio_header * file_hdr, EXATT * exatt);
void taru_exatt_delete(EXATT * p);
void taru_exattlist_delete_all(struct new_cpio_header * file_hdr);
EXATT * taru_exattlist_get_free(struct new_cpio_header * file_hdr);
void taru_exattshow_debug(struct new_cpio_header * file_hdr);
/* int taru_ustar_field_size_check(struct new_cpio_header * file_hdr); */

int taru_fill_exthdr_needs_mask(struct new_cpio_header * file_hdr, int tarheaderflags,
		int is_pax_header_allowed, int be_verbose);


#endif
