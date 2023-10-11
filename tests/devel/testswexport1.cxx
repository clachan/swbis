#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include "xstream_config.h"

extern "C" {
#include "swlib.h"
#include "swheader.h"
#include "swheaderline.h"
#include "usgetopt.h"
}

#include "swparser.h"
#include "swdefinitionfile.h"
#include "swpsf.h"
#include "swpackagefile.h"
#include "swexstruct.h"
#include "swexdistribution.h"

#include "swexfileset.h"
#include "swexproduct.h"
#include "swexpsf.h"
#include "swexdistribution.h"
#include "swscollection.h"
#include "swcommon.h"
//#include "swbuild.h"

#include "swmain.h"

#define SWPARSE_AT_LEVEL	0
#define LINELEN 200

int
main(int argc, char **argv)
{
	int error_code = 0;
	int ifd;
	char psffilename[512];

	swPSF * swpsf;
	swExFileset fset;
	swExProduct prod;
	swINFO iii("");;
	swExCat * swexdist;  // May be either an swExDistribution of swExHost.

	strcpy(psffilename, "-");

	ifd = STDIN_FILENO;

	//
	// Open a swDefinitionFile object.
	//
	swpsf = new swPSF("");

	//
	// Setup parser.
	//
	if (swpsf->open_parser(ifd)){
		cerr << "Error opening parser.\n";
	}

	//
	// Parse the PSF.
	//
	if (swpsf->run_parser(SWPARSE_AT_LEVEL, SWPARSE_FORM_MKUP_LEN) < 0){
		cerr << "parser error.\n";
		exit(1);
	}

	//
	// Expand extended defintiions in the PSF file.
	//
	if (swpsf->generateDefinitions()) {
		exit(1);
	}


	//
	// Construct the export object.
	//
	swexdist = swExCat::constructSwExDist(swpsf, &error_code, NULL);

	//
	// Write out something that looks like a PSF.
	//	
	swexdist->write_fd(STDOUT_FILENO);

	delete swexdist;
	delete swpsf;
	exit(0);
}

