#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "strar.h"

static STROB * buf1 = NULL;

char * 
strar_dump_string_s(STRAR * strar, char * prefix)
{
	char * p;
	int i;
	STROB * buf;
	if (buf1 == (STROB*)NULL) buf1 = strob_open(100);
	buf = buf1;

	strob_sprintf(buf, 0, "%s%p (STRAR*)\n", prefix,  (void*)strar);
	
	strob_sprintf(buf, 1, "%s%p->nsM                = [%s]\n", prefix, (void*)strar, strar->nsM);
	strob_sprintf(buf, 1, "%s%p->lenM               = [%s]\n", prefix, (void*)strar, strar->lenM);

	i = 0;
	p = strar_get(strar, i);
	while (p) {
		strob_sprintf(buf, 1, "%s%p->listM[%d]            = [%s]\n", prefix, (void*)strar, i, p);
		i++;
		p = strar_get(strar, i);
	}
	return strob_str(buf);
}


