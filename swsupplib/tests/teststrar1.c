#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "strar.h"
#include "swparser_global.h"
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
		nas[0] = '\0';
		/* putc(c, stdout); */
		strar_add(strar, nas);
	}

	strar_add(strar, "a");
	strar_add(strar, "b");
	strar_add(strar, "c");
	strar_add(strar, "d");
	
	while( (s=strar_get(strar, i++)) ) {
	 	putc((int)(*s), stdout);	
	}
	strar_close(strar);
	exit(0);
}

