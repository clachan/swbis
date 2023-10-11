
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "portablearchive.h"
#include "swmain.h"

#define LINELEN 200

int main (int argc, char ** argv)
{
	portableArchive xfmat(STDIN_FILENO, STDOUT_FILENO);
	int ret;
	int ofd;
 	int ifd;
	int n;	
	int htotal = 0;
	int ptotal = 0;
	int total = 0;
	int hret;
	int pret;
	int loop_count = 0;

	//if (argc < 2) {
	//	fprintf(stderr, "Usage: pa2pa output_filename pass-through_filename\n");
	//	fprintf(stderr, " pa2pa  is a test program for the pass-through data buffer.\n");
	//	exit(2);
	//}

	if (argc >= 2) {
		if ( strcmp("-", argv[1]) == 0) {
			ofd = STDOUT_FILENO;
		} else {
			ofd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC);
			if (ofd < 0) {
				fprintf(stderr,"error opening %s\n", argv[1]);
			}
		}
		xfmat.xFormat_set_ofd(ofd);
	}
	if (argc == 3) {
		if (strcmp("-", argv[2]) == 0) {
			ofd = STDOUT_FILENO;
		} else {
			ofd = open(argv[2], O_RDWR|O_CREAT|O_TRUNC);
			if (ofd < 0) {
				fprintf(stderr,"error opening %s\n", argv[1]);
			}
		}	
		xfmat.xFormat_set_pass_fd(ofd);
	}
	
	ret = xfmat.xFormat_open_archive(STDIN_FILENO, UINFILE_DETECT_NATIVE|UINFILE_DETECT_FORCEUNIXFD);
	if (ret) {
		exit(21);
	}

	ifd = xfmat.xFormat_get_ifd();
	ofd = xfmat.xFormat_get_ofd();
	xfmat.xFormat_set_false_inodes(0);
	while ((ret = xfmat.xFormat_read_header()) > 0) {
     		if (xfmat.xFormat_is_end_of_archive()){
			break;
		}
		hret = xfmat.xFormat_write_header();
		pret = xfmat.xFormat_copy_pass(ofd, ifd);
		if ( hret < 0 || pret < 0) {
			//fprintf(stderr, "ERROR hret = %d, pret = %d\n", hret, pret);
		}
		ptotal += pret;
		htotal += hret;
		total += (pret+hret);
		loop_count ++;
	}
	//fprintf(stderr, "final header total = %d\n", htotal);
	//fprintf(stderr, "final pass total = %d\n", ptotal);
	//fprintf(stderr, "final loop count = %d\n", loop_count);
	//fprintf(stderr, "final loop total = %d\n", total);
	if (ret >= 0) {
		n = xfmat.xFormat_write_trailer(); 
		//fprintf(stderr, "write_trailer n = %d\n", n);
		total += n;
		if (xfmat.xFormat_get_pass_fd()) {
			n = xfmat.xFormat_clear_pass_buffer();
			total += n;
			//fprintf(stderr, "clear_pass_buffer n = %d\n", n);
		}
	}
	//fprintf(stderr, "TOTAL=%d\n", total);
	exit(ret);
}
