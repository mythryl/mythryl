// runtime-exception-stuff.c

#include "../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "asm-to-c-request-codes.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "system-dependent-signal-stuff.h"
#include "mythryl-callable-c-libraries.h"
#include "profiler-call-counts.h"
#include "heapcleaner.h"

int foobar = 3;

void   raise_mythryl_exception   (Task* task,  Val exn)   {
    // =======================
    //
    // Modify the Task so that the given exception
    // will be raised when Mythryl is resumed.
    //

    ENTER_MYTHRYL_CALLABLE_C_FN("raise_mythryl_exception");

    Val	fate =  task->exception_fate;

    // We should have a macro defined in runtime-base.h for this.  XXX BUGGO FIXME

    task->argument	   =  exn;
    task->current_closure  =  fate;
    task->fate	   	   =  HEAP_VOID;

    task->program_counter =
    task->link_register	  =  GET_CODE_ADDRESS_FROM_CLOSURE( fate );
}

void   handle_uncaught_exception   (Val e)   {
    // =========================
    //
    // Handle an uncaught exception.
    //
    // We are invoked only from:
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c

    char buf[ 1024 ];

    Val	to_name   =  GET_TUPLE_SLOT_AS_VAL( e, 0 );
    Val	val       =  GET_TUPLE_SLOT_AS_VAL( e, 1 );
    Val	traceback =  GET_TUPLE_SLOT_AS_VAL( e, 2 );

    Val name      =  GET_TUPLE_SLOT_AS_VAL( to_name, 0 );


    if (IS_TAGGED_INT(val)) {
	sprintf (buf, "%ld\n", (long int) TAGGED_INT_TO_C_INT(val));
    } else {
	Val	desc = CHUNK_TAGWORD(val);
	if (desc == STRING_TAGWORD)
	    sprintf (buf, "\"%.*s\"", (int) GET_VECTOR_LENGTH(val), HEAP_STRING_AS_C_STRING(val));
	else
	    sprintf (buf, "<unknown>");
    }

    if (traceback != LIST_NIL) {
	//
        // Where was this exception was raised?
	//
	Val	next = traceback;
	do {
	    traceback = next;
	    next = LIST_TAIL( traceback );
	} while (next != LIST_NIL);

	val = LIST_HEAD( traceback );

	sprintf (buf+strlen(buf), " raised at %.*s", (int) GET_VECTOR_LENGTH(val), HEAP_STRING_AS_C_STRING(val));
    }

    die ("Uncaught exception %.*s with %s\n",	GET_VECTOR_LENGTH(name), GET_VECTOR_DATACHUNK_AS(char*, name), buf);

    print_stats_and_exit( 1 );

    exit(1);					// Just to quiet a gcc warning -- execution cannot reach this point.
}

// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

