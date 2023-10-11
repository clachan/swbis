#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "uxfio.h"

/* program to reverse stdin */

int main ( int argc, char ** argv ) {
  int ret;
  int buftype, size, bytesRead;
  int uxfio_fd, seekval;
  int uxfio_fd1;
  char buffer[1024];

  if (argc <= 2) {
   fprintf (stderr,"Usage: sample {3|2|1} size\n");
   fprintf (stderr,"3 is dynamic memory, 2 is file buffer, 1 is mem buffer, size is size of mem buffer in bytes.\n");
   exit (1);
  }

  buftype =  atoi (argv[1]);
  size =  atoi (argv[2]);

  uxfio_fd1 = uxfio_opendup (STDIN_FILENO, buftype);
  uxfio_fd = uxfio_opendup (uxfio_fd1, buftype);
  
  
 if (uxfio_fd < 0) exit (1);
 
  if (buftype == UXFIO_BUFTYPE_FILE) { 
	if ( uxfio_fcntl (uxfio_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_FILE) < 0 ) {
             fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SET_BUFTYPE  \n");
	     exit (1);
	}
   } else if (buftype == UXFIO_BUFTYPE_DYNAMIC_MEM) { 
	if ( uxfio_fcntl (uxfio_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_DYNAMIC_MEM) < 0 ) {
             fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SET_BUFTYPE  \n");
	     exit (1);
	}
   } else if (buftype == UXFIO_BUFTYPE_MEM) { 
        if ( uxfio_fcntl (uxfio_fd, UXFIO_F_SET_BUFTYPE, UXFIO_BUFTYPE_MEM) < 0 ) {
             fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SET_BUFTYPE MEM  \n");
	     exit (1);
	}

        if ( uxfio_fcntl (uxfio_fd, UXFIO_F_SETBL, size) < 0 ) {
             fprintf (stderr,"error in uxfio_fcntl UXFIO_F_SETBL  \n");
	     exit (1);
	}
   } else {
             fprintf (stderr,"error in usage.\n");
	     exit (1);
   }

  bytesRead = uxfio_read(uxfio_fd, buffer, 1024);
  while (bytesRead > 0) {
       /* if (write (STDOUT_FILENO, buffer, bytesRead) != bytesRead ) {
             fprintf (stderr,"error in write\n");
             exit (1);
       } */
      bytesRead = uxfio_read(uxfio_fd, buffer, 1024);
   }
   if (bytesRead < 0)
        fprintf (stderr,"read error\n");

   if (uxfio_lseek (uxfio_fd, 0, SEEK_SET) < 0) {
             fprintf (stderr,"error in lseek\n");
             exit (1);
   } 
   
   if (uxfio_lseek (uxfio_fd, 0, SEEK_END) < 0) {
             fprintf (stderr,"error in lseek\n");
             exit (1);
   } 
   
   seekval=uxfio_lseek (uxfio_fd, 0, SEEK_CUR);

   seekval=uxfio_lseek (uxfio_fd, -1, SEEK_CUR);
   if (seekval < 0) {
             fprintf (stderr,"error in lseek\n");
             exit (1);
   } 

   /* now write out the file in reverse order */

   while ( seekval >= 0) {
      ret = uxfio_read (uxfio_fd, buffer, 1);
      if ( ret != 1 ){
             fprintf (stderr,"error in read while reversing pipe\n");
		exit(1);
      }
      if (write (STDOUT_FILENO, buffer, 1) != 1 ) {
             fprintf (stderr,"error in write\n");
             exit (1);
      }
      seekval=uxfio_lseek (uxfio_fd, -2, SEEK_CUR);
   }

   uxfio_close (uxfio_fd); 
   uxfio_close (uxfio_fd1); 


 exit(0);

}





