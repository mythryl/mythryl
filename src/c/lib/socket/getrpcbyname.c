// getrpcbyname.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "system-dependent-unix-stuff.h"
#include "sockets-osdep.h"
#include <netdb.h>
#ifdef INCLUDE_RPCENT_H
#  include INCLUDE_RPCENT_H
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_NetDB_getrpcbyname   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:  String -> Null_Or(   (String, List(String), Int)   )
    //
    // 
    // This fn is NOWHERE INVOKED.  Nor listed in   src/c/lib/socket/cfun-list.h   Presumably should be either called or deleted:  XXX SUCKO FIXME.

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_NetDB_getrpcbyname");

    struct rpcent*   rentry;

    char* heap_name = HEAP_STRING_AS_C_STRING( arg );

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_name into C storage: 
    //
    Mythryl_Heap_Value_Buffer  name_buf;
    //
    {   char* c_name =  buffer_mythryl_heap_value( &name_buf, (void*) heap_name, strlen( heap_name ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_NetDB_getrpcbyname", arg );
	    //
	    rentry =  getrpcbyname( c_name );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_NetDB_getrpcbyname" );

	unbuffer_mythryl_heap_value( &name_buf );
    }

    if (rentry == NULL)   return OPTION_NULL;

    Val name    =  make_ascii_string_from_c_string__may_heapclean(		task,  rentry->r_name,	NULL );						Roots extra_roots = { &name, NULL };
    Val aliases =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task,  rentry->r_aliases /*, &extra_roots */);
    Val	result  =  make_three_slot_record(					task,   name, aliases, TAGGED_INT_FROM_C_INT(rentry->r_number)  );

    return OPTION_THE( task, result );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

