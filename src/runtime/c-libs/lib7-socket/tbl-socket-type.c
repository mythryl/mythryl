/* table-socket-type.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "socket-util.h"

/** The table of socket-type names **/
static sys_const_t	table[] = {
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

sysconst_table_t	_Sock_Type = {
	sizeof(table) / sizeof(sys_const_t),
	table
    };



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
