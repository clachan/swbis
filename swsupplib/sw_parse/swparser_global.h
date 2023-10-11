/* swparser_global: global variables required by the lex and yacc
 *                  generated C-code.
 */

#ifndef swparser_global_jhl12011998_h
#define swparser_global_jhl12011998_h

#include "swuser_config.h"
int  swlex_errorcode=0;
char swlex_filename[512];
int  swlex_debug=0;
int  swlex_inputfd=0;
int  swparse_debug=0;
int  swparse_outputfd=0;
int  swparse_atlevel=0;
int  swparse_form_flag=0;
int  swparse_swdef_filetype=0;
int  swlex_linenumber = 0;
int  swlex_definition_file = 0;
#endif
