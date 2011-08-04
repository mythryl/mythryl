// pathconf.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

// The following table is generated from all _PC_ values
// in unistd.h. For most systems, this will include
//  _PC_CHOWN_RESTRICTED
//  _PC_LINK_MAX
//  _PC_MAX_CANON
//  _PC_MAX_INPUT
//  _PC_NAME_MAX
//  _PC_NO_TRUNC
//  _PC_PATH_MAX
//  _PC_PIPE_BUF
//  _PC_VDISABLE
//
// The full POSIX list is given in section 5.7.1 of Std 1003.1b-1993.
//
// The Lib7L string used to look up these values has the same
// form but without the prefix, e.g., to lookup _PC_LINK_MAX,
// use pathconf (path, "LINK_MAX")
//
static name_val_t   values[]   = {
    //
    #include "ml_pathconf.h"
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))



static Val   mkValue   (Task* task,  int val)   {
    //       =======
    //
    // Mythryl type:
    //
    // Convert return value from (f)pathconf to Mythryl value.
    //
    Val p;
    Val chunk;

    if (val >= 0) {
	//
	WORD_ALLOC (task, p, val);
	OPTION_THE(task, chunk, p);

    } else if (errno == 0) {
	
	chunk = OPTION_NULL;

    } else {

        chunk = RAISE_SYSERR(task, val);
    }

    return chunk;
}


// One of the library bindings exported via
//     src/c/lib/posix-filesys/cfun-list.h
// and thence
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c



Val   _lib7_P_FileSys_pathconf   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:   (String, String) -> Null_Or(Unt)
    //                 filename attribute
    //
    // Get configurable pathname attribute given pathname
    //
    // This fn gets bound as   pathconf   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    int		val;

    Val	mlPathname = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	mlAttr     = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* pathname = HEAP_STRING_AS_C_STRING(mlPathname);

    name_val_t*	 attribute = _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(mlAttr), values, NUMELMS);

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR(task, -1);
    }
 
    errno = 0;
    while (((val = pathconf (pathname, attribute->val)) == -1) && (errno == EINTR)) {
        errno = 0;
        continue;
    }

    return  mkValue( task, val );
}


Val   _lib7_P_FileSys_fpathconf   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type:  (Int, String) -> Null_Or(Unt)
    //                 fd     attribute
    //
    // Get configurable pathname attribute given pathname
    //
    // This fn gets bound as   fpathconf'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg


    int		val;

    int         fd  = GET_TUPLE_SLOT_AS_INT(arg, 0);
    Val	mlAttr = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    name_val_t*  attribute =  _lib7_posix_nv_lookup( HEAP_STRING_AS_C_STRING(mlAttr), values, NUMELMS );

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR(task, -1);
    }
 
    errno = 0;
    while (((val = fpathconf (fd, attribute->val)) == -1) && (errno == EINTR)) {
        errno = 0;
        continue;
    }

    return mkValue (task, val);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

