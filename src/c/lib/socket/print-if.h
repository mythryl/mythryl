/* print-if.h
 *
 * Conditional tracing to a logfile
 * designed to work in concert with
 *
 *     src/lib/src/lib/thread-kit/src/lib/logger.pkg
 *
 * At the Mythryl level one calls
 *
 *     internet_socket::set_printif_fd
 *
 * from
 *
 *     src/lib/std/src/socket/internet-socket.pkg
 *
 * to enable this tracing by setting
 *
 *     print_if_fd
 *
 * after which desired C modules can call print_if
 * to write lines into the tracelog file.
 */

extern void   print_if   (const char * fmt, ...);
extern int    print_if_fd;
