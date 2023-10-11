/* testheader3.cxx
 */

#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <string>

#include <typeinfo>

#include "xstream_config.h"
#include "swstruct.h"
#include "swstruct_i.h"
#include "swdefinition.h"
#include "swpsf.h"
#include "swspsf.h"
#include "swparser.h"
#include "swdefinitionfile.h"
#include "switer.h"
#include "swstructiter.h"
#include "swspsf.h"
#include "swmain.h"

extern "C" {
#include "swparser_global.h"
#include "swheader.h"
#include "swheaderline.h"
static char	 		get_character(char ch);
static int 			get_integer(int j);
static void 			get_string(char *str);
/*
* static double 			get_double(double x);
* static unsigned long 		get_unsigned(unsigned long uu);
* static signed char 		get_signedchar(int gch);
* static unsigned long 		get_ulong(unsigned long j);
* static long 			get_long(long int j);
* static char *		uget_gets(char *s);
*/
}

int	global_tty_fd;
FILE	* global_tty_file;
FILE	* global_input_file;

#define UGET_STRLEN 120
#define uget_gets uget_gets_file

static void
swmd_write_debug(swMetaData * swmd, int i_inode)
{
		cout << "type=" << typeid(*swmd).name() << " ";
		cout << ":i_inode=" << i_inode 
		     << ":offset=" << swmd->get_p_offset() 
		     << ":ino=" << swmd->get_ino()
		     << ":";
		flush(cout);
}

int 
main(int argc, char **argv)
{
	char ch = '1', str[1024];
	char  keyword[512], value[512], tag[512];	
	char  object_keyword[512];
	char *line, *next_line;
	char *obj;	
	char * base;
	char nullchar = '\0';
	int data_len, buffer_len;
	int ofd;
	int len;
	int n;
	int u_offset;
	int level_spec1=UCHAR_MAX, level_spec2=UCHAR_MAX;
	int gi_inode=0;
	int ifd;
	int retval;
	int nullfd = open("/dev/null", O_RDWR, 0);
	SWVERID 	*swverid_user = NULL;
	swStruct	*sws, *sws1;
	SWHEADER 	*swheader;
	swAttribute	*sat;
	swMetaData	*swm;
	swMetaData 	*swmd;
	swMetaData 	*swm1;
	swDefinition	*u_swdef;
	swIter 		*switer, *switer1;
	swDefinitionFile *psf;
	swsPSF 		*swspsf;
	struct termios termiosp;

	strcpy(tag,"");
	strcpy(keyword,"");
	strcpy(value,"");
	strcpy(object_keyword,"");

	if (argc == 1 || (argc > 1 && !strcmp(argv[1], "tty"))) {
		global_tty_file = fopen("/dev/tty", "r");
		global_tty_fd = fileno(global_tty_file);
		global_input_file =  global_tty_file;
	 	if (tcgetattr(global_tty_fd, &termiosp) < 0) {
	 		fprintf(stderr, "tcgetattr failed.\n");
	 		exit(111);
	 	}
		 termiosp.c_lflag |= (ICANON);
		 if (tcsetattr(global_tty_fd, TCSANOW, &termiosp)) {
			fprintf(stderr, "tcsetattr failed.\n");
		 	exit(112);
		 }
		if (!global_tty_file || global_tty_fd < 0) {
			fprintf(stderr, "cant open /dev/tty\n");
			exit(110);
		}
	} else if (argc > 1 && !strcmp(argv[1], "stdin")) {
		global_input_file =  stdin;
	} else { }
	

	for (;;) {
		fprintf(stderr, "\n"
		       "M: SWHEADER test program menu:\n"
		       "M: 0. exit\n"
		       "M: 1. open swspsf 		2. close swspsf 		3. write PSF. \n"
		       "M: 4. write-by_offset		5. debug write 			6. reset. \n"
		       "M: 7. write using switer. 	8. get swStruct by inode 	9. print via swheader_\n"
		       "M: a. print object tree.	b. get swMetaData by ino.   	c. add attribute.\n"
		       "M: d. find object by tag. 	e. add swstruct.  		f. get by offset.\n"
		       "M: g. get_next_object(). 	h. get_attr in object.  	i. get next attr.in object.\n"
		       "M: j. write using swheader 	k. peek at next line. 		l. get next line.\n"
		       "M: m. get current line. 					n. Set swheader offset.	\n"
		       "M: o. insert attribute in swdefinition  			p. make version id.\n"
		       "M: q. find attribute using swheader_get_attribute_in_current_object().\n"
		       "M: z. debug dump.\n"
		       );
		
		
		fprintf(stderr, "M: ENTER CHOICE: <%c> ", ch);
		ch = get_character(ch);

		switch (ch) {
		
		case '0':
			exit(0);

		case 'y':
			strcpy(str, "./testfiles/psf4");
			printf("open: enter filename <%s> ", str);
			get_string(str);

			ifd=open(str,O_RDONLY);
			if (ifd < 0){
				fprintf(stderr,"open() error\n");
				exit(1);
			}

	                ofd = uxfio_open("/dev/null", O_RDWR, 0);
	                uxfio_fcntl(ofd, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
	                uxfio_fcntl(ofd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM);

			psf=new swPSF("");
			psf->open_parser(ifd, ofd);
			len=psf->run_parser(0, SWPARSE_FORM_MKUP_LEN);

			if (len < 0){
				cerr << "Error in parser.\n";
				exit(1);
			}

			uxfio_lseek(ofd, 0, SEEK_END);
			uxfio_write(ofd, (void *)(&nullchar), 1);
			uxfio_get_dynamic_buffer(ofd, &base, &data_len, &buffer_len);

			swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
			if (!swheader) {
				cerr << "swheader failed.\n";
				exit(1);
			}
			swheader_set_image_head(swheader, base);
			switer = NULL;
			swspsf = NULL;
			break;
		case '1':
			//fprintf(stderr, "DJL HERE in 1\n");
			strcpy(str, "./testfiles/psf4");
			printf("open: enter filename <%s> ", str);
			get_string(str);

			ifd=open(str,O_RDONLY);
			if (ifd < 0){
				fprintf(stderr,"open() error\n");
				exit(1);
			}
			psf=new swPSF("");
			if (psf->open_parser(ifd)){
				cerr << "Error opening  parser.\n";
			}
			if (psf->run_parser(0,1) < 0){
				cerr << "Error in parser.\n";
				exit(1);
			}

			swspsf=new swsPSF(psf);
			retval = swspsf->generateStructuresFromParser();
		
			if (retval != 0) {
				cerr << "Error in from generateStructuresFromParser.\n";
				cerr << "Will expand extended definitions and retry.\n";
				delete swspsf;
				if (psf->generateDefinitions()) {
					cerr << "generateDefinitions returned error, exiting.\n";
					exit(1);
				}
				psf->swFileMapPop();
				swspsf=new swsPSF(psf);
				retval = swspsf->generateStructuresFromParser();
				if (retval != 0) {
					cerr << "generateStructuresFromParser returned error, exiting.\n";
					exit(1);
				}
			}
		
			switer=new swIter(swspsf->get_swstruct());
			swheader=swheader_open(swIter::switer_get_nextline, static_cast<void*>(switer));
			swheader_set_current_offset_p(swheader, &gi_inode);
			
			swheader_reset(swheader);
			swheader_set_current_offset_p_value(swheader, gi_inode);
			
			break;
		case 'z':
			swheader__dump(swheader);	
			break;
		case '2':
			swheader_close(swheader);	
			delete psf;
			delete swspsf;
			delete switer;
			break;
		case '3':	
			switer->set_stack_index(1);
  			swspsf->iwrite(STDOUT_FILENO);
			switer->show_debug(stdout);
			break;

		case '4':	
	    		gi_inode=0;
			line=swheader_f_goto_next(swheader);
			while (line){
				::swheaderline_write(line, STDOUT_FILENO);
				line=swheader_f_goto_next(swheader);
			}
			if (switer) switer->show_debug(stdout);
			break;

		case '5':
			{
				int my_offset=0;
				int linode = 0;
				char * next_line;

				swheader_set_current_offset_p(swheader, &my_offset);
				next_line=swheader_goto_next_line(swheader, &linode, SWHEADER_GET_NEXT);	
				while (next_line){
					swm = switer->peek()->find_by_ino(switer, linode);	
					sws = switer->peek()->find_swstruct_by_ino(switer, linode);
				
					if (!swm) {
						fprintf(stdout, "E* null from find_by_ino inode=%d.\n", linode);
					} else {
						swmd_write_debug(swm, linode);
						swheaderline_write_debug(next_line, STDOUT_FILENO);
					}
					if (!sws) {
						fprintf(stdout, "E* null from find_struct_by_ino inode=%d.\n", linode);
					} else {
						swm1 = sws->get_attribute(linode);
						if (!swm1) {
							fprintf(stdout, "E* null from get_attribute inode=%d.\n", linode);
						} else {
							fprintf(stdout, "\n    Second swmd_write_debug %d. ", linode);
							swmd_write_debug(swm1, linode);
							swm1->write_fd(STDOUT_FILENO);
							//fprintf(stdout, "\n");
						}
					}

					next_line=swheader_goto_next_line(swheader, &linode, SWHEADER_GET_NEXT);
				}
			}
			break;

		case '6':
			swheader_reset(swheader);
			swheader_set_current_offset_p_value(swheader, 0);
			break;

		case '7':
			{
			swMetaData *swmd;
			swMetaData *swmdo;
			char * first_line;
			swIter *switer1=new swIter(swspsf->get_swstruct());
			//swIter *switer2=new swIter(swspsf->get_swstruct());
			switer->reset();
			gi_inode=INT_MAX;
			first_line = swheader_goto_next_line(swheader, &gi_inode, SWHEADER_PEEK_NEXT);
			gi_inode=0;
			line = swheader_f_goto_next(swheader);
			while (line){
				swmd=swIter::switer_get_attribute(static_cast<void*>(switer1), gi_inode);
				if (!swmd){
					fprintf(stderr, "null from switer_get_attribute.\n");
				} 
				
				/* sanity check */
				swmdo=swIter::switer_get_attribute_by_p_offset(static_cast<void*>(switer1), (int)(line - first_line));
				if (!swmdo){
					fprintf(stderr, "null swmd from switer_get_by_p_offset, %s\n", line);
				} 
				
				if (swmdo && swmdo->get_parserline() == line) {	
					cout << "type=" << typeid(*swmdo).name() << " ";
					cout << ":current=" << gi_inode 
					     << ":offset=" << swmd->get_p_offset() 
					     << ":ino=" << swmd->get_ino()
					     << ":";
					flush(cout);
					swheaderline_write_debug(line, STDOUT_FILENO);	
					flush(cout);
					//switer1->reset();
					//switer2->reset();
				} else {
					fprintf(stderr, "********************* ERROR ****************\n");
					exit(1);
				}
				line=swheader_f_goto_next(swheader);
			}
			delete switer1;
			switer->show_debug(stdout);
			}
			break;
		case '8':
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			switer->reset();
			sws=swIter::switer_get_swstruct(static_cast<void*>(switer), n);
			if (!sws){
				fprintf(stderr, "inode %d not found.\n", n);	
			} else {
				sws->iwrite(STDOUT_FILENO);
				flush(cout);
				swheader_set_current_offset_p_value(swheader, sws->get_swdefinition()->get_ino());
			}
			break;


		case '9':
			/* print every line */
			
			{
				int my_offset=0;
				int inode = 0;
				char * next_line;

				swheader_set_current_offset_p(swheader, &my_offset);
				next_line=swheader_goto_next_line(swheader, &inode, SWHEADER_GET_NEXT);	
				while (next_line){
					//swheaderline_write_debug(next_line, STDOUT_FILENO);
					next_line=swheader_goto_next_line(swheader, &inode, SWHEADER_GET_NEXT);
				}
			}
			swheader_set_current_offset_p(swheader, &gi_inode);
			break;
		case 'a':
			/* Write objects */
		        swheader_reset(swheader);
			swheader_set_current_offset_p_value(swheader, 0);
			next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
			while (next_line){
				swheaderline_write_debug(next_line, STDOUT_FILENO);
				next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
			}
			break;

		case 'b':
			/* Get metatdata by inode */
			switer1=new swIter(swspsf->get_swstruct());
			switer->reset();
			
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			
			swmd=swIter::switer_get_attribute(static_cast<void*>(switer1), n);
			if (!swmd){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				cout << "current=" << n 
				     << ":offset=" << swmd->get_p_offset() 
				     << ":ino=" << swmd->get_ino()
				     << ":";
				flush(cout);
				swheaderline_write_debug(swmd->get_parserline(), STDOUT_FILENO);	
				flush(cout);
			}
			delete switer1;
			break;
		case 'c':
			/* add attribute */
			switer->reset();
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			
			sws=swIter::switer_get_swstruct(static_cast<void*>(switer), n);
			if (!sws){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				fprintf(stdout,"enter keyword: "); get_string(keyword);
				fprintf(stdout,"enter value: "); get_string(value);
				sws->add_attribute(keyword, value);
			}
			break;

		case 'd':
			fprintf(stdout,"Find object by tag\n");
			switer->reset();
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

				//
				// Write it to dev/null.
				//
				swheaderline_write_debug(obj, nullfd);
				while((next_attr=swheader_get_next_attribute(swheader))) 
					swheaderline_write_debug(next_attr, nullfd);
			

				//
				// Now, prove we can reset the offset and reproduce the results.
				//
				swheader_set_current_offset_p_value(swheader, offset);
				obj = swheader_get_current_line(swheader);	
				level = swheaderline_get_level(obj);
				level ++;

				//
				// Write the object and its attributes.
				//
				swheaderline_write_debug(obj, STDOUT_FILENO);
				while((next_attr=swheader_get_next_attribute(swheader))) 
					swheaderline_write_debug(next_attr, STDOUT_FILENO);

				//
				// Now write all the logically contained objects.
				//
				next_line = swheader_get_next_object(swheader, level, (int)UCHAR_MAX);
				while (next_line){
					//
					// Write the object keyword line.
					//
					swheaderline_write_debug(next_line, STDOUT_FILENO);

					//
					// Write all the attributes.
					//
					while((next_attr=swheader_get_next_attribute(swheader))) 
						swheaderline_write_debug(next_attr, STDOUT_FILENO);

					//
					// Goto the next object.
					//
					next_line = swheader_get_next_object(swheader, level, (int)UCHAR_MAX);
				}

			}	
			break;	

		case 'e':
			switer->reset();
			fprintf(stdout,"enter object keyword: "); get_string(keyword);
			sws1=new swStruct_i (keyword, 1);	
			fprintf (stdout,"enter inode to add to <%d>: ",n=0);
			n=get_integer(n);
			sws=swIter::switer_get_swstruct(static_cast<void*>(switer), n);
			if (!sws){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				fprintf(stdout,"enter a attr. keyword: "); get_string(keyword);
				fprintf(stdout,"enter value: "); get_string(value);
				sws1->add_attribute(keyword, value);
				sws->add_swstruct(sws1);
			}
			break;
		case 'f':
			switer1=new swIter(swspsf->get_swstruct());
			switer->reset();
			
			fprintf (stdout,"enter offset<%d>: ",n=0);
			n=get_integer(n);
			
			swmd=swIter::switer_get_attribute(static_cast<void*>(switer1), n);
			if (!swmd){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				cout << "current=" << n 
				     << ":offset=" << swmd->get_p_offset() 
				     << ":ino=" << swmd->get_ino()
				     << ":";
				flush(cout);
				swheaderline_write_debug(swmd->get_parserline(), STDOUT_FILENO);	
				flush(cout);
			}
			break;
		
		case 'g':
			/* get next object */
	    		gi_inode=0;
			switer->reset();
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			sws=swIter::switer_get_swstruct(static_cast<void*>(switer), n);
			if (!sws){
				fprintf(stderr, "inode %d not found.\n", n);
				break;
			} else {
				swheader_set_current_offset_p_value(swheader, sws->get_swdefinition()->get_ino());
			}	
			fprintf (stdout,"enter level spec 1<%d>: ",level_spec1);
			level_spec1=get_integer(level_spec1);
			fprintf (stdout,"enter level spec 2<%d>: ",level_spec2);
			level_spec2=get_integer(level_spec2);
			switer->reset();
			obj=swheader_get_next_object(swheader, level_spec1, level_spec2);
			if (!obj) {
				fprintf(stderr, "next object not found.\n");
				break;
			}
			switer->reset();
			sws=swIter::switer_get_swstruct_by_p_offset((void*)(switer), (int)(obj - swheader_get_image_head(swheader)));
			if (!sws){
				fprintf(stderr, "offset %d  not found.\n", obj - swheader_get_image_head(swheader));	
				break;
			}
			sws->iwrite(STDOUT_FILENO);
			flush(cout);
			break;	
		
		case 'h':
			/* get attribute in object */
			fprintf(stdout,"find keyword in current object sws=%p\n", (void*)sws);
			/* switer->reset(); */
			fprintf(stdout,"enter keyword: <%s> ", keyword); get_string(keyword);
			obj=swheader_get_attribute(swheader, keyword, NULL);
			if (!obj) {
				fprintf(stderr, "not found.\n");
			} else {
				swheaderline_write_debug(obj, STDOUT_FILENO);
				free(obj);
			}
			break;
		
		case 'i':
			fprintf(stdout,"return next attr. in current object=%p\n", (void*)sws);
			obj=swheader_get_next_attribute(swheader);
			if (!obj) {
				fprintf(stderr, "not found.\n");
			} else {
				swheaderline_write_debug(obj, STDOUT_FILENO);
			}
			break;
		case 'j':
			
			{
				char * next_attr;
				char * next_line;
		        	swheader_reset(swheader);
				swheader_set_current_offset_p_value(swheader, 0);

				next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
				while (next_line){
					swheaderline_write_debug(next_line, STDOUT_FILENO);

					swheader_goto_next_line((void *)swheader, swheader_get_current_offset_p(swheader), SWHEADER_PEEK_NEXT);

					while((next_attr=swheader_get_next_attribute(swheader))) 
						swheaderline_write_debug(next_attr, STDOUT_FILENO);
					next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
				}
			}
			break;	
		case 'k':
			fprintf(stdout,"PEEK NEXT: %s\n",swheader_goto_next_line((void *)swheader, &gi_inode, SWHEADER_PEEK_NEXT));
			break;	
		case 'l':
			fprintf(stdout,"GET NEXT: %s\n",swheader_goto_next_line(swheader, &gi_inode, SWHEADER_GET_NEXT));		
			break;	
		case 'm':
			fprintf(stdout,"GET CURRENT: %s\n",swheader_get_current_line(swheader));
			break;	
		
		case 'n':
			/* get attribute in object */
			u_offset=swheader_get_current_offset(swheader);
			fprintf(stdout,"Set swheader offset.\n");
			fprintf(stdout,"enter offset: <%d> ", u_offset); u_offset=get_integer(u_offset);
			swheader_set_current_offset(swheader, u_offset);
			break;	
		case 'o':
			/* add attribute */
			swheader_reset(swheader);
			fprintf (stdout,"enter swdefinition inode <%d>: ",n=0);
			n=get_integer(n);
			swmd=swIter::switer_get_attribute(static_cast<void*>(switer), n);
			u_swdef=dynamic_cast<swDefinition*>(swmd);
			assert(u_swdef);
			
			fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
			fprintf(stdout,"enter value: <%s> ", tag); get_string(value);
			
			sat=new swAttribute(keyword, value);
			sat->set_level(u_swdef->get_level()+1);
			u_swdef->list_insert(sat, u_swdef->get_next_node());
			
			break;
		case 'p':
			/* make version id */
			fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
			fprintf(stdout,"enter version string: <%s> ", tag); get_string(tag);
			if (swverid_user) swverid_close(swverid_user);
			swverid_user = swverid_open(keyword, tag);
			break;

		case 'q':
			fprintf(stdout,"Find object by tag\n");
			switer->reset();
			fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
			fprintf(stdout,"enter tag: <%s> ", tag); get_string(tag);
			obj=swheader_get_object_by_tag(swheader, keyword, tag);
			if (!obj) {
				fprintf(stderr, "%s:%s Not found.\n", keyword, tag);
				fprintf(stdout, "%s:%s Not found.\n", keyword, tag);
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




static char *uget_gets(char *);
//static char *uget_gets_tty(char *);

static char
get_character(char ch)
{
	char d[UGET_STRLEN];
	int c = 0;
	uget_gets(d);
	//fprintf(stderr, "HERE in get_character\n");
	while (*(d + c) == 32)
		c++;
	if (*(d + c) == '\0')
		return ch;
	return *(d + c);
}


static int
get_integer(int j)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag, i;
	do {
		decflag = zeroflag = 0;
		uget_gets(s);
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



/**************************************
static double
get_double(double x)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	double retx;
	do {
		decflag = zeroflag = 0;
		uget_gets(s);
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
**************************************/

/**************************************
static unsigned long
get_unsigned(unsigned long uu)
{
	unsigned long ul;
	//unsigned long get_ulong(unsigned long u);
	while ((ul = get_ulong((unsigned long) uu)) > UINT_MAX);

	return (unsigned long) ul;

}

static signed char
get_signedchar(int gch)
{
	int c;


	while (((c = get_integer((signed char) gch)) > SCHAR_MAX) || (c < SCHAR_MIN));
	return (signed char) c;

}


static unsigned long
get_ulong(unsigned long j)
{
	char s[UGET_STRLEN], *p1, *endp;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		uget_gets(s);
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
**************************************/

static void
get_string(char *str)
{
	char d[UGET_STRLEN];
	uget_gets(d);
	if (d[0] == '\0')
		return;
	strcpy(str, &d[0]);
	return;
}


//static void
//get_string_tty(char *str)
//{
//	int n;
//	char d[UGET_STRLEN];
//
//	fflush(stdout);
//	fflush(stderr);
//	strcpy(str, "");
//	n = read(global_tty_fd, d, sizeof(d));
//	if (n <= 0) return;
//	d[sizeof(d) - 1] = '\0';
//	strcpy(str, d);
//	return;
//}
//


/**************************************
static long
get_long(long int j)
{
	char s[UGET_STRLEN], *p1;
	int decflag, zeroflag;
	do {
		decflag = zeroflag = 0;
		uget_gets(s);
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
uget_gets_tty(char *s)
{
	int n;
	char d[UGET_STRLEN];
	
	if (fflush(stdout))
		fprintf(stderr, "stdout flush error\n");
	if (fflush(stderr))
		fprintf(stderr, "stderr flush error\n");

	n = read(global_tty_fd, s, UGET_STRLEN - 2);
	if (n <= 0)  {
		fprintf(stderr, "uget_gets_tty(): failure, n=%d, returning empty string.\n", n);
		s[0] = '\0';
		return s;
	}

	s[UGET_STRLEN - 1] = '\0';

	if (strchr(s, (int) '\n'))
		*strchr(s, (int) '\n') = '\0';
	if (strchr(s, (int) '\r'))
		*strchr(s, (int) '\r') = '\0';
	//fprintf(stderr, "HERE [%s]\n", s);
	return s;
}
**************************************/


static char *
uget_gets_file(char *s)
{
	if (fflush(stdout))
		fprintf(stderr, "stdout flush error\n");
	if (fflush(stderr))
		fprintf(stderr, "stderr flush error\n");

	//if (fflush(global_input_file))
	//	fprintf(stderr, "stdin flush error\n");

	if (!fgets(s, UGET_STRLEN - 2, global_input_file)) {
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


