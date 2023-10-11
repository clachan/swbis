/* testheader4.cxx
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
#include "swparser.h"
#include "swattribute.h"
#include "swdefinition.h"
#include "swdefinitionfile.h"
#include "swpsf.h"
#include "swmain.h"

extern "C" {
#include "swparser_global.h"
#include "swheader.h"
#include "swheaderline.h"
static char	 		get_character(char ch);
static int 			get_integer(int j);
static void 			get_string(char *str);
}

int	global_tty_fd;
FILE	* global_tty_file;
FILE	* global_input_file;

#define UGET_STRLEN 120
#define uget_gets uget_gets_file

static void
i_write_debug(swMetaData * swmd, int i_inode)
{
		cout << "type=" << typeid(*swmd).name() << " ";
		cout << ":i_inode=" << i_inode 
		     << ":offset=" << swmd->get_p_offset() 
		     << ":ino=" << swmd->get_ino()
		     << ":";
		flush(cout);
}

int
line_write_debug(swDefinitionFile * psf, char * next_line, int fd)
{
        int type;
	swMetaData * swmd;
	
	swheaderline_write_debug(next_line, fd);

	swmd = psf->swdeffile_linki_find_by_parserline(NULL, next_line);
	assert(swmd);
	type = swmd->get_type();
	if (type == swstructdef::sdf_object_kw) {
		(static_cast<swDefinition*>(swmd))->swAttribute::write_fd_debug(fd, "");	
	} else {
		swmd->write_fd_debug(fd, "");
	}
	return 0;
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
	int nullfd = open("/dev/null", O_RDWR, 0);
	SWVERID 	*swverid_user = NULL;
	SWVARFS 	*swvarfs = NULL;
	SWHEADER 	*swheader;
	swAttribute	*sat;
	swMetaData	*swm;
	swMetaData 	*swmd;
	swDefinition	*swdef;
	swDefinition	*u_swdef;
	swPSF * psf;
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
		       "M: 1. open swpsf 		2. close swpsf 			3. write PSF. \n"
		       "M: 4. write by swheader		5. debug write 			6. reset. \n"
		       "M: 7. write using linki. 	8. get_current_offset		9. print via swheader_\n"
		       "M: a. print object tree.	b. set current object.   	c. add attribute.\n"
		       "M: d. find object by tag. 	e. Contruct swexport Object	f. get by offset.\n"
		       "M: g. swdefinitionfile debug	h. get attr in current object. 	i. get next attr.in object.\n"
		       "M: j. write using swheader 	k. peek at next line. 		l. get next line.\n"
		       "M: m. get current line. 					n. Set swheader offset.	\n"
		       "M: o. insert attribute in swdefinition  			p. make version id.\n"
		       "M: q. find attribute using swheader_get_attribute_in_current_object().  r. get next object.\n"
		       "M: z. debug dump.\n"
		       );
		
		
		fprintf(stderr, "M: ENTER CHOICE: <%c> ", ch);
		ch = get_character(ch);

		switch (ch) {
		
		case '0':
			exit(0);

		case 'y':
			strcpy(str, "./testfiles/psf7");
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
			uxfio_get_dynamic_buffer(ofd, &base, &buffer_len, &data_len);

			swheader = swheader_open((char *(*)(void *, int *, int))(NULL), NULL);
			if (!swheader) {
				cerr << "swheader failed.\n";
				exit(1);
			}
			swheader_set_image_head(swheader, base);
			break;
		case '1':
			strcpy(str, "./testfiles/psf7");
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
			if (psf->run_parser(0, SWPARSE_FORM_MKUP_LEN) < 0) {
				cerr << "Error in parser input.\n";
				exit(1);
			}
			psf->swPSF::open_directory_archive("/");
			swvarfs = static_cast<SWVARFS*>(psf->xFormat_get_swvarfs());
			psf->get_swextdef()->set_swvarfs(swvarfs);
	
			if (psf->generateDefinitions()) {
				cerr << "generateDefinitions returned error, exiting.\n";
				exit(1);
			}

			//
			// Need to initialize the link-lists in the <swDefinitionFile*> object.
			//
			psf->swdeffile_linki_init();

			swheader=swheader_open(swDefinitionFile::swdeffile_linki_nextline, static_cast<void*>(psf));
			
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
			break;
		case '3':	
			psf->write_fd(STDOUT_FILENO);
			break;

		case '4':	
	    		gi_inode=0;
			line=swheader_f_goto_next(swheader);
			while (line){
				::swheaderline_write(line, STDOUT_FILENO);
				line=swheader_f_goto_next(swheader);
			}
			break;

		case '5':
			psf->write_fd_debug(STDOUT_FILENO, "");
			break;

		case '6':
			swheader_reset(swheader);
			swheader_set_current_offset_p_value(swheader, 0);
			swheader_goto_next_line(swheader, &gi_inode, SWHEADER_GET_NEXT);		
			break;

		case '7':
			psf->swdeffile_linki_write_fd(STDOUT_FILENO);
			break;

		case '8':
			fprintf(stdout, "offset=%d\n", (int)swheader_get_current_offset(swheader));
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
					//swheaderline_write_debug(next_line, STDOUT_FILENO);
					line_write_debug(psf, next_line, STDOUT_FILENO);
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

		case 'b':
			/* 
			* Get metatdata by inode 
			*/
			
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			
			swmd = psf->swdeffile_linki_find_by_ino(NULL, n);		
			if (!swmd){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				flush(cout);
				swheaderline_write_debug(swmd->get_parserline(), STDOUT_FILENO);	
				flush(cout);
			}
			break;

		case 'c':
			/* 
			* add attribute 
			*/
			fprintf (stdout,"enter inode <%d>: ",n=0);
			n=get_integer(n);
			swmd = psf->swdeffile_linki_find_by_ino(NULL, n);		
			if (!swmd) {
				fprintf(stderr, "inode %d not found.\n", n);
			} else if (swmd->get_type() != swstructdef::sdf_object_kw) {
				fprintf(stderr, "inode %d [%s] type=[%d] is not swstructdef::sdf_object_kw type.\n", n, swmd->get_parserline(), swmd->get_type());
			} else {
				swdef = static_cast<swDefinition*>(swmd);
				fprintf(stdout,"enter keyword: "); get_string(keyword);
				fprintf(stdout,"enter value: "); get_string(value);
				swdef->add(keyword, value);
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
				int oldoffset;
				int level;
				char * next_attr;
				char * next_line;
		        	
				offset=swheader_get_current_offset(swheader);
				oldoffset = offset;

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
				swheader_set_current_offset_p(swheader, &gi_inode);
				swheader_reset(swheader);
				swheader_set_current_offset_p_value(swheader, oldoffset);

			}	
			break;	

		case 'e':
			break;

		case 'f':
			fprintf (stdout,"enter offset<%d>: ",n=0);
			n=get_integer(n);
			
			swmd = psf->swdeffile_linki_find_by_offset(NULL, n);
			if (!swmd){
				fprintf(stderr, "inode %d not found.\n", n);
			} else {
				flush(cout);
				swheaderline_write_debug(swmd->get_parserline(), STDOUT_FILENO);	
				flush(cout);
			}
			break;
		
		case 'g':
			//D fprintf(stdout, "%s", psf->swdefinitionfile_dump_string_s(""));
			break;	
		
		case 'h':
			/* 
			* get attribute in object 
			*/
			
			fprintf(stdout,"find keyword in current object swm=%p\n", (void*)swm);
			fprintf(stdout,"enter keyword: <%s> ", keyword); get_string(keyword);
			obj=swheader_get_attribute(swheader, keyword, NULL);
			if (!obj) {
				fprintf(stderr, "not found.\n");
			} else {
				fprintf(stderr, "found : [%s]\n", swheaderline_get_value(obj, NULL));
				swheaderline_write_debug(obj, STDOUT_FILENO);
			}
			break;
		
		case 'i':
			fprintf(stdout,"return next attr. in current object=%p\n", (void*)swm);
			obj=swheader_get_next_attribute(swheader);
			if (!obj) {
				fprintf(stderr, "not found.\n");
			} else {
				swheaderline_write_debug(obj, STDOUT_FILENO);
			}
			break;

		case 'j':
			/*
			* Write using swheader.
			*/
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
			/* 
			* get attribute in object 
			*/
			u_offset=swheader_get_current_offset(swheader);
			fprintf(stdout,"Set swheader offset.\n");
			fprintf(stdout,"enter offset: <%d> ", u_offset); u_offset=get_integer(u_offset);
			swheader_set_current_offset(swheader, u_offset);
			break;	
		case 'o':
			/* 
			* add attribute 
			*/
			swheader_reset(swheader);
			fprintf (stdout,"enter swdefinition inode <%d>: ",n=0);
			n=get_integer(n);
			
			swmd = psf->swdeffile_linki_find_by_ino(NULL, n);		
			if (!swmd) {
				fprintf(stderr, "inode %d not found.\n", n);
			} else if (swmd->get_type() != swstructdef::sdf_object_kw) {
				fprintf(stderr, "inode %d is not swstructdef::sdf_object_kw type.\n", n);
			} else {
				u_swdef = static_cast<swDefinition*>(swmd);
				assert(u_swdef);
				fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
				fprintf(stdout,"enter value: <%s> ", tag); get_string(value);
				sat=new swAttribute(keyword, value);
				sat->set_level(u_swdef->get_level()+1);
				u_swdef->list_insert(sat, u_swdef->get_next_node());
			}	
			break;
		case 'p':
			/* 
			* make version id 
			*/
			fprintf(stdout,"enter object keyword: <%s> ", keyword); get_string(keyword);
			fprintf(stdout,"enter version string: <%s> ", tag); get_string(tag);
			if (swverid_user) swverid_close(swverid_user);
			swverid_user = swverid_open(keyword, tag);
			break;

		case 'q':
			fprintf(stdout,"Find object by tag\n");
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
		case 'r':
			/* 
			* get next object
			*/
			next_line = swheader_get_next_object(swheader, (int)UCHAR_MAX, (int)UCHAR_MAX);
			while (next_line){
				swheaderline_write_debug(next_line, STDOUT_FILENO);
				next_line = NULL;
			}
			break;
		}
	}
}


static char *uget_gets(char *);

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

static void
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


