#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include "strob.h"
#include "uxfio.h"


void swlib_write_stats(char *filename, 
		char * linkname_p,  
		struct stat * pstatbuf, 
		int terse, 
		char * markup_prefix,
		int ofd, 
		STROB * pbuffer)
{
    int i = 0;
    char access[10];
    struct passwd *pw_ent;
    struct group *gw_ent;
    struct stat statbufo;
    struct stat * statbuf;
    char linkname[256];
    STROB * buffer;
    int append_flag0;

    append_flag0 = 0;
    if (ofd < 0) append_flag0 = 1;

    if (!pstatbuf) {
	statbuf = &statbufo;
	i = lstat(filename, statbuf);
	if (i < 0) {
		fprintf(stderr, "lstat error on file [%s] : %s", filename, strerror(errno));
		return;
	}
    } else {
	statbuf = pstatbuf;
    }

    if (!pbuffer) {
    	buffer = strob_open(12);
    } else {
    	buffer = pbuffer;
    }
   
    strob_strcpy(buffer, "");
    if (terse != 0) {
		strob_sprintf(buffer, append_flag0, "%s%s %u %u %x %d %d %x %d %d %x %x %d %d %d %d\n",
			filename,
			(unsigned int)statbuf->st_size,
			(unsigned int)statbuf->st_blocks,
			statbuf->st_mode,
			statbuf->st_uid,
			statbuf->st_gid,
			(int)statbuf->st_dev,
			(int)statbuf->st_ino,
			(int)statbuf->st_nlink,
			major(statbuf->st_rdev),
			minor(statbuf->st_rdev),
			(int)statbuf->st_atime,
			(int)statbuf->st_mtime,
			(int)statbuf->st_ctime,
			(int)statbuf->st_blksize);
		if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
		if (!pbuffer) strob_close(buffer);
		return;
	}



    if ((statbuf->st_mode & S_IFMT) == S_IFLNK) {
	if (linkname_p) {
		strncpy(linkname, linkname_p, sizeof(linkname) -1);
		linkname[sizeof(linkname) -1] = '\0';
	} else {
		if ((i = readlink(filename, linkname, 512)) == -1) {
		    perror(filename);
		    if (!pbuffer) strob_close(buffer);
		    return;
		}
		linkname[(i >= 512) ? 512-1 : i] = '\0';
	}
	strob_sprintf(buffer, append_flag0, "%s  File: \"%s\" -> \"%s\"\n", markup_prefix, filename, linkname);
	if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
    } else {
	strob_sprintf(buffer, append_flag0, "%s  File: \"%s\"\n", markup_prefix, filename);
	if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
    }

    strob_sprintf(buffer, append_flag0, "%s  Size: %-10u\tBlocks: %-10u IO Block: %-6d ", markup_prefix, (unsigned int)statbuf->st_size, (unsigned int)statbuf->st_blocks,(int)statbuf->st_blksize);
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    switch (statbuf->st_mode & S_IFMT) {
    case S_IFDIR:
	(void) strob_sprintf(buffer, append_flag0,"Directory\n");
	break;
    case S_IFCHR:
	(void) strob_sprintf(buffer, append_flag0,"Character Device\n");
	break;
    case S_IFBLK:
	(void) strob_sprintf(buffer, append_flag0,"Block Device\n");
	break;
    case S_IFREG:
	(void) strob_sprintf(buffer, append_flag0,"Regular File\n");
	break;
    case S_IFLNK:
	(void) strob_sprintf(buffer, append_flag0,"Symbolic Link\n");
	break;
    case S_IFSOCK:
	(void) strob_sprintf(buffer, append_flag0,"Socket\n");
	break;
    case S_IFIFO:
	(void) strob_sprintf(buffer, append_flag0,"Fifo File\n");
	break;
    default:
	(void) strob_sprintf(buffer, append_flag0,"Unknown\n");
    }
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    strob_sprintf(buffer, append_flag0,"%sDevice: %xh/%dd\tInode: %-10d  Links: %-5d", markup_prefix, (int)statbuf->st_dev, (int)statbuf->st_dev,
    	 (int)statbuf->st_ino, (int)statbuf->st_nlink);
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    i = statbuf->st_mode & S_IFMT;
    if (i == S_IFCHR || i == S_IFBLK) {
	strob_sprintf(buffer, append_flag0," Device type: %x,%x\n", major(statbuf->st_rdev), minor(statbuf->st_rdev));
    } else {
	strob_sprintf(buffer, append_flag0,"\n");
    }
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    access[9] = (statbuf->st_mode & S_IXOTH) ? ((statbuf->st_mode & S_ISVTX) ? 't' : 'x') : ((statbuf->st_mode & S_ISVTX) ? 'T' : '-');
    access[8] = (statbuf->st_mode & S_IWOTH) ? 'w' : '-';
    access[7] = (statbuf->st_mode & S_IROTH) ? 'r' : '-';
    access[6] = (statbuf->st_mode & S_IXGRP) ? ((statbuf->st_mode & S_ISGID) ? 's' : 'x') : ((statbuf->st_mode & S_ISGID) ? 'S' : '-');
    access[5] = (statbuf->st_mode & S_IWGRP) ? 'w' : '-';
    access[4] = (statbuf->st_mode & S_IRGRP) ? 'r' : '-';
    access[3] = (statbuf->st_mode & S_IXUSR) ? ((statbuf->st_mode & S_ISUID) ? 's' : 'x') :  ((statbuf->st_mode & S_ISUID) ? 'S' : '-');
    access[2] = (statbuf->st_mode & S_IWUSR) ? 'w' : '-';
    access[1] = (statbuf->st_mode & S_IRUSR) ? 'r' : '-';

    switch (statbuf->st_mode & S_IFMT) {
    case S_IFDIR:
	access[0] = 'd';
	break;
    case S_IFCHR:
	access[0] = 'c';
	break;
    case S_IFBLK:
	access[0] = 'b';
	break;
    case S_IFREG:
	access[0] = '-';
	break;
    case S_IFLNK:
	access[0] = 'l';
	break;
    case S_IFSOCK:
	access[0] = 's';
	break;
    case S_IFIFO:
	access[0] = 'p';
	break;
    default:
	access[0] = '?';
    }
    strob_sprintf(buffer, append_flag0,"%sAccess: (%04o/%10.10s)", markup_prefix, statbuf->st_mode & 07777, access);
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    setpwent();
    setgrent();
    pw_ent = getpwuid(statbuf->st_uid);
    gw_ent = getgrgid(statbuf->st_gid);
    strob_sprintf(buffer, append_flag0,"  Uid: (%5d/%8s)   Gid: (%5d/%8s)\n", statbuf->st_uid,
    		(pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN", statbuf->st_gid,
    		(gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));

    /*
    strob_sprintf(buffer, append_flag0,"%sAccess: %s", markup_prefix, ctime(&(statbuf->st_atime)));
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
    */ 

    strob_sprintf(buffer, append_flag0,"%sModify: %s", markup_prefix, ctime(&(statbuf->st_mtime)));
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
   
    strob_sprintf(buffer, append_flag0,"%sChange: %s\n", markup_prefix, ctime(&(statbuf->st_ctime)));
    if (ofd >= 0) uxfio_write(ofd, strob_str(buffer), strob_strlen(buffer));
    if (!pbuffer) strob_close(buffer);
}

