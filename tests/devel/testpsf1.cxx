
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "xstream_config.h"
#include "swpsf.h"
#include "swparser.h"
#include "swmain.h"

int
main (void)
{
  int len, len1, end;
  int fd;
  char * buf; 
  swPSF *index=new swPSF("");
  if (!index)
  	exit(1);
  index->open_parser(STDIN_FILENO);
  len=index->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  //cerr << len << "\n";

  fd = index->get_mem_fd();
  uxfio_lseek(fd, 0, SEEK_SET);

  uxfio_get_dynamic_buffer(fd, &buf, &end, &len1); 
  write (STDOUT_FILENO, buf, len); 
  if ( len != end) {
	fprintf(stderr," len != end %d %d\n", len ,end);
  }
  
  //cout << "end: " << end << "\n";
  //cout << "len1: " << len1 << "\n";

  //index->write_fd(STDOUT_FILENO);
 
  exit (0);
}


