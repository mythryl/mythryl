// libmythryl-etc.c

// This file defines the "kludge" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_script_name:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "kludge", fun_name => "get_script_name" };
// 
// or such.
//
// The functions we export here get used in
//

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"

#include "architecture-and-os-names-system-dependent.h"

#include "flush-instruction-cache-system-dependent.h"
#include "get-quire-from-os.h"

#include "heap.h"
#include "heapcleaner.h"
#include "heap-io.h"

#include "raise-error.h"
#include "mythryl-callable-c-libraries.h"
#include "make-strings-and-vectors-etc.h"

#if defined(OPSYS_UNIX)
    #include "system-dependent-unix-stuff.h"  		// For OS_NAME.
#elif defined(OPSYS_WIN32)
    #define OS_NAME "Win32"
#endif


#define FALSE_VALUE	"FALSE"
#define TRUE_VALUE	"TRUE"

//
//
static Val   do_get_script_name   (Task* task,  Val arg) {
    //       ==================
    //
    // Mythryl type:   Void -> Null_Or( String )
    //
    // If MYTHRYL_SCRIPT was set in the Posix "environment"
    // when the Mythryl C runtime started up, this call will
    // return its string value, otherwise NULL.
    //
    // The C runtime removes MYTHRYL_SCRIPT from the environment
    // immediately after checking for it (and caching its value)
    // because if it is left in the environment and then inherited
    // by a spawned subprocess it can cause totally unexpected
    // behavior in violent violation of the Principle of Least
    // Surprise. (Hue White encountered this.) 
    //
    // The MYTHRYL_SCRIPT thing remains an unholy kludge, but
    // this at least minimizes the kludges window of opportunity
    // to cause mayhem.
    //
    // This fn gets bound to 'get_script_name' in:
    //
    //     src/lib/src/kludge.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    if (!mythryl_script__global) {						// mythryl_script__global	is from   src/c/main/runtime-main.c
	//
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

	return  OPTION_NULL;							// OPTION_NULL			is from   src/c/h/make-strings-and-vectors-etc.h
    }

    Val script_name
	=
	make_ascii_string_from_c_string__may_heapclean( task, mythryl_script__global, NULL );

    Val result = OPTION_THE(task, script_name);
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
//
static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    {"get_script_name",	"get_script_name",	do_get_script_name,	"Void -> Null_Or(String)"	},
    //
    CFUNC_NULL_BIND
};
#undef CFUNC


// The Runtime library.
//
// Our record                Libmythryl_Kludge
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Kludge = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              =================
    "kludge",
    "1.0",
    "February 15, 2012",
    NULL,
    CFunTable
};



// Jeff Prothero Copyright (c) 2012,
// released under Gnu Public Licence version 3.






/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/
