/* rpmpsf.h 
 */

#ifndef RPMPSF_20000909_H
#define RPMPSF_20000909_H

#include "swuser_config.h"
#ifdef HAVE_RPM_RPMIO_H
#include "um_rpmio.h"
#else
#define FD_t int
#endif

#ifdef USE_WITH_RPM
#include "um_header.h"
#endif

#include "strob.h"
#include "swheader.h"
#include "topsf.h"


#define RPMPSF_FILE_PSF_BIN 0
#define RPMPSF_FILE_PSF_SRC 1
#define RPMPSF_FILE_PSF 5
#define RPMPSF_FILE_INDEX 10
#define RPMPSF_FILE_INFO 20

#define SWPM_SRCFILESET_SOURCE 0
#define SWPM_SRCFILESET_PATCH 1
#define SWPM_SRCFILESET_BUILD_CONTROL 2

/*  These are now in swattributes.h */
/*
#define RPMPSF_DEF_DFILES		"rpm_dfiles"
#define RPMPSF_FILESET_SOURCE		"sources"
#define RPMPSF_FILESET_BUILD		"build_control"
#define RPMPSF_FILESET_PATCHES		"patches"
#define RPMPSF_BIN_FILESET_TAG		"binpackage"
*/

#ifdef RPMPSF_RPM_VERSION_403
#include "rpmpsf_rpmi4.0.3.h"
#elif defined  RPMPSF_RPM_VERSION_42
#include "rpmpsf_rpmi4.1.h"
#elif defined  RPMPSF_RPM_VERSION_405
#include "rpmpsf_rpmi4.0.3.h"
#elif defined  RPMPSF_RPM_VERSION_402
#include "rpmpsf_rpmi4.0.2.h"
#elif defined  RPMPSF_RPM_VERSION_404
#include "rpmpsf_rpmi4.0.3.h"
#elif defined  RPMPSF_RPM_VERSION_41
#include "rpmpsf_rpmi4.1.h"
#else
#include "rpmpsf_rpmi2.5.5.h"
#endif



/* -------------- */
int fnmatch (const char * , const char *, int);

int rpmpsf_write_psf (TOPSF *topsf, int uxfio_ofd);
int rpmpsf_write_info (TOPSF *topsf, int uxfio_ofd);
int rpmpsf_write_beautify_psf (TOPSF *topsf, int uxfio_ofd);
int rpmpsf_write_beautify_info (TOPSF *topsf, int uxfio_ofd);

int rpmpsf_write_host (TOPSF *topsf, int uxfio_ofd, int filetype);
int rpmpsf_write_distribution (TOPSF *topsf, int uxfio_ofd, int filetype, char * control_directory); 
int rpmpsf_write_vendor (TOPSF *topsf, int uxfio_ofd, int filetype); 
int rpmpsf_write_product (TOPSF *topsf, int uxfio_ofd, STROB * all_filesets, int filetype); 
int rpmpsf_write_filesets (TOPSF *topsf, int uxfio_ofd, STROB * all_filesets, int filetype, int swdef_filetype); 
int rpmpsf_write_fileset (TOPSF *topsf, int uxfio_ofd, char * fileset_tag, char * dir, int filetype, int swdef_filetype); 
int rpmpsf_write_rpmtagvalue (TOPSF *topsf, char * keyword, int tagnumber, int index, int * count, char * defstring, int uxfio_ofd);
int rpmpsf_write_rpm_attribute (char * kw, char * value, int rpmtag, int index, Header h, char * defstring, int uxfio_ofd);
int rpmpsf_write_multilang_attribute (char * kw, int rpmtag, Header h, int uxfio_ofd);
char * rpmpsf_list_iterate ( char ** buf,  char * str , char * delim ); 
int rpmpsf_write_product_control_file (TOPSF *topsf, int uxfio_ofd, char * tag, char * rpmtag, int rpm_prog, int rpm_progtag ); 
int rpmpsf_write_product_control_files (TOPSF *topsf, int uxfio_ofd); 
int rpmpsf_write_requisites (TOPSF *topsf, int NAME_TAG, int VERSION_TAG, int FLAGS_TAG, int uxfio_ofd, char *);
int rpmpsf_get_rpmtagvalue (Header h, int tagnumber, int index, int * count, STROB * strb); 
int headerDumpSw  (TOPSF *topsf, FILE *f, int flags, struct_MI_headerTagTableEntry tags); 
int rpmpsf_write_changelogs (TOPSF *topsf, int uxfio_ofd);
int rpmpsf_write_bundle (TOPSF *topsf, int uxfio_ofd, int filetype); 
int rpmpsf_write_category (TOPSF *topsf, int uxfio_ofd, int filetype); 
int rpmpsf_write_file_psf  (TOPSF *topsf, int arrayindex, int uxfio_ofd, char * source_prefix, STROB * strb); 
int rpmpsf_write_psf_files (TOPSF *topsf, int uxfio_ofd, int typeflag, char * source_prefix, STROB * strb); 
int rpmpsf_write_value (char * value, int uxfio_ofd, int value_length);
int rpmpsf_write_keyword (char * keyword, int level, int attribute_type, int uxfio_fd);
int rpmpsf_write_attribute (char * keyword, char * value, int level, int len, int attribute_type, int uxfio_fd);
char * rpmpsf_make_package_prefix (TOPSF * topsf, char * cwdir);
int rpmpsf_get_psf_size(TOPSF * topsf);
int rpm_tagstring_to_tagno (char * tagstring, struct_MI_headerTagTableEntry tags);
int rpmpsf_write_out_tag(TOPSF *topsf, int ofd);
int rpmpsf_get_rpmtag_length(TOPSF *topsf, char * tag);
int rpmpsf_get_rpm_filenames_tagvalue(Header h, int index, int *pcount, STROB * strb);
int rpmpsf_get_index_by_name(TOPSF * topsf, char * filename);

#endif
