#ifdef DOTO_ID
static char testswheader_c[] = "testswheader2.c,v 1.1.1.1";
#endif


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <getopt.h>

#include "uxfio.h"
#include "cplob.h"
#include "swlib.h"
#include "shcmd.h"
#include "swparse.h"
#include "swheader.h"
#include "swheaderline.h"
#include "swlex_supp.h"
#include "swparser_global.h"
#include "swevents_array.h"
#include "swcommon_options.h"


char get_character(char ch);
int get_integer(int j);
double get_double(double x);
unsigned get_unsigned(unsigned uu);
signed char get_signedchar(int gch);
unsigned long get_ulong(unsigned long j);
void get_string(char *str);
long get_long(long int j);
static char *uget_gets(char *s);

int 
main(int argc, char **argv)
{
	char ch = '1';
	char str[1024];
	char keyword[512];
	char value[512];
	char tag[512];	
	char filetypestr[1024];
	char object_keyword[1024];
	char *line;
	char *obj;
	char * val;
	char *next_line;
	char *next_object;
	SWHEADER *swheader=NULL;
	char * (*f_goto_next)(void * cpp_object, int * image_offset);
	int nullfd = open("/dev/null", O_RDWR, 0);
	int n, ilen, offset=0, i;
	int gi_inode = 0;
	int ifd;
	int ofd;
	unsigned char nullchar = '\0';
	char * base;
	int ret;

	yylval.strb = strob_open(8);


	*object_keyword = '\0';
	*tag = '\0';
	*value = '\0';
	*keyword = '\0';
	*str = '\0';



	for (;;) {
		fprintf(stderr, "%s", "\n"
		       "M: SWHEADER test program menu:\n"
		       "M: 0. exit  x. Open a parsed image. \n"
		       "M: 1. open file 		2.              		3.            \n"
		       "M: 4.                   	5.                 		6. reset. \n"
		       "M: 7. write                  	8. Set offset 			9. print via swheader_\n"
		       "M: a. print object tree.	b.       		   	c. print.\n"
		       "M: d. get object by tag         q. find attribute in object.\n"
		       );
		
		printf(" ENTER CHOICE: <%c> ", ch);
		ch = get_character(ch);

		switch (ch) {
		case '0':
			exit(0);
			break;
		case 'x':	
			strcpy(str, "/tmp/swdef");
			printf("open: enter filename <%s> ", str);
			get_string(str);
			
			ifd = open(str, O_RDONLY, 0);
			if (ifd < 0){
				fprintf(stderr,"open() error\n");
				exit(1);
			}

			ofd = uxfio_open("/dev/null", O_RDONLY, 0);
			uxfio_fcntl(ofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
			uxfio_fcntl(ofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

			swlib_pipe_pump(ofd, ifd);
			close(ifd);
			ret = uxfio_write(ofd, (void *)(&nullchar), 1);
			uxfio_get_dynamic_buffer(ofd, &base, (int*)NULL, (int*)NULL);

			swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
			swheader_set_image_head(swheader, base);	

			swheader_set_current_offset_p(swheader, &gi_inode);
			swheader_set_current_offset_p_value(swheader, 0);
			swheader_reset(swheader);
			swheader_goto_next_line(swheader, swheader_get_current_offset_p(swheader), SWHEADER_GET_NEXT);
			break;
		case '1':	
			strcpy(str, "../var/psf.ieee.tar");
			printf("open: enter filename <%s> ", str);
			get_string(str);
			
			strcpy(filetypestr, "INDEX");
			printf("enter filetype: INDEX PSF or INFO <%s> ", filetypestr);
			get_string(filetypestr);
			
			ifd = open(str, O_RDONLY, 0);
			if (ifd < 0){
				fprintf(stderr,"open() error\n");
				exit(1);
			}

			ofd = uxfio_open("/dev/null", O_RDONLY, 0);
			uxfio_fcntl(ofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
			uxfio_fcntl(ofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

			ret = sw_yyparse(ifd, ofd, filetypestr, 0, SWPARSE_FORM_MKUP_LEN);
			if (ret < 0) exit(22);
			ret = uxfio_write(ofd, (void *)(&nullchar), 1);
			uxfio_get_dynamic_buffer(ofd, &base, (int*)NULL, (int*)NULL);

			swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
			swheader_set_image_head(swheader, base);	

			swheader_set_current_offset_p(swheader, &gi_inode);
			swheader_set_current_offset_p_value(swheader, 0);
			swheader_reset(swheader);
			swheader_goto_next_line(swheader, swheader_get_current_offset_p(swheader), SWHEADER_GET_NEXT);
			break;

		case '6':
			swheader_reset(swheader);
			/* swheader_set_current_offset_p_value(swheader, 0);
			swheader_goto_next_line(swheader, &gi_inode, SWHEADER_GET_NEXT);		
			*/
			break;

		case '7':
			f_goto_next = (char * (*)(void *, int*)) swheader_get_iter_function(swheader) ;
			next_line = swheader_get_current_line(swheader);
			while (next_line != NULL) {
				fprintf (stdout, "%d ", swheader_get_current_offset(swheader)); fflush (stdout);
				if (!swheaderline_write(next_line, STDOUT_FILENO)) {
					fprintf (stdout, "\n"); fflush (stdout);
				}
				next_line = (*f_goto_next)((void *)(swheader), swheader_get_current_offset_p(swheader));
			}
			break;

		case '8':
			fprintf (stdout,"enter offset <%d>: ",n=0);
			n=get_integer(n);
			swheader_set_current_offset(swheader, n);	
			break;
		
		case '9':
			/* 
			* print every line 
			*/
			
			{
				int my_offset=0;
				int inode = 0;
				char * next_line;

				swheader_set_current_offset_p(swheader, &my_offset);
				next_line=swheader_goto_next_line(swheader, &inode, SWHEADER_GET_NEXT);	
				while (next_line){
					swheaderline_write_debug(next_line, STDOUT_FILENO);
					next_line=swheader_goto_next_line(swheader, &inode, SWHEADER_GET_NEXT);
				}
			}
			swheader_set_current_offset_p(swheader, &gi_inode);
			break;
		
		case 'a':
			/* 
			* Write objects 
			*/
		        swheader_reset(swheader);
			swheader_set_current_offset_p_value(swheader, 0);
			next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
			while (next_line){
				swheaderline_write_debug(next_line, STDOUT_FILENO);
				next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
			}
			break;

		case 'c':
			i=0;
			next_object = swheader_get_current_line(swheader);
			while (next_object != NULL) {
				next_line=next_object;	
				swheader_set_current_offset_p(swheader, &offset);
			        offset=swheader_get_current_offset(swheader);	
				next_object=swheader_get_next_object(swheader, 0, UCHAR_MAX);	
				swheader_set_current_offset_p(swheader, NULL);
				while (next_line != NULL) {
					fprintf (stdout, "%d ", swheader_get_current_offset(swheader)); fflush (stdout);
					if (!swheaderline_write(next_line, STDOUT_FILENO)) {
						fprintf (stdout, "\n"); fflush (stdout);
					}
					next_line = swheader_get_next_attribute(swheader);
					swheader_set_current_offset(swheader, next_line-swheader_get_image_head(swheader));
				}
				swheader_set_current_offset(swheader, next_object-swheader_get_image_head(swheader));
			}	
			break;

		case 'd':
			fprintf(stdout,"Find object by tag\n");
			fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
			fprintf(stdout,"enter tag: <%s> ", tag); get_string(tag);
			obj=swheader_get_object_by_tag(swheader, keyword, tag);
			if (!obj) {
				fprintf(stderr, "%s:%s Not found.\n", keyword, tag);
				fprintf(stdout, "%s:%s Not found.\n", keyword, tag);
			} else {
				int offset;
				int level;
				char * next_attr;
				char * next_line;
		        	
				offset=swheader_get_current_offset(swheader);

				fprintf(stdout, "Found OK. offset = %d\n", offset);

				/* //
				// Write it to dev/null.
				// */
				swheaderline_write_debug(obj, nullfd);
				while((next_attr=swheader_get_next_attribute(swheader))) 
					swheaderline_write_debug(next_attr, nullfd);
			

				/* //
				// Now, prove we can reset the offset and reproduce the results.
				// */
				swheader_set_current_offset_p_value(swheader, offset);
				obj = swheader_get_current_line(swheader);	
				level = swheaderline_get_level(obj);
				level ++;

				/* //
				// Write the object and its attributes.
				// */
				swheaderline_write_debug(obj, STDOUT_FILENO);
				while((next_attr=swheader_get_next_attribute(swheader))) 
					swheaderline_write_debug(next_attr, STDOUT_FILENO);

				/* //
				// Now write all the logically contained objects.
				// */
				next_line = swheader_get_next_object(swheader, level, (int)UCHAR_MAX);
				while (next_line){
					/* //
					// Write the object keyword line.
					// */
					swheaderline_write_debug(next_line, STDOUT_FILENO);

					/* //
					// Write all the attributes.
					// */
					while((next_attr=swheader_get_next_attribute(swheader))) 
						swheaderline_write_debug(next_attr, STDOUT_FILENO);

					/* //
					// Goto the next object.
					// */
					next_line = swheader_get_next_object(swheader, level, (int)UCHAR_MAX);
				}

			}
			break;	
		case 'r':
			swheader_reset(swheader);
			swheader_get_next_object(swheader, (int)(UCHAR_MAX), (int)UCHAR_MAX);
			val = swheader_get_single_attribute_value(swheader, "blah");
			fprintf(stderr, "blah=%s\n", val);
			break;
		case 'q':
			fprintf(stdout,"Find object by tag\n");
			fprintf(stdout,"enter object keyword: <%s> ", object_keyword); get_string(object_keyword);
			fprintf(stdout,"enter tag: <%s> ", tag); get_string(tag);
			obj=swheader_get_object_by_tag(swheader, object_keyword, tag);
			if (!obj) {
				fprintf(stderr, "%s:%s Not found.\n", object_keyword, tag);
				fprintf(stdout, "%s:%s Not found.\n", object_keyword, tag);
			} else {
				int is_multi_value = 0;
				fprintf(stdout,"enter keyword: "); get_string(keyword);
				line = swheader_get_attribute_in_current_object(swheader,  keyword, object_keyword, &is_multi_value);
				fprintf(stdout, "is_multi_value = [%d]\n", is_multi_value);
				while (line) {
					swheaderline_write_debug(line, STDOUT_FILENO);
					line = swheader_get_attribute_in_current_object(swheader, keyword, object_keyword, &is_multi_value);
				}
			}
		break;

		}
	}
}



#define UGET_STRLEN 120
#define gets uget_gets

static char *uget_gets(char *);

char
get_character(char ch)
{
	char d[UGET_STRLEN];
	int c = 0;
	if (fflush(stdin))
		printf("flush error\n");
	gets(d);
	while (*(d + c) == 32)
		c++;
	if (*(d + c) == '\0')
		return ch;
	return *(d + c);
}


int
get_integer(j)
int j;
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag, i;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}
	}
	while (!(i = atoi(s)));
	return i;
}




double
get_double(double x)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	double retx;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return x;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && (decflag <= 1 && zeroflag))
				return (0);
		}

	}
	while (!(retx = atof(s)));
	return retx;
}

unsigned
get_unsigned(unsigned uu)
{
	unsigned long ul;
	unsigned long get_ulong(unsigned long u);
	while ((ul = get_ulong((unsigned long) uu)) > UINT_MAX);

	return (unsigned) ul;

}

signed char
get_signedchar(int gch)
{
	int c;


	while (((c = get_integer((signed char) gch)) > SCHAR_MAX) || (c < SCHAR_MIN));
	return (signed char) c;

}


unsigned long
get_ulong(unsigned long j)
{
	char s[UGET_STRLEN], *p1, *endp;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}

	}
	while (!strtoul(s, &endp, 10));
	return strtoul(s, &endp, 10);
}

void
get_string(char *str)
{
	char d[UGET_STRLEN];
	uget_gets(d);
	if (d[0] == '\0')
		return;
	if (strcmp(d, "\"\"") == 0) d[0] = '\0';
	strcpy(str, &d[0]);
	return;
}

long
get_long(long int j)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		if (fflush(stdin))
			printf("flush error\n");
		gets(s);
		if (s[0] == '\0')
			return j;
		p1 = s;
		while (*p1 && isspace(*p1))
			p1++;
		while (*p1) {
			if ((*p1 != '0') && (*p1 != '.'))
				break;
			if (*p1 == '.')
				decflag++;
			if (*p1 == '0')
				zeroflag++;
			p1++;
			if (!*p1 && ((decflag == 0) && zeroflag))
				return (0);
		}

	}
	while (!atol(s));
	return atol(s);

}


static char *
uget_gets(char *s)
{
	if (!fgets(s, UGET_STRLEN - 2, stdin)) {
		fprintf
		    (stderr,
		     "usr_get.c: uget_gets(): fgets() failure, returning empty string.\n"
		    );
		s[0] = '\0';
		return s;
	}
	if (strchr(s, (int) '\n'))
		*strchr(s, (int) '\n') = '\0';
	if (strchr(s, (int) '\r'))
		*strchr(s, (int) '\r') = '\0';
	return s;
}
