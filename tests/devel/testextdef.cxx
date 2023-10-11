// testextdef.cxx

/*
 Copyright (C) 2001  James H. Lowe, Jr.  <jhlowe@acm.org>

*/
/*
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "swuser_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <string>

#include <typeinfo>
#include "xstream_config.h"
#include "usgetopt.h"
#include "swstruct.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swparser.h"
#include "swdefinitionfile.h"
#include "switer.h"
#include "swstructiter.h"
#include "swspsf.h"

#include "swparser_global.h"

extern "C" {
#include "swmain.h"
#include "swheader.h"
#include "swheaderline.h"
char	 		get_character(char ch);
int 			get_integer(int j);
double 			get_double(double x);
unsigned long 		get_unsigned(unsigned long uu);
signed char 		get_signedchar(int gch);
unsigned long 		get_ulong(unsigned long j);
void 			get_string(char *str);
long 			get_long(long int j);
static char *		uget_gets(char *s);
}
		
static int 
usage(char * progname) {
	fprintf (stderr, "invalid args.\n");
	fprintf (stderr, "Usage: %s {-A|-B|-C}\n", progname);
	fprintf (stderr, "       %s -D psf_filename [archivefilename]\n", progname);
	fprintf (stderr, "       %s -E psffilename_in_filesystem [archivefilename]\n", progname);
	fprintf (stderr, "       %s -F psffilename_in_package [archivefilename]\n", progname);
	exit (1);
}



static int
do_A(int ifd) {
	int len;
	swPSF *psf; 
	int swfilemapfd;
	//swDefinitionFile *psf; 
	swsPSF *swspsf; 

	psf=new swPSF("");
	psf->open_parser(ifd);
	len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);

	if (psf->generateDefinitions()) {
		cerr << "generateDefinitions returned error, exiting.\n";
		exit(4);
	}
	fprintf(stderr, "done.\n");
	psf->write_fd(STDOUT_FILENO);
	return 0;

	swspsf=new swsPSF(psf);
	
	swfilemapfd = swspsf->rewriteExpandedPsf();

	swspsf->generateStructuresFromParser(swfilemapfd);

	len=swspsf->iwrite(STDOUT_FILENO);
	cerr << len << endl;
	return 0;
}


static int
do_D(char * psf_filename, char * archive_filename)
{
	int len;
	int retval;
	int swfilemapfd;
	swPSF *psf;
	swsPSF 		 *swspsf;
	int ifd;


	//
	// The archive_filename may be a directory or a tar/cpio file.
	//
	psf=new swPSF("psf_id_name");
	assert(psf);


	//
	// Now, open the archive.
	//
	psf->xFormat_open_archive(archive_filename, UINFILE_DETECT_NATIVE,  0);

	//
	// Open the psf file which is in the archive.
	//
	ifd = psf->xFormat_u_open_file(psf_filename);
	if (ifd < 0) {
		cerr << "Error opening psf file.\n";
		delete psf;
		return 1;
	}

	//swlib_pipe_pump(STDOUT_FILENO, ifd);
	//return 0;

	//
	// Now open the parser on this file.
	//
	if (psf->open_parser(ifd)){
		cerr << "Error opening parser.\n";
	}

	//
	// Now run the parser. Equvalent to
	// The output of ``# swbisparse --psf -l 0 -n''
	//
  	len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (len < 0) {
		cerr << "Error in parser.\n";
		exit(3);
	}

	//
	// Close the PSF file. Its no longer needed.
	//
	psf->xFormat_u_close_file(ifd);

	//
	// The parsed output is now a flat memory file (identical
	// to the output obtained by running on the command line.
	//
	
	//psf->setSwExtendedDefStrictPosix_diraccess(0);


	//
	// Now, generate software definitions and expand the extended definitions.
	//
	if (psf->generateDefinitions()) {
		cerr << "generateDefinitions returned error, exiting.\n";
		exit(4);
	}

	//
	// Now make a swspsf object.
	//
	swspsf = new swsPSF(psf);
	assert(swspsf);
	
	//
	// Re-write a new parsed flat file into a
	// memory file indexed by swfilemapfd.
	//
	swfilemapfd = swspsf->rewriteExpandedPsf();

	//
	// The output of ``swbisparse --psf -l 0 -n''
	// is in the descriptor pointed at by the swfilemap index.
	//
	// Now generate the object tree from the flat file output.
	//
	retval = swspsf->generateStructuresFromParser(swfilemapfd);

	if (retval != 0) {
		cerr << "generateStructuresFromParser returned error, exiting.\n";
		exit(5);
	}
	
	swspsf->iwrite(STDOUT_FILENO);	

	delete swspsf;
	// Core dump: delete psf;
	return 0;
}


static int
do_E(int psf_ifd, int package_ifd)
{
	int rc;
	int len;
	int retval;
	int swfilemapfd;
	swPSF *psf;
	swsPSF 		 *swspsf;

	psf=new swPSF("optional_psf_id_name");
	assert(psf);

	rc = psf->xFormat_open_archive(package_ifd, UINFILE_DETECT_NATIVE,  0);
	if (rc) {
		cerr << "Error opening archive.\n";
		exit(3);
	}


	rc = psf->open_parser(psf_ifd);
	if (rc) {
		cerr << "Error opening parser.\n";
		exit(3);
	}

  	len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (len < 0){
		cerr << "Error in parser.\n";
		exit(3);
	}

	//psf->setSwExtendedDefStrictPosix_diraccess(0);
	
	if (psf->generateDefinitions()) {
		cerr << "generateDefinitions returned error, exiting.\n";
		exit(4);
	}

	swspsf = new swsPSF(psf);
	assert(swspsf);
	
	swfilemapfd = swspsf->rewriteExpandedPsf();

	retval = swspsf->generateStructuresFromParser(swfilemapfd);
	if (retval != 0) {
		cerr << "generateStructuresFromParser returned error, exiting.\n";
		exit(5);
	}
	
	swspsf->iwrite(STDOUT_FILENO);	

	delete swspsf;
	return 0;
}


static int
do_B(int ifd)
{
	int len;
	swPSF *psf;
	psf=new swPSF("");
	if (psf->open_parser(ifd)){
		cerr << "Error opening parser.\n";
	}
  	len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (len < 0){
		cerr << "Error in do_B 000.1.\n";
		exit(3);
	}
	if (uxfio_lseek(psf->get_mem_fd(), (off_t)0, SEEK_SET) < 0) {
		cerr << "Error in do_B 000.2.\n";
		exit(3);
	}
	swlib_pipe_pump(STDOUT_FILENO, psf->get_mem_fd());
	return 0;
}


static int
do_C(int ifd)
{
	int len;
	swPSF *psf;
	psf=new swPSF("");
	if (psf->open_parser(ifd, STDOUT_FILENO)){
		cerr << "Error opening parser.\n";
	}
  	len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);
	if (len < 0){
		cerr << "Error in do_C 000.1.\n";
		exit(3);
	}
	return 0;
}


int 
main(int argc, char **argv)
{
	int psf_ifd;
	int ifd;
	int retval=32000;
	int testnumber=0;
	int c=0;
	char  *progname;
	char * psffilename = NULL;

	progname = argv[0];
	if (argc < 2) { 
		usage(progname);
		exit(1); 
	}
  
	while (1)
	{
             int option_index = 0;
             static struct option long_options[] =
             {
               {"ext-psf", 0, 0, 'A'},
               {"testB", 0, 0, 'B'},
               {"testC", 0, 0, 'C'},
               {"testD", 1, 0, 'D'},
               {"testE", 1, 0, 'E'},
               {"testF", 1, 0, 'F'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "ABCD:E:",
                        long_options, &option_index);
             if (c == -1)
                 break;

		switch (c)
		{
		case 'A':
			testnumber='A';
			break;
		case 'B':
			testnumber='B';
			break;
		case 'C':
			testnumber='C';
			break;
		case 'D':
			psffilename = strdup(optarg);
			testnumber='D';
			break;
		case 'E':
			psffilename = strdup(optarg);
			testnumber='E';
			if (strcmp(psffilename, "-") == 0) {
				psf_ifd = STDIN_FILENO;
			} else {
				psf_ifd = open(psffilename, O_RDONLY);
				if (ifd <  0) {
					perror("open");
					exit(2);
				}
			}
			break;
		case 'F':
				psf_ifd = -112;
			break;
		default:
			usage(progname);
			break;
		}
	}

	if (optind >= argc && testnumber == 'E' && psf_ifd == STDIN_FILENO) {
		exit(14);
	}

	if (optind < argc && testnumber != 'D' ) {
		ifd = open(argv[optind], O_RDONLY);
		if (ifd <  0) {
			fprintf(stderr,"open() error\n");
			exit(2);
		}
	} else {
		ifd = STDIN_FILENO;
	}

	switch(testnumber) {
		case 'A':
			//retval = do_swspsf(ifd);
			retval = do_A(ifd);
			break;
		case 'B':
			retval = do_B(ifd);
			break;
		case 'C':
			retval = do_C(ifd);
			break;
		case 'D':
			if (optind >= argc) {
				retval = do_D(psffilename, "-");
			} else {
				retval = do_D(psffilename, argv[optind]);
			}
			break;
		case 'E':
			retval = do_E(psf_ifd, ifd);
			break;
		case 'F':
			break;
		default:
                 	fprintf(stderr, "invalid args\n");
			exit(999);
			break;
	}
	close(ifd);
	exit(retval);
}
