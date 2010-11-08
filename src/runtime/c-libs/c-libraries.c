/* c-libraries.c
 *
 * This is the home of the CLibrary table, C library initialization code,
 * and C function lookup code.  It is part of the run-time proper (not part
 * of libcfuns.a).
 */

#include "../config.h"

#ifdef OPSYS_UNIX
#  include "runtime-unixdep.h"	/* for the HAS_POSIX_LIBRARIES option flag */
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "c-library.h"
#include <stdio.h>

#define C_LIBRARY(lib)  extern c_library_t lib;
#include "clib-list.h"
#undef C_LIBRARY

static c_library_t	*CLibs[] = {
#define C_LIBRARY(lib)	&lib,
#	include "clib-list.h"
#undef C_LIBRARY
	NULL
    };

/* InitCFunList:
 * Initialize the list of C functions callable from lib7.
 */
void InitCFunList ()
{
    int             i, j, libNameLen;
    char	    *nameBuf;

    for (i = 0;  CLibs[i] != NULL;  i++) {
	c_library_t	*clib = CLibs[i];
	cfunc_naming_t	*cfuns = CLibs[i]->cfuns;

	if (clib->initFn != NULL) {
	  /* call the libraries initialization function */
	    (*(clib->initFn)) (0, 0/** argc, argv **/);
	}

      /* register the C functions in the C symbol table */
	libNameLen = strlen(clib->libName) + 2; /* incl "." and "\0" */
	for (j = 0;  cfuns[j].name != NULL;  j++) {
	    nameBuf = NEW_VEC(char, strlen(cfuns[j].name) + libNameLen);
	    sprintf (nameBuf, "%s.%s", clib->libName, cfuns[j].name);
#ifdef INDIRECT_CFUNC
	    RecordCSymbol (nameBuf, PTR_CtoLib7(&(cfuns[j])));
#else
	    RecordCSymbol (nameBuf, PTR_CtoLib7(cfuns[j].cfunc));
#endif
	}
    }

} /* end of InitCFunList */

/* BindCFun:
 *
 * Search the C function table for the given function; return LIB7_void, if
 * not found.
 * NOTE: eventually, we will raise an exception when the function isn't found.
 */
lib7_val_t BindCFun (char *moduleName, char *funName)
{
    int		i, j;

/* SayDebug("BIND: %s.%s\n", moduleName, funName); */
    for (i = 0;  CLibs[i] != NULL;  i++) {
	if (strcmp(CLibs[i]->libName, moduleName) == 0) {
	    cfunc_naming_t	*cfuns = CLibs[i]->cfuns;
	    for (j = 0;  cfuns[j].name != NULL;  j++) {
		if (strcmp(cfuns[j].name, funName) == 0)
#ifdef INDIRECT_CFUNC
		    return PTR_CtoLib7(&(cfuns[j]));
#else
		    return PTR_CtoLib7(cfuns[j].cfunc);
#endif
	    }
	  /* here, we didn't find the library so we return LIB7_void */
	    return LIB7_void;
	}
    }

  /* here, we didn't find the library so we return LIB7_void */
    return LIB7_void;

} /* end of BindCFun */


/*
 * COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 */
