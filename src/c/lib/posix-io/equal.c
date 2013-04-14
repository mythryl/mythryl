// equal.c


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



Val   _lib7_P_IO_equal   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type:   (String, String) -> Int
    //
    // Compare contents of two files, return TRUE iff they are byte-for-byte identical.
    //
    // This fn gets bound as   equal   in:
    //
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg				# Actually, I haven't gotten around to this yet.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val	filename1 =  GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	filename2 =  GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* heap_filename1 =  HEAP_STRING_AS_C_STRING( filename1 );
    char* heap_filename2 =  HEAP_STRING_AS_C_STRING( filename2 );


    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  filename1_buf;
    Mythryl_Heap_Value_Buffer  filename2_buf;

    int ok    = TRUE;									// No irrecoverable I/O errors as yet.
    int equal = TRUE;									// Start by assuming the two files are equal.
    int done  = FALSE;									// Still have useful work to do.


    {	char* c_filename1 =  buffer_mythryl_heap_value( &filename1_buf, (void*) heap_filename1, strlen( heap_filename1 ) +1 );		// '+1' for terminal NUL on string.
	char* c_filename2 =  buffer_mythryl_heap_value( &filename2_buf, (void*) heap_filename2, strlen( heap_filename2 ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    int fd1 = open(c_filename1, O_RDONLY);					// Open first  input file.
	    int fd2 = open(c_filename2, O_RDONLY);					// Open second input file.
	    if (fd1 >= 0								// If both opened without error.
	    &&	fd2 >= 0
	    ){
		char buffer1[ 4096 ];
		char buffer2[ 4096 ];
		
		int end_of_data1 = 0;							// 1 + index of last valid byte in buffer.
		int end_of_data2 = 0;

		int next_byte_in_buffer1 = 0;						// Index of first uncompared byte in buffer.
		int next_byte_in_buffer2 = 0;

		int eof_on_file1 = FALSE;
		int eof_on_file2 = FALSE;

		while (ok && !done) {
		    //
		    // Re-fill buffer1[] and/or buffer2[] if empty:
		    //
		    if (next_byte_in_buffer1 == end_of_data1  &&  !eof_on_file1) {	// Time to read more bytes from file into buffer.
			next_byte_in_buffer1 = 0;
			do {
			    end_of_data1 = read( fd1, buffer1, 4096 );
			} while (end_of_data1 < 0 && errno == EINTR);			// Retry if we got interrupted by SIGALRM or such.
			if (end_of_data1 <  0) { ok = FALSE; break; }			// Error on read.
			if (end_of_data1 == 0)   eof_on_file1 = TRUE;
		    };
		    if (next_byte_in_buffer2 == end_of_data2  &&  !eof_on_file2) {
			next_byte_in_buffer2 = 0;
			do {
			    end_of_data2 = read( fd2, buffer2, 4096 );
			} while (end_of_data2 < 0 && errno == EINTR);
			if (end_of_data2 <  0) { ok = FALSE; break; }			// Error on read.
			if (end_of_data2 == 0)   eof_on_file2 = TRUE;
		    };


		    // Make sure there are bytes available to
		    // compare in both buffers; if not, handle it:
		    //
		    if (next_byte_in_buffer1 == end_of_data1
                    &&  next_byte_in_buffer2 == end_of_data2
		    ){
			break;								// All bytes compared; files are equal; done.
		    }
		    if (next_byte_in_buffer1 == end_of_data1
                    ||  next_byte_in_buffer2 == end_of_data2
		    ){
			equal = FALSE;
			break;								// One file is longer, so files are not equal; done.
		    }



		    // We have uncompared bytes in both buffers,
		    // so compare as many as possible:

		    int bytes1 = end_of_data1 - next_byte_in_buffer1;			// Bytes available to process in buffer1.
		    int bytes2 = end_of_data2 - next_byte_in_buffer2;

		    int bytes_to_compare = (bytes1 < bytes2) ? bytes1 : bytes2;		// bytes_to_compare = min( bytes1, bytes2 );

		    for (int i = 0;   i < bytes_to_compare;   ++i) {
			//
			if (buffer1[ next_byte_in_buffer1 + i ]
			!=  buffer2[ next_byte_in_buffer2 + i ]
			){
			    equal =  FALSE;
			    done  =  TRUE;
			    break;
			}
		    }

		    if (done) break;

		    next_byte_in_buffer1 +=  bytes_to_compare;
		    next_byte_in_buffer2 +=  bytes_to_compare;

		}
		close( fd1 );
		close( fd2 );
	    } else {
	        ok = FALSE;
	    }
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

	unbuffer_mythryl_heap_value( &filename1_buf );
	unbuffer_mythryl_heap_value( &filename2_buf );
    }

    Val        result;

    if (!ok)   result = RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);						// XXX SUCKO FIXME I'm being totally sloppy about accurate diagnostics here.  Feel free to submit a patch improving this.
    else       result = equal ? HEAP_TRUE : HEAP_FALSE;


									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

