#ifndef swgpg_h_2007
#define swgpg_h_2007

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include "strar.h"
#include "shcmd.h"


#define SWGPG_PASSPHRASE_LENGTH		240
#define SWGPG_SWP_PASS_AGENT		"agent"   
#define SWGPG_SWP_PASS_ENV		"env"    

#define ARMORED_SIGLEN		1024	/* signature length */

#define SWGPG_FIFO_DIR		"/tmp"
#define SWGPG_FIFO_PFX		"sigfifo"
#define SWGPG_FIFO_NAME		"sig"
#define GPG_STATUS_PREFIX 	"[GNUPG:] "

#define SWGPG_GPG_BIN		"gpg"
#define SWGPG_GPG2_BIN		"gpg2"
#define SWGPG_PGP5_BIN		"pgps"
#define SWGPG_PGP26_BIN		"pgp"

#define GPG_STATUS_ERRSIG	"ERRSIG"
#define GPG_STATUS_NO_PUBKEY	"NO_PUBKEY"
#define GPG_STATUS_GOODSIG	"GOODSIG"
#define GPG_STATUS_BADSIG	"BADSIG"
#define GPG_STATUS_EXPSIG	"EXPSIG"
#define	GPG_STATUS_NODATA	"NODATA"

#define SWGPG_SIG_VALID		0
#define SWGPG_SIG_NOT_VALID	1
#define SWGPG_SIG_NO_PUBKEY	2
#define SWGPG_SIG_NODATA	3
#define SWGPG_SIG_ERROR		4  /* swbis internal error */

typedef struct {
	char * gpg_prognameM;		/* name of verifier program */
	STRAR * list_of_sigsM;		/* contents of sig file in installed catalog */
	STRAR * list_of_sig_namesM;	/* name of sig file in installed catalog */
	STRAR * list_of_status_blobsM;  /* status lines result of gpg's verification attempt */
	STRAR * list_of_logger_blobsM;  /* stderr lines result of gpg's verification attempt */
	STROB * status_arrayM;		/* List of unsigned char values for each sig's status */
} SWGPG_VALIDATE;

SWGPG_VALIDATE * swgpg_create(void);
void swgpg_delete(SWGPG_VALIDATE*);
void swgpg_reset(SWGPG_VALIDATE*);
int swgpg_determine_signature_status(char * gpg_status_lines, int which_sig);
int swgpg_run_checksig2(char * sigfilename, char * thisprog,
		char * filearg, char * gpg_prog, int uverbose,
		char * which_sig_arg);
char * swgpg_create_fifo(STROB * buf);
int swgpg_remove_fifo(void);
int swgpg_run_gpg_verify(SWGPG_VALIDATE * swgpg, int signed_bytes_fd, char * signature, int uverbose, STROB * status_fd_contents);
void swgpg_reset(SWGPG_VALIDATE * swgpg);
int swgpg_get_status(SWGPG_VALIDATE * w, int index);
void swgpg_set_status(SWGPG_VALIDATE * w, int index, int value);
int swgpg_get_number_of_sigs(SWGPG_VALIDATE*);
void swgpg_set_status_array(SWGPG_VALIDATE*);
int swgpg_show_all_signatures(SWGPG_VALIDATE*, int fd);
int swgpg_show(SWGPG_VALIDATE * w, int index, int sig_fd, int status_fd, int logger_fd);
int swgpg_disentangle_status_lines(SWGPG_VALIDATE * w, char * gpg_output_lines);
void swgpg_set_passphrase_fd(int fd);
int swgpg_get_passphrase_fd(void);
void swgpg_init_passphrase_fd(void);
char * swgpg_get_package_signature( SHCMD * sigcmd, int * statusp, char * wopt_passphrase_fd, char * passfile,
		int pkg_fd, int do_dummy_sign, char * g_passphrase);
SHCMD * swgpg_get_package_signature_command( char * signer, char * gpg_name, char * gpg_path,
		char * passphrase_fd);

#endif
