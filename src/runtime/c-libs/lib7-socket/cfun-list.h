/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Sockets"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"June 10, 1995"
#endif

/* Network database functions */
CFUNC("getHostName",	_lib7_NetDB_gethostname,		"Void -> String")
CFUNC("getNetByName",	_lib7_NetDB_getnetbyname,		"")
CFUNC("getNetByAddr",	_lib7_NetDB_getnetbyaddr,		"")
CFUNC("getHostByName",	_lib7_NetDB_gethostbyname,	"")
CFUNC("getHostByAddr",	_lib7_NetDB_gethostbyaddr,	"")
CFUNC("getProtByName",	_lib7_NetDB_getprotbyname,	"")
CFUNC("getProtByNum",	_lib7_NetDB_getprotbynum,		"")
CFUNC("getServByName",	_lib7_NetDB_getservbyname,	"")
CFUNC("getServByPort",	_lib7_NetDB_getservbyport,	"")

CFUNC("ctlDEBUG",	_lib7_Sock_ctlDEBUG,	"(socket * Bool option) -> Bool")
CFUNC("ctlREUSEADDR",	_lib7_Sock_ctlREUSEADDR,	"")
CFUNC("ctlKEEPALIVE",	_lib7_Sock_ctlKEEPALIVE,	"")
CFUNC("ctlDONTROUTE",	_lib7_Sock_ctlDONTROUTE,	"")
CFUNC("ctlLINGER",	_lib7_Sock_ctlLINGER,	"")
CFUNC("ctlBROADCAST",	_lib7_Sock_ctlBROADCAST,	"")
CFUNC("ctlOOBINLINE",	_lib7_Sock_ctlOOBINLINE,	"")
CFUNC("ctlSNDBUF",	_lib7_Sock_ctlSNDBUF,	"")
CFUNC("ctlRCVBUF",	_lib7_Sock_ctlRCVBUF,	"")
CFUNC("ctlNODELAY",	_lib7_Sock_ctlNODELAY,	"")
CFUNC("getTYPE",	_lib7_Sock_getTYPE,	"")
CFUNC("getERROR",	_lib7_Sock_getERROR,	"")
CFUNC("setNBIO",	_lib7_Sock_setNBIO,	"(socket * int) -> Void")
CFUNC("getNREAD",	_lib7_Sock_getNREAD,	"socket -> int")
CFUNC("getATMARK",	_lib7_Sock_getATMARK,	"socket -> Bool")
CFUNC("getPeerName",	_lib7_Sock_getpeername,	"")
CFUNC("getSockName",	_lib7_Sock_getsockname,	"")

CFUNC("getAddrFamily",	_lib7_Sock_getaddrfamily,	"addr -> af")
CFUNC("listAddrFamilies", _lib7_Sock_listaddrfamilies, "")
CFUNC("listSockTypes",	_lib7_Sock_listsocktypes,	"")
CFUNC("inetany",	_lib7_Sock_inetany,	"int -> addr")
CFUNC("fromInetAddr",	_lib7_Sock_frominetaddr,	"addr -> (in_addr*int)")
CFUNC("toInetAddr",	_lib7_Sock_toinetaddr,	"(in_addr*int) -> addr")

CFUNC("accept",		_lib7_Sock_accept,	"socket -> (socket * unt8_vector.Vector)")
CFUNC("bind",		_lib7_Sock_bind,		"")
CFUNC("connect",	_lib7_Sock_connect,	"")
CFUNC("listen",		_lib7_Sock_listen,	"")
CFUNC("close",		_lib7_Sock_close,		"")
CFUNC("shutdown",	_lib7_Sock_shutdown,	"")
CFUNC("sendBuf",	_lib7_Sock_sendbuf,	"")
CFUNC("sendBufTo",	_lib7_Sock_sendbufto,	"")
CFUNC("recv",		_lib7_Sock_recv,		"")
CFUNC("recvBuf",	_lib7_Sock_recvbuf,	"")
CFUNC("recvFrom",	_lib7_Sock_recvfrom,	"")
CFUNC("recvBufFrom",	_lib7_Sock_recvbuffrom,	"")

CFUNC("socket",		_lib7_Sock_socket,	"(int * int * int) -> socket")

CFUNC("setPrintIfFd",	_lib7_Sock_setprintiffd,"Int -> Void")

#ifdef HAS_UNIX_DOMAIN
CFUNC("socketPair",	_lib7_Sock_socketpair,	"(int * int * int) -> (socket * socket)")
CFUNC("fromUnixAddr",	_lib7_Sock_fromunixaddr,	"addr -> String")
CFUNC("toUnixAddr",	_lib7_Sock_tounixaddr,	"String -> addr")
#endif



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
