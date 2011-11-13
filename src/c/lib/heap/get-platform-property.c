// get-platform-property.c
//
// General interface to query system properties.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "architecture-and-os-names-system-dependent.h"

#if defined(OPSYS_UNIX)
    #include "system-dependent-unix-stuff.h"  		// For OS_NAME.
#elif defined(OPSYS_WIN32)
    #define OS_NAME "Win32"
#endif

#define SAME_STRING(s1, s2)	(strcmp((s1), (s2)) == 0)


#define FALSE_VALUE	"FALSE"
#define TRUE_VALUE	"TRUE"


Val   _lib7_runtime_get_platform_property   (Task* task,  Val arg)   {
    //===================================
    //
    //
    //
    //
    // Mythryl type:   String -> Null_Or(String)
    //
    //
    //
    // Current queries:
    //
    //     "OS_NAME"     -> "Linux"/"BSD"/"Cygwin" /"SunOS"/"Solaris"/"Irix"/"OSF/1"/"AIX"/"Darwin"/"HPUX"
    //     "OS_VERSION"  -> "<unknown>"
    //
    //     "HOST_ARCH"   -> "INTEL32"/"PWRPC32"/"SPARC32"/"<unknown>"
    //     "TARGET_ARCH" -> "INTEL32"/"PWRPC32"/"SPARC32"/"<unknown>"
    //
    //     "HAS_SOFTWARE_GENERATED_PERIODIC_EVENTS" -> "YES" / "NO"
    //     "HAS_MP"                                 -> "YES" / "NO"
    //
    //
    //
    // Returns:   THE <string>   for a valid query;
    //	          NULL           for an invalid one.
    //
    //
    // This fn gets bound as   get_platform_property   in:
    //
    //     src/lib/std/src/nj/platform-properties.pkg

    char* name = HEAP_STRING_AS_C_STRING(arg);

    Val	  result;

    if (SAME_STRING("OS_NAME", name))
	result = make_ascii_string_from_c_string(task, OS_NAME);
    else if (SAME_STRING("OS_VERSION", name))
	result = make_ascii_string_from_c_string(task, "<unknown>");
    else if (SAME_STRING("HOST_ARCH", name))
#if defined(HOST_PWRPC32)
	result = make_ascii_string_from_c_string(task, "PWRPC32");
#elif defined(HOST_SPARC32)
	result = make_ascii_string_from_c_string(task, "SPARC32");
#elif defined(HOST_INTEL32)
	result = make_ascii_string_from_c_string(task, "INTEL32");
#else
	result = make_ascii_string_from_c_string(task, "<unknown>");
#endif
    else if (SAME_STRING("TARGET_ARCH", name))
#if defined(TARGET_PWRPC32)
	result = make_ascii_string_from_c_string(task, "PWRPC32");
#elif defined(TARGET_SPARC32)
	result = make_ascii_string_from_c_string(task, "SPARC32");
#elif defined(TARGET_INTEL32)
	result = make_ascii_string_from_c_string(task, "INTEL32");
#else
	result = make_ascii_string_from_c_string(task, "<unknown>");
#endif
    else if (SAME_STRING("HAS_SOFTWARE_GENERATED_PERIODIC_EVENTS", name))
#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	result = make_ascii_string_from_c_string(task, TRUE_VALUE);
#else
	result = make_ascii_string_from_c_string(task, FALSE_VALUE);
#endif
    else if (SAME_STRING("HAS_MP", name))

#if NEED_PTHREAD_SUPPORT
	result = make_ascii_string_from_c_string(task, TRUE_VALUE);
#else
	result = make_ascii_string_from_c_string(task, FALSE_VALUE);
#endif
    else
	return OPTION_NULL;

    OPTION_THE(task, result, result);

    return result;
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


