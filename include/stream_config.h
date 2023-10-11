/**/
#ifndef SSTREAM_CONFIG_H
#define SSTREAM_CONFIG_H
#include "config.h"


#ifdef HAVE_OSTREAM
#include<ostream>
/*#else
#include<ostream.h>
*/
#endif

#ifdef HAVE_IOSTREAM
#include<iostream>
/*
#else
#include<iostream.h>
#include <iostream.h>
*/
#endif

#endif
