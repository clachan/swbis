
#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>

#include <typeinfo>
#include "xstream_config.h"
#include "swparser.h"
#include "swindex.h"
#include "swpackagefile.h"

#include "swmain.h"

int
main (int argc, char ** argv)
{
  int size;
  unsigned long crc=0;
  char md5digest[40];

  if (argc < 2) exit(2);
  swPackageFile *index=new swPackageFile("/usr/tmp/___________", argv[1]);
  if (!index) exit(1);

  size=index->swfile_get_size();
  index->swfile_get_posix_cksum(&crc);
  assert (index->swfile_get_ascii_md5(md5digest));
  cerr << "size is :" << size << "\n";
  cerr << "crc is :" << crc << "\n";
  cerr << "md5 is:" << md5digest << "\n";
  
  
  index->write_fd(STDOUT_FILENO);
  //index->xFormat_write_file();
  
  exit (0);
}

