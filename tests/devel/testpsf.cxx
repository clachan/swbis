
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "xstream_config.h"
#include <assert.h>
#include "swindex.h"
#include "swparser.h"
#include "swmain.h"
 
  
int  
main (void)
{
  int len, len1, end;
  int fd;
  char * buf; 
  swINDEX *index=new swINDEX("");
  if (!index)
  	exit(1);
  index->open_parser(STDIN_FILENO);
  len=index->run_parser(0, SWPARSE_FORM_MKUP_LEN);
  if (len < 0) exit(1);
  fd = index->get_mem_fd();

  //uxfio_lseek(fd, index->swMemFileGetOffset(), SEEK_SET);
  //len=index->swMemFileGetLength(); 

  if (uxfio_fcntl(fd, UXFIO_F_SET_VEOF, len) < 0) {
	cerr << "error in uxfio_fcntl\n";
  }

  swlib_pump_amount(STDOUT_FILENO, fd, -1);
 
  cerr << len << "\n"; 
  exit (0);
}


