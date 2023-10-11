/* shell_lib.h  -- retrieve shell functions from static array
*/

#ifndef SHELL_LIB_H_200610
#define SHELL_LIB_H_200610

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "strob.h"

struct shell_lib_function {
	char * nameM;          /* Name of the function */
	char * controlM;       /* A Control header, holds the compressed size of textM */
	char * textM;          /* The (compressed, or not) POSIX Shell compatible function */
	char * functionM;      /* not used, always NULL */
};

struct shell_lib_function *	 shlib_get_function_struct	(char * function_name);
char * 				 shlib_get_function_text(struct shell_lib_function *, STROB * buf);
char *				 shlib_get_function_text_by_name(char * function_name, STROB * buf, int * ret);
struct shell_lib_function *	 shlib_get_function_array	(void);
void				 shlib_print_all_function	(STROB * buf);

#endif
