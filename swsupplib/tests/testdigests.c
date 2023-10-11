#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "usgetopt.h"
#include "swutillib.h"
#include "swlib.h"
#include "swi.h"
#include "xformat.h"
#include "swevents_array.h"
#include "swcommon_options.h"


static
void
print_dig(int nofile, char * name, char * digtype, char * md5, char * sha1, char * sha512)
{
	if (nofile == 0) {
		if (strcmp(digtype, "md5") == 0) 
			fprintf(stdout, "%s  %s\n",  md5, name);
		else if (strcmp(digtype, "sha512") == 0) 
			fprintf(stdout, "%s  %s\n",  sha512, name);
		else 
			fprintf(stdout, "%s  %s\n", sha1, name);
	} else {
		if (strcmp(digtype, "md5") == 0) 
			fprintf(stdout, "%s\n",  md5);
		else if (strcmp(digtype, "sha512") == 0) 
			fprintf(stdout, "%s\n",  sha512);
		else 
			fprintf(stdout, "%s\n", sha1);
	}
}

int 
main (int argc, char ** argv ) {
	int fd;
	int ret = 0;
	int nofile = 0;
	char digest[100];
	char sha1[100];
	char sha512[129];
	char * name;

	digest[0] = '\0';
	sha1[0] = '\0';

	/* 
	ret = swlib_sha1(STDIN_FILENO, sha1);
	*/


	if (strcmp(argv[1+nofile], "--no-name") == 0) {
		nofile = 1;
	}
	
	if (argc < 2+nofile) {
		fprintf(stderr, "testdigests [--no-name] {md5|sha1|sha512} file\n");
		exit(2);
	}

	if (argc < 3+nofile || strcmp(argv[2+nofile], "-") == 0) {
		fd = STDIN_FILENO;
		name = "-";
		ret = swlib_digests(fd, digest, sha1, NULL, sha512);
		if (ret < 0) {
			fprintf(stderr, "error %d\n", ret);
			exit(1);
		}
		print_dig(nofile, name, argv[1+nofile], digest, sha1, sha512);
	} else {
		int i;
		i = 0;
		while(argv[2+nofile+i] != NULL) {
			struct stat st;
			fd = open(argv[2+nofile+i], O_RDONLY, 0);
			fstat(fd, &st);
			name = argv[2+nofile+i];
			if (! S_ISREG(st.st_mode))  {
				fprintf(stderr, "%s not regular file\n", name);
				i++;
				continue;
			}
			if (fd < 0) exit(2);
			ret = swlib_digests(fd, digest, sha1, NULL, sha512);
			if (ret < 0) {
				fprintf(stderr, "error %d\n", ret);
				exit(1);
			}
			close(fd);
			print_dig(nofile, name, argv[1+nofile], digest, sha1, sha512);
			i++;
		}
	}

	exit(0);
}

