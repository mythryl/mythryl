/* sysinfo.c
 *
 * General interface to query system properties.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "machine-id.h"

#if defined(OPSYS_UNIX)
#  include "runtime-unixdep.h"  /* for OS_NAME */
#elif defined(OPSYS_WIN32)
#  define OS_NAME "Win32"
#endif

#define STREQ(s1, s2)	(strcmp((s1), (s2)) == 0)


#define FALSE_VALUE	"NO"
#define TRUE_VALUE	"YES"


lib7_val_t   _lib7_runtime_sysinfo   (   lib7_state_t*   lib7_state,
                                           lib7_val_t      arg
                                       )
{
    /* _lib7_runtime_sysinfo : String -> String option
     *
     * Current queries:
     *   "OS_NAME"
     *   "OS_VERSION"
     *   "HOST_ARCH"   
     *   "TARGET_ARCH"
     *   "HAS_SOFT_POLL"
     *   "HAS_MP"
     */

    char	*name = STR_LIB7toC(arg);
    lib7_val_t	result;

    if (STREQ("OS_NAME", name))
	result = LIB7_CString(lib7_state, OS_NAME);
    else if (STREQ("OS_VERSION", name))
	result = LIB7_CString(lib7_state, "<unknown>");
    else if (STREQ("HOST_ARCH", name))
#if defined(HOST_M68)
	result = LIB7_CString(lib7_state, "M68");
#elif defined(HOST_PPC)
	result = LIB7_CString(lib7_state, "PPC");
#elif defined(HOST_RS6000)
	result = LIB7_CString(lib7_state, "RS6000");
#elif defined(HOST_SPARC)
	result = LIB7_CString(lib7_state, "SPARC");
#elif defined(HOST_X86)
	result = LIB7_CString(lib7_state, "X86");
#else
	result = LIB7_CString(lib7_state, "<unknown>");
#endif
    else if (STREQ("TARGET_ARCH", name))
#if defined(TARGET_M68)
	result = LIB7_CString(lib7_state, "M68");
#elif defined(TARGET_PPC)
	result = LIB7_CString(lib7_state, "PPC");
#elif defined(TARGET_RS6000)
	result = LIB7_CString(lib7_state, "RS6000");
#elif defined(TARGET_SPARC)
	result = LIB7_CString(lib7_state, "SPARC");
#elif defined(TARGET_X86)
	result = LIB7_CString(lib7_state, "X86");
#elif defined(TARGET_C)
	result = LIB7_CString(lib7_state, "C");
#elif defined(TARGET_BYTECODE)
	result = LIB7_CString(lib7_state, "BYTECODE");
#else
	result = LIB7_CString(lib7_state, "<unknown>");
#endif
    else if (STREQ("HAS_SOFT_POLL", name))
#ifdef SOFT_POLL
	result = LIB7_CString(lib7_state, TRUE_VALUE);
#else
	result = LIB7_CString(lib7_state, FALSE_VALUE);
#endif
    else if (STREQ("HAS_MP", name))
#ifdef MP_SUPPORT
	result = LIB7_CString(lib7_state, TRUE_VALUE);
#else
	result = LIB7_CString(lib7_state, FALSE_VALUE);
#endif
    else
	return OPTION_NONE;

    OPTION_SOME(lib7_state, result, result);

    return result;
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

