/*
Copyright (C) 1996-2001 Michael Meskes

STAT is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

STAT is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.
*/
#include "../../include/config.h"

#include <stdio.h>
#include <sys/stat.h>

#ifdef HAVE_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
/* #include <sys/vfs.h> */

#include "fs.h"

/* stat the filesystem and print what we find */
/*
void do_statfs(char *filename, int terse)
{
        struct statfs statfsbuf;

        if (statfs(filename, &statfsbuf) == -1) {
        	perror(filename);
       	        return;
        }

	if (terse != 0) {
#ifdef __USE_FILE_OFFSET64
		printf("%s %x %x %lu %lx %lld %lld %lld %ld %lld %lld\n",
#else
		printf("%s %x %x %d %x %ld %ld %ld %d %ld %ld\n",
#endif
			filename,
			statfsbuf.f_fsid.__val[0],
			statfsbuf.f_fsid.__val[1],
			statfsbuf.f_namelen,
			statfsbuf.f_type,
			statfsbuf.f_blocks,
			statfsbuf.f_bfree,
			statfsbuf.f_bavail,
			statfsbuf.f_bsize,
			statfsbuf.f_files,
			statfsbuf.f_ffree);
			
		return;
	}
	
	printf("  File: \"%s\"\n", filename);
#ifdef __USE_FILE_OFFSET64 
	printf("    ID: %-8x %-8x Namelen: %-7ld Type: ", statfsbuf.f_fsid.__val[0], statfsbuf.f_fsid.__val[1], statfsbuf.f_namelen);
#else
	printf("    ID: %-8x %-8x Namelen: %-7d Type: ", statfsbuf.f_fsid.__val[0], statfsbuf.f_fsid.__val[1], statfsbuf.f_namelen);
#endif

        switch (statfsbuf.f_type) {
        	case AFFS_SUPER_MAGIC:
        		printf("AFFS\n");
        		break;
        	case EXT_SUPER_MAGIC:
        		printf("EXT\n");
        		break;
        	case EXT2_OLD_SUPER_MAGIC:
        		printf("EXT2\n");
        		break;
        	case EXT2_SUPER_MAGIC:
        		printf("EXT2\n");
        		break;
		case HPFS_SUPER_MAGIC:
			printf("HPFS\n");
			break;
		case ISOFS_SUPER_MAGIC:
			printf("ISOFS\n");
			break;
		case MINIX_SUPER_MAGIC:
			printf("MINIX\n");
		case MINIX_SUPER_MAGIC2:
			printf("MINIX (30 char.)\n");
			break;
		case MINIX2_SUPER_MAGIC:
			printf("MINIX V2\n");
			break;
		case MINIX2_SUPER_MAGIC2:
			printf("MINIX V2 (30 char.)\n");
			break;
		case MSDOS_SUPER_MAGIC:
			printf("MSDOS\n");
			break;
		case NCP_SUPER_MAGIC:
			printf("NOVELL\n");
			break;
		case NFS_SUPER_MAGIC:
			printf("NFS\n");
			break;
		case PROC_SUPER_MAGIC:
			printf("PROC\n");
			break;
		case SMB_SUPER_MAGIC:
			printf("SMB\n");
			break;
		case XENIX_SUPER_MAGIC:
			printf("XENIX\n");
			break;
		case SYSV4_SUPER_MAGIC:
			printf("SYSV4\n");
			break;
		case SYSV2_SUPER_MAGIC:
			printf("SYSV2\n");
			break;
		case COH_SUPER_MAGIC:
			printf("COH\n");
			break;
		case UFS_MAGIC:
			printf("UFS\n");
			break;
		case _XIAFS_SUPER_MAGIC:
			printf("XIA\n");
			break;
		case NTFS_SUPER_MAGIC:
			printf("NTFS\n");
			break;
        	default:
#ifdef __USE_FILE_OFFSET64 
        		printf("UNKNOWN (0x%lx)\n", statfsbuf.f_type);
#else
        		printf("UNKNOWN (0x%x)\n", statfsbuf.f_type);
#endif
        }
#ifdef __USE_FILE_OFFSET64
	printf("Blocks: Total: %-10lld Free: %-10lld Available: %-10lld Size: %ld\n",  statfsbuf.f_blocks, statfsbuf.f_bfree, statfsbuf.f_bavail, statfsbuf.f_bsize);
	printf("Inodes: Total: %-10lld Free: %-10lld\n", statfsbuf.f_files, statfsbuf.f_ffree);
#else
	printf("Blocks: Total: %-10ld Free: %-10ld Available: %-10ld Size: %d\n", statfsbuf.f_blocks, statfsbuf.f_bfree, statfsbuf.f_bavail, statfsbuf.f_bsize);
	printf("Inodes: Total: %-10ld Free: %-10ld\n", statfsbuf.f_files, statfsbuf.f_ffree);
#endif
}
*/                                                                                                                                                         
/* stat the file and print what we find */
void do_stat(char *filename, int link, int terse)
{
    struct stat statbuf;
    int i = (link == 1) ? stat(filename, &statbuf) : lstat(filename, &statbuf);
    char access[10];
    struct passwd *pw_ent;
    struct group *gw_ent;
    char linkname[256];

    if (i == -1) {
	perror(filename);
	return;
    }
    
    if (terse != 0) {
		printf("%s %u %u %x %d %d %x %d %d %x %x %d %d %d %d\n",
			filename,
			(unsigned int)statbuf.st_size,
			(unsigned int)statbuf.st_blocks,
			statbuf.st_mode,
			statbuf.st_uid,
			statbuf.st_gid,
			(int)statbuf.st_dev,
			(int)statbuf.st_ino,
			(int)statbuf.st_nlink,
			major(statbuf.st_rdev),
			minor(statbuf.st_rdev),
			(int)statbuf.st_atime,
			(int)statbuf.st_mtime,
			(int)statbuf.st_ctime,
			(int)statbuf.st_blksize);
		return;
	}

    if ((statbuf.st_mode & S_IFMT) == S_IFLNK) {
	if ((i = readlink(filename, linkname, 256)) == -1) {
	    perror(filename);
	    return;
	}

	linkname[(i >= 256) ? 255 : i] = '\0';
	printf("  File: \"%s\" -> \"%s\"\n", filename, linkname);
    } else
	printf("  File: \"%s\"\n", filename);

    printf("  Size: %-10u\tBlocks: %-10u IO Block: %-6d ", (unsigned int)statbuf.st_size, (unsigned int)statbuf.st_blocks,(int)statbuf.st_blksize);

    switch (statbuf.st_mode & S_IFMT) {
    case S_IFDIR:
	(void) printf("Directory\n");
	break;
    case S_IFCHR:
	(void) printf("Character Device\n");
	break;
    case S_IFBLK:
	(void) printf("Block Device\n");
	break;
    case S_IFREG:
	(void) printf("Regular File\n");
	break;
    case S_IFLNK:
	(void) printf("Symbolic Link\n");
	break;
    case S_IFSOCK:
	(void) printf("Socket\n");
	break;
    case S_IFIFO:
	(void) printf("Fifo File\n");
	break;
    default:
	(void) printf("Unknown\n");
    }

    printf("Device: %xh/%dd\tInode: %-10d  Links: %-5d", (int)statbuf.st_dev, (int)statbuf.st_dev,
    	 (int)statbuf.st_ino, (int)statbuf.st_nlink);

    i = statbuf.st_mode & S_IFMT;
    if (i == S_IFCHR || i == S_IFBLK)
	printf(" Device type: %x,%x\n", major(statbuf.st_rdev), minor(statbuf.st_rdev));
    else
	(void) printf("\n");
    	 
    access[9] = (statbuf.st_mode & S_IXOTH) ? ((statbuf.st_mode & S_ISVTX) ? 't' : 'x') : ((statbuf.st_mode & S_ISVTX) ? 'T' : '-');
    access[8] = (statbuf.st_mode & S_IWOTH) ? 'w' : '-';
    access[7] = (statbuf.st_mode & S_IROTH) ? 'r' : '-';
    access[6] = (statbuf.st_mode & S_IXGRP) ? ((statbuf.st_mode & S_ISGID) ? 's' : 'x') : ((statbuf.st_mode & S_ISGID) ? 'S' : '-');
    access[5] = (statbuf.st_mode & S_IWGRP) ? 'w' : '-';
    access[4] = (statbuf.st_mode & S_IRGRP) ? 'r' : '-';
    access[3] = (statbuf.st_mode & S_IXUSR) ? ((statbuf.st_mode & S_ISUID) ? 's' : 'x') :  ((statbuf.st_mode & S_ISUID) ? 'S' : '-');
    access[2] = (statbuf.st_mode & S_IWUSR) ? 'w' : '-';
    access[1] = (statbuf.st_mode & S_IRUSR) ? 'r' : '-';

    switch (statbuf.st_mode & S_IFMT) {
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
    (void) printf("Access: (%04o/%10.10s)", statbuf.st_mode & 07777, access);

    setpwent();
    setgrent();
    pw_ent = getpwuid(statbuf.st_uid);
    gw_ent = getgrgid(statbuf.st_gid);
    printf("  Uid: (%5d/%8s)   Gid: (%5d/%8s)\n", statbuf.st_uid,
    		(pw_ent != 0L) ? pw_ent->pw_name : "UNKNOWN", statbuf.st_gid,
    		(gw_ent != 0L) ? gw_ent->gr_name : "UNKNOWN");

    printf("Access: %s", ctime(&statbuf.st_atime));
    printf("Modify: %s", ctime(&statbuf.st_mtime));
    printf("Change: %s\n", ctime(&statbuf.st_ctime));
}

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-l] [-f] file1 [file2 ...]\n", progname);
    exit(1);
}

int main(int argc, char *argv[])
{
    int c, i, link = 0, fs = 0, terse = 0;

    while ((c = getopt(argc, argv, "lfvt")) != EOF) {
	switch (c) {
	case 'l':
	    link = 1;
	    break;
	case 'f':
	    fs = 1;
	    break;
	case 't':
	    terse = 1;
	    break;
	case 'v':
	    printf("stat version: 2.5\n");
	default:
	    usage(argv[0]);
	}
    }
    if (argc == 1)
	usage(argv[0]);

    for (i = optind; i < argc; i++)
    	(fs == 0) ? do_stat(argv[i], link, terse) : exit(23); /*do_statfs(argv[i], terse);*/

    return (0);
}
