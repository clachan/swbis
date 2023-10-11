#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strob.h"
#include "swlex_supp.h"

int swlex_definition_file = SW_INDEX;
int swlex_debug=0;
int swlex_errorcode=0;
extern struct swbis_keyword sw_keywords[];

int swlex_inputfd=0;

YYSTYPE yylval;

int swparse_debug=0;
int yydebug=0;
char swlex_filename[512];

int swparse_outputfd=0;

int main (int argc, char *argv[])
{
  int val;

  if ( argc < 2 ) {
    fprintf (stderr, "%s : error.\n", argv[0]);
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    %s {INFO|PSF|INDEX}\n", argv[0]);
    exit (1);
  }

  yylval.strb = strob_open (8);

  if (!yylval.strb) {
      fprintf (stderr," swlex_test: object allocation error.\n");
      exit (1);
  }

  if ( ! strcmp ("INFO", argv[1])) {
     swlex_definition_file = SW_INFO;
  } else if ( ! strcmp ("INDEX", argv[1])) {
     swlex_definition_file = SW_INDEX;
  } else if ( ! strcmp ("PSF", argv[1])) {
     swlex_definition_file = SW_PSF;
  } else if ( ! strcmp ("OPTION", argv[1])) {
     swlex_definition_file = SW_OPTION;
  } else {
      fprintf (stderr," swlex_test: invalid file name.\n");
      exit (1);
  }


     while ( (val=yylex()) || 1 ) {
        switch (val) {
		case SW_SHELL_TOKEN_STRING:
			printf ("SW_SHELL_TOKEN_STRING {%s}\n", strob_str(yylval.strb));
			break;
		case SW_WHITE_SPACE_STRING:
			printf ("SW_WHITE_SPACE_STRING\n");
			break;
		case SW_EXT_WHITE_SPACE_STRING:
			printf ("SW_EXT_WHITE_SPACE_STRING\n");
			break;
		case SW_NEWLINE_STRING:
			printf ("SW_NEWLINE_STRING\n");
			break;
		case SW_TERM_NEWLINE_STRING:
			printf ("SW_TERM_NEWLINE_STRING\n");
			break;
		case SW_OK_NEWLINE_STRING:
			printf ("SW_OK_NEWLINE_STRING\n");
			break;
		case SW_ATTRIBUTE_KEYWORD:
			printf ("SW_ATTRIBUTE_KEYWORD {%s}", strob_str(yylval.strb));
			break;
		case SW_OBJECT_KEYWORD:
			printf ("SW_OBJECT_KEYWORD {%s}", strob_str(yylval.strb));
			break;
		case SW_PATHNAME_CHARACTER_STRING:
			printf ("SW_PATHNAME_CHARACTER_STRING {%s}\n", strob_str(yylval.strb));
			break;
	        case SW_INDEX:	
			printf ("{INDEX}\n");
			break;
	        case SW_PSF:	
			printf ("{SW_PSF}\n");
			break;
	        case SW_INFO:	
			printf ("{INFO}\n");
			break;
		case 0: /* SW_LEXER_EOF: */
			printf ("SW_LEXER_EOF end of file\n");
			exit (0);
			break;
		case '<':
			printf ("less than symbol{%s}\n", strob_str(yylval.strb));
			break;

		case SW_OK_DISTRIBUTION:
			printf ("SW_OK_DISTRIBUTION {%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_INSTALLED_SOFTWARE:
			printf ("SW_OK_INSTALLED_SOFTWARE {%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_BUNDLE:
			printf ("SW_OK_BUNDLE{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_PRODUCT:
			printf ("SW_OK_PRODUCT{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_SUBPRODUCT:
			printf ("SW_OK_SUBPRODUCT{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_FILESET:
			printf ("SW_OK_FILESET{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_CONTROL_FILE:
			printf ("SW_OK_CONTROL_FILE{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_FILE:
			printf ("SW_OK_FILE{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_VENDOR:
			printf ("SW_OK_VENDOR{%s}\n", strob_str(yylval.strb));
			break;
		case SW_OK_MEDIA:
			printf ("SW_OK_MEDIA{%s}\n", strob_str(yylval.strb));
			break;
		case SW_EXT_KEYWORD:
			printf ("SW_EXT_KEYWORD{%s}\n", strob_str(yylval.strb));
			break;
		case SW_RPM_KEYWORD:
			printf ("SW_RPM_KEYWORD{%s}\n", strob_str(yylval.strb));
			break;
		case SW_AK_LAYOUT_VERSION:
			printf ("SW_AK_LAYOUT_VERSION{%s}\n", strob_str(yylval.strb));
			break;
		default:
			printf ("swlex_test: error %s\n", strob_str(yylval.strb));
			exit (1);
                        break;

	}
     }
    printf ("swlex_test: yylex error\n");
    exit (1);
}



