/* pathconf.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

 /* The following table is generated from all _PC_ values
  * in unistd.h. For most systems, this will include
    _PC_CHOWN_RESTRICTED
    _PC_LINK_MAX
    _PC_MAX_CANON
    _PC_MAX_INPUT
    _PC_NAME_MAX
    _PC_NO_TRUNC
    _PC_PATH_MAX
    _PC_PIPE_BUF
    _PC_VDISABLE
  *
  * The full POSIX list is given in section 5.7.1 of Std 1003.1b-1993.
  *
  * The Lib7L string used to look up these values has the same
  * form but without the prefix, e.g., to lookup _PC_LINK_MAX,
  * use pathconf (path, "LINK_MAX")
  */
static name_val_t values[] = {
#include "ml_pathconf.h"
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))

/* mkValue
 *
 * Convert return value from (f)pathconf to Lib7 value.
 */
static lib7_val_t mkValue (lib7_state_t *lib7_state, int val)
{
    lib7_val_t    p, chunk;

    if (val >= 0) {
	WORD_ALLOC (lib7_state, p, val);
	OPTION_SOME(lib7_state, chunk, p);

    } else if (errno == 0) {
	chunk = OPTION_NONE;
    } else {
        chunk = RAISE_SYSERR(lib7_state, val);
    }

    return chunk;

}  /* end of mkValue */

/* _lib7_P_FileSys_pathconf : String * String -> word option
 *                          filename attribute
 *
 * Get configurable pathname attribute given pathname
 */
lib7_val_t _lib7_P_FileSys_pathconf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		val;

    lib7_val_t	mlPathname = REC_SEL(arg, 0);
    lib7_val_t	mlAttr     = REC_SEL(arg, 1);

    char	*pathname = STR_LIB7toC(mlPathname);
    name_val_t	*attribute;

    attribute = _lib7_posix_nv_lookup (STR_LIB7toC(mlAttr), values, NUMELMS);

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR(lib7_state, -1);
    }
 
    errno = 0;
    while (((val = pathconf (pathname, attribute->val)) == -1) && (errno == EINTR)) {
        errno = 0;
        continue;
    }

    return (mkValue (lib7_state, val));

} /* end of _lib7_P_FileSys_pathconf */

/* _lib7_P_FileSys_fpathconf : int * String -> word option
 *                           fd     attribute
 *
 * Get configurable pathname attribute given pathname
 */
lib7_val_t _lib7_P_FileSys_fpathconf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		val;

    int         fd  = REC_SELINT(arg, 0);
    lib7_val_t	mlAttr = REC_SEL(arg, 1);

    name_val_t  *attribute;

    attribute = _lib7_posix_nv_lookup (STR_LIB7toC(mlAttr), values, NUMELMS);

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR(lib7_state, -1);
    }
 
    errno = 0;
    while (((val = fpathconf (fd, attribute->val)) == -1) && (errno == EINTR)) {
        errno = 0;
        continue;
    }

    return mkValue (lib7_state, val);
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
