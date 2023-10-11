/* debpsf.h  --  deb-to-PSF translation
 */

#ifndef DEBPSF_20050509_H
#define DEBPSF_20050509_H

#include "taru.h"
#include "strob.h"
#include "tarhdr.h"

#define DEBPSF_PSF_DATA_DIR	"data"
#define DEBPSF_PSF_CONTROL_DIR	"control"

#define DEBPSF_DATA_PREFIX	"./"
#define DEBPSF_CONTROL_PREFIX	"./"
/* #define DEBPSF_FILE_TEMPLATES	"templates" */
#define DEBPSF_FILE_CONTROL	"control"
#define DEBPSF_FILE_MD5SUM	"md5sums"
#define DEBPSF_FILE_POSTINSTALL	"postinst"
#define DEBPSF_FILE_PREINSTALL	"preinst"
#define DEBPSF_FILE_PREREMOVE	"prerm"
#define DEBPSF_FILE_POSTREMOVE	"postrm"
#define DEBPSF_FILE_CONFFILES	"conffiles"
#define DEBPSF_FILE_SHLIBS	"shlibs"

#define DEBPSF_ATTR_Package		"Package"
#define DEBPSF_ATTR_Version		"Version"
#define DEBPSF_ATTR_Architecture	"Architecture"
#define DEBPSF_ATTR_Maintainer		"Maintainer"
#define DEBPSF_ATTR_Description 	"Description"

typedef struct {
	char * nameM;
	struct tar_header * hdrM;
	char * dataM;
	void * nextM; /* Next DEB_CONTROL_MEMBER */
} DEB_CONTROL_MEMBER;

typedef struct {
	STROB * Package;
	STROB * Version;
	STROB * Version_epoch;
	STROB * Version_revision;
	STROB * Version_release;
	STROB * Architecture;
	STROB * Maintainer;
	STROB * Description;
} DEB_ATTRIBUTES;

typedef struct {
	DEB_CONTROL_MEMBER * headM;
	void * topsfM;
	int header_fdM;
	int control_fdM;
	int data_fdM;
	int source_data_fdM;
	DEB_ATTRIBUTES * daM;
} DEBPSF;

DEBPSF * debpsf_create(void);
void     debpsf_delete(DEBPSF * debpsf);
int debpsf_open(DEBPSF * d, void * v_topsf);
/*
DEB_CONTROL_MEMBER * debpsf_ctm_create(void);
void debpsf_ctm_delete(void);
*/

void debpsf_deb_attributes_init(DEB_ATTRIBUTES * da);
int debpsf_write_psf_buf(void * topsf, STROB * psf);
int debpsf_write_psf(void * topsf, int fd_out);
int debpsf_close(DEBPSF * dp);
#endif
