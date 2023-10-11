#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "taru.h"
#include "swfork.h"
#include "defer.h"

/* BROKEN  defer uninitialized */


int
taru_format_translate (int ifd, int ofd, enum archive_format output_format ) {
   int parent;
   int fpipe[2];
   int status=0, ret=0;
   DEFER * defer = NULL;
   TARU * taru = taru_create();

   pipe (fpipe);
   parent = swfork((sigset_t*)(NULL));

   if (parent == 0) { /* child */
        close (fpipe[0]); 
	_exit (taru_process_copy_in (NULL, ifd, fpipe[1])) ;
   } else if (parent > 0) {
        close (fpipe[1]); 
        ret = taru_process_copy_out (taru, fpipe [0] , ofd, defer, NULL, output_format, -1, -1, (intmax_t*)NULL, NULL); 
        while ( !waitpid ( (pid_t)parent, &status, WNOHANG)) {
               ;
	}
        if ( ! status && ! ret ) return 0;
   } else { 
      return -3; 
   }
  return -1;
}






