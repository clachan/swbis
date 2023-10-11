#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "swlib.h"
#include "swvarfs.h"
#include "usgetopt.h"
#include "swevents_array.h"
#include "swcommon_options.h"


static char *
getnextpath(FILE * fp)
{
	char * ret;
	static char buf[1024];
	char * s;

	ret = fgets(buf, sizeof(buf), fp);
	if (!ret) return ret;
	buf[sizeof(buf) - 1] = '\0';
	if ((s=strrchr(buf, '\n'))) *s = '\0';
	if ((s=strrchr(buf, '\r'))) *s = '\0';
	return ret;
}


static void usage(char ** argv)
{
	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, " --to-stdout \n");
        fprintf(stderr, " --stat-dump \n");
        fprintf(stderr, " -D,--debug\n");
        fprintf(stderr, " -v,--verbose\n");
        fprintf(stderr, " -f,--filelist list\n");
        fprintf(stderr, " -N,--non-interactive\n");
        fprintf(stderr, " -S,--no-seek-flags\n");
        fprintf(stderr, " -x,--archive-list\n");
}


static void help(void) {
			fprintf(stderr, "Show help:\n");
			fprintf(stderr, "Commands:\n");
			fprintf(stderr, "? <filename><enter>   Process filename.\n");
			fprintf(stderr, "? <enter>             Process next filename.\n");
			fprintf(stderr, "? #vchdir <dirname>       vchdir to dirname.\n");
			fprintf(stderr, "? #setdir <dirname>       Set directory to dirname.\n");
			fprintf(stderr, "? #exit               exit.\n");
			fprintf(stderr, "? #help               Show this help.\n");
			fprintf(stderr, "? #no-stop-mode       Do not quit on null directory entry.\n");
			fprintf(stderr, "? #stop-mode          Do quit on null directory entry.\n");
			fprintf(stderr, "? #automode          process all remaining files non-interactively.\n");
			fprintf(stderr, "? #show-state        show debug info for object.\n");
}

int 
main (int argc, char ** argv) {
	int c;
	struct stat st;
	SWVARFS * swvarfs;
	char * path;
	int to_stdout = 0;
	int stat_dump = 0;
	int devnullfd;
	int opt_verbose = 0;
	int non_interactive = 0;
	int archive_list = 0;
	int debugmode = 0;
	int automode = 0;
	int no_stop_mode = 0;
	int no_seek_flags = 0;
	int flags = 0;
	char * filelistname = NULL;
	FILE * flp = NULL;

	while (1)
           {
             int option_index = 0;
             static struct option long_options[] =
             {
               {"to-stdout", 0, 0, 'O'},
               {"stat-dump", 0, 0, 'd'},
               {"help", 0, 0, 'H'},
               {"debug", 0, 0, 'D'},
               {"verbose", 0, 0, 'v'},
               {"file-list", 1, 0, 'f'},
               {"non-interactive", 1, 0, 'N'},
               {"no-seek-flags", 0, 0, 'S'},
               {"archive-list", 0, 0, 'x'},
               {0, 0, 0, 0}
             };

             c = ugetopt_long (argc, argv, "HOdvf:xNSD", long_options, &option_index);
             if (c == -1)
                 break;

             switch (c)
               {
               case 'x':
	       		archive_list = 1;
	       		non_interactive = 1;
			break;
               case 'd':
	       		stat_dump = 1;
			break;
               case 'H':
	       		usage(argv);
	       		help();
			exit(2);
			break;
               case 'D':
	       		debugmode = 1;
			break;
               case 'v':
	       		opt_verbose = 1;
			break;
               case 'N':
	       		non_interactive = 1;
			break;
               case 'O':
               	  	to_stdout=1;        
		 	break;
               case 'S':
               	  	no_seek_flags=1;        
		 	break;
               case 'f':
			filelistname = strdup(optarg);
			flp = fopen(filelistname, "r");
			if (!flp) {
				fprintf(stderr, "file %s not found\n", filelistname);
				exit(3);
			}
		 	break;
               default:
		 exit(1);
                 break;
               }
	     }

	if (!flp && non_interactive == 0) {
		flp = fopen("/dev/tty", "r");
		if (!flp) {
			fprintf(stderr, "/dev/tty open error\n");
			exit(3);
		}
	}
	devnullfd = open("/dev/null", O_RDWR, 0);
	if (optind < argc) {
		/*
		* open a directory or portable archive file.
		*/
		flags = UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		swvarfs=swvarfs_open(argv[optind], flags, (mode_t)(0));
	} else {
		/*
		* Open an portable archive on stdin.
		*/
		flags = UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_DYNAMIC_MEM;
		if (no_seek_flags == 1) {
			flags = UINFILE_DETECT_FORCEUXFIOFD|UINFILE_DETECT_NATIVE|UINFILE_UXFIO_BUFTYPE_MEM;
		}
		swvarfs=swvarfs_open("-", flags, (mode_t)(0));
	}

	if (!swvarfs) {
		fprintf(stderr, "swvarfs_open() failed.\n");
		exit(2);
	}

	if (opt_verbose) {
		swvarfs->format_descM->verboseM = 1;
	}


	if (flp) {
		if (non_interactive == 0) fprintf(stderr, "? "); fflush(stderr);
		path = getnextpath(flp);
		if (strcmp(path, "") == 0) {
			path = swvarfs_get_next_dirent(swvarfs, &st);
		}
	} else {
		path = swvarfs_get_next_dirent(swvarfs, &st);
	}

	while (path && strcmp(path, "#exit")) {


		if (strstr(path, "#null") == path) {
			fprintf(stderr, "path is null\n");
		} else if (strstr(path, "#vchdir ") == path) {
			path+=8;
			while (isspace(*path)) path++;
			fprintf(stderr, "executing swvarfs_vchdir on [%s]\n", path);
			if (swvarfs_vchdir(swvarfs, path)) {
				fprintf(stderr, "swvarfs_vchdir on %s failed.\n", path);
			}
			fprintf(stderr, "vcwdM = [%s]\n", strob_str(swvarfs->vcwdM));
		} else if (strstr(path, "#setdir ") == path) {
			path+=8;
			while (isspace(*path)) path++;
			fprintf(stderr, "executing swvarfs_setdir on [%s]\n", path);
			if (swvarfs_setdir(swvarfs, path)) {
				fprintf(stderr, "swvarfs_setdir on %s failed.\n", path);
			}
		} else if (strstr(path, "#automode") == path) {
			automode = 1;	
		} else if (strstr(path, "#help") == path) {
			help();
		} else if (strstr(path, "#stop-mode") == path) {
			no_stop_mode = 0;
			fprintf(stderr, "no-stop-mode=%d\n", no_stop_mode);
		} else if (strstr(path, "#no-stop-mode") == path) {
			no_stop_mode = 1;
			fprintf(stderr, "no-stop-mode=%d\n", no_stop_mode);
		} else if (strstr(path, "#show-state") == path) {
			/*D fprintf(stderr, "%s\n", swvarfs_dump_string_s(swvarfs, "")); */
		} else {
			if (swvarfs_u_lstat(swvarfs, path, &st)) {
				fprintf(stderr, "swvarfs_u_lstat failed on %s\n", path);
			} else {
				if (archive_list == 1) {
					fprintf(stdout, "%s\n", path);
				} else {
					swlib_write_stats(path, NULL, &st, 0, "", STDOUT_FILENO, NULL);	
				}
			}
		}
		/*D if (debugmode) fprintf(stderr, "%s", swvarfs_dump_string_s(swvarfs, "")); */
		if (non_interactive == 0) fprintf(stderr, "? "); fflush(stderr);

		if (flp && automode == 0) {
			path = getnextpath(flp);
			if (strcmp(path, "") == 0)  {
				path = swvarfs_get_next_dirent(swvarfs, &st);
				if (!path && no_stop_mode == 1 ) path = "#null";
			}
		} else {
			path = swvarfs_get_next_dirent(swvarfs, &st);
			if (!path && no_stop_mode == 1 ) path = "#null";
		}
	}
	if (flp) fclose(flp);
	swvarfs_close(swvarfs);
	exit(0);
}
