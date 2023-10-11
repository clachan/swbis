#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "taru.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int 
main (int argc, char ** argv ) {
   enum archive_format arf = arf_crcascii; 
   DEFER * defers=defer_open(arf);
   PORINODE * porinode=porinode_open();
   TARU * taru = taru_create();
   return taru_process_copy_out(taru, STDIN_FILENO, STDOUT_FILENO, defers, porinode,  arf, -1, -1, (intmax_t*)NULL, NULL);
}


