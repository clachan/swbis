#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

#define LINELEN 900


int main (int argc, char ** argv)
{
  int sum;
  char buf[512];
  unsigned char oct[512];

  if (read(0, buf, 512) != 512) {
	fprintf(stderr, "read error\n");
        exit(1);
  }

  sum = taru_tar_checksum(buf);

  fprintf(stdout, "dec:%d\n", sum);
  sprintf(oct, "%08o\n", sum);
  fprintf(stdout, "%8s\n", oct);
  fprintf(stdout, "%02x%02x %02x%02x %02x%02x %02x%02x\n", *oct, *(oct+1),
				*(oct + 2) ,
				*(oct + 3) ,
				*(oct + 4) ,
				*(oct + 5) ,
				*(oct + 6) ,
				*(oct + 7) 
					);

  exit(0);
}


