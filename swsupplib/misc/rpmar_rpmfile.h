/* rpmar_rpmfile.h
 */

/*
   Copyright 1997 James Lowe, Jr. <jhl@richmond.infi.net>
   This file may be copied under terms of GNU GPL.
*/

/* Not currently used */
#define RPMINCLUDE_RPMFILE_H


#ifndef RPMINCLUDE_RPMFILE_H
#define RPMINCLUDE_RPMFILE_H
#include "uinfile.h"

#define RPMFILE_COMPRESSED_NOT UINFILE_COMPRESSED_NOT
#define RPMFILE_COMPRESSED_Z UINFILE_COMPRESSED_Z
#define RPMFILE_COMPRESSED_GZ UINFILE_COMPRESSED_GZ  
#define RPMFILE_COMPRESSED_NA UINFILE_COMPRESSED_NA  

#define RPMFILE_FORMAT_INTERCHANGE_RHS UINFILE_FORMAT_INTERCHANGE_RHS 
#define RPMFILE_FORMAT_INTERCHANGE_POSIX UINFILE_FORMAT_INTERCHANGE_POSIX   
#define RPMFILE_FORMAT_PACKAGE_RHS  UINFILE_FORMAT_PACKAGE_RHS    
#define RPMFILE_FORMAT_PACKAGE_TAR2 UINFILE_FORMAT_PACKAGE_TAR2   
#define RPMFILE_FORMAT_PACKAGE_POSIX UINFILE_FORMAT_PACKAGE_POSIX     

#define RPMFILE_NOSAVE UINFILE_NOSAVE 
#define RPMFILE_SAVE UINFILE_SAVE

#define RPMFILE_SW_MAGIC UINFILE_SW_MAGIC 
#define RPMFILE_RPMTAR_V1_MAGIC UINFILE_RPMTAR_V1_MAGIC 
#define RPMFILE_SWBIS_PATH0 UINFILE_SWBIS_PATH0 
#define RPMFILE_SWBIS_MAGIC UINFILE_SWBIS_MAGIC 
#define RPMFILE_SWBIS_MAGIC_BIN UINFILE_SWBIS_02_MAGIC_BIN 
#define RPMFILE_SWBIS_MAGIC_SRC UINFILE_SWBIS_02_MAGIC_SRC 

typedef UINFORMAT RPMFORMAT;

int rpmfile_open (char * rpmfilename, int oflag, mode_t mode , RPMFORMAT ** rpmformat , 
                                           int save_leading_bytes, void * vparbag );
int rpmfile_close ( RPMFORMAT * rpmformat );
int rpmfile_fix_rpminfo_tarheader ( unsigned char * oldheader, unsigned char * newheader, int rpminfo_size );
/* int rpmfile_rpminfo_size ( int read_write , int value ); */

#endif
