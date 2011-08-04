// socket-util.h
//
// Utility functions for the network database and socket routines.


#ifndef _SOCK_UTIL_
#define _SOCK_UTIL_

typedef struct hostent *hostent_ptr_t;
typedef struct netent *netent_ptr_t;
typedef struct servent *servent_ptr_t;

extern Val _util_NetDB_mkhostent (Task *task, hostent_ptr_t hentry);
extern Val _util_NetDB_mknetent (Task *task, netent_ptr_t nentry);
extern Val _util_NetDB_mkservent (Task *task, servent_ptr_t sentry);
extern Val get_or_set_boolean_socket_option (Task *task, Val arg, int option);

extern System_Constants_Table	_Sock_AddrFamily;
extern System_Constants_Table	_Sock_Type;

#endif // _SOCK_UTIL_ 



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

