// copy.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif




// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_copy   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type:   (String, String) -> Int
    //
    // Copy a file  and return its length.
    //
    // This fn gets bound as   copy   in:
    //
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg				# Actually, I haven't gotten around to this yet.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val	existing =  GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	new_name =  GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* heap_existing =  HEAP_STRING_AS_C_STRING( existing );
    char* heap_new_name =  HEAP_STRING_AS_C_STRING( new_name );


    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  existing_buf;
    Mythryl_Heap_Value_Buffer  new_name_buf;

    int ok = TRUE;

    ssize_t total_bytes_written  = 0;

    {	char* c_existing =  buffer_mythryl_heap_value( &existing_buf, (void*) heap_existing, strlen( heap_existing ) +1 );		// '+1' for terminal NUL on string.
	char* c_new_name =  buffer_mythryl_heap_value( &new_name_buf, (void*) heap_new_name, strlen( heap_new_name ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    struct stat statbuf;
	    int fd_out;
	    int fd_in = open(c_existing, O_RDONLY);								// Open the input file.
	    if (fd_in >= 0) {
		if (0 <= fstat(fd_in, &statbuf)) {								// Get the mode of the input file so that we can ...
		    fd_out = creat( c_new_name, statbuf.st_mode );						// ... open the output file with same mode as input file.
		    if (0 <= fd_out) {
			char buffer[ 4096 ];
			ssize_t bytes_read;
			int ok = TRUE;
			while (ok) {										// Read up to one buffer[]-load from fd_in.
			    do {	
				bytes_read = read( fd_in, buffer, 4096 );
			    } while (bytes_read <  0 && errno == EINTR);					// Retry if interrupted by SIGALRM or such.
			    if      (bytes_read <  0) { ok = FALSE; break; }
			    if      (bytes_read == 0) {             break; }
			    ssize_t buffer_bytes_written  = 0;
			    while (ok  &&  (buffer_bytes_written < bytes_read)) {				// Write buffer[] contents to fd_out. Usually one write() will do it, but this is not guaranteed.
				ssize_t bytes_to_write = bytes_read - buffer_bytes_written;
				ssize_t bytes_written;
				do {
				    bytes_written  = write( fd_out, buffer+buffer_bytes_written, bytes_to_write );
				} while (bytes_written < 0 && errno == EINTR);					// Retry if interrupted by SIGALRM or such.
				ok = ok && (bytes_written > 0);
				buffer_bytes_written += bytes_written;
				 total_bytes_written += bytes_written;
			    }
			}
			close(fd_out);
		    } else {
			ok = FALSE; 
		    }
		    close(fd_in);
		} else {
		    ok = FALSE; 
		}
	    } else {
		ok = FALSE; 
	    }
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

	unbuffer_mythryl_heap_value( &existing_buf );
	unbuffer_mythryl_heap_value( &new_name_buf );
    }

    Val        result;

    if (!ok)   result = RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);						// XXX SUCKO FIXME I'm being totally sloppy about accurate diagnostics here.  Feel free to submit a patch improving this.
    else       result = TAGGED_INT_FROM_C_INT( total_bytes_written );


									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

