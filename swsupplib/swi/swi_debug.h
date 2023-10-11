#ifndef swi__debug20030523
#define swi__debug20030523

#include "swi.h"

char * swi_script_list_dump_string_s(SWI_SCRIPTS * xx, char * prefix);
char * swi_xfile_member_dump_string_s(SWI_FILE_MEMBER * xx, char * prefix);
char * swi_xfile_script_dump_string_s(SWI_CONTROL_SCRIPT * xx, char * prefix);
char * swi_xfile_dump_string_s(SWI_XFILE * xx, char * prefix);
char * swi_product_dump_string_s(SWI_PRODUCT * xx, char * prefix);

char * swi_package_dump_string_s(SWI_PACKAGE * xx, char * prefix);
char * swi_dump_string_s(SWI * xx, char * prefix);

#endif
