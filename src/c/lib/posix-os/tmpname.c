// exit.c


#include "../../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



// XXX BUGGO FIXME:
// tmpname.c:(.text+0xe): warning: the use of `tmpnam' is dangerous, better use `mkstemp'
// -- but the mkstemp() manpage says:
//
//      "Don't use this function, use tmpfile(3) instead.
//       It is better  defined and more portable."



// One of the library bindings exported via
//     src/c/lib/posix-os/cfun-list.h
// and thence
//     src/c/lib/posix-os/libmythryl-posix-os.c



Val   _lib7_OS_tmpname   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type:   Void -> String
    //
    //
    // This fn gets bound as   tmp_name   in:
    //
    //     src/lib/std/src/posix/winix-file.pkg


    char buf[ L_tmpnam ];
    //
    tmpnam( buf );
    //
    return make_ascii_string_from_c_string (task, buf);
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

