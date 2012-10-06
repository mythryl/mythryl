// opendir.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_DIRENT_H
    #include <dirent.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_opendir   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  String -> Chunk
    //
    // Open and return a directory stream.
    //
    // This fn gets bound as   opendir'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    DIR* dir;

    char* heap_path = HEAP_STRING_AS_C_STRING(arg);

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_opendir", NULL );
	    //
	    dir = opendir( c_path );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_opendir" );
	//
	unbuffer_mythryl_heap_value( &path_buf );
    }

    if (dir == NULL)  return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return PTR_CAST( Val, dir);						// PTR_CAST is from   src/c/h/runtime-values.h
}									//
									// I would think just casting a C pointer to a Val and returning
									// it should crash the garbage collector, but doing
									//
									//    foo = posixlib::open_directory_stream ".";
									//
									// and then
									//
									//    heapcleaner_control::clean_heap 2;
									//
									// repeatedly does seem to work.
									// Is there heavy magic in the runtime::Chunk type...?
									// If so, how does it get conveyed to the heapcleaner gut?  -- 2011-11-19 CrT
									///////////////////////////////////////////////////////////
									// This works because the garbage collector has a global
									// map of address space which it uses to distinguish pointers
									// into the heap from pointers into other parts of RAM.
									// 
									// The global map is book_to_sibid__global[] whose slots are
									// initialized to UNMAPPED_BOOK_SIBID in
									//     src/c/heapcleaner/heapcleaner-initialization.c
									//
									// The critical tests include in  src/c/heapcleaner/heapclean-agegroup0.c
									//
									//      if (IS_POINTER(w)) {
									//	    //
									//	    Sibid  sibid =  SIBID_FOR_POINTER( book2sibid, w );
									//	    //
									//	    if (sibid == AGEGROUP0_SIBID)
									//      }
									//
									// and in  src/c/heapcleaner/heapclean-n-agegroups.c
									//
									//	if (IS_POINTER( pointee )) {								// Ignore Tagged_Int values.
									//	    //
									//	    Sibid  sibid =  SIBID_FOR_POINTER(b2s, pointee );					COUNT_CODECHUNKS( sibid );
									//	    //
									//	    if (SIBID_IS_IN_FROMSPACE( sibid, max_sibid )) {
									//
									// where in both cases if  sibid == UNMAPPED_BOOK_SIBID
									// the heapcleaner does nothing.			-- 2012-10-06 CrT


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

