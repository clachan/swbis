%{
/*
swlex.l
*/
/* 
//  swlex.l
//  Copyright (c) 1997-98 James Lowe, Jr. <jhl@richmond.infi.net>

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2, or (at your option)
//  any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/* 
  A lex(1) Scanner for POSIX.7.2 Software Package Metadata Files


File Format Syntax Description :
    The file consists mostly of KEYWORD/VALUE pairs.
    Whitespace separate the value from the keyword.
    Each Keyword (and Value) is on its own line.
    Quoted Values can be multi-Line.
    Comments are preceeded by a '#'.
    Comments can be on the same line as a unquoted value
    at the end of the line.
    A '#' can be escaped within a unquoted value.
    A '\"' can be escaped within a quoted value.
 
    Some Keywords are Object keywords and have nothing on
    the same line after them except whitespace and a newline.

    A '<' less-than symbol means the contents of the file
    given by the succeeding PATHNAME is the quoted value for 
    the preceeding keyword. 

*/

#include "swuser_config.h"
#include "string.h"
#include "strob.h"
#include "swlex_supp.h"
#include "swutilname.h"

/*
 * The struct swbis_keyword must be the same as the
 * struct in sw_keyword.h
 */

struct swbis_keyword {
   char * name;
   int flag;
};

#undef YY_INPUT
#define YY_INPUT(b,r,ms) (r=swlex_input(b,ms))

int yyerror(char*);

/**********************************************************************/
/*  -----------------  External Global Variables ------------------   */    
/*             ___Must be Provided by calling program____             */
extern int swlex_debug;            /* Debug switch                    */
extern int swlex_definition_file;  /* file name that is being scanned */
extern int swlex_inputfd;          /* type <UXFIO> file descriptor    */  
extern int swlex_errorcode;        /* error code, 0 is no error       */  
/* ------------------------------------------------------------------ */
/**********************************************************************/


/**********************************************************************/
/* ----------------- Internally Satisfied External Global ------------*/
extern struct swbis_keyword sw_keywords[];  /*       See sw_keyword.h */
                                      /* Put your keywords here ^^^   */
/* ------------------------------------------------------------------ */
/**********************************************************************/


/**********************************************************************/
/* ---------- Parser feed-back variable Global ---------------------- */
int swlex_yacc_feedback_directory=0;    /* set to 1 by the parser      */
int swlex_yacc_feedback_fileset=0;    /* set to 1 by the parser      */
                                       /* when parsing PSF filesets.  */

int swlex_yacc_feedback_file_object=0; /* set to 1 when in file or    */
                                       /* controlfile objects in PSF  */
				       /* filesets.                   */
int swlex_yacc_feedback_controlfile_object=0;  /*                     */
/* ------------------------------------------------------------------ */
/**********************************************************************/

static int utf8_enforce = 0;     /* 0 error, 1 translate, 2 ignore  */  
static STROB * swlex_unmatched;
static STROB * swlex_strb;
static unsigned char * user_cp;
static unsigned char  user_ch;
static int not_first_time = 0;
static int psfi_first_read = 1;
static int is_extended_keyword = 0;
static int first_keyword=1;
static int have_terminated=0;
static int not_very_first_time = 0;
static int in_object_definition=0;
static int swlex_l_i, keytype;
extern int swlex_linenumber;

int swlex_handle_keyword ( char *string, int keycode , int * keytype);
int swlex_input (char * buf, int max_size);
int do_test_ext(void);
size_t uxfio_sfread (int swlex_inputfd, void * buf, size_t max_size); 
void swlex_squash_trailing_white_space (char *);

%}

objectkeywordws	[ \t]*\n
ws		[ \t]+
nows		[^ \t\n]*
lws		^[ \t]+
optiondelim	[ \t]*=[ \t]*
comment		[ \t]*#.*
uqcomment	[^#\n]+#
uqstring	[^\n#]+\n
optuqstring	[^\n#]*(\n|#)
unquotedvalue	[^<\"#]
optunquotedvalue [^<\"#]|#
beginquote	\"
endquote	\"
qstring		[^\"]*\"
nonUTF8_bincharsA	[\000-\007]|[\x7f-\xff] 
nl		([ \t]*\n)|([ \t]*#.*\n)|(\n)|(end[ ]*.*\n)
keyword		([a-zA-Z_][a-zA-Z0-9_]+\.[a-zA-Z0-9_\.\-]+)|([a-zA-Z_][a-zA-Z0-9_\-]*)
/* Object keywords are 4 or more chars long, this is a hack to support the HP 'end' keyword */
objectkeyword	[a-zA-Z_][a-zA-Z_][a-zA-Z_][a-zA-Z_][a-zA-Z0-9_]*[ \t]*\n
input_redir	\<
invalid_input	.+
default_input	.|\n

%s QUOTEDSTRING
%s VALUE
%s ENDQUOTE
%s OPTVALUE
%s UNQUOTEDSTR
%s OPTUNQUOTEDSTR
%s OPTIONVALUEWS
%s VALUEWS
%s INPUT_REDIR
%s INITIAL_NEWLINE
%s INVALID_INPUT
%s FINAL_EOF

%%

%{
  if (!not_first_time) { /* tell the parser what type of file it is */
        not_first_time=1; 
	if ( not_very_first_time ) { 
           yyrestart(NULL); 
	   yy_n_chars=0;
	   yy_start = 0;
	   yy_did_buffer_switch_on_eof=0;
	} 
        swlex_unmatched = strob_open(8); 
        swlex_strb = strob_open(8); 
	return swlex_definition_file;
  }
  not_very_first_time = 1;
%}

{nonUTF8_bincharsA} { 
		/* I think this will never be matched, effectively dead code */
		yyerror("invalid or binary input"); 
		exit(1); 
	}

<QUOTEDSTRING>{qstring} {
                          if ( handle_termination ('\"',  swlex_strb, yytext, "loc=A" )) {
			      BEGIN ENDQUOTE;
                              unput(*(strob_str(swlex_strb)+strlen(strob_str(swlex_strb))-1)); 
			      user_cp = (unsigned char *)(strob_str (swlex_strb) + strlen(strob_str (swlex_strb)) - 1); 
			      user_ch = *user_cp;
			      *user_cp = '\0';
			      strob_strcpy(yylval.strb, "\""); 
			      strob_strcat(yylval.strb, strob_str(swlex_strb)); 
			      strob_strcat(yylval.strb, "\""); 
			      if (swlex_debug) fprintf (stderr,"return SW_SHELL_TOKEN_STRING {%s}\n", yytext);
			      return SW_SHELL_TOKEN_STRING;
                          }
		      }

<ENDQUOTE>{endquote} {
                           if (handle_termination ('\"',  swlex_strb, yytext, "loc=B" )) {
                             BEGIN INITIAL_NEWLINE; 
			     if (swlex_debug) 
			         fprintf (stderr,"BEGIN INITIAL_NEWLINE {endquote}%s{end of endquote}\n", yytext);
		           }
                     }

<UNQUOTEDSTR>{uqcomment} {
                          if ( handle_termination ('#',  swlex_strb, yytext, "loc=C" )) {
			      BEGIN INITIAL_NEWLINE;
                              unput(*(strob_str(swlex_strb)+strlen(strob_str(swlex_strb))-1));
			      user_cp  = (unsigned char *)(strob_str(swlex_strb) + strlen(strob_str (swlex_strb))-1);
			      user_ch = *user_cp;
			      *user_cp  = '\0';
                              swlex_squash_trailing_white_space(strob_str(swlex_strb));
			      strob_strcpy(yylval.strb, strob_str(swlex_strb)); 
			      if (swlex_debug) fprintf (stderr,"return SW_SHELL_TOKEN_STRING {%s}\n", yytext);
		              return SW_SHELL_TOKEN_STRING; 
			   }   
			 }

<OPTUNQUOTEDSTR>{optuqstring} {
                           if ( handle_termination ('#',  swlex_strb, yytext, "loc=D" )) {
                              BEGIN INITIAL_NEWLINE;
			      if (swlex_debug) fprintf (stderr,"in <OPTUNQUOTEDSTR>{optuqstring}: handle_termination is TRUE\n");
                              unput(*(strob_str(swlex_strb)+strlen(strob_str(swlex_strb))-1));
			      user_cp = (unsigned char *)(strob_str(swlex_strb) + strlen(strob_str(swlex_strb))-1);
			      user_ch = *user_cp;
			      *user_cp = '\0';
                              swlex_squash_trailing_white_space(strob_str(swlex_strb));
			      strob_strcpy(yylval.strb, strob_str(swlex_strb));
			      if (swlex_debug) fprintf (stderr,"return SW_SHELL_TOKEN_STRING {%s}\n", yytext);
		              return SW_SHELL_TOKEN_STRING; 
                            } else {
			      if (swlex_debug) fprintf (stderr,"in <OPTUNQUOTEDSTR>{optuqstring}: handle_termination is FALSE\n");
			    }
                         }


<UNQUOTEDSTR>{uqstring} {
                           if ( handle_termination ('\n',  swlex_strb, yytext, "loc=E" )) {
                              BEGIN INITIAL_NEWLINE;
			      if (swlex_debug) fprintf (stderr,"in <UNQUOTEDSTR>{uqstring}: handle_termination is TRUE\n");
                              unput(*(strob_str(swlex_strb)+strlen(strob_str(swlex_strb))-1));
			      user_cp = (unsigned char *)(strob_str(swlex_strb) + strlen(strob_str(swlex_strb))-1);
			      user_ch = *user_cp;
			      *user_cp = '\0';
                              swlex_squash_trailing_white_space(strob_str(swlex_strb));
			      strob_strcpy(yylval.strb, strob_str(swlex_strb));
			      if (swlex_debug) fprintf (stderr,"return SW_SHELL_TOKEN_STRING {%s}\n", yytext);
		              return SW_SHELL_TOKEN_STRING; 
                            } else {
			      if (swlex_debug) fprintf (stderr,"in <UNQUOTEDSTR>{uqstring}: handle_termination is FALSE\n");
			    }
                         }

<VALUE>{beginquote} { BEGIN QUOTEDSTRING; 
                      if (swlex_debug)
		          fprintf (stderr,"BEGIN QUOTEDSTRING <VALUE>{beginquote}%s{end of beginquote}}\n", yytext);
                    }

<OPTVALUE>{beginquote} { BEGIN QUOTEDSTRING; 
                      if (swlex_debug)
		          fprintf (stderr,"BEGIN QUOTEDSTRING <VALUE>{beginquote}%s{end of beginquote}}\n", yytext);
                    }

<OPTVALUE>{optunquotedvalue} {   
                        BEGIN OPTUNQUOTEDSTR;
			if (swlex_debug) fprintf (stderr, "<OPTVALUE>{unquotedvalue}%s{end}\n", yytext);
		        unput(yytext[strlen(yytext)-1]);
                        if (swlex_debug)
			    fprintf (stderr,"BEGIN OPTUNQUOTEDSTR {unquotedvalue Look Ahead }%s{end unquotedvalue Look Ahead}\n", yytext); 
                       }


<VALUE>{unquotedvalue} {   
                        BEGIN UNQUOTEDSTR;
			if (swlex_debug) fprintf (stderr, "<VALUE>{unquotedvalue}%s{end}\n", yytext);
		        unput(yytext[strlen(yytext)-1]);
                        if (swlex_debug)
			    fprintf (stderr,"BEGIN UNQUOTEDSTR {unquotedvalue Look Ahead }%s{end unquotedvalue Look Ahead}\n", yytext); 
                       }

<VALUE>{input_redir} {
			BEGIN INPUT_REDIR;
			
                        if (swlex_debug) fprintf (stderr,"BEGIN INPUT_REDIR {input_redir Look Ahead }%s{end input_redir Look Ahead}\n", yytext); 
		
                        strob_strcpy(yylval.strb, yytext);
	
                        if (swlex_debug) fprintf (stderr,"in VALUE{input_redir} %c\n", yytext[0]); 

                        return yytext[0];  
		      }

<VALUE>{comment} {
                    swlex_errorcode = 1;
                    return -1;  
                  }


<INPUT_REDIR>{ws} {
		    if (swlex_debug) fprintf(stderr,"In INPUT_REDIR:  returning SW_WHITE_SPACE_STRING\n");
		    return SW_WHITE_SPACE_STRING;
                  }


<INITIAL_NEWLINE>{nl} {
		    BEGIN INITIAL; 
		    if (swlex_debug) fprintf(stderr,"return SW_NEWLINE_STRING\n");
		    return SW_NEWLINE_STRING;
                  }

<INPUT_REDIR>{nows} {
                     BEGIN INITIAL_NEWLINE;
                     strob_strcpy(yylval.strb, yytext);
                     swlex_squash_trailing_white_space(strob_str(yylval.strb));
		     if (swlex_debug) fprintf (stderr,"return SW_PATHNAME_CHARACTER_STRING\n");
		     if ( swlex_yacc_feedback_file_object|| swlex_yacc_feedback_controlfile_object) { 
		    		/*
				* error.
				*/
				fprintf(stderr, "%s: swparser: attribute file inclusion not allowed in file and control_file objects.\n", swlib_utilname_get());
			 	return SW_OK_NEWLINE_STRING;  /* This causes a syntax error. */
		     }
		     return SW_PATHNAME_CHARACTER_STRING;
                    }

<OPTIONVALUEWS>{optiondelim}     {   
		    BEGIN OPTVALUE;  
		    first_keyword=0; 
		    if (swlex_debug) fprintf(stderr, "<OPTIONVALUEWS>{ws}%s{end}\n", yytext);
		    if (is_extended_keyword==1) {
		       is_extended_keyword=0;
		       if (swlex_debug) fprintf(stderr,"return SW_EXT_WHITE_SPACE_STRING\n");
		       return SW_EXT_WHITE_SPACE_STRING;
                    }
		    if (swlex_debug) fprintf(stderr,"return SW_WHITE_SPACE_STRING\n");
		    return SW_WHITE_SPACE_STRING;
		   }

<VALUEWS>{ws}     {   
		    BEGIN VALUE;  
		    first_keyword=0; 
		    if (swlex_debug) fprintf(stderr, "<VALUE>{ws}%s{end}\n", yytext);
		    if (is_extended_keyword==1) {
		       is_extended_keyword=0;
		       if (swlex_debug) fprintf(stderr,"return SW_EXT_WHITE_SPACE_STRING\n");
		       return SW_EXT_WHITE_SPACE_STRING;
                    }
		    if (swlex_debug) fprintf(stderr,"return SW_WHITE_SPACE_STRING\n");
		    return SW_WHITE_SPACE_STRING;
		   }


<VALUEWS>{objectkeywordws} {  
			 
			 BEGIN INITIAL; 
		         first_keyword=0; 
			 if (swlex_debug) 
			    fprintf (stderr,"<VALUEWS> {objectkeywordws}%s{end of object keywordws}\n",yytext);
		         if (swlex_debug) fprintf (stderr,"return SW_OK_NEWLINE_STRING\n");
			 return SW_OK_NEWLINE_STRING;
			 }



<INITIAL>{lws}     { if (swlex_debug) fprintf(stderr,"<INITIAL>{lws}\n");}
<INITIAL>{ws}      { if (swlex_debug) fprintf(stderr,"<INITIAL>{ws}\n"); }
<INITIAL>{comment} { 
                         if (swlex_debug) printf("<INITIAL>{comment}%s{end of comment}\n", yytext);
			 
		    }

<INITIAL>{objectkeyword} {

                     BEGIN VALUEWS; 
		     if (swlex_debug) fprintf(stderr, "<INITIAL>{objectkeyword}%s{end}\n", yytext);
                     strob_strcpy(swlex_strb, "");
	             strob_strcpy(yylval.strb, yytext);
		     swlex_squash_trailing_white_space(strob_str(yylval.strb) );
		     swlex_l_i =  swlex_handle_keyword(strob_str(yylval.strb), -1, &keytype );
                     keytype = SWLEX_KEYTYPE_OBJECT;
		     if (swlex_l_i <0) return 0; 
		     
		     if ( (!first_keyword) && ( !have_terminated ) ) { 
		               /* return termination newline token */ 
		               have_terminated = 1;           
			       BEGIN INITIAL; /* issue the terminating newline and redo this state */
		               yyless(0); 
		               in_object_definition=0;
		               if (swlex_debug) fprintf (stderr,"return SW_TERM_NEWLINE_STRING\n");
			       return SW_TERM_NEWLINE_STRING;  /* this token terminates software_definitions */
		     } else {
			       first_keyword=0;
		               have_terminated = 0;
		               unput ('\n'); 
		               in_object_definition=1;
		               if (swlex_debug) fprintf (stderr,"return swlex_l_i %d\n", swlex_l_i);
			       return swlex_l_i;  /* return the keyword token */
		     }
                   }
<INITIAL>{keyword} {
   		     if (swlex_definition_file != SW_OPTION) {
                     	BEGIN VALUEWS; 
                     } else {
                     	BEGIN OPTIONVALUEWS; 
		     }
		     if (swlex_debug) fprintf (stderr, "<INITIAL>{keyword}%s{end}\n", yytext);
                     strob_strcpy(swlex_strb, "");
	             strob_strcpy(yylval.strb, yytext);
		     
		     swlex_l_i = swlex_handle_keyword(strob_str(yylval.strb), -1, &keytype );
		     
		     /* allow unrecognized keywords */
		     if (swlex_l_i < 0) { 
		        swlex_l_i = SW_ATTRIBUTE_KEYWORD; 
                     }

		     /* if in `fileset' , then extended keyword terminates object keyword */ 
		     
		      
		     if ( keytype == SWLEX_KEYTYPE_EXT && 
		              (swlex_yacc_feedback_fileset||
			      swlex_yacc_feedback_file_object||
			      swlex_yacc_feedback_controlfile_object)) { 
		              return do_test_ext();   
		     }
		     if ( keytype == SWLEX_KEYTYPE_EXT && !(swlex_yacc_feedback_fileset||
		                                                 swlex_yacc_feedback_file_object|| 
		                                                 swlex_yacc_feedback_controlfile_object) ) { 
			  fprintf (stderr,"error in fileset state feedback %s\n", yytext ); 
		     }

		     /* 'directory' or 'path' keyword type ambiguity */
		     if (keytype == SWLEX_KEYTYPE_AMBIG_AE ) { 
			 
			 if ( swlex_yacc_feedback_fileset == 1 && 
			      swlex_yacc_feedback_directory == 0 &&
			      strcmp(strob_str(yylval.strb), "directory") == 0
			 ) {
				 return do_test_ext();   
			 } else if ( (swlex_yacc_feedback_fileset == 0 || swlex_yacc_feedback_controlfile_object) ||
					swlex_yacc_feedback_directory 
			 ) {
		                    is_extended_keyword=0;
		                    if (swlex_debug) fprintf (stderr,"return SW_ATTRIBUTE_KEYWORD\n");
			            return SW_ATTRIBUTE_KEYWORD;
			 } else if ( (swlex_yacc_feedback_fileset == 0 || swlex_yacc_feedback_file_object) ||
					swlex_yacc_feedback_directory 
			 ) {
		                    is_extended_keyword=0;
		                    if (swlex_debug) fprintf (stderr,"return SW_ATTRIBUTE_KEYWORD\n");
			            return SW_ATTRIBUTE_KEYWORD;
		         } else {
				 return do_test_ext();   
			 }
		     } 
		     
		     if (keytype == SWLEX_KEYTYPE_AMBIG_OE || keytype == SWLEX_KEYTYPE_AMBIG_OA) { /* handle file and control_file type ambiguity*/
		          is_extended_keyword=1;
			  if ((!first_keyword) && (!have_terminated)) {
	                       if(!strcmp(strob_str(yylval.strb), "category")) {
		          		is_extended_keyword=0;
		               		if (swlex_debug) fprintf(stderr,"return SW_ATTRIBUTE_KEYWORD\n");
			       		return SW_ATTRIBUTE_KEYWORD;
			       } else {
			       		/* return termination newline token */ 
		  	       		have_terminated = 1;           
			       		BEGIN INITIAL; /* issue the terminating newline and redo this state */
		               		yyless(0); 
		               		in_object_definition=0;
		               		if (swlex_debug) fprintf(stderr,"return SW_TERM_NEWLINE_STRING\n");
			       		return SW_TERM_NEWLINE_STRING;  /* this token terminates software_definitions */
			       }
		          } else {
		               if (swlex_debug) fprintf (stderr,"return SW_EXT_KEYWORD\n");
			       return SW_EXT_KEYWORD;
		          }
		     }
		     return swlex_l_i; 
		    
		    }

<INITIAL>{nl}      { if (swlex_debug) fprintf (stderr,"<INITIAL>{nl}\n"); }
<INITIAL><<EOF>>   { BEGIN FINAL_EOF; 
                             if (swlex_debug) fprintf (stderr,"<INITIAL>{EOF}\n"); unput('_'); 
                             
			     if (in_object_definition) {
		                if (swlex_debug) fprintf(stderr,"return SW_TERM_NEWLINE_STRING\n");
                                return SW_TERM_NEWLINE_STRING;  
		   	     }
		   }


<FINAL_EOF>_   {
		   if (strob_strlen(swlex_unmatched)) {
                   	fprintf(stderr, "%s: warning (swlex.l): unmatched input: %s\n", swlib_utilname_get(), strob_str(swlex_unmatched));  
		   }
                   if (swlex_debug) fprintf (stderr, "<FINAL>{EOF}\n");  
		   yy_delete_buffer(YY_CURRENT_BUFFER);


/* this flex specific code is needed to
   reset the parser so it can be used more than once.
*/
yy_hold_char = 0;
yy_n_chars = 0;  
yy_c_buf_p = (char *) 0;
yy_init = 1;  
yy_start = 0; 
yy_did_buffer_switch_on_eof = 0;
/* yy_current_buffer = 0; */
/* yy_last_accepting_state = 0; */
/* yy_last_accepting_cpos = 0; */

		   swlex_yacc_feedback_fileset=0;  
                   swlex_yacc_feedback_file_object=0;
                   swlex_yacc_feedback_controlfile_object=0; 

                   not_first_time = 0;
                   is_extended_keyword = 0;
                   first_keyword=1;
                   have_terminated=0;
                   in_object_definition=0;
                   swlex_l_i=0;
                   keytype=0;
		   psfi_first_read = 1;
                   not_very_first_time = 1;
                   strob_close (swlex_strb);
                   strob_close (swlex_unmatched);
		   
		   BEGIN INITIAL;
		   if (swlex_debug) fprintf (stderr,"BEGIN INITIAL\n");
		   return 0; /* SW_LEXER_EOF */ 
}

<*>{default_input} {
          /* fprintf(stderr, " warning: unmatched input detected [%s]\n", yytext); */
          strob_strcat(swlex_unmatched, yytext);
}

%%

int yyerror (char * s)  { fprintf (stderr,"%s: swparser: %s\n", swlib_utilname_get(), s);
                          fprintf (stderr,"%s: swparser: at or near line %d: %s\n", swlib_utilname_get(), swlex_linenumber, strob_str(yylval.strb));
                          swlex_errorcode = 1;
			  return SW_LEXER_FATAL_ERROR;
			}

int swlex_input (char * buf, int max_size) {
   /* FIXME */
   /* For the SW_PSF_INCL filetype, feed an initial empty object
    *  to boot the lexer into a state where it works.  This is 
    *  a terrible work around due to a bug in an insanely
    *  complicated lexer.
    */
   int r;
   unsigned char * c;
   if (psfi_first_read && swlex_definition_file == SW_PSF_INCL) {
	psfi_first_read = 0;
	strcpy(buf, "file\n");
   	return strlen("file\n");
   }
   r = uxfio_sfread(swlex_inputfd, buf, max_size);
   if (r > 0) {
     c = (unsigned char *)buf;
     while (c && ((char*)c - buf) < r) {
	/* Perform a rough range check for valid UTF-8 chars */
	if (utf8_enforce == SWLEX_INPUT_UTF_ALLOW) {
		;
	} else {
		if (*c >= 245 || *c == 192 || *c == 193 || *c == 0 ) {
			fprintf(stderr, "%s: non UTF-8 input detected, Octet=%d (decimal), setting to '~'\n", swlib_utilname_get(), (int)(*c)); 
			*c = '~';
			if (utf8_enforce == SWLEX_INPUT_UTF_FATAL) {
				swlex_errorcode = 1;
				return -1;
			}
		}
	}
	/* Strip carriage returns cr */
        if (*c == '\r' && ((char*)(c+1) - buf) < r && *((char*)(c+1)) == '\n') {
		/* stripcr */
		memmove(c, c+1, (size_t)r - (size_t)((char*)(c+1) - buf));
		r--;
	}
        if (*c == '\n') swlex_linenumber++;
        c++;
     }
   }
   return r;
}

int
swlex_get_input_policy(void)
{
	return utf8_enforce;     /* 0 error, 1 translate, 2 ignore  */  
}

void
swlex_set_input_policy(int i)
{
	utf8_enforce = i;     /* 0 error, 1 translate, 2 ignore  */  
}

int
do_test_ext() {			 
         is_extended_keyword=1;
	 if ( (!first_keyword) && ( !have_terminated ) ) { 
       /* return termination newline token */ 
         have_terminated = 1;           
          BEGIN INITIAL; 
          yyless(0); 
          in_object_definition=0;
          if (swlex_debug) fprintf (stderr,"return SW_TERM_NEWLINE_STRING\n");
       	       return SW_TERM_NEWLINE_STRING;  /* this token terminates software_definitions */
          } else {
               if (swlex_debug) fprintf (stderr,"return SW_EXT_KEYWORD\n");
               return SW_EXT_KEYWORD;
	  }
}
