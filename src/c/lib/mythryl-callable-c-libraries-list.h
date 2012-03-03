// mythryl-callable-c-libraries-list.h
//
// This file gets included into
//
//     mythryl_callable_c_libraries__local []
//
// in:
//
//     src/c/lib/mythryl-callable-c-libraries.c
//
											// MYTHRYL_CALLABLE_C_LIBRARY		def in    src/c/lib/mythryl-callable-c-libraries.c

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Heap				)		// Libmythryl_Heap			def in    src/c/lib/heap/libmythryl-heap.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Kludge				)		// Libmythryl_Kludge			def in    src/c/lib/kludge/libmythryl-kludge.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Signal				)		// Libmythryl_Signal			def in    src/c/lib/signal/libmythryl-signal.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Time_And_Space_Profiling		)		// Libmythryl_Time_And_Space_Profiling	def in    src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c

// Basis libraries:
//
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Time				)		// Libmythryl_Time			def in    src/c/lib/time/libmythryl-time.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Date				)		// Libmythryl_Date			def in    src/c/lib/date/libmythryl-date.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Math				)		// Libmythryl_Math			def in    src/c/lib/math/libmythryl-math.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Socket				)		// Libmythryl_Socket			def in    src/c/lib/socket/libmythryl-socket.c

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Gtk				)		// Libmythryl_Gtk			def in    src/c/lib/gtk/libmythryl-gtk.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Ncurses				)		// Libmythryl_Ncurses			def in    src/c/lib/ncurses/libmythryl-ncurses.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Opencv				)		// Libmythryl_Opencv			def in    src/c/lib/opencv/libmythryl-opencv.c

#ifdef HAS_POSIX_LIBRARIES

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Error			)		// Libmythryl_Posix_Error		def in    src/c/lib/posix-error/libmythryl-posix-error.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Filesys			)		// Libmythryl_Posix_Filesys		def in    src/c/lib/posix-file-system/libmythryl-posix-file-system.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Io				)		// Libmythryl_Posix_Io			def in    src/c/lib/posix-io/libmythryl-posix-io.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Process_Environment	)		// Libmythryl_Posix_Process_Environment	def in    src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Process			)		// Libmythryl_Posix_Process		def in    src/c/lib/posix-process/libmythryl-posix-process.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Signal			)		// Libmythryl_Posix_Signal		def in    src/c/lib/posix-signal/libmythryl-posix-signal.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Passwd_Db			)		// Libmythryl_Posix_Passwd_Db		def in    src/c/lib/posix-passwd/libmythryl-posix-passwd-db.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Tty			)		// Libmythryl_Posix_Tty			def in    src/c/lib/posix-tty/libmythryl-posix-tty.c

#endif

#ifdef OPSYS_UNIX

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Posix_Os				)		// Libmythryl_Posix_Os			def in    src/c/lib/posix-os/libmythryl-posix-os.c

#elif defined(OPSYS_WIN32)

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Win32				)		// Libmythryl_Win32			def in    src/c/lib/win32/libmythryl-win32.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Win32_Io				)		// Libmythryl_Win32_Io			def in    src/c/lib/win32-io/libmythryl-win32-io.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Win32_Filesys			)		// Libmythryl_Win32_Filesys		def in    src/c/lib/win32-file-system/libmythryl-win32-file-system.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Win32_Process			)		// Libmythryl_Win32_Process		def in    src/c/lib/win32-process/libmythryl-win32-process.c

#endif

MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Pthread				)		// Libmythryl_Pthread			def in    src/c/lib/pthread/libmythryl-pthread.c

#ifdef C_CALLS										// C_CALLS is nowhere defined; it is referenced only here and in   src/c/heapcleaner/call-heapcleaner.c
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Ccalls				)		// Libmythryl_Ccalls			def in    src/c/lib/ccalls/libmythryl-ccalls.c
#endif


#ifdef DLOPEN
MYTHRYL_CALLABLE_C_LIBRARY( Libmythryl_Dynamic_Loading			)		// Libmythryl_Dynamic_Loading		def in    src/c/lib/dynamic-loading/libmythryl-dynamic-loading.c
#endif


// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

