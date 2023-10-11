#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


void *
malloc_fprintf(size_t size, char * format, ...)
{
	/* static char buf[300]; */
	void * x;
	va_list ap;
	
	va_start(ap, format);
	x = malloc(size);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "size=%d arg=%p\n", (int)size, x);
	va_end(ap);
	return x;
}

void *
realloc_fprintf(void * old, size_t size, char * format, ...)
{
	/* static char buf[300]; */
	void * x;
	va_list ap;
	
	va_start(ap, format);
	x = realloc(old, size);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "old=%p size=%d arg=%p\n", old, (int)size, x);
	va_end(ap);
	return x;
}











