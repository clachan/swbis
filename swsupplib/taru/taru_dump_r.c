#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "strob.h"
#include "swlib.h"
#include "tarhdr.h"
#include "cpiohdr.h"

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

static STROB * buf = NULL;

char *
taru_header_dump_string_s(struct new_cpio_header * file_hdr, char * prefix)
{
	char *name; 
	char *linkname; 
	char tarheader[512];
	char modestring[32];
	char user_name[128];
	char group_name[128];
	TARU * taru = taru_create();

	if (!buf) buf = strob_open(100);
	
	if (!prefix) prefix = "";

	memset(user_name, (int)('\0'), sizeof(user_name));
	memset(group_name, (int)('\0'), sizeof(group_name));
	memset(modestring, (int)('\0'), sizeof(modestring));
	
	name = ahsStaticGetTarFilename(file_hdr);
	linkname = ahsStaticGetTarLinkname(file_hdr);
	if (!name) name = "<null>";
	if (!linkname) linkname = "<null>";

	strob_sprintf(buf, 0, "%s%p (taru_header*)\n", prefix, (void*)file_hdr);

	strob_sprintf(buf, 1, "%s%p c_name =         [%s]\n",  prefix, (void*)file_hdr, name);
	strob_sprintf(buf, 1, "%s%p c_tar_linkname = [%s]\n",  prefix, (void*)file_hdr, linkname);
	strob_sprintf(buf, 1, "%s%p c_ino =          [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_ino);
	strob_sprintf(buf, 1, "%s%p c_mode =         [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_mode);
	strob_sprintf(buf, 1, "%s%p c_uid =          [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_uid);
	strob_sprintf(buf, 1, "%s%p c_gid =          [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_gid);
	strob_sprintf(buf, 1, "%s%p c_nlink =        [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_nlink);
	strob_sprintf(buf, 1, "%s%p c_mtime =        [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_mtime);
	strob_sprintf(buf, 1, "%s%p c_filesize =     [%lu]\n", prefix, (void*)file_hdr, (unsigned long)file_hdr->c_filesize);
	strob_sprintf(buf, 1, "%s%p uname =          [%s]\n",  prefix, (void*)file_hdr, ahsStaticGetTarUsername(file_hdr));
	strob_sprintf(buf, 1, "%s%p gname =          [%s]\n",  prefix, (void*)file_hdr, ahsStaticGetTarGroupname(file_hdr));
	
		swlib_filemodestring(file_hdr->c_mode, modestring);	

	strob_sprintf(buf, 1, "%s%p modestring =     [%s]\n", prefix, (void*)file_hdr, modestring);
	strob_sprintf(buf, 1, "%s%p c_dev_maj =      [%ld]\n", prefix, (void*)file_hdr, file_hdr->c_dev_maj);
	strob_sprintf(buf, 1, "%s%p c_dev_min =      [%ld]\n", prefix, (void*)file_hdr, file_hdr->c_dev_min);
	strob_sprintf(buf, 1, "%s%p c_rdev_maj =     [%ld]\n", prefix, (void*)file_hdr, file_hdr->c_rdev_maj);
	strob_sprintf(buf, 1, "%s%p c_rdev_min =     [%ld]\n", prefix, (void*)file_hdr, file_hdr->c_rdev_min);
	strob_sprintf(buf, 1, "%s%p c_namesize =     [%lu]\n", prefix, (void*)file_hdr, file_hdr->c_namesize);
	strob_sprintf(buf, 1, "%s%p c_cu       =     [%d]\n", prefix, (void*)file_hdr, (int)file_hdr->c_cu);
	strob_sprintf(buf, 1, "%s%p c_cg       =     [%d]\n", prefix, (void*)file_hdr, (int)file_hdr->c_cg);
	taru_delete(taru);
	return strob_str(buf);
}


int
taru_header_dump(struct new_cpio_header * file_hdr, FILE * fp)
{
	char *name; 
	char *linkname; 
	struct tar_header *tar_hdr;
	char tarheader[512];
	char modestring[32];
	unsigned long name_sum;
	char user_name[128];
	char group_name[128];
	TARU * taru = taru_create();

	memset(user_name, (int)('\0'), sizeof(user_name));
	memset(group_name, (int)('\0'), sizeof(group_name));
	memset(modestring, (int)('\0'), sizeof(modestring));
	
	name = ahsStaticGetTarFilename(file_hdr);
	linkname = ahsStaticGetTarLinkname(file_hdr);
	if (!name) name = "<null>";
	if (!linkname) linkname = "<null>";

	taru_write_out_tar_header2(taru, file_hdr, -1, tarheader, (char*)NULL, (char*)NULL, 0);
	tar_hdr=(struct tar_header*)(tarheader);

	memcpy(user_name, tar_hdr->uname, sizeof(tar_hdr->uname));
	memcpy(group_name, tar_hdr->gname, sizeof(tar_hdr->gname));

	name_sum = (unsigned long)swlib_bsd_sum_from_mem((unsigned char*)name, strlen(name));

	fprintf(fp,"%05lu = bsd sum of name\n", name_sum);
	fprintf(fp,"%05lu attribute = [<attribute_value>]\n", name_sum);
	fprintf(fp,"%05lu c_name = [%s]\n", name_sum, name);
	fprintf(fp,"%05lu c_tar_linkname = [%s]\n", name_sum, linkname);
	fprintf(fp,"%05lu c_ino =  [%lu]\n", name_sum, file_hdr->c_ino);
	fprintf(fp,"%05lu c_mode = [%lu]\n", name_sum, file_hdr->c_mode);
	fprintf(fp,"%05lu c_uid = [%lu]\n", name_sum, file_hdr->c_uid);
	fprintf(fp,"%05lu c_gid = [%lu]\n", name_sum, file_hdr->c_gid);
	fprintf(fp,"%05lu c_nlink = [%lu]\n", name_sum, file_hdr->c_nlink);
	fprintf(fp,"%05lu c_mtime = [%lu]\n", name_sum, file_hdr->c_mtime);
	fprintf(fp,"%05lu c_filesize = [%lu]\n", name_sum, (unsigned long)(file_hdr->c_filesize));
	fprintf(fp,"%05lu uname = [%s]\n", name_sum, user_name);
	fprintf(fp,"%05lu gname = [%s]\n", name_sum, group_name);
	swlib_filemodestring(file_hdr->c_mode, modestring);	
	fprintf(fp,"%05lu modestring = [%s]\n", name_sum, modestring);
	fprintf(fp,"%05lu c_dev_maj = [%ld]\n", name_sum, file_hdr->c_dev_maj);
	fprintf(fp,"%05lu c_dev_min = [%ld]\n", name_sum, file_hdr->c_dev_min);
	fprintf(fp,"%05lu c_rdev_maj = [%ld]\n", name_sum, file_hdr->c_rdev_maj);
	fprintf(fp,"%05lu c_rdev_min = [%ld]\n", name_sum, file_hdr->c_rdev_min);
	fprintf(fp,"%05lu c_namesize = [%lu]\n", name_sum, file_hdr->c_namesize);
	taru_delete(taru);
	return 0;
}

