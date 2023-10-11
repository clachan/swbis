%{

/*
swparse.y
*/

/*

 Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>

*/

/*
 COPYING TERMS AND CONDITIONS:

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "strob.h"
#include "swparse.h"


#define SWPARSE_ACMD_COPY 0
#define SWPARSE_ACMD_CAT 1
#define SWPARSE_ACMD_EMIT 2

#define SWPARSE_EMITTED_TYPE_ATT 'A'   /* attribute keyword */
#define SWPARSE_EMITTED_TYPE_EXT 'E'   /* extended keyword */
#define SWPARSE_EMITTED_TYPE_OBJ 'O'   /* object keyword */

#define SWPARSE_SWDEF_FILETYPE_INFO	0
#define SWPARSE_SWDEF_FILETYPE_INDEX	1
#define SWPARSE_SWDEF_FILETYPE_PSF	2

#define SWPARSE_LEVEL_IVAL_MIN		-9
#define SWPARSE_LEVEL_IVAL_MAX		9
#define SWPARSE_UNKNOWN_EXTRA		40

extern int yydebug; 
extern int swlex_yacc_feedback_fileset;
extern int swlex_yacc_feedback_file_object;
extern int swlex_yacc_feedback_controlfile_object;
extern int swlex_yacc_feedback_directory;
extern char swlex_filename[];
extern int swparse_outputfd;
extern int swparse_atlevel;
extern int swparse_form_flag;
extern int swparse_swdef_filetype;

static int did_see_vendor_misleading;
static int location;
static char swparse_keytype;
static char swparse_leading_whitespace[]="\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"\x20\x20\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x2\x20\x20"
				"";
static char *swws;

static int level_push(char ** sw);
static int level_pop(char ** sw);
static STROB * tmp_strob;
static int swparse_strob_ptr_len;
char * swparse_strob_ptr;
int swparse_construct_attribute ( STROB * strb, int output_fd, char * src, int cmd, int level, char s_keytype, int offset );
int swparse_write_attribute_obj ( int outputfd, char * key, int level, int offset);
int swparse_print_filename (char * buf, int len, char * filetype, char * ws_level_string, char * swlex_filename, int form_flag);
static int swparse_i_emit(STROB * strb, int output_fd, char * src, int cmd, int level, char s_keytype, int offset);
static int swparse_i_emit_object_keyword(int outputfd, char * key, int level, int offset);
static int swparse_i_emit_filename (char * buf, int buflen, char * filetype, char * ws_level_string, char * swlex_filename, int form_flag);

%}

%union {
	STROB * strb;
}


%token <strb> SW_ATTRIBUTE_KEYWORD
%token <strb> SW_EXT_KEYWORD
%token <strb> SW_RPM_KEYWORD
%token <strb> SW_OBJECT_KEYWORD
%token	SW_NEWLINE_STRING
%token	SW_OK_NEWLINE_STRING
%token	SW_TERM_NEWLINE_STRING
%token <strb> SW_PATHNAME_CHARACTER_STRING
%token <strb> SW_SHELL_TOKEN_STRING
%token	SW_WHITE_SPACE_STRING
%token	SW_OPTION_DELIM
%token	SW_EXT_WHITE_SPACE_STRING
%token	SW_OPTION
%token	SW_INDEX
%token	SW_INSTALLED
%token	SW_INFO
%token	SW_PSF
%token	SW_PSF_INCL
%token	SW_SWBISINFO
%token  SW_LEXER_EOF /* not used */
%token  SW_LEXER_FATAL_ERROR


%token <strb> SW_OK_HOST
%token <strb> SW_OK_DISTRIBUTION
%token <strb> SW_OK_INSTALLED_SOFTWARE
%token <strb> SW_OK_BUNDLE
%token <strb> SW_OK_PRODUCT
%token <strb> SW_OK_SUBPRODUCT
%token <strb> SW_OK_FILESET
%token <strb> SW_OK_CONTROL_FILE
%token <strb> SW_OK_FILE
%token <strb> SW_OK_VENDOR
%token <strb> SW_OK_MEDIA
%token <strb> SW_AK_LAYOUT_VERSION
%token <strb> SW_OK_CATEGORY


%type <strb> attribute_keyword
%type <strb> extended_keyword
%type <strb> single_value
%type <strb> pathname

%start software_definition_file
%%


software_definition_file : sw_INIT_y  sw_INFO_tree  	sw_INFO_file 	sw_QUIT_y
			 | sw_INIT_y  sw_INDEX_tree 	sw_INDEX_file 	sw_QUIT_y
			 | sw_INIT_y  sw_INSTALLED_tree sw_INSTALLED_file sw_QUIT_y
			 | sw_INIT_y  sw_PSF_tree   	sw_PSF_file 	sw_QUIT_y
			 | sw_INIT_y  sw_PSF_tree_incl  sw_PSF_file_incl 	sw_QUIT_y
			 | sw_INIT_y  sw_OPTION_tree 	sw_OPTION_file 	sw_QUIT_y
			 ;

sw_INIT_y		: { 
                               if (swparse_atlevel < SWPARSE_LEVEL_IVAL_MIN || swparse_atlevel > SWPARSE_LEVEL_IVAL_MAX) exit(1);
                               swws = &(swparse_leading_whitespace[0]) + (strlen(swparse_leading_whitespace) - swparse_atlevel) ;
			       tmp_strob = strob_open (8); 
			  }
			  ;

sw_QUIT_y		: {
				strob_close (tmp_strob);
			  }
			  ;


sw_INFO_tree		: SW_INFO {
                                     swparse_strob_ptr_len = strlen(swlex_filename) + SWPARSE_UNKNOWN_EXTRA + swparse_atlevel;
                                     strob_setlen(tmp_strob, swparse_strob_ptr_len);
				     swparse_strob_ptr=strob_str(tmp_strob);
				     swparse_i_emit_filename(swparse_strob_ptr, swparse_strob_ptr_len, "INFO", swws, swlex_filename, swparse_form_flag); 
				     level_push (&swws);
				     swparse_swdef_filetype = SWPARSE_SWDEF_FILETYPE_INFO;
				  }
			;

sw_INSTALLED_tree	: SW_INSTALLED { 
                                     swparse_strob_ptr_len = strlen(swlex_filename) + SWPARSE_UNKNOWN_EXTRA + swparse_atlevel;
                                     strob_setlen(tmp_strob, swparse_strob_ptr_len);
				     swparse_strob_ptr=strob_str(tmp_strob);
				     swparse_i_emit_filename(swparse_strob_ptr, swparse_strob_ptr_len, "INSTALLED", swws, swlex_filename, swparse_form_flag); 
				     level_push (&swws);
				     swparse_swdef_filetype = SWPARSE_SWDEF_FILETYPE_INSTALLED;
				   }
			;
sw_INDEX_tree		: SW_INDEX { 
                                     swparse_strob_ptr_len = strlen(swlex_filename) + SWPARSE_UNKNOWN_EXTRA + swparse_atlevel;
                                     strob_setlen(tmp_strob, swparse_strob_ptr_len);
				     swparse_strob_ptr=strob_str(tmp_strob);
				     swparse_i_emit_filename(swparse_strob_ptr, swparse_strob_ptr_len, "INDEX", swws, swlex_filename, swparse_form_flag); 
				     level_push (&swws);
				     swparse_swdef_filetype = SWPARSE_SWDEF_FILETYPE_INDEX;
				   }
			;

sw_PSF_tree		: SW_PSF { 
                                     swparse_strob_ptr_len = strlen(swlex_filename) + SWPARSE_UNKNOWN_EXTRA + swparse_atlevel;
                                     strob_setlen(tmp_strob, swparse_strob_ptr_len);
				     swparse_strob_ptr=strob_str(tmp_strob);
				     swparse_i_emit_filename(swparse_strob_ptr, swparse_strob_ptr_len, "PSF", swws, swlex_filename, swparse_form_flag); 
				     level_push (&swws); 
				     swparse_swdef_filetype = SWPARSE_SWDEF_FILETYPE_PSF;
				 }
				 ;

sw_PSF_tree_incl	: SW_PSF_INCL {
                                     swparse_strob_ptr_len = strlen(swlex_filename) + SWPARSE_UNKNOWN_EXTRA + swparse_atlevel;
                                     strob_setlen(tmp_strob, swparse_strob_ptr_len);
				     swparse_strob_ptr=strob_str(tmp_strob);
				     swparse_i_emit_filename(swparse_strob_ptr, swparse_strob_ptr_len, "PSF", swws, swlex_filename, swparse_form_flag); 
				     level_push (&swws); 
				     swparse_swdef_filetype = SWPARSE_SWDEF_FILETYPE_PSF;
				 }
				 ;

sw_OPTION_tree		: SW_OPTION {
				     level_push (&swws);
				}
				;

sw_OPTION_file		: option_contents
			;

sw_INFO_file		: info_contents
			;

sw_INDEX_file		: soc_definition_index
			  soc_contents_index
			  level_pop
			;

sw_INSTALLED_file	: soc_installed_software
			  soc_contents_psf
			  level_pop
			;

sw_PSF_file		: host_definition_i
			 sw_PSF_file_i
			;

sw_PSF_file_i		: distribution_definition_psf
			  level_push
			  soc_contents_psf
			;

sw_PSF_file_incl	: {swlex_yacc_feedback_fileset=1;} 
			  file_psf_boot_incl 		/* see swlex.l: swlex_input() for explanation. */
			  fileset_contents_psf
			;

option_contents		:  option_content_items
			;

info_contents		:  info_content_items
			;

option_content_items	: option_lines
			;

info_content_items	: files
			;

level_push		: {
                               level_push ( &swws );
			  }
                        ;

level_pop		: {
                               level_pop ( &swws );
			  }
                        ;

option_lines		: option_lines option_items
			| option_items
			;

files			: files file_items
			| file_items
			;

option_items		:
			| option_definition
			;	


file_items		:  file_normal
			|  file_extended_any
			|  control_file_normal
			;	

soc_definition_index	: host_definition_i
			  soc_definition_index_ii 
			;

soc_installed_software	: host_definition_i
			  installed_software_definition_index
			;

host_definition_i	:  /* empty */
			| host_normal level_push
			;

soc_definition_index_ii	: distribution_definition_index
			| installed_software_definition_index
			;

soc_contents_psf		: 
				| soc_contents_psf_a
				;

soc_contents_psf_a		: soc_contents_psf_a soc_content_items_psf
				| soc_content_items_psf
				;

soc_contents_index		: 
				| soc_contents_index_a
				;

soc_contents_index_a		: soc_contents_index_a soc_content_items_index
				| soc_content_items_index
				;

soc_content_items_index	: vendor_normal
			| category_normal
			| bundle_normal
 			| products_index
			;

soc_content_items_psf	: vendor_normal
			| category_normal
			| bundle_normal
 			| products_psf
			;

distribution_definition_index	: kwo_distribution level_push
			 	  layout_version_definition
			  	  attribute_value_list
				  SW_TERM_NEWLINE_STRING
				  media
				;

installed_software_definition_index: kwo_installed_software level_push
			 	  layout_version_definition_o
			  	  attribute_value_list
                                  /* installed_software_att_list */
				  SW_TERM_NEWLINE_STRING
				  media
				;

installed_software_att_list: layout_version_definition |
			  	  attribute_value_list
				;
distribution_definition_psf	: distribution_normal_psf
				  level_push media level_pop
				;


fileset_specification_index	: fileset_normal
				;


fileset_specification_psf	: fileset_normal 
				  level_push 
				  fileset_contents_psf { swlex_yacc_feedback_fileset=0;}
			          level_pop	
				;

fileset_contents_psf	: 
			| fileset_contents_psf_a
			;

fileset_contents_psf_a	: fileset_contents_psf_a fileset_content_items_psf 
		 	| fileset_content_items_psf
			;	

fileset_content_items_psf	: fileset_psf_ctrlfile_on control_file_normal fileset_psf_ctrlfile_off
				| fileset_psf_object_on file_normal fileset_psf_object_off
				| file_extended_any 
				;	

fileset_psf_ctrlfile_on: {swlex_yacc_feedback_controlfile_object=1;}
			;

fileset_psf_ctrlfile_off: {swlex_yacc_feedback_controlfile_object=0;}
			;

fileset_psf_object_on: {swlex_yacc_feedback_file_object=1;}
			;

fileset_psf_object_off: {swlex_yacc_feedback_file_object=0;}
			;


filesets_psf			: 	{swlex_yacc_feedback_fileset=1;} 
					fileset_specification_psf 
				;


            /* PSF Product */


products_psf			:  product_specification_psf
				;


product_specification_psf	: {swlex_yacc_feedback_fileset=1;} 
				  {swlex_yacc_feedback_directory=1;} 
				  {location = SWPARSE_ILOC_PRODUCT; }
				  product_normal
				  product_control_files_psf { swlex_yacc_feedback_fileset=0; swlex_yacc_feedback_directory=0; } 
				  product_contents_psf
				  {location = SWPARSE_ILOC_OFF; }
			          level_pop	
				;


/* the above rule imposes that `product_control_files' comes before `product_contents_psf'
   because unless it does there is a ambiguity in the language grammar as specified in the standard 
   because `product_contents_psf' can contain control_files itself and therefore it is unknown if a trailing
   control_file definition belongs to the product or the fileset */


product_contents_psf	: product_contents_psf product_content_items_psf
			| product_content_items_psf
			;

product_content_items_psf	: filesets_psf
				/* | vendor_normal */
				| subproduct_normal
				;


product_control_files_psf	:  product_control_files_psf_a
				;

product_control_files_psf_a	: 
				| product_control_files_psf_n
				;

product_control_files_psf_n	: product_control_files_psf_n product_control_file_items_psf
				| product_control_file_items_psf
				;

product_control_file_items_psf	:  fileset_psf_ctrlfile_on control_file_normal fileset_psf_ctrlfile_off
				|  file_extended_any 
				;


        /* PSF Product END */



         /* INDEX Product */


products_index			: product_specification_index
				;

product_specification_index	: product_normal
			  	  product_contents_index
			          level_pop	
				;

product_contents_index	: product_contents_index product_content_items_index
			| product_content_items_index
			;

product_content_items_index	: fileset_specification_index
				| vendor_normal
				| subproduct_normal
				;


        /* INDEX Product END */



media			: 
			| media_normal
			;


           /* Ten (10) object keywords headers*/

kwo_control_file	: SW_OK_CONTROL_FILE SW_OK_NEWLINE_STRING kwo_common_action { ; };

kwo_file		: SW_OK_FILE SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_fileset		: SW_OK_FILESET SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_host		: SW_OK_HOST SW_OK_NEWLINE_STRING kwo_common_action { ; } 
			;

kwo_distribution	: SW_OK_DISTRIBUTION SW_OK_NEWLINE_STRING kwo_common_action { ; } 
			;

kwo_installed_software	: SW_OK_INSTALLED_SOFTWARE SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_bundle		: SW_OK_BUNDLE SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_category		: SW_OK_CATEGORY SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_product		: SW_OK_PRODUCT SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_subproduct		: SW_OK_SUBPRODUCT SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_vendor		: SW_OK_VENDOR SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_media		: SW_OK_MEDIA SW_OK_NEWLINE_STRING kwo_common_action { ; }
			;

kwo_common_action :   {
                        swparse_i_emit_object_keyword(swparse_outputfd, strob_str ($<strb>-1), strlen(swws), swparse_form_flag);
		        strob_strcpy (tmp_strob, "");
                      }
	              ;
	     
	     
	     /* end of object keywords */

             /* these are the typical object definitions */

host_normal		: kwo_host level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

vendor_normal		: kwo_vendor level_push
			  { did_see_vendor_misleading = 0; }
			  attribute_value_list
			  { if (did_see_vendor_misleading == 0 &&  swparse_swdef_filetype == SWPARSE_SWDEF_FILETYPE_PSF)
				{
					;
					/* FIXME: maybe enable this warning conditionally based on ./configure options 
					fprintf(stderr,
						"swpackage: Warning: Because the object keyword \"vendor\" is misleading, add the\n"
						"swpackage: following attribute to the vendor object in the PSF (this is only a warning):\n"
						"swpackage:        the_term_vendor_is_misleading \"true\" # One of: true, false\n"
						);
					*/
				}
			   ; }
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

bundle_normal		: kwo_bundle level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

category_normal		: kwo_category level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

subproduct_normal	: kwo_subproduct level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

product_normal		: kwo_product level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			;


control_file_normal	: kwo_control_file  level_push
			  {location = SWPARSE_ILOC_CONTROL_FILE; }
			  attribute_value_list
			  {location = SWPARSE_ILOC_OFF; }
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

file_psf_boot_incl	: SW_OK_FILE SW_OK_NEWLINE_STRING level_push
			  SW_TERM_NEWLINE_STRING
			  level_pop { ; }
			;

file_normal		: kwo_file level_push
			  {location = SWPARSE_ILOC_FILE; }
			  attribute_value_list
			  {location = SWPARSE_ILOC_OFF; }
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

media_normal		: kwo_media level_push
			  attribute_value_list
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

distribution_normal_psf	: kwo_distribution level_push
			  layout_version_definition_o
			  {location = SWPARSE_ILOC_DISTRIBUTION; }
			  attribute_value_list
			  {location = SWPARSE_ILOC_OFF; }
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

layout_version_definition_o  :
                             | layout_version_definition
			     ;



fileset_normal		: kwo_fileset level_push
			  {location = SWPARSE_ILOC_FILESET; }
			  attribute_value_list
			  {location = SWPARSE_ILOC_OFF; }
			  SW_TERM_NEWLINE_STRING
			  level_pop
			;

	    /* end of object definitions */

          /* extended definitions */

layout_version_definition	: 	layout_version_keyword  SW_WHITE_SPACE_STRING attribute_value SW_NEWLINE_STRING { 
					swparse_i_emit(tmp_strob, swparse_outputfd, NULL, SWPARSE_ACMD_EMIT, strlen(swws), swparse_keytype, swparse_form_flag);
                                                                                                                  }
				;
layout_version_keyword  : SW_AK_LAYOUT_VERSION {
						 swparse_construct_attribute
						     (tmp_strob, swparse_outputfd, "", SWPARSE_ACMD_COPY, strlen(swws), 
						     swparse_keytype,swparse_form_flag);
					         swparse_construct_attribute 
						     (tmp_strob, swparse_outputfd, strob_str($1), SWPARSE_ACMD_CAT, 
						     strlen(swws), swparse_keytype, swparse_form_flag);
					         swparse_construct_attribute
						     (tmp_strob, swparse_outputfd, " ", SWPARSE_ACMD_CAT, 
						      strlen(swws), swparse_keytype, swparse_form_flag);
						 swparse_keytype=SWPARSE_EMITTED_TYPE_ATT;
					       }
				;

file_extended_any	: extended_definition
			;

attribute_value_list	: 
			| attribute_value_list_n
			 ;

attribute_value_list_n	: attribute_value_list_n attribute_definition
			| attribute_definition
			;


attribute_definition	: attribute_keyword SW_WHITE_SPACE_STRING attribute_value SW_NEWLINE_STRING 
                                                              {
								        /* write out the attribute */	
									if (strstr(strob_str(tmp_strob), "the_term_vendor_is_misleading")) {
			  							did_see_vendor_misleading = 1;
									}	
									if (
										((swparse_form_flag & SWPARSE_FORM_POLICY_POSIX_IGNORES)) &&
										swparse_ignore_attribute
											(
												swparse_swdef_filetype,
												location,
												strob_str(tmp_strob)
											) == 1
									) {
										/*
										* Ignore attribute as specified in the Std.
										*/
										;
									} else {
										swparse_i_emit
										   (tmp_strob, swparse_outputfd, NULL, 
										   SWPARSE_ACMD_EMIT, strlen(swws), swparse_keytype, 
										   swparse_form_flag);
							      		}
							      }
			;

option_definition	: attribute_keyword SW_WHITE_SPACE_STRING attribute_value SW_NEWLINE_STRING 
                                                              {
									swparse_i_emit
									   (tmp_strob, swparse_outputfd, NULL, 
									   SWPARSE_ACMD_EMIT, strlen(swws), swparse_keytype, 
									   swparse_form_flag);
							      }
			;

extended_definition	: extended_keyword SW_EXT_WHITE_SPACE_STRING attribute_value SW_NEWLINE_STRING 
                                                               {
								        /* write out the attribute */	
									swparse_i_emit
									   (tmp_strob, swparse_outputfd, NULL, 
									   SWPARSE_ACMD_EMIT, strlen(swws), swparse_keytype, 
									   swparse_form_flag);
							       } 
			;


attribute_keyword	: SW_ATTRIBUTE_KEYWORD {
						 swparse_construct_attribute
						    (tmp_strob, swparse_outputfd, "", SWPARSE_ACMD_COPY, 
						    strlen(swws), swparse_keytype, swparse_form_flag);
					         swparse_construct_attribute
						    (tmp_strob, swparse_outputfd, strob_str($1), SWPARSE_ACMD_CAT, 
						    strlen(swws), swparse_keytype,swparse_form_flag);
					         swparse_construct_attribute
						    (tmp_strob, swparse_outputfd, " ", SWPARSE_ACMD_CAT, 
						    strlen(swws), swparse_keytype, swparse_form_flag);
						 swparse_keytype=SWPARSE_EMITTED_TYPE_ATT;
					       }
			;

extended_keyword	: SW_EXT_KEYWORD {
						swparse_construct_attribute
						    (tmp_strob, swparse_outputfd, "", SWPARSE_ACMD_COPY, 
						    strlen(swws), swparse_keytype,swparse_form_flag);
					        swparse_construct_attribute(tmp_strob, swparse_outputfd, 
						    strob_str($1), SWPARSE_ACMD_CAT, strlen(swws), 
						    swparse_keytype, swparse_form_flag);
					        swparse_construct_attribute
						    (tmp_strob, swparse_outputfd, " ", SWPARSE_ACMD_CAT, 
						    strlen(swws), swparse_keytype, swparse_form_flag);
						swparse_keytype=SWPARSE_EMITTED_TYPE_EXT;
					     
					 }
			;

attribute_value		: single_value { ; } 
			| '<' SW_WHITE_SPACE_STRING  pathname  
			| '<' pathname
			;

single_value		: SW_SHELL_TOKEN_STRING  {
					             swparse_construct_attribute
						        (tmp_strob, swparse_outputfd, strob_str($1), SWPARSE_ACMD_CAT, 
							strlen(swws), swparse_keytype, swparse_form_flag);
						  }
			;
pathname		: SW_PATHNAME_CHARACTER_STRING  {
					                   swparse_construct_attribute
							      (tmp_strob, swparse_outputfd, "<", SWPARSE_ACMD_CAT, 
							      strlen(swws), swparse_keytype, swparse_form_flag);
					                   swparse_construct_attribute
							      (tmp_strob, swparse_outputfd, strob_str($1), SWPARSE_ACMD_CAT, 
							      strlen(swws), swparse_keytype, swparse_form_flag);
						         }
			;
%%

static
int
level_push ( char ** sw ) {
  (*sw)--;
  return 0;
}

static
int
level_pop ( char ** sw ) {
  if ( (*sw) == '\0' ) { 
       fprintf (stderr, "swparse.y: pop error\n");
       return -1;
  }
  (*sw)++;
  return 0;
}

static
int 
swparse_i_emit(STROB * strb, int output_fd, char * src, int cmd, int level, char s_keytype, int form_flag)
{
	return swparse_construct_attribute(strb, output_fd, src, cmd, level, s_keytype, form_flag);
}

static int 
swparse_i_emit_object_keyword(int outputfd, char * key, int level, int form_flag)
{
	return swparse_write_attribute_obj(outputfd, key, level, form_flag);
}

static 
int 
swparse_i_emit_filename (char * buf, int len, char * filetype, char * ws_level_string, char * swlex_filename, int form_flag)
{
	if ((form_flag & SWPARSE_FORM_INDENT) == 0)
		return swparse_print_filename(buf, len, filetype, ws_level_string, swlex_filename, form_flag);
	else
		return 0;
}
