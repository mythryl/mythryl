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
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    struct utsname      name;

    Val  p;
    Val  s;
    Val  field;

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int status =  uname( &name );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (status == -1)    return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

		    // NOTE: We should do something about possible heapcleaning in the middle here. See src/c/lib/posix-os/select.c for a model. XXX BUGGO FIXME

    Val result =  LIST_NIL;										Roots extra_roots1 = { &result, NULL };

    field  =  make_ascii_string_from_c_string__may_heapclean(task, "machine",    &extra_roots1 );	Roots extra_roots2 = { &field, &extra_roots1 };
    s      =  make_ascii_string_from_c_string__may_heapclean(task, name.machine, &extra_roots2 );	Roots extra_roots3 = { &s,     &extra_roots2 };
    p      =  make_two_slot_record(task, field, s); 
    result =  LIST_CONS(task, p, result);

    field  =  make_ascii_string_from_c_string__may_heapclean(task, "version",	&extra_roots3 );
    s      =  make_ascii_string_from_c_string__may_heapclean(task, name.version,	&extra_roots3 );
    p      =  make_two_slot_record(task, field, s); 
    result =  LIST_CONS(task, p, result);

    field  =  make_ascii_string_from_c_string__may_heapclean(task, "release",	&extra_roots3 );
    s      =  make_ascii_string_from_c_string__may_heapclean(task, name.release,	&extra_roots3 );
    p      =  make_two_slot_record(task, field, s); 
    result =  LIST_CONS(task, p, result);

    field  =  make_ascii_string_from_c_string__may_heapclean(task, "nodename",	&extra_roots3 );
    s      =  make_ascii_string_from_c_string__may_heapclean(task, name.nodename,&extra_roots3 );
    p      =  make_two_slot_record(task, field, s); 
    result =  LIST_CONS(task, p, result);

    field  =  make_ascii_string_from_c_string__may_heapclean(task, "sysname",	&extra_roots3 );
    s      =  make_ascii_string_from_c_string__may_heapclean(task, name.sysname,	&extra_roots3 );
    p      =  make_two_slot_record(task, field, s); 
    result =  LIST_CONS(task, p, result);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

