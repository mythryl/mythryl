// import-heap-stuff.c
//
// Support fns to import a Mythryl
// heap image, called mainly from
//
//     src/c/heapcleaner/import-heap.c 


#include "../mythryl-config.h"

#include "runtime-base.h"
#include "heap.h"
#include "runtime-values.h"
#include "runtime-heap-image.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "import-heap-stuff.h"
#include <string.h>

#ifndef SEEK_SET
#  define SEEK_SET	0
#  define SEEK_END	2
#endif

static Status   read_block   (FILE* file,  void* blk,  long len);


/*
###           "Every big computing disaster has
###            come from taking too many ideas
###            and putting them in one place."
###
###                          -- Gordon Bell
*/



Val*   heapio__read_externs_table   (Inbuf *bp)   {
    // =========================
    //

    // Read the header:
    //
    Externs_Header	    header;
    heapio__read_block( bp, &header, sizeof( header ) );

    Val* externs =  MALLOC_VEC( Val, header.externs_count );

    // Read in the names of the exported symbols:
    //
    Unt8*                  buf =  MALLOC_VEC( Unt8, header.externs_bytesize );
    heapio__read_block( bp, buf, header.externs_bytesize );

    // Map the names of the external symbols
    // to addresses in the run-time system:
    //
    Unt8* cp = buf;
    for (int i = 0;  i < header.externs_count;  i++) {
        //
        Val  heapval  =  find_cfun ((char*) cp);          if (heapval == HEAP_VOID)    die ("Run-time system does not provide \"%s\"", cp);

        externs[i] = heapval;

	cp +=  strlen((char*)cp) + 1;
    }

    FREE( buf );

    return externs;
}


Status   heapio__seek   (Inbuf* bp,  long offset) {
    //   ============
    //
    // Adjust the next character position to
    // the given position in the input stream.
    //
    if (bp->file == NULL) {

        // The stream is in-memory:
        //
	Unt8	*newPos = bp->base + offset;

	if (bp->buf + bp->nbytes <= newPos)   return FALSE;

	bp->nbytes -= (newPos - bp->buf);

	bp->buf = newPos;

	return TRUE;

    } else {

        if (fseek (bp->file, offset, SEEK_SET) != 0)   die ("unable to seek on heap image\n");

        bp->nbytes = 0;					// Just in case?

	return TRUE;
    }
}							// fun heapio__seek


Status   heapio__read_block   (Inbuf* bp,  void* blk,  long len) {
    //   ==================
    //
    Status  status =  TRUE;

    if (bp->nbytes == 0) {
        //
	if (bp->file != NULL) {
	    status = read_block (bp->file, blk, len);
	} else {
	    say_error( "missing data in pickle bytevector" );
	    return FALSE;
	}

    } else if (bp->nbytes >= len) {
	//
	memcpy (blk, bp->buf, len);
	bp->nbytes -= len;
	bp->buf += len;
	//
    } else {
        //
	memcpy (blk, bp->buf, bp->nbytes);
	status = read_block (bp->file, ((Unt8 *)blk) + bp->nbytes, len - bp->nbytes);
	bp->nbytes = 0;
    }

    if (bp->needs_to_be_byteswapped)   die ("byte-swapping not implemented yet");

    return status;

}								// fun heapio__read_block

static Status   read_block   (FILE* file,  void* blk,  long len) {
    //          ==========
    int status;

    Unt8* bp = (Unt8*) blk;

    while (len > 0) {
        //
	status = fread (bp, 1, len, file);
	len -= status;
	bp  += status;
	//
	if ((status < len) && (ferror(file) || feof(file))) {
	    //
	    say_error( "Unable to read %d bytes from image.\n", len );
	    return FALSE;
	}
    }

    return TRUE;
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.







/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/
