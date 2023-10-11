#ifndef ls_list_h_20030523
#define ls_list_h_20030523

#include "swuser_config.h"
#include <tar.h>
#include "strob.h"
#include "cpiohdr.h"

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

#define LS_LIST_LINKNAME_MARK	"[linkname="
#define LS_LIST_NAME_LENGTH	"name_length"
#define LS_LIST_LINKNAME_LENGTH	"linkname_length"

#define LS_LIST_VERBOSE_L0		(LS_LIST_VERBOSE_OFF)
#define LS_LIST_VERBOSE_L1		(LS_LIST_VERBOSE_NORMAL|LS_LIST_VERBOSE_WITH_ALL_DATES|LS_LIST_VERBOSE_WITH_SYSTEM_NAMES)
#define LS_LIST_VERBOSE_L2		(LS_LIST_VERBOSE_NORMAL|LS_LIST_VERBOSE_WITH_NAMES|LS_LIST_VERBOSE_WITH_ALL_DATES|LS_LIST_VERBOSE_WITH_SYSTEM_NAMES|LS_LIST_VERBOSE_WITH_SYSTEM_IDS|LS_LIST_VERBOSE_WITH_SIZE)

#define LS_LIST_VERBOSE_COMP2		(LS_LIST_VERBOSE_ALTER_FORM|LS_LIST_VERBOSE_NORMAL|LS_LIST_VERBOSE_STRIPSLASH|LS_LIST_VERBOSE_WITH_REG_DATES|LS_LIST_VERBOSE_WITH_MD5|LS_LIST_VERBOSE_WITH_SHA1|LS_LIST_VERBOSE_WITH_SHA512|LS_LIST_VERBOSE_WITH_SIZE)

#define LS_LIST_VERBOSE_OFF		(1 << 0)  /* filenames only */
#define LS_LIST_VERBOSE_NORMAL		(1 << 1) 
#define LS_LIST_VERBOSE_WITH_NAMES	(1 << 2)  /* with user and group names */
#define LS_LIST_VERBOSE_WITH_DIGS	(1 << 3)  /* Deprecated */ 
#define LS_LIST_VERBOSE_WITH_MD5	(1 << 4) 
#define LS_LIST_VERBOSE_WITH_SHA1	(1 << 5) 
#define LS_LIST_VERBOSE_WITH_SHA512	(1 << 6) 
#define LS_LIST_VERBOSE_STRIPSLASH	(1 << 7) 
#define LS_LIST_VERBOSE_WITH_ALL_DATES	(1 << 8)  /* dates included for every file */
#define LS_LIST_VERBOSE_WITH_REG_DATES	(1 << 9)  /* dates only included for regular file */
#define LS_LIST_VERBOSE_WITH_SYSTEM_IDS (1 << 10) /* list uid and gid */
#define LS_LIST_VERBOSE_WITH_SYSTEM_NAMES (1 << 11) /* list uname and gname */
#define LS_LIST_VERBOSE_ALTER_FORM	(1 << 12)  /* used for verification comparison */
#define LS_LIST_VERBOSE_WITH_SIZE	(1 << 13)
#define LS_LIST_VERBOSE_PREPEND_DOTSLASH (1 << 14)
#define LS_LIST_VERBOSE_WITHOUT_OWNERS 	(1 << 15)
#define LS_LIST_VERBOSE_WITHOUT_PERMISSIONS (1 << 16)
#define LS_LIST_VERBOSE_LINKNAME_PLAIN (1 << 17)

void ls_list_safe_print_to_strob(char *str, STROB * fp, int do_prepend_dotslash);

void ls_list(char * name, char * ln_name, struct new_cpio_header * sbp,
	time_t now, FILE *fp, char * uname, char * gname, int type, int vflag);
void ls_list_to_string(char * name, char * ln_name, struct new_cpio_header * file_hdr,
	time_t now, STROB * buf, char * uname, char * gname, int type, int vflag);
void swbis_strmode (mode_t mode, char *p);
int ls_list_get_encoding_flag(void);
void ls_list_set_encoding_flag(int flag);
int ls_list_set_encoding_by_lang(void);

#endif
