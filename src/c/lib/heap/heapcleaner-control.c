// heapcleaner-control.c
//
// General interface for heapcleaner control functions.
//

#include "../../mythryl-config.h"

#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "get-multipage-ram-region-from-os.h"
#include "heap.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

#define STREQ(s1, s2)	(strcmp((s1), HEAP_STRING_AS_C_STRING(s2)) == 0)

static void   set_max_retained_idle_fromspace_agegroup      (Task* task,  Val  cell);
static void   clean_i_agegroups     (Task* task,  Val  cell, Val* next);
static void   clean_all_agegroups   (Task* task,             Val* next);


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_cleaner_control   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   List( (String, Ref(Int))) -> Void
    //
    // Current control operations:
    //
    //   ("set_max_retained_idle_fromspace_agegroup", ref n)	- Set max retained-idle-fromspace agegroup to n; return old agegroup.
    //   ("DoGC", ref n)	- Clean the first "n" agegroups.
    //   ("AllGC", _)		- Clean all agegroups.
    //   ("Messages", ref 0)	- Turn cleaner messages on.
    //   ("Messages", ref n)	- Turn cleaner messages off. (n > 0)
    //
    // This fn gets bound to   cleaner_control   in:
    //
    //     src/lib/std/src/nj/heapcleaner-control.pkg


    while (arg != LIST_NIL) {
      //
	Val	cmd = LIST_HEAD(arg);
	Val	op = GET_TUPLE_SLOT_AS_VAL(cmd, 0);
	Val	cell = GET_TUPLE_SLOT_AS_VAL(cmd, 1);

	arg = LIST_TAIL(arg);

	if      (STREQ("DoGC",  op))	    clean_i_agegroups   (task, cell, &arg);
	else if (STREQ("AllGC", op))	    clean_all_agegroups (task, &arg);
        //
	else if (STREQ("Messages",  op))   cleaner_messages_are_enabled__global = (TAGGED_INT_TO_C_INT(DEREF(cell)) > 0);
	else if (STREQ("LimitHeap", op))   unlimited_heap_is_enabled__global       = (TAGGED_INT_TO_C_INT(DEREF(cell)) <= 0);
        //
        else if (STREQ("set_max_retained_idle_fromspace_agegroup", op))	    set_max_retained_idle_fromspace_agegroup (task, cell);
    }

    return HEAP_VOID;
}


static void   set_max_retained_idle_fromspace_agegroup   (
    //        ========================================
    Task*   task,
    Val     arg
) {
    int age =  TAGGED_INT_TO_C_INT(DEREF( arg ));

    Heap*  heap  =  task->heap;

    if      (age < 0)			age = 0;
    else if (age > MAX_AGEGROUPS)	age = MAX_AGEGROUPS;

    if (age < heap->oldest_agegroup_keeping_idle_fromspace_buffers) {
	//
        // Free any retained memory regions:
        //
	for (int i = age;  i < heap->oldest_agegroup_keeping_idle_fromspace_buffers;  i++) {
	    //
	    return_multipage_ram_region_to_os( heap->agegroup[i]->saved_fromspace_ram_region );
	}
    }

    ASSIGN( arg, TAGGED_INT_FROM_C_INT(heap->oldest_agegroup_keeping_idle_fromspace_buffers) );

    heap->oldest_agegroup_keeping_idle_fromspace_buffers
	=
        age;
}


static void   clean_i_agegroups   (
    //        =================
    //
    Task*   task,
    Val      arg,
    Val*     next
) {
    // Force a cleaning of the given agegroups.

    Heap* heap  =  task->heap;

    int   age =  TAGGED_INT_TO_C_INT( DEREF( arg ) );

    // Clamp 'age' to sane range:
    //
    if      (age < 0)			        age =  0;
    else if (age > heap->active_agegroups)	age =  heap->active_agegroups;

    call_heapcleaner_with_extra_roots( task, age, next, NULL );				// call_heapcleaner_with_extra_roots		def in   src/c/heapcleaner/call-heapcleaner.c
}



static void   clean_all_agegroups   (
    //        ===================
    Task*   task,
    Val*     next
) {
    //
  call_heapcleaner_with_extra_roots(   task,							// call_heapcleaner_with_extra_roots		def in   src/c/heapcleaner/call-heapcleaner.c
                                   task->heap->active_agegroups,
                                   next,
                                   NULL
                               );
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


