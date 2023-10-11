#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define LINELEN 500

#include "swptrlist.h"
#include "swpackagefile.h"
#include "swattributefile.h"
#include "swpackagedir.h"
#include "swmain.h"

//	swPackageFile * package = new swPackageFile();


int
main (int argc, char ** argv)
{
	struct stat st;
	//swPackageFile * dir;
	swPackageFile * dir;

	AHS * h = ahs_open();

	dir = new swPackageDir(argv[1]);	
	//dir = new swPackageFile();	

	lstat("/etc", &st);
	

	//package->xFormat_set_name("anyname");
	//package->xFormat_set_mode(0);

	ahs_set_tar_username(h, "bin");
	ahs_set_tar_groupname(h, "bin");

	//fprintf(stderr, "%s\n", ahs_dump_string_s(h, ""));
	

	// dir->xFormat_write_file();
	// dir->xFormat_write_file(NULL, "newdirname", -1);
	


	// dir->swfile_set_default_statbuf();
	
	dir->xFormat_set_username("");
	dir->xFormat_set_username("goodbye");
	dir->xFormat_set_groupname("hello");
	dir->xFormat_set_mode(0555);
	//fprintf(stderr, "%s\n", swpackagefile_dump_string_s(dir, ""));

	dir->xFormat_set_mode(0755);
	dir->xFormat_set_uid(8);
	dir->xFormat_set_gid(8);
	
	dir->xFormat_set_user_systempair("bin");
	dir->xFormat_set_group_systempair("bin");
	
	//dir->xFormat_set_name(argv[1]);


	//fprintf(stderr, "%s\n", swpackagefile_dump_string_s(dir, ""));

	dir->xFormat_set_filetype_from_tartype(DIRTYPE);
	dir->xFormat_set_perms(01744);
	//dir->xFormat_set_format(arf_ustar);


	dir->swfile_set_package_filename(argv[1]+1);
	dir->xFormat_write_file();
	dir->xFormat_write_file((struct stat*)NULL, "1234");
	dir->xFormat_write_trailer();


	//fprintf(stdout, "\nmode = %o\n", package->xFormat_get_mode());

	//fprintf(stdout, "\n%d %d\n", 493 & CP_IFMT, CP_IFDIR);
	//fprintf(stdout, "\n%d %d\n", st.st_mode & CP_IFMT, CP_IFDIR);

	delete dir;
	close(STDOUT_FILENO);
	exit(0);
}

