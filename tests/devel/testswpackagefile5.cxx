#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define LINELEN 500

#include "swptrlist.h"
#include "swpackagefile.h"
#include "swmain.h"

//	swPackageFile * package = new swPackageFile();


int
main (int argc, char ** argv)
{
	char line[LINELEN], *t; 
	char * name, * source;
	uxFormat * uxt;
	swPtrList<swPackageFile> * archiveMemberList = new swPtrList<swPackageFile>();
	swPackageFile * archive;
	swPackageFile * archiveMember;
	int index;

	//fprintf(stderr, "sizeof is swPackageFile %d\n", (int)sizeof(swPackageFile));
	//fprintf(stderr, "sizeof is swPathName %d\n", (int)sizeof(swPathName));


	//
	// Make the archive object.
	//
	uxt = new uxFormat();


	//
	// Make a single swPackageFile object which assumes the
	// the uxFormat object.
	//
	archive = new swPackageFile(uxt);
	
	//
	// Read stdin and assemble a list of swPackageFile objects.
	//
	while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
	{
		if (strlen(line) >= LINELEN - 2) {
			fprintf (stderr, "line too long : %s\n", line);
		} else {
			//fprintf (stderr, ":: %s", line);
			if ((t=strpbrk (line,"\n\r"))) {
				*t = '\0';
			}
		}

		if (!strlen(line)) {
			continue;
		}

		if (strchr(line, ' ')) {
			name = line;
			source = strchr(line, ' ') + 1;
		} else {
			name = line;
			source = line;
		}

		archiveMember = new swPackageFile(name, source);
		archiveMemberList->list_add(archiveMember);

		//if (xfmat.xFormat_set_from_statbuf(line) == 0)
		//	xfmat.xFormat_write_file(line);
	}


	//
	// Now iterate over the list and write the package.
	//
	index = 0;
	archiveMember = archiveMemberList->get_pointer_from_index(index++);
	while(archiveMember) {
		mode_t mode;
		name = archiveMember->swfile_get_filename();	
		archiveMember->swfile_set_package_filename("foo");
		//fprintf(stdout, "filename is %s\n", name);
		if (archive->xFormat_set_from_statbuf(name) == 0) {
			//archive->xFormat_write_file(name);
			mode = archive->xFormat_get_mode();
			//fprintf(stderr, "mode = %o\n", mode);
			mode &= ~(S_IRWXU|S_IRWXG|S_IRWXO);
			mode |= (S_IXUSR);
			//fprintf(stderr, "mode = %o\n", mode);
			archive->xFormat_set_mode(mode);

			archive->xFormat_set_mtime(0);
			archive->xFormat_set_username("root");
			archive->xFormat_set_groupname("root");
			archive->xFormat_set_name("foo");
			archive->xFormat_write_file(static_cast<char*>(NULL), name);
			//archive->xFormat_write_file("foo", name);
		}
		// fprintf(stderr, "index write = %d\n", index);
		archiveMember = archiveMemberList->get_pointer_from_index(index++);
	}
	archive->xFormat_write_trailer(); 

	//
	// Now delete the archive members.
	//
	index = 0;
	archiveMember = archiveMemberList->get_pointer_from_index(index++);
	while(archiveMember) {
		delete archiveMember;
		// fprintf(stderr, "index delete = %d\n", index);
		archiveMember = archiveMemberList->get_pointer_from_index(index++);
	}

	delete archive;
	delete uxt;
	exit(0);
}

