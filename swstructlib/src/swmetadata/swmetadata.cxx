//  swmetadata.cxx
//
//  Copyright (C) 1998-2004 James H. Lowe, Jr. <jhlowe@acm.org>
//  All Rights Reserved.
//
//  COPYING TERMS AND CONDITIONS.
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3, or (at your option)
//  any later version.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  

#include "swuser_config.h"
#include "swmetadata.h"
#include "system.h"

swAttributeMem * swMetaData::data_p_;
int swMetaData::use_count_;
int swMetaData::logical_modeM;
swstructdef * swMetaData::defaults_;

		// To add a element you must increment the namesM array length in swmetadata.h
char * swMetaData::namesM[] = {"type", "mode", "mtime", "ctime", "owner", 
				"uid", "group", "gid", "umask", "size", 
				"is_volatile", "compression_state", "cksum", 
				"md5sum", "sha1sum", "sha512sum", "link_source", "minor", "major",
				"_ino", "_nlink", "_complete", ""};



swMetaData::swMetaData (void) {
    init();
    set_p_offset(uxfio_lseek(data_p_->get_mem_fd(), 0, SEEK_CUR));
    set_ino(get_p_offset());
    level_ = -1; //  Running  set_level(-1);  causes a seg fault.
}

swMetaData::~swMetaData (void){ 
   use_count_--;
   if (!use_count_) {
       delete data_p_;
       delete defaults_;
   }
}

swMetaData::swMetaData(char * keyword, char * value) {
    init();
    add (keyword, value, this, 1);
}

int  swMetaData::list_add_if_new (swMetaData * newnode) { return 0; }

void  swMetaData::list_replace (swMetaData * newnode) { }

void  swMetaData::list_replace_if_not_explicitly_set (swMetaData * newnode) { }

void  swMetaData::list_insert (swMetaData * newnode, swMetaData * location) { }

int swMetaData::insert (char * r, swMetaData * newnode) { return 0; }

int swMetaData::insert (char * key, char * value) { return 0; }

// --------------- swAttributeMem ------------------

void swMetaData::set_type (int type) { 
    kw_type_ = swMetaData::determine_type ((char)type); 
}

int swMetaData::get_type (void) {
   return kw_type_;
}

int swMetaData::get_level (void) {
   return level_;
}

void swMetaData::set_level (int level) {
   ::swheaderline_set_level((char*)(get_mem_addr()) + get_p_offset(), level_=level);
}

void swMetaData::set_ino(int off) {
   inode_ = off;
}

void swMetaData::set_p_offset (int off) {
   if (inode_ < 0)
   	inode_=off;
   offset_ = off;
}

int swMetaData::write_fd (int i) {
   return -1;
}

swMetaData* swMetaData::find_object (char * object_keyword, int occurance_number ) {
   swMetaData *p;
   char *keyword; 
   int count=0; 
   p = this;
   keyword = p->get_keyword(); 
   do {
      if ( swstructdef::return_entry_group(p->get_type()) == swstructdef::sdf_objs && !::strcmp (keyword, object_keyword)) {
         count++;
         if (count == occurance_number) {
           return p;
         } 
      } 
      p = p->get_next_node();
   } while (p);
   return static_cast<swMetaData*>(NULL);
}

int  swMetaData::determine_type (char  type) {
    int kew; 
    if ( type  == SWPARSE_MD_TYPE_ATT) {
        kew = swstructdef::sdf_attribute_kw;
    } else if ( type == SWPARSE_MD_TYPE_EXT) {
        kew = swstructdef::sdf_extended_kw;
    } else if ( type == SWPARSE_MD_TYPE_OBJ) {
        kew = swstructdef::sdf_object_kw;
    } else if ( type == SWPARSE_MD_TYPE_FILEREF) {
        kew = swstructdef::sdf_filereference_kw;
    } else {
        kew = swstructdef::sdf_unknown;
    }
    return kew;
}

int  swMetaData::determine_type (char * typestring) {
    return determine_type(::swheaderline_get_type(typestring));
}

// --------------- swstructdef ------------------

inline struct swsdflt_defaults * swMetaData::defaults() {
  return  defaults_->swstructdef::defaults();
}

inline int swMetaData::get_attr_group (int index) {
   return defaults_->swstructdef::get_attr_group(index);
}

inline int swMetaData::is_sw_keyword(char * object, char * keyword ) {
	return defaults_->swstructdef::is_sw_keyword(object, keyword);
}

inline int swMetaData::return_entry_group(int entry_index) {
   return defaults_->swstructdef::return_entry_group(entry_index);
}

inline struct swsdflt_defaults * swMetaData::return_entry(char * object, char * keyword) {
  return defaults_->swstructdef::return_entry(object, keyword);
}

inline int swMetaData::return_entry_index (char * object, char * keyword) {
 return defaults_->swstructdef::return_entry_index(object, keyword);
}

inline char * swMetaData::return_entry_keyword (char * buf, int entry_index) {
 return defaults_->swstructdef::return_entry_keyword(buf, entry_index);
}

// -------------------------------------------------

int
swMetaData::add (char * keyword, char * value, swMetaData * newnode, int level) {
   	int off=uxfio_lseek(get_mem_fd(), 0, SEEK_END);
	return swMetaData::add_(keyword, value, newnode, level, off);
}

char * swMetaData::set_value (char * val) {
	return swMetaData::set_value_(val, SEEK_END);
}

char * swMetaData::get_value (int * len) {
  char * ret;
  char * buffer;
  int off = get_p_offset();
  if (off < 0) return (char*)NULL;
  if (status_ > 0 && logical_modeM > 0) return (char*)NULL;

  ret = swheaderline_get_value((((UXFIO*)(data_p_->uxfio_addrM))->bufM) + off, len);
  // Good Unoptimized: uxfio_get_dynamic_buffer(get_mem_fd(), &buffer, NULL, NULL);
  // Good Unoptimized: ret=swheaderline_get_value(buffer + off, len);
  
  
  // It will be assumed that this type of swMetaData has an
  // ascii text value and therefore the *len is not needed.
  // Other types will override get_value() and not set *len
  // to -1 .
  if (len) *len=-1; 
  return ret;
}

char *
swMetaData::set_keyword (char * newkw) {
	int off;
	int len;
	char *kw = get_keyword();
	char *val = get_value(&len);

	if (!kw) {
   		off=uxfio_lseek(get_mem_fd(), 0, SEEK_END);
		swMetaData::add_(newkw, "", this, 1, off);
		return get_keyword();
	} else {
		if (strlen(newkw) < strlen(kw)) {
   			off=get_p_offset();
   			if (off != uxfio_lseek(get_mem_fd(), (off_t)(off), SEEK_SET)) {
				fprintf(stderr, "fatal internal error : set_keyword loc=1\n");
				exit(44);
			}
		} else {
   			off=uxfio_lseek(get_mem_fd(), 0, SEEK_END);
			if (off < 0) {
				fprintf(stderr, "fatal internal error : set_keyword loc=2\n");
				exit(44);
			}
		}
		swMetaData::add_(newkw, val, this, 1, off);
		return get_keyword();
	}
}

char * swMetaData::get_keyword (void) {
  char * buffer;
  int off = get_p_offset();
  if (off < 0) return (char*)NULL;
  return ::swheaderline_get_keyword((((UXFIO*)(data_p_->uxfio_addrM))->bufM) + off);
  //Good unoptimized: uxfio_get_dynamic_buffer(get_mem_fd(), &buffer, NULL, NULL);
  //Good unoptimized: return ::swheaderline_get_keyword(buffer + off);
}

int swMetaData::get_next_line_offset (void) {
  int offset, amount;
  const int RBLK=512; 
  char  buf[RBLK+2], *p; 
  
  offset=uxfio_lseek (get_mem_fd() ,0,SEEK_CUR);
  buf[RBLK]='\0'; 
    
  // Now read to end of line. 
  while ((amount=uxfio_sfread(get_mem_fd(), buf, RBLK)) > 0) {
     p=strchr (buf, (int)('\n'));
     if (p) {
       // seek back to start of next line 
       uxfio_lseek (get_mem_fd(), (int)(p-buf) - amount + 1, SEEK_CUR);
       return offset;
     }
   }
   return -1;
}


char * swMetaData::set_value (swMetaData * node) {
   return set_value (node->get_value((int*)NULL));
}

//  ----- private functions --------

void swMetaData::init_mem(void){
    if (!data_p_) {
         data_p_=new swAttributeMem();
    }
    if (!defaults_) {
         defaults_=new swstructdef();
    }
    use_count_++; 
}

int
swMetaData::add_ (char * keyword, char * value, swMetaData * newnode, int level, int off) {
   int c;

   if (!strlen(keyword)) return 0;
   newnode->set_p_offset(off);
   newnode->set_ino(off);
   c = SWPARSE_MD_TYPE_ATT;

   handle_utf_verbose_top();
   ::swparse_write_attribute_att(get_mem_fd(), keyword, value, level, SWPARSE_FORM_MKUP_LEN);
   handle_utf_verbose_bot();

   newnode->set_level(level);
   newnode->set_type (c);
   if (newnode != this) list_add(newnode);
   return 0;
}

char * swMetaData::set_value_ (char * val, int whence) {
    char * fret;
    char * va = get_value((int*)NULL);
    char * ke = get_keyword();

    if (!va) return NULL;
    if (::strlen(val) < ::strlen(va) ) {
	::strncpy(va, val, ::strlen(val) + 1);
    	fret = val;
    } else {
	int ret;
	int memfd = get_mem_fd();
	ret = uxfio_lseek(memfd, 0, whence);
	if (ret < 0) {
		fprintf(stderr, "internal error in set_value_\n");
		return NULL;
	}
	set_p_offset(ret);
   	handle_utf_verbose_top();
	::swparse_write_attribute_att(memfd, ke, val, get_level(), SWPARSE_FORM_MKUP_LEN);
   	handle_utf_verbose_bot();
    	fret = get_value((int*)NULL);
    }
    return fret;
}

void swMetaData::init(void)
{
    init_mem();
    //length_=-1;
    offset_=-1;
    inode_=-1;
    level_=-1;
    kw_type_=-1;
    status_= 0;
    verbose_state_= 1;
    is_set_explicit_ = 0;
    contained_byM = NULL;
    next_ = NULL;
}

int swMetaData::swmetadata_local_open(SWVARFS * swvarfs, char * name, int * p_seek_set) {
	int u_fd;
	if (swvarfs && swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM) {
		*p_seek_set = UXFIO_SEEK_VSET;
		u_fd = swvarfs_u_open(swvarfs, name);
	} else {
		*p_seek_set = SEEK_SET;	
		u_fd = open(name, O_RDONLY, 0);
	}
	return u_fd;
}

int swMetaData::swmetadata_local_close(SWVARFS * swvarfs, int u_fd) {
	int ret;
	if (swvarfs && swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM) {
		ret = swvarfs_u_close(swvarfs, u_fd);
	} else {
		ret = close(u_fd);
	}
	return ret;
}

mode_t
swMetaData::getModeFromFileType(mode_t mode, int ch) {
	switch (ch) {
		case SW_ITYPE_f:
			mode &= ~CP_IFMT; mode |= CP_IFREG;
			break;
		case SW_ITYPE_d:
			mode &= ~CP_IFMT; mode |= CP_IFDIR;
			break;
		case SW_ITYPE_c:
			mode &= ~CP_IFMT; mode |= CP_IFCHR;
			break;
		case SW_ITYPE_b:
			mode &= ~CP_IFMT; mode |= CP_IFBLK;
			break;
		case SW_ITYPE_p:
			mode &= ~CP_IFMT; mode |= CP_IFIFO;
			break;
		case SW_ITYPE_s:
			mode &= ~CP_IFMT; mode |= CP_IFLNK;
			break;
		case SW_ITYPE_h:
			mode &= ~CP_IFMT; // FIXME, what else to do?
			break;
		default:
			fprintf(stderr, "swpackage: internal error in getModeFromFileType\n");
			return '\0';
	}
	return mode;
}

int
swMetaData::fileStats2statbuf( struct stat * statbuf, char array[][file_permsLength], int array_is_set[])
{
	int ret;
	int ret2;
	unsigned long ulmode;
	unsigned long ul;
	mode_t mode;
	time_t tm;
	gid_t guid;
	uid_t uuid;

	ret = 0;
	memset(statbuf, '\0', sizeof(struct stat));
	ulmode = 0;

	// Set the mode
	if (array_is_set[modeE]) {
		taru_otoul (array[modeE], &ulmode);
		mode = (mode_t)ulmode;
		statbuf->st_mode = mode;
	} else {
		statbuf->st_mode = 0;
		ret++;		
	}

	//
	// Set the file type
	if (array_is_set[typeE] && ret == 0) {
		mode = getModeFromFileType(mode, (int)((unsigned char)(*array[typeE])));
		statbuf->st_mode |= mode;
	} else {
		ret++;
	}
	
	//
	// Set the uid
	//
	if (array_is_set[uidE] && ret == 0) {
		ul = 0;
		if (taru_datoul(array[uidE], &ul))
			ret++;
		statbuf->st_uid = (uid_t)ul;
	} else if (array_is_set[ownerE] && ret == 0) {
		ret2 = taru_get_uid_by_name(array[ownerE], &uuid);
		if (ret2) {
			fprintf(stderr, "swpackage: owner name not found: %s\n", array[ownerE]);
			ret++;
		} else {
			statbuf->st_uid = uuid;
		}
	} else {
		ret++;
	}

	//
	// Set the gid
	//
	if (array_is_set[gidE] && ret == 0) {
		ul = 0;
		if (taru_datoul(array[gidE], &ul))
			ret++;
		statbuf->st_gid = (gid_t)ul;
	} else if (array_is_set[groupE] && ret == 0) {
		ret2 = taru_get_gid_by_name(array[groupE], &guid);
		if (ret2) {
			fprintf(stderr, "swpackage: group name not found: %s\n", array[groupE]);
			ret++;
		} else {
			statbuf->st_gid = guid;
		}
	} else {
		ret++;
	}

	//
	// Set the mtime;
	//
	if (array_is_set[mtimeE] && ret == 0) {
		if (taru_datoul(array[mtimeE], &ul))
			ret ++;
		statbuf->st_mtime = (time_t)ul;
		statbuf->st_ctime = statbuf->st_mtime;
		statbuf->st_atime = statbuf->st_mtime;
	} else if (array_is_set[mtimeE] == 0 && ret == 0) {
		statbuf->st_mtime = time(NULL);
		statbuf->st_ctime = statbuf->st_mtime;
		statbuf->st_atime = statbuf->st_mtime;
	} else {
		ret ++;
	}


	// FIXME? this abandons hard link information
	statbuf->st_nlink = 0;

	//
	// Set the size
	//
	if (*array[typeE] == SW_ITYPE_f && array_is_set[sizeE] && ret == 0) {
		ul = 0;
		if (taru_datoul(array[sizeE], &ul))
			ret++;
		statbuf->st_size = (off_t)ul;
	} else if (*array[typeE] != SW_ITYPE_f) {
		statbuf->st_size = (off_t)(0);
	} else {
		ret++;
	}

	return ret;
}


char
swMetaData::getFileTypeFromMode(mode_t mode) {
	int ch;
	ch = taru_get_tar_filetype(mode);
	switch (ch) {
		case REGTYPE:
		case CONTTYPE:
			return SW_ITYPE_f;
			break;
		case DIRTYPE:
			return SW_ITYPE_d;
			break;
		case CHRTYPE:
			return SW_ITYPE_c;
			break;
		case BLKTYPE:
			return SW_ITYPE_b;
			break;
		case FIFOTYPE:
			return SW_ITYPE_p;
			break;
		case SYMTYPE:
			return SW_ITYPE_s;
			break;
		case LNKTYPE:
			return SW_ITYPE_h;
			break;
		case AREGTYPE:
			return SW_ITYPE_f;
			break;
		case NOTDUMPEDTYPE:
			return SW_ITYPE_y;
			break;
		case -1:
		default:
			return '\0';
	}
}

/* Private */ void
swMetaData::setTarLinkname(char * dest, char * linkname, int size) {
	if (strlen(linkname) > file_permsLength) {
		fprintf(stderr, "link source name too long: link_name=[%s]\n", linkname);
		fprintf(stderr, "fatal... exiting with status 1\n");
		_exit(1);
	}
	strncpy(dest, linkname, size - 1);
	dest[size - 1] = '\0';
}

/* Private */ 
int
swMetaData::did_sense_hard_link(SWVARFS * swvarfs, struct stat * st) {
	struct new_cpio_header * file_hdr = swvarfs_header(swvarfs);
	char * linkname = ahsStaticGetTarLinkname(file_hdr);
	if (
		(file_hdr->c_nlink > 1 || file_hdr->c_is_tar_lnktype == 1) &&
		linkname && strlen(linkname)
	)
	{
		return 1;
	}
	return 0;
}

int swMetaData::swmetadata_decode_file_stats(SWVARFS * swvarfs, 
			char *source,  char * hl_path,
				struct stat *st,
					char array[][file_permsLength],
						int array_is_set[],
							int cksumflags ) {
		char * p_owner;
		char * p_mode;
		char * p_group;
		char * p_type;
		char * p_size;
		char * p_uid;
		char * p_gid;
		char * p_mtime;
		char * p_minor;
		char * p_ino;
		char * p_nlink;
		char * p_major;
		char * p_ctime;
		char * p_swbi;
		char * p_linksource;
		char * p_tmp;
		unsigned long filetime;
		mode_t mode;
		int intval;
		int uxfio_seek_set;
		int u_fd = -1;
		int do_create_cksum;
		int do_create_file_digests;
		int do_create_file_digests2;
		int ret = 0;
		int devnum;
		HLLIST * hllist = swvarfs_get_hllist(swvarfs);
		hllist_entry * link_record_buf = NULL;
		int hlink_nfound = 0;
		struct new_cpio_header * file_hdr;

		SWMETADATA_DEBUG("");
		do_create_cksum = cksumflags & SWFILE_DO_CKSUM;
		do_create_file_digests = cksumflags & SWFILE_DO_FILE_DIGESTS;
		do_create_file_digests2 = cksumflags & SWFILE_DO_FILE_DIGESTS_SHA2;
	
		uxfio_seek_set = SEEK_SET;	
		//
		// File Type
		//
		p_type =  array[typeE];
		if (*p_type == '\0') {
			SWMETADATA_DEBUG("");
			*p_type = swMetaData::getFileTypeFromMode(st->st_mode);
			if (*p_type == '\0') {
				fprintf(stderr, "swpackage: unrecognized file type for file: %s\n", source?source:"<null>");
			}
			*(p_type + 1) = '\0';
			if (*p_type == SW_ITYPE_y) {
				// fprintf(stderr, "swpackage: %s: socket ignored\n", source?source:"<null>");
				ret = 1;
			}
		}
		if (did_sense_hard_link(swvarfs, st) == 1) {
			SWMETADATA_DEBUG("did sense hard link");
			if (*p_type == SW_ITYPE_f) {
				*p_type = SW_ITYPE_h;
				SWMETADATA_DEBUG("did sense hard link: type f");
			} else {
				/*
				fprintf(stderr, "This implementation only supports linked regular files.\n");
				fprintf(stderr, "Exiting with status 90.\n");
				exit(90);
				*/
			}
		}

		array_is_set[typeE] = 1;
		array_is_set[cksumE] = 0;

		if ((do_create_cksum || do_create_file_digests || do_create_file_digests2) && *p_type == SW_ITYPE_f ) {
			unsigned long ck;
			u_fd = swMetaData::swmetadata_local_open(swvarfs, source, &uxfio_seek_set);
			if (u_fd < 0) {
				fprintf(stderr, "swMetaData::swmetadata_local_open [%s] failed.\n", source);
				return -1;
			} else {
				int uxret;
				char bufc[60];
				char bufc_sha1[60];
				char bufc_size[40];
				char bufc_sha2[132];
				char * p_bufc_sha2;
				bufc[0] = '\0';
				bufc_sha1[0] = '\0';
				bufc_sha2[0] = '\0';
				bufc_size[0] = '\0';


				if (do_create_cksum) {
					uxret = uxfio_lseek(u_fd, 0, uxfio_seek_set);
					if (uxret < 0) {
						fprintf(stderr, "Internal software error. location=a.\n");
						exit(52);
					}
					ck = ::swlib_cksum(u_fd);
					snprintf(bufc, sizeof(bufc), "%lu", ck);
					bufc[sizeof(bufc) - 1] = '\0';
					::swlib_strncpy(array[cksumE], bufc, file_permsLength - 1);
					array_is_set[cksumE] = 1;
				}
				if (do_create_file_digests || do_create_file_digests2) {
					if (bufc[0] != '\0') {  // Optimization.
						//
						// Need to rewind the file.
						//
						uxret = uxfio_lseek(u_fd, 0, uxfio_seek_set);
						if (uxret < 0) {
							fprintf(stderr, "Internal software error. location=b.\n");
							exit(52);
						}
					}
					if (do_create_file_digests2) {
						p_bufc_sha2 = bufc_sha2;			
					} else {
						p_bufc_sha2 = NULL;
					}
					if (swlib_digests(u_fd, bufc, bufc_sha1, bufc_size, p_bufc_sha2)) {
						// Fatal error
						fprintf(stderr, 
						"Internal software error in swlib_digests\n");
						exit(52);
					}
					
					// Sanity check
					//
					if (st->st_size != atoi(bufc_size)) {
						fprintf(stderr, "Internal software fatal error. (digests) wrong size\n");
						exit(55);
					}

					::swlib_strncpy(array[md5sumE], bufc, file_permsLength - 1);
					array_is_set[md5sumE] = 1;
					::swlib_strncpy(array[sha1sumE], bufc_sha1, file_permsLength - 1);
					array_is_set[sha1sumE] = 1;
					if (do_create_file_digests2) {
						::swlib_strncpy(array[sha512sumE], bufc_sha2, file_permsLength - 1);
						array_is_set[sha512sumE] = 1;
					}
				}
				swMetaData::swmetadata_local_close(swvarfs, u_fd);
			}
		}

		p_mode = array[modeE];
		p_owner = array[ownerE];
		p_group = array[groupE];
		p_size =  array[sizeE];
		p_uid = array[uidE];
		p_gid = array[gidE];
		p_mtime = array[mtimeE];
		p_minor = array[minorE];
		p_ino = array[inoE];
		p_nlink = array[nlinkE];
		p_major = array[majorE];
		p_ctime = array[ctimeE];
		p_linksource = array[linksourceE];
		p_swbi = array[swbiE]; 	// Internal attribute used as a flag.
		
		// array_is_set[linksourceE] = 0;
		if (*p_type == SW_ITYPE_s && array_is_set[linksourceE] == 0) {
			//
			// Get the link source.
			//
			SWMETADATA_DEBUG("");
			array_is_set[linksourceE] = 1;
			if (swvarfs_u_readlink(swvarfs, source, p_linksource, (size_t)(file_permsLength-1)) <= 0) {
				fprintf(stderr, "error reading link for %s : error or link source too long.\n", source);
				ret = -1;
				*p_linksource = '\0';
			}
		}
		
		//
		// Handle hard links.
		//
		if (swvarfs && swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM) {
			SWMETADATA_DEBUG("");
			if ((int)(st->st_nlink) > 1 && ((*p_type == SW_ITYPE_f) || (*p_type == SW_ITYPE_h))) {
				SWMETADATA_DEBUG("");
				file_hdr = swvarfs_header(swvarfs);
				setTarLinkname(p_linksource, ahsStaticGetTarLinkname(file_hdr), file_permsLength);
			}
		}

		if ((int)(st->st_nlink) > 1 && (*p_type == SW_ITYPE_f)) {
			SWMETADATA_DEBUG("");
			if (swvarfs && swvarfs_get_format(swvarfs) != UINFILE_FILESYSTEM) {
				//
				// Sourcing on the archive.  The hard links are already a hard link.
				//
				// This is an invalid case but it shows up with the unfixed lxpsf programs.
				//
				//*p_type = 'h';  // Setting to 'h' causes downstream tar archive corruption.
				//file_hdr = swvarfs_header(swvarfs);
				//setTarLinkname(p_linksource, ahsStaticGetTarLinkname(file_hdr), file_permsLength);
				SWMETADATA_DEBUG("");
			} else {
				//
				// Source from the file system.
				//
				//fprintf(stderr, "hardlink: %s [%s] nlink=%d\n", source, p_linksource, (int)(st->st_nlink));
				SWMETADATA_DEBUG("");
				link_record_buf = hllist_find_file_entry(hllist, st->st_dev, st->st_ino, 1, &hlink_nfound);
				if (hlink_nfound) {
					//fprintf(stderr, "hardlink: setting link_source [%s]\n", link_record_buf->path_);
					*p_type = SW_ITYPE_h;
					setTarLinkname(p_linksource, link_record_buf->path_, file_permsLength);
				} else {
					hllist_add_record(hllist, source, (dev_t)(st->st_dev), st->st_ino);
				}
			}
		}

		//
		// File Size
		// Always get the size.
		//
		if (*p_type == SW_ITYPE_f) {
			SWMETADATA_DEBUG("");
			// snprintf(p_size, file_permsLength,  "%d", (int)(st->st_size));
			p_tmp = ::swlib_imaxtostr((intmax_t)(st->st_size), NULL); 
			strncpy(p_size, p_tmp, file_permsLength);
			p_size[file_permsLength-1] = '\0';
			array_is_set[sizeE] = 1;
		} else if (*p_type == SW_ITYPE_s) {
			SWMETADATA_DEBUG("");
			snprintf(p_size, file_permsLength, "%d", (int)(strlen( p_linksource)));
			p_size[file_permsLength-1] = '\0';
			array_is_set[sizeE] = 1;
		} else if (*p_type == SW_ITYPE_h) {
			SWMETADATA_DEBUG("");
			snprintf(p_size, file_permsLength, "%d", (int)(0));
			p_size[file_permsLength-1] = '\0';
			array_is_set[sizeE] = 1;
		} else {
			SWMETADATA_DEBUG("");
			snprintf(p_size, file_permsLength, "%d", (int)(0));
			p_size[file_permsLength-1] = '\0';
			array_is_set[sizeE] = 1;
		}

		//
		// File Mode
		//
		if (*p_mode == '\0') {
			SWMETADATA_DEBUG("p_mode == nil");
			mode = st->st_mode;	
			mode &= (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX);
			snprintf(p_mode, file_permsLength, "%o", (unsigned int)mode);
			p_mode[file_permsLength-1] = '\0';
			array_is_set[modeE] = 1;
		} else {
			;
			SWMETADATA_DEBUG2("p_mode already set: [%s]", p_mode);
		}
		
		//
		// File ctime
		//
		if (*p_ctime == '\0') {
			SWMETADATA_DEBUG("p_ctime == nil");
			filetime = (unsigned long)(st->st_ctime);
			snprintf(p_ctime, file_permsLength, "%lu", filetime);
			p_ctime[file_permsLength-1] = '\0';
			array_is_set[ctimeE] = 0;  // Don't include ctime.
		} else {
			;
			SWMETADATA_DEBUG2("p_ctime already set: [%s]", p_ctime);
		}	
		//
		// File mtime
		//
		if (*p_mtime == '\0') {
			SWMETADATA_DEBUG("p_mtime == nil");
			filetime = (unsigned long)(st->st_mtime);
			snprintf(p_mtime, file_permsLength, "%lu", filetime);
			p_mtime[file_permsLength-1] = '\0';
			array_is_set[mtimeE] = 1;
		} else {
			;
			SWMETADATA_DEBUG2("p_mtime already set: [%s]", p_mtime);
		}
		
		//
		// File ino
		//
		if (*p_ino == '\0') {
			SWMETADATA_DEBUG("p_ino == nil");
			filetime = (unsigned long)(st->st_ino);
			snprintf(p_ino, file_permsLength, "%lu", filetime);
			p_ino[file_permsLength-1] = '\0';
			SWMETADATA_DEBUG2("p_ino set: [%s]", p_ino);
			array_is_set[inoE] = 1;
		} else {
			;
			SWMETADATA_DEBUG2("p_ino already set: [%s]", p_ino);
		}
		
		//
		// File nlink
		//
		if (*p_nlink == '\0') {
			SWMETADATA_DEBUG("p_nlink == nil");
			intval = (int)(st->st_nlink);
			snprintf(p_nlink, file_permsLength, "%d", intval);
			p_nlink[file_permsLength-1] = '\0';
			array_is_set[nlinkE] = 1;
			SWMETADATA_DEBUG2("p_nlink set: [%s]", p_nlink);
		} else {
			;
			SWMETADATA_DEBUG2("p_nlink already set: [%s]", p_nlink);
		}
		
		//
		// File major
		//
		if (*p_major == '\0') {
			SWMETADATA_DEBUG("p_major == nil");
			devnum = major(st->st_rdev);
			snprintf(p_major, file_permsLength, "%d", devnum);
			p_major[file_permsLength-1] = '\0';
			array_is_set[majorE] = 1;
			SWMETADATA_DEBUG2("p_major set: [%s]", p_major);
		} else {
			;
			SWMETADATA_DEBUG2("p_major already set: [%s]", p_major);
		}
		
		//
		// File minor
		//
		if (*p_minor == '\0') {
			SWMETADATA_DEBUG("p_minor == nil");
			devnum = minor(st->st_rdev);
			snprintf(p_minor, file_permsLength, "%d", devnum);
			p_minor[file_permsLength-1] = '\0';
			array_is_set[minorE] = 1;
			SWMETADATA_DEBUG2("p_minor now set: [%s]", p_minor);
		} else {
			;
			SWMETADATA_DEBUG2("p_minor already set: [%s]", p_minor);
		}

		//
		// File Owner
		//
		if (*p_owner == '\0') {
			uid_t uid = st->st_uid;
			SWMETADATA_DEBUG("p_owner == nil");
			taru_get_tar_user_by_uid(uid,  p_owner);
			snprintf(p_uid,  file_permsLength, "%d",(int)uid);
			p_uid[file_permsLength-1] = '\0';
			array_is_set[ownerE] = 1;
			array_is_set[uidE] = 1;
			SWMETADATA_DEBUG2("p_owner set: [%s]", p_owner);
		} else if (strcmp(p_owner, TARU_C_DO_NUMERIC)  == 0) {
			uid_t uid = st->st_uid;
			SWMETADATA_DEBUG2("p_owner Doing numeric: [%s]", p_owner);
			snprintf(p_uid,  file_permsLength, "%d",(int)uid);
			p_uid[file_permsLength-1] = '\0';
			*p_owner = '\0';
			array_is_set[ownerE] = 0;
			array_is_set[uidE] = 1;
		} else {
			;
			SWMETADATA_DEBUG2("p_owner already set: [%s]", p_owner);
		}
			
		//
		// File Group
		//
		if (*p_group == '\0') {
			gid_t gid = st->st_gid;
			SWMETADATA_DEBUG("p_group == nil");
			taru_get_tar_group_by_gid(gid, p_group);
			snprintf(p_gid,  file_permsLength, "%d",(int)gid);
			p_gid[file_permsLength-1] = '\0';
			array_is_set[groupE] = 1;
			array_is_set[gidE] = 1;
			SWMETADATA_DEBUG2("p_group set: [%s]", p_group);
		} else if (strcmp(p_group, TARU_C_DO_NUMERIC)  == 0) {
			gid_t gid = st->st_gid;
			SWMETADATA_DEBUG2("p_group Doing numeric: [%s]", p_group);
			snprintf(p_gid,  file_permsLength, "%d",(int)gid);
			p_gid[file_permsLength-1] = '\0';
			*p_group = '\0';
			array_is_set[groupE] = 0;
			array_is_set[gidE] = 1;
		} else {
			;
			SWMETADATA_DEBUG2("p_group already set: [%s]", p_group);
		}

		//
		// Indicate that this file does not need to be stat'ed again.
		//
		array_is_set[swbiE] = 1;
		strncpy(p_swbi, "true", 10);

	return ret;
}
