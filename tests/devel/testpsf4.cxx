#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "xstream_config.h"
#include "swparser.h"
#include "swpsf.h"
#include "swdefinitionfile.h"
#include "swparser_global.h"
#include "swexcat.h"
#include "swexdistribution.h"
#include "swmain.h"

extern "C" {
#include "usgetopt.h"
#include "ugetopt_help.h"
}

static int foo(int fd) { return 0; }



static void
usage(char * pgm, struct option  lop[], struct ugetopt_option_desc helpdesc[])
{
	fprintf(stderr, "Usage: %s [-p [@]psffilename] {-a name |-d name}\n", pgm);
	fprintf(stderr, "        where @psffilename indicates PSF is sourced in the archive\n");
	fprintf(stderr, "Reads a PSF, expands extended definition and writes to stdout\n");
	fprintf(stderr, "the resulting PSF file.\n");
	ugetopt_print_help(stderr, pgm, lop, helpdesc);
	exit(1);
}


int
main (int argc, char ** argv)
{
	int c;
	int verbose = 0;
	int error_code = 0;
	int do_exdist_test = 0;
	int format = arf_ustar;
	int write_index = 0;
	int write_index_tar = 0;
	char * archive_name = NULL;
	char * dir_name = NULL;
	char * psf_name = NULL;
	int psf_fd = STDIN_FILENO;
	int do_cksum = 0;
	int do_md5sum = 0;
	swPSF *psf;
	int option_index = 0;
      	SWVARFS * swvarfs; 
	swExCat * swexdist;
      

       static struct option long_options[] =
             {
               {"archive", 1, 0, 'a'},
               {"directory", 1, 0, 'd'},
               {"format", 1, 0, 'H'},
               {"write-index", 0, 0, 'I'},
               {"write-index-tar", 0, 0, 'T'},
               {"with-cksum", 0, 0, 'c'},
               {"with-md5", 0, 0, 'm'},
               {"psf", 1, 0, 'p'},
               {"test-exdist", 0, 0, 'x'},
               {"verbose", 0, 0, 'v'},
               {"help", 0, 0, '\007'},
               {0, 0, 0, 0}
             };
       
       static struct ugetopt_option_desc help_desc[] =
             {
               {"archive", "archive_name", "Open package in serial access archive file."},
               {"directory", "dirname", "Open package in directory"},
               {"format", "FORMAT", "format: ustar, crc, newc, odc"},
               {"write-index", "", "write the index file"},
               {"write-index-tar", "", "write the index file in a tar archive"},
               {"with-cksum", "", "add cksum"},
               {"with-md5sum", "", "add md5sum"},
               {"psf", "psf_filename", "Use psf_filename"},
               {"test-exdist", "", "Contruct a swExCat object and use to write out a PSF."},
               {"verbose", "", "Print debug dump string."},
               {"help", "", "Show this help"},
               {0, 0, 0}
             };
	
	while (1)
           {

             c = ugetopt_long (argc, argv, "vH:TIcmxa:d:p:", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
		case 'v':
			verbose = 1;
			break;
		case 'x':
			do_exdist_test = 1;
			break;
		case 'c':
			do_cksum = 1;
			break;
		case 'H':
			if (!strcmp(optarg,"ustar")) {
  				format=arf_ustar;
			} else if (!strcmp(optarg,"newc")) {
  				format=arf_newascii;
			} else if (!strcmp(optarg,"crc")) {
  				format=arf_crcascii;
			} else if (!strcmp(optarg,"odc")) {
  				format=arf_oldascii;
			} else {
				fprintf (stderr,"unrecognized format: %s\n", optarg);
				exit(2);
			}
			break;
		case 'I':
			write_index = 1;
			break;
		case 'T':
			write_index_tar = 1;
			break;
		case 'm':
			do_md5sum = 1;
			break;
		case 'a':
			archive_name = strdup(optarg);
			break;
		case 'd':
			dir_name = strdup(optarg);
			break;
		case 'p':
			psf_name = strdup(optarg);
			/*
			psf_fd = open(psf_name, O_RDONLY, 0);
			if (psf_fd < 0) {
				fprintf(stderr, "error opening %s : %s\n", psf_name, strerror(errno));
			}
			*/
			break;
               case '\007':
			usage(argv[0], long_options, help_desc);
		 	break;
               default:
		 	exit(1);
                 break;
               }
	}

	psf=new swPSF("");

	if (!psf) exit(1);

	psf->set_cksum_creation(do_cksum);
	psf->set_digests1_creation(do_md5sum);

	if (archive_name == NULL && dir_name == NULL) {
		fprintf(stderr, "invalid usage\n");
		usage(argv[0], long_options, help_desc);
	}
	if (archive_name) {
		if (strcmp(archive_name, "-") == 0) {
			psf->open_serial_archive(STDIN_FILENO, 
							UINFILE_DETECT_FORCEUXFIOFD |
							UINFILE_DETECT_NATIVE | 
							UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM);
		} else {
			if (psf_fd == STDIN_FILENO) {
				fprintf(stderr, "invalid usage\n");
				usage(argv[0], long_options, help_desc);
			}
			psf->open_serial_archive(archive_name, UINFILE_DETECT_NATIVE);
		}
	} else if (dir_name) {
		psf->open_directory_archive(dir_name);
	}


	swvarfs = static_cast<SWVARFS*>(psf->xFormat_get_swvarfs());
	psf->get_swextdef()->set_swvarfs(swvarfs);
	
	if (psf_name) {
		if (*psf_name != '@') {
			psf_fd = open(psf_name, O_RDONLY, 0);
		} else if (*psf_name == '@' && (archive_name || dir_name)) {
			psf_name ++;
			psf_fd = swvarfs_u_open(swvarfs, psf_name);
		} else {
			usage(argv[0], long_options, help_desc);
			exit(2);
		}
	}
	if (psf_fd < 0) {
		fprintf(stderr, "error opening %s\n", psf_name);
		exit(3);
	}


	psf->open_parser(psf_fd);
	
	if (psf->run_parser(0, SWPARSE_FORM_MKUP_LEN) < 0 ) {
		exit(1);
  	}	

	if (psf->generateDefinitions()) {
		exit(2);
	}

	if (psf_fd != STDIN_FILENO) 
		close(psf_fd);

	if (do_exdist_test) {
		swexdist = swExCat::constructSwExDist(psf, &error_code, NULL);
		if (swexdist && error_code == 0) {
			if (write_index) {
				swexdist->registerWithGlobalIndex();
				//cerr << swexdist->getGlobalIndex()->swdeffile_linki_write_fd(STDOUT_FILENO) << "\n" ;
				fprintf(stderr, "%d\n", swexdist->getGlobalIndex()->swdeffile_linki_write_fd(STDOUT_FILENO));
			} else if (write_index_tar) {
				int (*foo_write)(int);
				int (swDefinitionFile::*def_write)(int);
				swDefinitionFile * swdef;

				swexdist->registerWithGlobalIndex();
				foo_write = foo;
				swdef = swexdist->getGlobalIndex();
				
				//swExStruct_i::setEmitObject(swdef);

				def_write = &swDefinitionFile::swdeffile_linki_write_fd;
				//swExStruct_i::bindEmitMethod(swdef);
				//def_write = &swExStruct_i:runEmitMethod;
	
				swdef->xFormat_set_output_format(format);
				swdef->xFormat_set_ofd(STDOUT_FILENO);
				swdef->xFormat_set_false_inodes(1);
				swdef->xFormat_set_mode(0);			// Zero out the mode.
				swdef->xFormat_set_perms(0600);			// Set the permissions.
		                swdef->xFormat_set_filetype_from_tartype(REGTYPE);

				swdef->xFormat_write_file(def_write);
				swdef->xFormat_write_trailer();

			} else {
				fprintf(stderr, "%d\n", swexdist->write_fd(STDOUT_FILENO));
				//cerr << swexdist->write_fd(STDOUT_FILENO) << "\n" ;
			}
		}

		//D if (verbose)
		//D 	fprintf(stderr, "%s\n", swExDistribution::swexdistribution_dump_string_s(static_cast<swExDistribution*>(swexdist), ""));

		
		if (error_code) {
			fprintf(stderr, "package construction error.\n");
		}
		delete swexdist;
	} else {
		//cerr << psf->write_fd(STDOUT_FILENO) << "\n" ;
		fprintf(stderr, "%d\n",  psf->write_fd(STDOUT_FILENO));
	}

	delete psf;

	//
	// Note:   If exit(0) is called instead of _exit(0) a extraneous space ' ' is
	// output is some usages of this program.  I suspect a bug in the integration
	// of the ostream.h, the standard library and posix descriptors with regard
	// the exit() call.
	//
	// The swstructlib.a and swsupplib.a libraries use POSIX descriptors exclusively.
	//
	// However:
	// A better solution is to Posix close() the file descriptor explicitly and
	// use exit() not _exit().
	//
	// Update to the "extra space saga" :  the use of cerr << blah << "\n"; is what
	// adds the extra space on RedHat 5.0.  When the line was changed to the fprintf(stderr, ...);
	// rendition the extra space went away, Hence my conclusion is bolstered that the
	// streams stuff is busted. But, maybe wrong header files are being included...
	// I dont care - jhl).
	//


	close(STDOUT_FILENO);
	exit(0);
}
