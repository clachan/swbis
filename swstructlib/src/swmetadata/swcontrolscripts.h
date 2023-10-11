/* swcontrolscripts.h
 */

/*
 * Copyright (C) 1998  James H. Lowe, Jr.  <jhlowe@acm.org>
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


#ifndef swcontrolscripts_h_0925
#define swcontrolscripts_h_0925

extern "C" {
#include "swuser_config.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "swcontrolscript.h"

class swCheckInstall: public swControlScript {
public:
    swCheckInstall(char * path): swControlScript (path) { }
    ~swCheckInstall(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "checkinstall"); }
    static swCheckInstall * make_definition (char * pkgpath) { return new swCheckInstall (pkgpath); }
};

class swPreInstall: public swControlScript {
public:
    swPreInstall(char * path): swControlScript (path) { }
    ~swPreInstall(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "preinstall"); }
    static swPreInstall * make_definition (char * pkgpath) { return new swPreInstall (pkgpath); }
};

class swPostInstall: public swControlScript {
public:
    swPostInstall(char * path): swControlScript (path) { }
    ~swPostInstall(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "postinstall"); }
    static swPostInstall * make_definition (char * pkgpath) { return new swPostInstall (pkgpath); }
};

class swVerify: public swControlScript {
public:
    swVerify(char * path): swControlScript (path) { }
    ~swVerify(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "verify"); }
    static swVerify * make_definition (char * pkgpath) { return new swVerify (pkgpath); }
};

class swFix: public swControlScript {
public:
    swFix(char * path): swControlScript (path) { }
    ~swFix(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "fix"); }
    static swFix * make_definition (char * pkgpath) { return new swFix (pkgpath); }
};

class swCheckRemove: public swControlScript {
public:
    swCheckRemove(char * path): swControlScript (path) { }
    ~swCheckRemove(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "checkremove"); }
    static swCheckRemove * make_definition (char * pkgpath) { return new swCheckRemove (pkgpath); }
};

class swPreRemove: public swControlScript {
public:
    swPreRemove(char * path): swControlScript (path) { }
    ~swPreRemove(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "preremove"); }
    static swPreRemove * make_definition (char * pkgpath) { return new swPreRemove (pkgpath); }
};

class swPostRemove: public swControlScript {
public:
    swPostRemove(char * path): swControlScript (path) { }
    ~swPostRemove(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "postremove"); }
    static swPostRemove * make_definition (char * pkgpath) { return new swPostRemove (pkgpath); }
};

class swConfigure: public swControlScript {
public:
    swConfigure(char * path): swControlScript (path) { }
    ~swConfigure(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "configure"); }
    static swConfigure * make_definition (char * pkgpath) { return new swConfigure (pkgpath); }
};

class swunConfigure: public swControlScript {
public:
    swunConfigure(char * path): swControlScript (path) { }
    ~swunConfigure(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "unconfigure"); }
    static swunConfigure * make_definition (char * pkgpath) { return new swunConfigure (pkgpath); }
};

class swRequest: public swControlScript {
public:
    swRequest(char * path): swControlScript (path) { }
    ~swRequest(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "request"); }
    static swRequest * make_definition (char * pkgpath) { return new swRequest (pkgpath); }
};

class swunPreInstall: public swControlScript {
public:
    swunPreInstall(char * path): swControlScript (path) { }
    ~swunPreInstall(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "unpreinstall"); }
    static swunPreInstall * make_definition (char * pkgpath) { return new swunPreInstall (pkgpath); }
};

class swunPostInstall: public swControlScript {
public:
    swunPostInstall(char * path): swControlScript (path) { }
    ~swunPostInstall(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "unpostinstall"); }
    static swunPostInstall * make_definition (char * pkgpath) { return new swunPostInstall (pkgpath); }
};

class swSpace: public swControlScript {
public:
    swSpace(char * path): swControlScript (path) { }
    ~swSpace(void) {}
    int get_type(void) { return swstructdef::return_entry_index(NULL, "space"); }
    static swSpace * make_definition (char * pkgpath) { return new swSpace (pkgpath); }
};



#endif


