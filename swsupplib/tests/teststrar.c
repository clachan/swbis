#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "swparser_global.h"
#include "strar.h"
#include "swevents_array.h"
#include "swcommon_options.h"

int 
main (int argc, char ** argv ) {
	char nas[2];
	int c;
	char * s;
	STRAR * strar = strar_open();
	int i = 0;
	
	while( (c=fgetc(stdin)) != EOF ) {
		nas[0] = (char)c;
		nas[1] = '\0';
		/* putc(c, stdout); */
		strar_add(strar, nas);
	}
	
	while( (s=strar_get(strar, i++)) ) {
	 	putc((int)(*s), stdout);	
	}
	strar_close(strar);
	exit(0);
}

