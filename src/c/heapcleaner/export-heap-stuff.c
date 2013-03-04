// export-heap-stuff.c
//
// Utility routines to export a Mythryl heap
// or to pickle a Mythryl heap datastructure.


#include "../mythryl-config.h"

#include <string.h>
#include "runtime-base.h"
#include "heap.h"
#include "runtime-values.h"
#include "runtime-heap-image.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "export-heap-stuff.h"

#include "shebang-line.h"

Status   heapio__write_image_header   (Writer* wr,  int kind) {
    //   ==========================
    // 
    // Write out the  Heapfile_Header.
    //
    // We get called from:
    //     src/c/heapcleaner/export-heap.c
    //     src/c/heapcleaner/datastructure-pickler.c

    Heapfile_Header	header;

    {   int i;
        for (i = SHEBANG_SIZE;  i --> 0; )  header.shebang[i] = 0;
        strcpy(
            header.shebang,
            SHEBANG_LINE
        );
    }

    header.byte_order = ORDER;

    header.magic  = ((kind == EXPORT_HEAP_IMAGE) || (kind == EXPORT_FN_IMAGE))
			? IMAGE_MAGIC : PICKLE_MAGIC;

    header.kind	  = kind;

    // header.arch[]
    // header.opsys[]

    WR_WRITE(wr, &header, sizeof(header));

    return !WR_ERROR(wr);
}


int   heapio__write_cfun_table   (Writer* wr,  Heapfile_Cfun_Table* table)   {
    //=========================
    // 
    // Write out the external symbol table,
    // returning the number of bytes written
    // else -1 on error.
    //
    // We get called once each from:
    //
    //     src/c/heapcleaner/export-heap.c
    //     src/c/heapcleaner/datastructure-pickler.c


    int	total_bytes_written =  0;

    // Construct and return a vector containing the names
    // of all C functions (etc) referenced by the Mythryl
    // heap.  In general these names have the form
    //
    //     lib_name.fun_name
    //
    // where lib_name is #defined as CLIB_NAME
    // in (e.g.) one of our
    //
    //     src/c/lib/*/cfun-list.h
    //
    // and function-name is from one of the CFUNC() lines
    // in the same file.
    //     
    // We will save these strings in our heap image file -- they
    // will be used during loading to look up C functions via
    //
    //     find_cfun()    from    src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //     
    int			            cfun_names_count;
    const char**	                               cfun_names;
    get_names_of_all_cfuns_in_heapfile_cfun_table( table, &cfun_names_count, &cfun_names );		// get_names_of_all_cfuns_in_heapfile_cfun_table	def in   src/c/heapcleaner/mythryl-callable-cfun-hashtable.c

    int	bytes_of_strings = 0;
    //
    for (int i = 0;  i < cfun_names_count;  i++) {
        //
	bytes_of_strings +=  strlen( cfun_names[i] ) + 1;				// '+1' for terminal '\0' char.
    }
    // Pad to WORD_BYTESIZE bytes:
    //
    int  bytes_of_padding_needed =   ROUND_UP_TO_POWER_OF_TWO( bytes_of_strings, WORD_BYTESIZE )
                                     -
                                     bytes_of_strings;
    //
    bytes_of_strings    +=  bytes_of_padding_needed;
    total_bytes_written +=  bytes_of_strings;

    // Write out the header:
    //
    {   Externs_Header	header;
	//
	header.externs_count         =  cfun_names_count;
	header.externs_bytesize =  bytes_of_strings;
	//
	WR_WRITE(wr, &header,  sizeof(header));
    	total_bytes_written += sizeof(header);
    }

    // Write out the external symbols:
    //
    for (int i = 0;  i < cfun_names_count;  i++) {
        //
	WR_WRITE( wr, cfun_names[i], strlen( cfun_names[i] ) +1 );
    }

    // Write the padding:
    //
    if (bytes_of_padding_needed != 0)  {			static char pad[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	//
	WR_WRITE( wr, pad, bytes_of_padding_needed );
    }

    FREE( cfun_names );

    if (WR_ERROR(wr))	return -1;
    else		return total_bytes_written;
}							// fun heapio__write_cfun_table


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.







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
