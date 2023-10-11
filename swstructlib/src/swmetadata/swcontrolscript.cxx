/* swcontrolscript.cxx
 *
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
#include "swcontrolscript.h"
#include "swcontrolscripts.h"


swControlScript::swControlScript(char * pkg_path): swControlFile(pkg_path) { }
swControlScript::swControlScript(char * pkg_path, char * src_path): swControlFile(pkg_path, src_path) { }
swControlScript::~swControlScript (void) { }


int swControlScript::is_controlscript(char * object_keyword) {

             if ( !::strcmp ("checkinstall",object_keyword )) { return 1;}
        else if ( !::strcmp ("install",object_keyword )) { return 1;}
        else if ( !::strcmp ("remove",object_keyword )) { return 1;}
        else if ( !::strcmp ("preinstall",object_keyword )) { return 1;}
        else if ( !::strcmp ("postinstall",object_keyword )) { return 1;}
        else if ( !::strcmp ("verify",object_keyword )) { return 1;}
        else if ( !::strcmp ("fix",object_keyword )) { return 1;}
        else if ( !::strcmp ("checkremove",object_keyword )) { return 1;}
        else if ( !::strcmp ("preremove",object_keyword )) { return 1;}
        else if ( !::strcmp ("postremove",object_keyword )) { return 1;}
        else if ( !::strcmp ("configure",object_keyword )) { return 1;}
        else if ( !::strcmp ("unconfigure",object_keyword )) { return 1;}
        else if ( !::strcmp ("request",object_keyword )) { return 1;}
        else if ( !::strcmp ("unpreinstall",object_keyword )) { return 1;}
        else if ( !::strcmp ("unpostinstall",object_keyword )) { return 1;}
        else if ( !::strcmp ("space",object_keyword )) { return 1;}
        else { return 0; }
}

