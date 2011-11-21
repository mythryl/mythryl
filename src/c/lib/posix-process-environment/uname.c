// uname.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_uname   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Void -> List(  (String, String)  )
    //
    // Return names of current system.
    //
    // This fn gets bound as   ctermid   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    struct utsname      name;

    Val  l, p, s;
    Val  field;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_uname", arg );
	//
	int status =  uname( &name );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_uname" );

    if (status == -1)    RAISE_SYSERR(task, status);

		    // NOTE: We should do something about possible cleaning. XXX BUGGO FIXME

    l = LIST_NIL;

    field = make_ascii_string_from_c_string(task, "machine");
    s = make_ascii_string_from_c_string(task, name.machine);
    REC_ALLOC2(task, p, field, s);
    LIST_CONS(task, l, p, l);

    field = make_ascii_string_from_c_string(task, "version");
    s = make_ascii_string_from_c_string(task, name.version);
    REC_ALLOC2(task, p, field, s);
    LIST_CONS(task, l, p, l);

    field = make_ascii_string_from_c_string(task, "release");
    s = make_ascii_string_from_c_string(task, name.release);
    REC_ALLOC2(task, p, field, s);
    LIST_CONS(task, l, p, l);

    field = make_ascii_string_from_c_string(task, "nodename");
    s = make_ascii_string_from_c_string(task, name.nodename);
    REC_ALLOC2(task, p, field, s);
    LIST_CONS(task, l, p, l);

    field = make_ascii_string_from_c_string(task, "sysname");
    s = make_ascii_string_from_c_string(task, name.sysname);
    REC_ALLOC2(task, p, field, s);
    LIST_CONS(task, l, p, l);

    return l;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

