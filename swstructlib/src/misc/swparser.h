/* swparser -- parser for POSIX.7.2 metadata files.
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */


#ifndef swparser_jhl1998_h
#define swparser_jhl1998_h

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <typeinfo>

extern int swlex_errorcode;
extern char swlex_filename[512];
extern int swlex_debug;
extern int swparse_outputfd;
extern int swparse_atlevel;
extern int swlex_inputfd;
extern int swlex_linenumber;

extern "C" {
int sw_yyparse(int,int,char *,int,int);
#include "swlex_supp.h"
#include "swparse.h"
#include "strob.h"
#include "uxfio.h"
}
extern YYSTYPE yylval;

   class swparser {
	
	char type_[32];
        int outputfd_;
	int inputfd_;
      
     public:

	//enum swParserOutputForm { 	typeMarkup = SWPARSE_FORM_MKUP,
	//				typeLengthMarkup = SWPARSE_FORM_MKUP_LEN,
	//				indentOnly = SWPARSE_FORM_INDENT 
	//			};
	   
	swparser (char * filename, char *swdeffile_type, int outfd) { 
		common_new (filename, swdeffile_type, outfd);
	}
           
	swparser (int infd, char *swdeffile_type, int outfd) { 
		init_members(); 
		set_up1(swdeffile_type);
		swlex_inputfd = uxfio_opendup(infd, UXFIO_BUFTYPE_DYNAMIC_MEM);
		if (swlex_inputfd < 0) swlex_errorcode=1; 
		set_up2(outfd);
	} 
	   
	~swparser(void) {
		if (outputfd_ < 0 ) uxfio_close(swparse_outputfd);
		if (inputfd_ < 0 ) uxfio_close(swlex_inputfd);
		strob_close(yylval.strb); 
	}

	int openfile(char * name) {
		return (swlex_inputfd = uxfio_open ( name,O_RDONLY,0));
	}

	int run_parser(int atlevel, int mark_up_flag) {
		off_t pos = uxfio_lseek(swparse_outputfd, 0, SEEK_CUR);  
		off_t start, end;
    		swlex_linenumber = 0; 
		start = uxfio_lseek(swparse_outputfd, (off_t)(0), SEEK_END);
		if ( common_parser(atlevel, mark_up_flag)) return -1;
		end  = uxfio_lseek(swparse_outputfd, 0, SEEK_CUR);  
		uxfio_lseek(swparse_outputfd, pos, SEEK_SET);
		return (int)(end - start);
	}

	int get_errorcode (void) { 
		return swlex_errorcode ; 
	}
           
	void set_swfilename (char *te){
		::strncpy(swlex_filename, te, sizeof(swlex_filename));
		swlex_filename[sizeof(swlex_filename) - 1] = '\0';
	}
           
	void set_swdeffiletype (char *typestring) { 
		::strncpy(type_, typestring, 31); 
	}

	void set_inputfd (int ifd) { 
		inputfd_=swlex_inputfd = ifd; 
	}

	void set_outputfd (int ofd) { 
		outputfd_=swparse_outputfd = ofd; 
	} 

	int get_inputfd (void) { 
		return swlex_inputfd; 
	}

	int get_outputfd (void) { 
		return swparse_outputfd; 
	}

     private:
	void common_new (char * filename, char *swdeffile_type, int outfd);
	int common_parser (int atlevel, int mark_up_flag);
	void set_up1 (char *typestring);
	void set_up2 (int outputfd);
	void init_members (void);
    };
#endif
