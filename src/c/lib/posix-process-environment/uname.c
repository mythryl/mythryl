// uname.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_uname");

    struct utsname      name;

    Val  p;
    Val  s;
    Val  field;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_uname", NULL );
	//
	int status =  uname( &name );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_uname" );

    if (status == -1)    return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

		    // NOTE: We should do something about possible cleaning. XXX BUGGO FIXME

    Val l =  LIST_NIL;											Roots extra_roots1 = { &l, NULL };

    field =  make_ascii_string_from_c_string__may_heapclean(task, "machine",    &extra_roots1 );	Roots extra_roots2 = { &field, &extra_roots1 };
    s     =  make_ascii_string_from_c_string__may_heapclean(task, name.machine, &extra_roots2 );	Roots extra_roots3 = { &s,     &extra_roots2 };
    p     =  make_two_slot_record(task, field, s); 
    l     =  LIST_CONS(task, p, l);

    field =  make_ascii_string_from_c_string__may_heapclean(task, "version",	&extra_roots3 );
    s     =  make_ascii_string_from_c_string__may_heapclean(task, name.version,	&extra_roots3 );
    p     =  make_two_slot_record(task, field, s); 
    l     =  LIST_CONS(task, p, l);

    field =  make_ascii_string_from_c_string__may_heapclean(task, "release",	&extra_roots3 );
    s     =  make_ascii_string_from_c_string__may_heapclean(task, name.release,	&extra_roots3 );
    p     =  make_two_slot_record(task, field, s); 
    l     =  LIST_CONS(task, p, l);

    field =  make_ascii_string_from_c_string__may_heapclean(task, "nodename",	&extra_roots3 );
    s     =  make_ascii_string_from_c_string__may_heapclean(task, name.nodename,&extra_roots3 );
    p     =  make_two_slot_record(task, field, s); 
    l     =  LIST_CONS(task, p, l);

    field =  make_ascii_string_from_c_string__may_heapclean(task, "sysname",	&extra_roots3 );
    s     =  make_ascii_string_from_c_string__may_heapclean(task, name.sysname,	&extra_roots3 );
    p     =  make_two_slot_record(task, field, s); 
    l     =  LIST_CONS(task, p, l);

    return l;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

