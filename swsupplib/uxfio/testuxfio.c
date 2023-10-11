#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include "swmain.h"
#include "swfork.h"
#include "uxfio.h"



void get_string (char *str);
int get_integer (int j);

char get_character (char ch);
int
main ()
{
   int amount;
   int ret;
   int uxfio_fd1, pipe_fd[2], fd1, bytesRead;
   int i=0,j=0, m,  index, value, whence;
   int  offset, cmd, child;
   char ch='0', str[1024], buf[10240], buffer[10240];
   struct stat st;
   memset (buf,0x00,10240);
   memset (buffer,0x00,10240);

   i=0;
   for(;;)
     {
       printf("\n"
"UXFIO test program menu:\n"       
"0.exit  1.uxfio_open  2.uxfio_close  3. uxfio_lseek  4. uxfio_fnctl 5. uxfio_read 6. reopen 7. show\n"
"8. print buffer  9. Open Pipe  a. OpenDup b. write file to stdout. c. write file porttion\n");
       printf(" ENTER CHOICE: <%c> ", ch); ch=get_character (ch);

        switch(ch)
          {
            case '0':
              exit(0);

            case '1':
	      strcpy (str,"testdata");
              printf("uxfio_open: enter filename <%s> ", str );
              get_string(str);
	      uxfio_fd1=uxfio_open(str, O_RDONLY, 0 );
              if(uxfio_fd1 < 0 )
                { 
                  printf(" Open failure on filename: \n");
                  break;
                } 
	      uxfio_fcntl(uxfio_fd1, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_MEM);
	      uxfio_fcntl(uxfio_fd1, UXFIO_F_SET_BUFACTIVE, UXFIO_ON);
              printf(" %s opened on descriptor %d\n", str, uxfio_fd1);
              break;  

            case '2':
             printf("uxfio_close:\n");
             printf("Enter descriptor <%d> ", index=UXFIO_FD_MIN);
             index=get_integer (index);
             if(uxfio_close( index))
                fprintf(stderr, "uxfio_close error on descriptor %d\n",index );
              break;  

            case '3':
              printf("uxfio_seek\n "); 
              printf("Enter file descriptor <%d> ", index=UXFIO_FD_MIN);
              index=get_integer (index);
              
	      printf( "Enter offset <%d> ", offset=0 );
              offset=get_integer (offset);

	      printf( "whence SEEK_SET 0\n");
	      printf( "whence SEEK_CUR 1\n");
	      printf( "whence SEEK_END 2\n");
	      printf( "whence UXFIO_SEEK_VCUR  use 3\n");
	      printf( "Enter whence <%d> ", whence=0 );
              whence=get_integer (whence);

       	      if (whence == 3) whence = UXFIO_SEEK_VCUR;

	      if ( (ret=uxfio_lseek (index, (off_t)offset, whence)) < 0 ) {
                fprintf(stderr, "uxfio_lseek error on descriptor %d\n",index );
	      }
              fprintf(stdout, "uxfio_lseek returned %d\n", ret);
	      break;

            case '4':
              printf("uxfio_fcntl\n");
              printf(" CONTROLS \n");
	      printf("   UXFIO_F_SETBL:%d \n",UXFIO_F_SETBL);
	      printf("   UXFIO_F_SET_INITTR: %d \n",UXFIO_F_SET_INITTR);
	      printf("   UXFIO_F_SET_DISATR: %d \n",UXFIO_F_SET_DISATR);
	      printf("   UXFIO_F_SET_CANSEEK: %d \n",UXFIO_F_SET_CANSEEK);
	      printf("   UXFIO_F_SET_BUFTYPE: %d \n",UXFIO_F_SET_BUFTYPE);
	      printf("   UXFIO_BUFTYPE_MEM: %d \n",UXFIO_BUFTYPE_MEM);
	      printf("   UXFIO_BUFTYPE_FILE: %d \n",UXFIO_BUFTYPE_FILE);
	      printf("   UXFIO_BUFTYPE_DYNAMIC_MEM: %d \n",UXFIO_BUFTYPE_DYNAMIC_MEM);

	      
	      printf("Enter descriptor <%d> ", index=UXFIO_FD_MIN);
              index=get_integer (index);
              printf("Enter cmd <%d> ", cmd=UXFIO_F_SETBL);
              cmd=get_integer (cmd);
	      value=16; 
	      printf("Enter value <%d> ", value);
              value=get_integer (value);
	      if (uxfio_fcntl (index, cmd, value)) {
                    fprintf(stderr, "uxfio_fcntl error: descriptor  %d\n",  uxfio_fd1);
              } 
	      break;

            case '5':
              memset (buf,0x00,10240);
              printf("uxfio_read\n "); 
              printf("Enter descriptor <%d> ", index=uxfio_fd1);
              index=get_integer (index);
              printf("Enter length to read <%d> ", offset=1);
              offset=get_integer (offset);
              
	      j=uxfio_read ( index, buf, offset);
              if (j != offset)
	         fprintf (stderr, "uxfio_read error: %d\n", j);
	      fprintf (stdout, "\nBuffer :\n");  
	      for (i=0; i < j; i++) {
                 fprintf (stdout,"%c", *(buf+i));
	      }
	      fprintf (stdout, "\n");  
	      break;

            case '7':
              printf("show uxfio structure\n"); 
              printf("Enter descriptor <%d> ", index=UXFIO_FD_MIN);
              index=get_integer (index);
	      uxfio_debug_dump (index); 
	      break;
            case '8':
              printf("show buffer\n"); 
              printf ("%s\n",buf);
	      break;
            case '9':
	      pipe(pipe_fd);
	      strcpy (str,"testdata");
              printf("uxfio_open PIPE: enter filename <%s> ", str );
              get_string(str);
	      fd1=open(str, O_RDONLY, 0 );
              if(fd1 < 0 )
                { 
                  printf(" Open failure on filename: \n");
                  break;
                } 
              printf(" %s opened on descriptor %d\n", str, fd1);

	      child = swfork((sigset_t*)(NULL));
	    
              if (! child ) {
		 close (pipe_fd[0]);
                 bytesRead = read ( fd1, buffer,1);
                 while (bytesRead > 0) {
                   if (write (pipe_fd[1], buffer, bytesRead) != bytesRead ) {
		     fprintf (stderr," error in child\n");
                     _exit (1);
                   }
                   bytesRead = read ( fd1, buffer,1 );
		  }
		  if (bytesRead < 0){
		     fprintf (stderr," error in child 0001\n");
                     _exit (1);
                   } else {
		    fprintf (stderr," normal exit in child 0001\n");
                    _exit(0);
                   } 
              } else {
                 close (pipe_fd[1]);
                 /* uxfio_fd1 = uxfio_alloc (pipe_fd[0],-1); */
                 uxfio_fd1 = uxfio_opendup (pipe_fd[0] ,UXFIO_BUFTYPE_FILE);
		 
		 printf(" pipe opened on descriptor %d\n",  uxfio_fd1);
	         
		 if (uxfio_fcntl (uxfio_fd1, UXFIO_F_SET_CANSEEK, 0)) {
                    fprintf(stderr, "uxfio_fcntl error: descriptor  %d\n",  uxfio_fd1);
                 } 
                 
	       }
		 
		 

	     break;  
		case 'a':
			lstat("/etc/passwd", &st);
			st.st_size=5;
			uxfio_fd1=uxfio_opendup(uxfio_fd1, UXFIO_BUFTYPE_NOBUF);	
			uxfio_ioctl(uxfio_fd1, UXFIO_IOCTL_SET_STATBUF, (void*)&st);
		 	printf("opened on descriptor %d\n",  uxfio_fd1);
		break;

		case 'b':
              			printf("uxfio read file\n "); 
				printf("Enter descriptor <%d> ", index=uxfio_fd1);
				index=get_integer (index);
			 	bytesRead=200; 
			 	while (bytesRead > 0) {
                   			bytesRead=uxfio_read(index, buffer, 200);
                   			if (bytesRead < 0) {
		     				fprintf (stderr," error in read\n");
			 		}	
					write(1,buffer,bytesRead);
                   	 	}
		break;
		case 'c':
              			printf("uxfio write file portion.\n "); 
				printf("Enter descriptor <%d> ", index=uxfio_fd1);
				index=get_integer (index);
				printf("Enter amount <%d> ", index=512);
				amount=get_integer(amount);

				fprintf(stdout, "\noffset is %d\n", (int)uxfio_lseek(index, 0, SEEK_CUR));
				fprintf(stdout, "---,\n");
                   		if (uxfio_read(index, buffer, amount) != amount) {
					fprintf(stderr, "read error\n");	
				}
				fprintf(stdout, "\n^---\n");
				if (write(STDOUT_FILENO, buffer, amount) != amount) {
					fprintf(stderr, "write error\n");	
				}
		break;

            }
     }
}



#define UGET_STRLEN 120
#define gets uget_gets

static char * uget_gets (char *);

char 
get_character (ch)
     char ch;
{
  char d[UGET_STRLEN];
  int c = 0;
  if (fflush (stdin))
    printf ("flush error\n");
  gets (d);
  while (*(d + c) == 32)
    c++;
  if (*(d + c) == '\0')
    return ch;
  return *(d + c);
}


int
get_integer (j)
     int j;
{
  char s[UGET_STRLEN], *p1;
  int decflag, zeroflag, i;
  do
    {
      decflag = zeroflag = 0;
      if (fflush (stdin))
	printf ("flush error\n");
      gets (s);
      if (s[0] == '\0')
	return j;
      p1 = s;
      while (*p1 && isspace (*p1))
	p1++;
      while (*p1)
	{
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
  while (!(i = atoi (s)));
  return i;
}




double 
get_double (x)
     double x;
{
  char s[UGET_STRLEN], *p1;
  int decflag, zeroflag;
  double retx;
  do
    {
      decflag = zeroflag = 0;
      if (fflush (stdin))
	printf ("flush error\n");
      gets (s);
      if (s[0] == '\0')
	return x;
      p1 = s;
      while (*p1 && isspace (*p1))
	p1++;
      while (*p1)
	{
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
  while (!(retx = atof (s)));
  return retx;
}

unsigned 
get_unsigned (uu)
     unsigned uu;
{
  unsigned long ul;
  unsigned long get_ulong (unsigned long u);
  while ((ul = get_ulong ((unsigned long) uu)) > UINT_MAX);

  return (unsigned) ul;

}

signed char 
get_signedchar (gch)
     int gch;
{
  int c;


  while (((c = get_integer ((signed char) gch)) > SCHAR_MAX) || (c < SCHAR_MIN));
  return (signed char) c;

}


unsigned long 
get_ulong (j)
     unsigned long j;
{
  char s[UGET_STRLEN], *p1, *endp;
  int decflag, zeroflag;
  do
    {
      decflag = zeroflag = 0;
      if (fflush (stdin))
	printf ("flush error\n");
      gets (s);
      if (s[0] == '\0')
	return j;
      p1 = s;
      while (*p1 && isspace (*p1))
	p1++;
      while (*p1)
	{
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
  while (!strtoul (s, &endp, 10));
  return strtoul (s, &endp, 10);
}


void 
get_string (str)
     char *str;
{
  char d[UGET_STRLEN];
  if (fflush (stdin))
    printf ("flush error\n");
  gets (d);
  if (d[0] == '\0')
    return;
  strcpy (str, &d[0]);
  return;
}


long 
get_long (j)

     long int j;

{
  char s[UGET_STRLEN], *p1;
  int decflag, zeroflag;
  do
    {
      decflag = zeroflag = 0;
      if (fflush (stdin))
	printf ("flush error\n");
      gets (s);
      if (s[0] == '\0')
	return j;
      p1 = s;
      while (*p1 && isspace (*p1))
	p1++;
      while (*p1)
	{
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
  while (!atol (s));
  return atol (s);

}


static char *
uget_gets (s)
     char * s;
{
  if (! fgets (s, UGET_STRLEN-2, stdin)) 
    {
      fprintf 
        (stderr,
           "usr_get.c: uget_gets(): fgets() failure, returning empty string.\n"
        );
      s[0] = '\0';
      return s;
    }
  if (strchr (s,(int)'\n')) * strchr(s,(int)'\n') = '\0';
  if (strchr (s,(int)'\r')) * strchr(s,(int)'\r') = '\0';
  return s;
}


