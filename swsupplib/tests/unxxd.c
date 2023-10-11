#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
#include "swevents_array.h"
#include "swcommon_options.h"

#define LINELEN 900


int main (int argc, char ** argv)
{
  int ret;
  char * s, *e;
  char line[LINELEN];
  unsigned char dumpline[100];

  while (fgets (line, LINELEN - 1, stdin) != (char *) (NULL))
    {
      line[strlen(line)-1]='\0';

      if (strlen (line) >= LINELEN - 2)
        {
          fprintf (stderr, "filesize: line too long : %s\n", line);
        }
      else
        {
		if ((s=strchr(line, ':')) == NULL) {
			fprintf(stderr, "line error\n");
			exit(1);
		}
		
		s++;
		s++;

		e = strstr(s, "  ");
		if (!e) {
			fprintf(stderr, "2space not found \n");
			exit(2);
		}	
		*e = '\0';

		ret = sscanf(s, "%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x",
			dumpline + 0,
			dumpline + 1,
			dumpline + 2,
			dumpline + 3,
			dumpline + 4,
			dumpline + 5,
			dumpline + 6,
			dumpline + 7,
			dumpline + 8,
			dumpline + 9,
			dumpline + 10,
			dumpline + 11,
			dumpline + 12,
			dumpline + 13,
			dumpline + 14,
			dumpline + 15
		);
		write(1, dumpline, ret);
	}
    }


}


