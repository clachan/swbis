/* swdefcontrolfile.cxx
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.
 *
 * COPYING TERMS AND CONDITIONS
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */


#include "swuser_config.h"
#include "swdefcontrolfile.h"


swDefControlFile::swDefControlFile(void): swDefinition() { }

swDefControlFile::swDefControlFile(int level): swDefinition("controlfile", level) { }

swDefControlFile::~swDefControlFile(void) {
}

char * swDefControlFile::getPathAttribute(void)
{
	char * path = find(SW_A_path);
	if (!path) {
		char * source = find(SW_A_source);
		assert(source);
		char * basename = ::strrchr(source,'/');
		if (!basename) {
			path = source;
		} else {
			path = basename + 1;
			while (*path && *path == '/') path++;
		}
	}
	return path;
}

char * swDefControlFile::getTagAttribute(void)
{
	char * tag = find(SW_A_tag);
	if (!tag) {
		tag = getPathAttribute();
	}
	return tag;
}

void swDefControlFile::apply_file_stat_specialization(char fileArray[][file_permsLength], int fileArrayWasSet[])
{
	int i;
	swMetaData * swmd;

	fileArrayWasSet[ctimeE] = 0;
	// fileArrayWasSet[mtimeE] = 0;
	fileArrayWasSet[umaskE] = 0;
	fileArrayWasSet[sha1sumE] = 0;
	fileArrayWasSet[sha512sumE] = 0;
	fileArrayWasSet[md5sumE] = 0;
	fileArrayWasSet[cksumE] = 0;
	fileArrayWasSet[gidE] = 0;
	fileArrayWasSet[uidE] = 0;
	// fileArrayWasSet[modeE] = 0;

	for (i=0; i<lastE; i++) {
		if (fileArrayWasSet[i] == 0) {
			swmd = findAttribute(namesM[i]);
			if (swmd) swmd->vremove();
		}
	}

}

void 
swDefControlFile::set_path_attribute(char * source)
{
	STROB * tmp;
	char * path = find(SW_A_path);
	if (path) return;
	
	tmp = strob_open(10);
	if (!source) {
		fprintf(stderr, "swDefControlFile::set_path_attribute: source attribute missing.\n");
		exit(1);
	}
	swlib_basename(tmp, source);

	//
	// Section 5.2.11 Lines 677-678
	//
	add(SW_A_path, strob_str(tmp));
	strob_close(tmp);
}

void 
swDefControlFile::set_tag_attribute(void)
{
	char * path;
	char * source;
	STROB * tmp;
	char * tag = find(SW_A_tag);
	
	if (tag) return;
	tmp = strob_open(10);
	//
	// Section 5.2.10  Lines 637-638
	//
	source = find(SW_A_source);
	
	path = find(SW_A_path);
	swlib_basename(tmp, path);

	add(SW_A_tag, strob_str(tmp));
	strob_close(tmp);
}
