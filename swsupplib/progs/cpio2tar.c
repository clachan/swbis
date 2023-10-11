#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "strob.h"
#include "uinfile.h"
#include "swlib.h"
#include "taru.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"
int 
main (int argc, char ** argv ) {
   return taru_process_copy_in ((TARU*)NULL, STDIN_FILENO, STDOUT_FILENO);
}



