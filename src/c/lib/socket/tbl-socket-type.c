// table-socket-type.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "socket-util.h"

// The table of socket-type names:
//
static System_Constant	table[] = {
	{SOCK_STREAM,		"STREAM"},
	{SOCK_DGRAM,		"DGRAM"},
#ifdef SOCK_RAW
	{SOCK_RAW,		"RAW"},
#endif
#ifdef SOCK_RDM
	{SOCK_RDM,		"RDM"},
#endif
#ifdef SOCK_SEQPACKET
	{SOCK_SEQPACKET,	"SEQPACKET"},
#endif
    };

Sysconsts	_Sock_Type = {
	sizeof(table) / sizeof(System_Constant),
	table
    };



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

