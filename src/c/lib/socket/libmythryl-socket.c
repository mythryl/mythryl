// libmythryl-socket.c

#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"

/*
###         "In a room full of top software designers, if
###          any two of them agree -- that's a majority!"
###
###                                -- Bill Curtis
*/

// This file defines the "socket" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_host_name:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "socket", fun_name => "get_host_name" };
// 
// or such -- see   src/lib/std/src/socket/dns-host-lookup.pkg
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in   src/c/lib/socket/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


void   init_g   (int argc, char **argv) {
    // ======
    //
    #if defined(OPSYS_WIN32)
	static int nCode = -1;
	if( nCode!=0 )
	  {
	    WSADATA wsaData;
	    nCode = WSAStartup(MAKEWORD(1, 1), &wsaData);      // XXX BUGGO FIXME: What to do if WSAStartup fails (nCode!=0)?
	  }
    #endif
}


// The Sockets library:
//
// Our record                Libmythryl_Socket
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Socket = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              =================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    init_g,
    CFunTable
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

