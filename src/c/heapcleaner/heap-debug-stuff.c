// heap-debug-stuff.c
//
// Heap-centric debug-support code for Heisenbug hunting.

#include "../mythryl-config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "get-multipage-ram-region-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"



void   zero_agegroup0_overrun_tripwire_buffer( Task* task ) {
    // ==========================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we zero that out:
    //
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = 0;
             i < AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS;
             ++i
    ){
	//
	p[i] = 0;
    }
//  log_if("zero_agegroup0_overrun_tripwire_buffer: Done zeroing %x -> %x", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1));	// Commented out because it spams the logfile with gigabytes of text.
}

static char*  val_sized_unt_as_ascii(  char* buf,  Val_Sized_Unt u ) {
    //        ======================
    //
    char* p = buf;
    //
    for (int i = 0;  i < sizeof(Val_Sized_Unt);  ++i) {
	//
	char c =  u & 0xFF;
	u      =  u >> 8;
	*p++ = (c >= ' ' && c <= '~') ? c : '.';
    } 

    *p++ = '\0';

    return buf;
}

void   check_agegroup0_overrun_tripwire_buffer( Task* task, char* caller ) {
    // ==========================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we verify that it is all zeros:
    //
#ifndef SOON
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS; i --> 0; ) {
	//
	if (p[i] != 0) {
	    //
	    log_if("check_agegroup0_overrun_tripwire_buffer:  While checking %x -> %x agegroup0 buffer overrun of %d words detected at %s", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1), i, caller);
	    //
	    for (int j = 0;   j <= i;   ++j) {
		//
		char buf[ 132 ];
		log_if("check_agegroup0_overrun_tripwire_buffer: tripwire_buffer[%3d] x=%08x s='%s'", j, p[j], val_sized_unt_as_ascii(buf, (Val_Sized_Unt)(p[j])));
	    }
	    die( "check_agegroup0_overrun_tripwire_buffer:  Overran agegroup0 buffer by %d words -- see logfile for details.", i);
	    exit(1);										// die() should never return, so this should never execute. But gcc understands it better.
	}
    }
#endif
}

// Write to logfile contents of a Task record:
//
void log_task( Task* task, char* caller ) {
    //
    log_if("log_task:                     caller s=%s",  caller);
    log_if("log_task:                       task x=%p",  task);
    log_if("log_task:                       heap x=%x",  task->heap);
    log_if("log_task:                    pthread x=%x",  task->pthread);
    log_if("log_task:                pthread->id x=%x",  (unsigned int)(task->pthread->tid));
    log_if("log_task:    heap_allocation_pointer x=%x",  task->heap_allocation_pointer);
    log_if("log_task:      heap_allocation_limit x=%x",  task->heap_allocation_limit);
    log_if("log_task: real_heap_allocation_limit x=%x",  task->real_heap_allocation_limit);
    log_if("log_task:                   argument x=%x",  task->argument);
    log_if("log_task:                       fate x=%x",  task->fate);
    log_if("log_task:            current_closure x=%x",  task->current_closure);
    log_if("log_task:              link_register x=%x",  task->link_register);
    log_if("log_task:            program_counter x=%x",  task->program_counter);
    log_if("log_task:             exception_fate x=%x",  task->exception_fate);
    log_if("log_task:             current_thread x=%x",  task->current_thread);
    log_if("log_task:             heap_changelog x=%x",  task->heap_changelog);
    log_if("log_task:            fault_exception x=%x",  task->fault_exception);
    log_if("log_task:   faulting_program_counter x=%x",  task->faulting_program_counter);
    log_if("log_task:            protected_c_arg x=%x",  task->protected_c_arg);
    log_if("log_task:                  &heapvoid x=%x", &task->heapvoid);
    log_if("log_task: Following stuff is in task->heap:");
    log_if("log_task:           agegroup0_buffer x=%x",  task->heap->agegroup0_buffer);
    log_if("log_task:  agegroup0_buffer_bytesize x=%x",  task->heap->agegroup0_buffer_bytesize);
    log_if("log_task:           sum of above two x=%x",  (char*)(task->heap->agegroup0_buffer) + task->heap->agegroup0_buffer_bytesize);
    log_if("log_task:       multipage_ram_region x=%x",  task->heap->multipage_ram_region);
    log_if("log_task:           active_agegroups d=%d",  task->heap->active_agegroups);
    log_if("log_task: oldest_agegroup_keeping_idle_fromspace_buffers d=%d",  task->heap->oldest_agegroup_keeping_idle_fromspace_buffers);
    log_if("log_task:  hugechunk_ramregion_count d=%d",  task->heap->hugechunk_ramregion_count);
    log_if("log_task:      total_bytes_allocated x=(%x,%x) (millions, 1s)",  task->heap->total_bytes_allocated.millions, task->heap->total_bytes_allocated.ones );

    for (int i = 0; i < task->heap->active_agegroups; ++i) {
	//
	Agegroup* a = task->heap->agegroup[ i ];
	log_if("log_task:           agegroup[%d] x=%x (holds agegroup %d)",  i, a, i+1);
	log_if("log_task:           a->age       d=%d",  a->age);
	log_if("log_task:           a->cleanings d=%d",  a->cleanings);
	log_if("log_task:           a->ratio     x=%x (Desired number of collections of the previous agegroup for one collection of this agegroup)",  a->ratio);
	log_if("log_task:           a->last_cleaning_count_of_younger_agegroup d=%d",  a->last_cleaning_count_of_younger_agegroup);

	for (int s = 0; s < MAX_PLAIN_ILKS; ++s) {
	    //
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d]    x=%x",  s, a->sib[s]);
	    log_if("log_task:          a->sib[%d].id d=%d",  s, a->sib[s]->id);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].tospace            x=%x",  s, a->sib[s]->tospace);
	    log_if("log_task:          a->sib[%d].tospace_bytesize   x=%x",  s, a->sib[s]->tospace_bytesize);
	    log_if("log_task:          a->sib[%d].tospace_limit      x=%x",  s, a->sib[s]->tospace_limit);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].fromspace          x=%x",  s, a->sib[s]->fromspace);
	    log_if("log_task:          a->sib[%d].fromspace_bytesize x=%x",  s, a->sib[s]->fromspace_bytesize);
	    log_if("log_task:          a->sib[%d].fromspace_used_end x=%x",  s, a->sib[s]->fromspace_used_end);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].next_tospace_word_to_allocate x=%x",  s, a->sib[s]->next_tospace_word_to_allocate);
	    log_if("log_task:          a->sib[%d].next_word_to_sweep_in_tospace x=%x",  s, a->sib[s]->next_word_to_sweep_in_tospace);
	    log_if("log_task:          a->sib[%d].repairlist x=%x",  s, a->sib[s]->repairlist);
	    log_if("log_task:          a->sib[%d].end_of_fromespace_oldstuff x=%x",  s, a->sib[s]->end_of_fromspace_oldstuff);
	}
    }
}

// Write to logfile contents of the generation0
// buffer for a given Task.  No attempt is made
// to distinguish garbage from live data.
//
void log_gen0( Task* task ) {
    //
    log_if("");
    log_if("log_gen0: generation-0 heapbuffer dump for pthread->id x=%x",  (unsigned int)(task->pthread->tid));
    log_if("log_gen0:           agegroup0_buffer x=%x",  task->heap->agegroup0_buffer);
    log_if("log_gen0:  agegroup0_buffer_bytesize x=%x",  task->heap->agegroup0_buffer_bytesize);
    log_if("log_gen0:           sum of above two x=%x",  (char*)(task->heap->agegroup0_buffer) + task->heap->agegroup0_buffer_bytesize);
    log_if("log_gen0:    heap_allocation_pointer x=%x",  task->heap_allocation_pointer);
    log_if("log_gen0:      heap_allocation_limit x=%x",  task->heap_allocation_limit);
    log_if("log_gen0: real_heap_allocation_limit x=%x",  task->real_heap_allocation_limit);
    log_if("log_gen0: gen0 buffer contents");

    {   
	//
	for (Val* p = task->heap->agegroup0_buffer;
	     p < task->heap_allocation_pointer;
	     ++p
	){
	    //
	    log_if("log_gen0: %08x: %08x",  p, *p);
	}
    }
}

// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

