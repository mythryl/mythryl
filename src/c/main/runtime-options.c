// runtime-options.c
//
// Command-line argument processing utilities.

#include "../mythryl-config.h"

#include <ctype.h>
#include "runtime-base.h"
#include "runtime-commandline-argument-processing.h"

Bool   is_runtime_option   (char* commandline_arg,  char* option,  char** arg)   {
    // 
    // Check a command line argument to see if it is a possible runtime
    // system argument (i.e., has the form "--runtime-xxx" or "--runtime-xxx=yyy").
    // If the command-line argument is a runtime-system argument, then
    // return TRUE, and copy the "xxx" part into option, and set arg to
    // point to the start of the "yyy" part.

    char   c;
    char* cp = commandline_arg;

    if ((cp[0] == '-')
    &&  (cp[1] == '-')
    &&  (cp[2] == 'r')
    &&  (cp[3] == 'u')
    &&  (cp[4] == 'n')
    &&  (cp[5] == 't')
    &&  (cp[6] == 'i')
    &&  (cp[7] == 'm')
    &&  (cp[8] == 'e')
    &&  (cp[9] == '-')
    ){
        cp += 10;
	while (((c = *cp++) != '\0') && (c != '=')) {
            *option++ = c;   // Can you say "Invitation to buffer overrun"? I thought you could! XXX BUGGO FIXME
	}
	*option = '\0';
	*arg = cp;
	return TRUE;

    } else {
	return FALSE;
    }
}


int   get_size_option   (int scale,  char* size)   {
    //
    // Get a size specification, accepting K, M and G suffixes.

    char* p;

    // Find first non-digit in the string:
    //
    for (p = size;  isdigit(*p); p++)  continue;

    if (p == size) {
	return -1;
    } else {
	switch (*p) {

	  case '\0':
	    break;

	  case 'k':
	  case 'K':
	    scale = ONE_K_BINARY;
	    break;

	  case 'm':
	  case 'M':
	    scale = ONE_MEG_BINARY;
	    break;

	  case 'g':
	  case 'G':
	    scale = ONE_MEG_BINARY * ONE_K_BINARY;
	    break;

	  default:
	    return -1;
	}
	return  scale * atoi(size);
    }
}



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


