#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "swparser_global.h"
#include "strob.h"
#include "uinfile.h"
#include "swlib.h"
#include "taru.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int 
main (int argc, char ** argv ) {
	struct new_cpio_header *file_hdr=ahsStaticCreateFilehdr();
	int end;
	int fd;
	UINFORMAT * format_desc;
	int format;
	char  package_name[200];
	TARU * taru = taru_create();

	if (argc <= 1)
		strcpy(package_name,"-");
	else
		strcpy(package_name,argv[1]); 


	fd=uinfile_open(package_name, (mode_t)(NULL), &format_desc,  UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCEUNIXFD);
	if (fd < 0)
		exit(1);
	format=uinfile_get_type(format_desc);

	end=1;
	while(1) {
		if ((end=taru_read_header(taru, file_hdr, fd, uinfile_get_type(format_desc), NULL, 0)) < 0) {
			fprintf(stderr,"header error %s\n", ahsStaticGetTarFilename(file_hdr));
			exit(1);
		}

		if ((file_hdr->c_mode & CP_IFMT) == CP_IFREG) {
			if (taru_write_archive_member_data(taru, file_hdr, -1, fd, (int(*)(int))NULL, uinfile_get_type(format_desc), -1, NULL)){
				fprintf(stderr,"data error %s\n", ahsStaticGetTarFilename(file_hdr));
				exit(1);
			}
			taru_tape_skip_padding (fd, file_hdr->c_filesize, format);
		}		
		if (!strcmp(ahsStaticGetTarFilename(file_hdr),"TRAILER!!!"))
			break;

		fprintf (stdout,"%s\n", ahsStaticGetTarFilename(file_hdr));
	}
	exit(0);
}



