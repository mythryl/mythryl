// heapcleaner.h
//
// The external interface to the heapcleaner ("garbage collector").


#ifndef CLEANER_H
#define CLEANER_H

#include "runtime-base.h"

/* typedef   struct heap   Heap; */	// From runtime-base.h

extern void	set_up_heap				(Task* task,  Bool is_boot,  Cleaner_Args* params);	// set_up_heap					def in    src/c/heapcleaner/heapcleaner-initialization.c
extern void	clean_heap				(Task* task,  int  level);				// clean_heap					def in    src/c/heapcleaner/call-heapcleaner.c
extern void	clean_heap_with_extra_roots		(Task* task,  int  level, ...);				// clean_heap_with_extra_roots			def in    src/c/heapcleaner/call-heapcleaner.c

extern Bool	need_to_clean_heap			(Task* task,  Val_Sized_Unt nbytes);			// need_to_clean_heap				def in   src/c/heapcleaner/call-heapcleaner.c	
extern int	get_chunk_age				(Val chunk);						// get_chunk_age				def in   src/c/heapcleaner/get-chunk-age.c
extern Val	concatenate_two_tuples			(Task* task,  Val r1,  Val r2);				// concatenate_two_tuples			def in   src/c/heapcleaner/tuple-ops.c

Unt8*		codechunk_comment_string_for_program_counter	(Val_Sized_Unt pc);				// codechunk_comment_string_for_program_counter	def in   src/c/heapcleaner/hugechunk.c



#endif   // CLEANER_H


// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


