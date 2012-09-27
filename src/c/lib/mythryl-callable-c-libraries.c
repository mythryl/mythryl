// mythryl-callable-c-libraries.c
//
// This is the home of the CLibrary table, C library initialization code,
// and C function lookup code.  It is part of the run-time proper, rather
// than part of the libmythryl-*.a libraries.
//
//
//
// Problem
// =======
//
// There are various C library functions which we
// want to call from Mythryl code.
//
//
// Solution
// ========
//
// We maintain a global table of all such functions
// by name and then provide a mechanism whereby the
// Mythryl code can request their addresses by name,
// more precisely by name of library plus name of
// function, for example:
//
//     my  change_directory:  String -> Void
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_filesys", fun_name => "chdir" };
//
// -- see   src/lib/std/src/psx/posix-file.pkg  
//
// Our
//
//     find_mythryl_callable_c_function ()
//
// in this file is the core lookup function.
//
// This leaves an obvious bootstrap problem: If we have
// to call a C function to get a C function, how do we
// get the initial C function?
// 
// We solve this with a genial little kludge:  The package
// 
//     runtime		from    src/lib/core/init/runtime.pkg
//
// looks like a vanilla Mythryl package, but at linktime
//
//     src/c/main/load-compiledfiles.c
//
// replaces calls to the (nonexistent) runtime::asm Mythryl functions
// with calls to the corresponding functions in
//
//     src/c/machine-dependent/prim.intel32.asm
//     src/c/machine-dependent/prim.sparc32.asm
//     src/c/machine-dependent/prim.pwrpc32.asm
//     src/c/machine-dependent/prim.intel32.masm
//     
// (depending upon the host platform).  In particular this gives us
// Mythryl-level access to the critical find_mythryl_callable_c_function () function:
//
// it is invoked by     find_cfun    		in    src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg
// via                  find_cfun_asm		in    src/c/machine-dependent/prim.intel32.asm  (or similar)
// and then             REQUEST_FIND_CFUN	in    src/c/main/run-mythryl-code-and-runtime-eventloop.c
//
// find_cfun_asm itself is ultimately accessed via
// via   RunVec   and   CStruct  stored in
//
//     runtime_package__global 
//
// by  src/c/main/construct-runtime-package.c
//
// and then made available via a special hack in
//
//     src/c/main/load-compiledfiles.c
//
//
// Our tables are constructed by such files as:
//
//     src/c/lib/space-and-time-profiling/cfun-list.h
//     src/c/lib/posix-passwd/cfun-list.h
//     src/c/lib/math/cfun-list.h
//     src/c/lib/date/cfun-list.h
//     src/c/lib/dynamic-loading/cfun-list.h
//     src/c/lib/hostthread/cfun-list.h
//     src/c/lib/win32-file-system/cfun-list.h
//     src/c/lib/gtk/cfun-list.h
//     src/c/lib/posix-io/cfun-list.h
//     src/c/lib/posix-file-system/cfun-list.h
//     src/c/lib/win32-io/cfun-list.h
//     src/c/lib/templates/cfun-list.h
//     src/c/lib/win32/cfun-list.h
//     src/c/lib/time/cfun-list.h
//     src/c/lib/posix-error/cfun-list.h
//     src/c/lib/posix-signal/cfun-list.h
//     src/c/lib/socket/cfun-list.h
//     src/c/lib/signal/cfun-list.h
//     src/c/lib/posix-tty/cfun-list.h
//     src/c/lib/posix-process-environment/cfun-list.h
//     src/c/lib/opencv/cfun-list.h
//     src/c/lib/posix-process/cfun-list.h
//     src/c/lib/win32-process/cfun-list.h
//     src/c/lib/posix-os/cfun-list.h
//     src/c/lib/ncurses/cfun-list.h
//     src/c/lib/heap/cfun-list.h
//     src/c/lib/ccalls/cfun-list.h
//
// which get collected into the central index
//
//     src/c/lib/mythryl-callable-c-libraries-list.h
//
//

#include "../mythryl-config.h"

#include <stdio.h>

#ifdef OPSYS_UNIX
#  include "system-dependent-unix-stuff.h"		// For the HAS_POSIX_LIBRARIES option flag.
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "mythryl-callable-c-libraries.h"
#include "mythryl-callable-cfun-hashtable.h"

#define MYTHRYL_CALLABLE_C_LIBRARY(lib)  extern Mythryl_Callable_C_Library lib;						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
#include "mythryl-callable-c-libraries-list.h"
#undef MYTHRYL_CALLABLE_C_LIBRARY

static Mythryl_Callable_C_Library*   mythryl_callable_c_libraries__local   [] = {
	//                           ==================================
        //
	#define MYTHRYL_CALLABLE_C_LIBRARY(lib)	&lib,
	    #include "mythryl-callable-c-libraries-list.h"				// src/c/lib/mythryl-callable-c-libraries-list.h
	#undef MYTHRYL_CALLABLE_C_LIBRARY
	//
	NULL
    };

void   set_up_list_of_c_functions_callable_from_mythryl   () {
    // ================================================
    // 
    // We get called (only) once, early in   main ()   from
    //     src/c/main/runtime-main.c
    //
    int   i, j;
    int   library_name_bytesize;
    char* name;
char* nickname;	// See Hashtable_Entry comment in src/c/heapcleaner/mythryl-callable-cfun-hashtable.c

    for (i = 0;  mythryl_callable_c_libraries__local[i] != NULL;  i++) {
      //
	Mythryl_Callable_C_Library*	clib  =  mythryl_callable_c_libraries__local[i];
	//
	Mythryl_Name_With_C_Function*	cfuns =  mythryl_callable_c_libraries__local[i]->vector_of_mythryl_names_and_c_functions;

	if (   clib->initialize_mythryl_callable_c_library != NULL) {
	    (*(clib->initialize_mythryl_callable_c_library)) (0, 0 /* argc, argv */);
	}

        // Register the C functions in the C symbol table:
        //
	library_name_bytesize = strlen(clib->library_name);
        //
	for (j = 0;  cfuns[j].name != NULL;  j++) {
	    //
	    name     =  MALLOC_VEC(char, strlen(cfuns[j].name    ) + library_name_bytesize + 3);					// "+3" to include space for medial "::" and terminal '\0'
            nickname =  MALLOC_VEC(char, strlen(cfuns[j].nickname) + library_name_bytesize + 3);					// "+3" to include space for medial "::" and terminal '\0'
	    //
	    sprintf (name,     "%s::%s", clib->library_name, cfuns[j].name    );
            sprintf (nickname, "%s::%s", clib->library_name, cfuns[j].nickname);
//          sprintf (nickname, "%s.%s", clib->library_name, cfuns[j].nickname);

	    #ifdef INDIRECT_CFUNC									// Defined (only) on darwin and win32.
	        publish_cfun( name, PTR_CAST( Val, &(cfuns[j]))	);					// publish_cfun		def in   src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
	    #else
//		publish_cfun( name, PTR_CAST( Val, cfuns[j].cfunc)	);
		publish_cfun2( name, nickname, PTR_CAST( Val, cfuns[j].cfunc)	);
	    #endif
	}
    }

}								// fun set_up_list_of_c_functions_callable_from_mythryl



Val   find_mythryl_callable_c_function  (char* library_name, char* function_name) {
    //===============================
    //
    // Return HEAP_VOID if the named function is not found.
    //
    // NOTE:  Eventually, we will raise an exception when the function isn't found.	XXX BUGGO FIXME

    // debug_say("BIND: %s.%s\n", library_name, function_name);

    for (int i = 0;  mythryl_callable_c_libraries__local[i] != NULL;  i++) {
	//
	if (!strcmp(mythryl_callable_c_libraries__local[i]->library_name,  library_name)){
	    //
	    Mythryl_Name_With_C_Function*   cfuns
		=
		mythryl_callable_c_libraries__local[i]->vector_of_mythryl_names_and_c_functions;

	    for (int j = 0;  cfuns[j].name != NULL;  j++) {
		//
		if (!strcmp(cfuns[j].name, function_name)) {
		    //
		    #ifdef INDIRECT_CFUNC
			return PTR_CAST( Val,  &(cfuns[j])    );
		    #else
			return PTR_CAST( Val,  cfuns[j].cfunc );
		    #endif
		}
if (!strcmp(cfuns[j].nickname, function_name)) {
    //
    #ifdef INDIRECT_CFUNC
	return PTR_CAST( Val,  &(cfuns[j])    );
    #else
	return PTR_CAST( Val,  cfuns[j].cfunc );
    #endif
}
	    }


	    return HEAP_VOID;			// Didn't find the library.
	}
    }

    return HEAP_VOID;				// Didn't find the library.
}						// fun find_mythryl_callable_c_function



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

