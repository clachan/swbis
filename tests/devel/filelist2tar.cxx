
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

  char line[LINELEN], *t; 
  portableArchive xfmat(STDIN_FILENO, STDOUT_FILENO);
  int format_code = arf_ustar;
			

  if (taru_set_tar_header_policy(xfmat.xFormat_get_xformat()->taruM, "ustar0", &format_code))
				exit(1);

	if (argc > 1) {
		if (!strcmp(argv[1], "ustar")) {
			format_code = arf_ustar;
		} else if (!strcmp(argv[1], "newc")) {
			format_code = arf_newascii;
		} else if (!strcmp(argv[1], "crc")) {
			format_code = arf_crcascii;
		} else if (!strcmp(argv[1], "odc")) {
			format_code = arf_oldascii;
		} else {
			format_code = arf_ustar;
		}
  		if (taru_set_tar_header_policy(xfmat.xFormat_get_xformat()->taruM, argv[1], &format_code))
				exit(1);
	}

  xfmat.xFormat_set_false_inodes(0);
  if (format_code == arf_oldascii) {
  	xfmat.xFormat_set_false_inodes(1);
  }
  xfmat.xFormat_set_format(format_code);

  while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
    {
      if (strlen (line) >= LINELEN - 2)
	{
	  fprintf (stderr, "line too long : %s\n", line);
	}
      else
	{
	  if ( (t=strpbrk (line,"\n\r"))) {
             *t = '\0';
	  }
	  if ( strlen(line)) {
	       if (xfmat.xFormat_set_from_statbuf(line) == 0)
			xfmat.xFormat_write_file(line);
          } 
       }
    }
  xfmat.xFormat_write_trailer(); 
  exit (0);
}
