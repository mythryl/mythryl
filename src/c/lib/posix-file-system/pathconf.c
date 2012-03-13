// pathconf.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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

    if (val >= 0) {
	//
	p =  make_one_word_unt(task, val );
	//
	return OPTION_THE( task, p );

    } else if (errno == 0) {
	
	return OPTION_NULL;

    } else {

	return RAISE_SYSERR__MAY_HEAPCLEAN(task, val, NULL);
    }
}


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



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
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_pathconf");

    int		val;

    Val	mlPathname = GET_TUPLE_SLOT_AS_VAL( arg, 0);
    Val	mlAttr     = GET_TUPLE_SLOT_AS_VAL( arg, 1);

    char* heap_pathname = HEAP_STRING_AS_C_STRING( mlPathname );

    name_val_t*	 attribute = _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(mlAttr), values, NUMELMS);

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
    }
 
    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  pathname_buf;
    //
    {	char* c_pathname
	    = 
	    buffer_mythryl_heap_value( &pathname_buf, (void*) heap_pathname, strlen( heap_pathname ) +1 );		// '+1' for terminal NUL on string.


	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_pathconf", NULL );
	    //
	    errno = 0;
	    while (((val = pathconf (c_pathname, attribute->val)) == -1) && (errno == EINTR)) {
		errno = 0;
		continue;
	    }
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_pathconf" );

	unbuffer_mythryl_heap_value( &pathname_buf );
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
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_fpathconf");

    int	val;

    int fd     =  GET_TUPLE_SLOT_AS_INT( arg, 0);
    Val	mlAttr =  GET_TUPLE_SLOT_AS_VAL( arg, 1);

    name_val_t*  attribute =  _lib7_posix_nv_lookup( HEAP_STRING_AS_C_STRING(mlAttr), values, NUMELMS );

    if (!attribute) {
	errno = EINVAL;
	return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
    }
 
    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fpathconf", NULL );
	//
	errno = 0;
	while (((val = fpathconf (fd, attribute->val)) == -1) && (errno == EINTR)) {
	    errno = 0;
	    continue;
	}
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fpathconf" );

    return mkValue (task, val);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

