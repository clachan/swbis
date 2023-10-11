// swparser.cxx

/*
 Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>

*/

/*
 COPYING TERMS AND CONDITIONS:

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swparser.h"
	   
// --------------  Private Functions ----------------------- //

void swparser::common_new (char * filename, char *swdeffile_type, int outfd) { 
     init_members(); 
     ::strncpy(swlex_filename, filename, sizeof(swlex_filename));
     swlex_filename[sizeof(swlex_filename) -1] = '\0';
     set_up1(swdeffile_type);
     swlex_inputfd = uxfio_open(filename, O_RDONLY, 0);
     if (swlex_inputfd < 0) swlex_errorcode=1; 
     set_up2(outfd);
}
      
int swparser::common_parser (int atlevel, int mark_up_flag) {
     if (sw_yyparse (swlex_inputfd, swparse_outputfd, type_, atlevel, mark_up_flag)) {
         // fprintf (stderr, "parser error.\n");
         swlex_errorcode=1;
         return -1;
      }
      return 0;
}

void swparser::set_up1 (char *te) {
      ::strncpy(type_, te, sizeof(type_)); 
      type_[sizeof(type_) -1] = '\0'; 
} 
      
void swparser::set_up2 (int outputfd) {
     if (outputfd >=0) {
          swparse_outputfd = outputfd; 
     } else {
	  fprintf (stderr,"swparser set_up error \n"); 
     } 
} 
      
void swparser::init_members (void) {
	//::strcpy (swlex_filename,"");
	swlex_debug=0;
        swparse_outputfd=-1;
        swlex_inputfd=-1;
        swlex_errorcode=0;
        inputfd_ = outputfd_ = -1;	
        yylval.strb = strob_open (8); 
}
