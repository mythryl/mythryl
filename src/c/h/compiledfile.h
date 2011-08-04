// compiledfile.h -- layout of a .compiled file, e.g., foo.pkg.compiled.
//
// The layout is:
//   header
//   import Picklehashes (16 bytes each)
//   export Picklehashes (16 bytes each)
//   makelib dependency information
//   inlinable lambda expression
//   reserved area 1 (typically empty)
//   reserved area 2 (typically empty)
//   code chunks
//     This section contains a sequence of code
//     chunks, each of which is lead by its size.
//     The individual sizes must sum up to
//     bytes_of_compiled_code.
//   pickled static environment


#ifndef COMPILED_FILE_H
#define COMPILED_FILE_H

#include "runtime-base.h"


// Picklehashes.  These are essentially message digests:
// 16-byte hashes of the contents of an .compiled file which
// we use as a convenient name for that version of that file.
//
#define PICKLEHASH_BYTES	16
//
typedef struct {
    Unt8	bytes[ PICKLEHASH_BYTES ];
} Picklehash;


// The header of a .compiled file, e.g. foo.pkg
//
// Note that the fields are in
// big-endian representation. XXX BUGGO FIXME
//
typedef struct {
    // XXX BUGGO FIXME Should maybe put a shebang line here
    // leading to our odump7 utility -- once it is written. :)
    //
    Unt8	magic[16];	                // Magic number.
    Int32	number_of_imported_picklehashes;
    Int32	number_of_exported_picklehashes;
    Int32	bytes_of_import_tree;		// Bytes of references to values in other compiled_files.
    Int32	bytes_of_dependency_info;	// The size of the makelib dependency information area.
    Int32	bytes_of_inlinable_code;	// Nubmer of bytes of  inlinable intermediate code.
    Int32	reserved;			// Reserved for future use.
    Int32	pad;	        		// Padding for code segment alignment.
    Int32	bytes_of_compiled_code;
    Int32	bytes_of_symbolmapstack;		// Holds type the information for our exported values.
    //
}   Compiledfile_Header;

#endif						// COMPILED_FILE_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


