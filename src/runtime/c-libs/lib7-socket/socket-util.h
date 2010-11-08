/* socket-util.h
 *
 *
 * Utility functions for the network database and socket routines.
 */

#ifndef _SOCK_UTIL_
#define _SOCK_UTIL_

typedef struct hostent *hostent_ptr_t;
typedef struct netent *netent_ptr_t;
typedef struct servent *servent_ptr_t;

extern lib7_val_t _util_NetDB_mkhostent (lib7_state_t *lib7_state, hostent_ptr_t hentry);
extern lib7_val_t _util_NetDB_mknetent (lib7_state_t *lib7_state, netent_ptr_t nentry);
extern lib7_val_t _util_NetDB_mkservent (lib7_state_t *lib7_state, servent_ptr_t sentry);
extern lib7_val_t _util_Sock_ControlFlg (lib7_state_t *lib7_state, lib7_val_t arg, int option);

extern sysconst_table_t	_Sock_AddrFamily;
extern sysconst_table_t	_Sock_Type;

#endif /* !_SOCK_UTIL_ */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
