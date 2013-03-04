// cfun-list.h
//
//
// This file lists the "socket" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_host_name:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "socket", fun_name => "get_host_name" };
// 
// or such -- see   src/lib/std/src/socket/dns-host-lookup.pkg
// 
// We get #included by both:
//
//     src/c/lib/socket/libmythryl-socket.c
//     src/c/lib/socket/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"socket"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"June 10, 1995"
#endif

// Network database functions:
//
CFUNC("get_host_name","get_host_name",	_lib7_netdb_get_host_name,		"Void -> String")
CFUNC("get_network_by_name","get_network_by_name",	_lib7_netdb_get_network_by_name,		"")
CFUNC("get_network_by_address","get_network_by_address",	_lib7_netdb_get_network_by_address,		"")
CFUNC("get_host_by_name","get_host_by_name",	_lib7_netdb_get_host_by_name,	"")
CFUNC("get_host_by_address","get_host_by_address",	_lib7_netdb_get_host_by_address,	"")
CFUNC("get_protocol_by_name","get_protocol_by_name",	_lib7_netdb_get_protocol_by_name,	"")
CFUNC("get_protocol_by_number","get_protocol_by_number",	_lib7_netdb_get_protocol_by_number,		"")
CFUNC("get_service_by_name","get_service_by_name",	_lib7_netdb_get_service_by_name,	"")
CFUNC("get_service_by_port","get_service_by_port",	_lib7_netdb_get_service_by_port,	"")

CFUNC("get_or_set_socket_debug_option","ctlDEBUG",	get_or_set_socket_debug_option,	"(socket, Null_Or(Bool)) -> Bool")
CFUNC("get_or_set_socket_reuseaddr_option","ctlREUSEADDR",	get_or_set_socket_reuseaddr_option,	"")
CFUNC("get_or_set_socket_keepalive_option","ctlKEEPALIVE",	get_or_set_socket_keepalive_option,	"")
CFUNC("get_or_set_socket_dontroute_option","ctlDONTROUTE",	get_or_set_socket_dontroute_option,	"")
CFUNC("get_or_set_socket_linger_option","ctlLINGER",	get_or_set_socket_linger_option,	"")
CFUNC("get_or_set_socket_broadcast_option","ctlBROADCAST",	get_or_set_socket_broadcast_option,	"")
CFUNC("get_or_set_socket_oobinline_option","ctlOOBINLINE",	get_or_set_socket_oobinline_option,	"")
CFUNC("get_or_set_socket_sndbuf_option","ctlSNDBUF",	get_or_set_socket_sndbuf_option,	"")
CFUNC("get_or_set_socket_rcvbuf_option","ctlRCVBUF",	get_or_set_socket_rcvbuf_option,	"")
CFUNC("get_or_set_socket_nodelay_option","ctlNODELAY",	get_or_set_socket_nodelay_option,	"")
CFUNC("getTYPE","getTYPE",	_lib7_Sock_getTYPE,	"")
CFUNC("getERROR","getERROR",	_lib7_Sock_getERROR,	"")
CFUNC("setNBIO","setNBIO",	_lib7_Sock_setNBIO,	"(socket * int) -> Void")
CFUNC("getNREAD","getNREAD",	_lib7_Sock_getNREAD,	"socket -> int")
CFUNC("getATMARK","getATMARK",	_lib7_Sock_getATMARK,	"socket -> Bool")
CFUNC("getPeerName","getPeerName",	_lib7_Sock_getpeername,	"")
CFUNC("getSockName","getSockName",	_lib7_Sock_getsockname,	"")

CFUNC("getAddrFamily","getAddrFamily",	_lib7_Sock_getaddrfamily,	"addr -> af")
CFUNC("listAddrFamilies","listAddrFamilies", _lib7_Sock_listaddrfamilies, "")
CFUNC("listSockTypes","listSockTypes",	_lib7_Sock_listsocktypes,	"")
CFUNC("inetany","inetany",	_lib7_Sock_inetany,	"int -> addr")
CFUNC("fromInetAddr","fromInetAddr",	_lib7_Sock_frominetaddr,	"addr -> (in_addr*int)")
CFUNC("toInetAddr","toInetAddr",	_lib7_Sock_toinetaddr,	"(in_addr*int) -> addr")

CFUNC("accept","accept",		_lib7_Sock_accept,	"socket -> (socket * vector_of_one_byte_unts.Vector)")
CFUNC("bind","bind",		_lib7_Sock_bind,		"")
CFUNC("connect","connect",	_lib7_Sock_connect,	"")
CFUNC("listen","listen",		_lib7_Sock_listen,	"")
CFUNC("close","close",		_lib7_Sock_close,		"")
CFUNC("shutdown","shutdown",	_lib7_Sock_shutdown,	"")
CFUNC("sendBuf","sendBuf",	_lib7_Sock_sendbuf,	"")
CFUNC("sendBufTo","sendBufTo",	_lib7_Sock_sendbufto,	"")
CFUNC("recv","recv",		_lib7_Sock_recv,		"")
CFUNC("recvBuf","recvBuf",	_lib7_Sock_recvbuf,	"")
CFUNC("recvFrom","recvFrom",	_lib7_Sock_recvfrom,	"")
CFUNC("recvBufFrom","recvBufFrom",	_lib7_Sock_recvbuffrom,	"")

CFUNC("socket","socket",		_lib7_Sock_socket,	"(Int, Int, Int) -> Socket")

CFUNC("setPrintIfFd","setPrintIfFd",	_lib7_Sock_setprintiffd,"Int -> Void")
CFUNC("to_log","to_log",		_lib7_Sock_to_log,"String -> Void")

#ifdef HAS_UNIX_DOMAIN
CFUNC("socketPair","socketPair",	_lib7_Sock_socketpair,	"(int * int * int) -> (socket * socket)")
CFUNC("fromUnixAddr","fromUnixAddr",	_lib7_Sock_unix_domain_socket_address_to_string, "Internet_Address -> String")
CFUNC("toUnixAddr","toUnixAddr",	_lib7_Sock_string_to_unix_domain_socket_address, "String -> Internet_Address")
    //
    // Trivially renaming toUnixAddr gies  bin/mythryld: Fatal error -- Run-time system does not provide "socket.toUnixAddr"

#endif



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

