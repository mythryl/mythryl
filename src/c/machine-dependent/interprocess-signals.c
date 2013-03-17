// interprocess-signals.c
//
// Code to support Mythryl-level handling of host-OS interprocess signals.


#include "../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "system-dependent-unix-stuff.h"
#include "system-dependent-signal-get-set-etc.h"
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "make-strings-and-vectors-etc.h"
// #include "system-dependent-signal-stuff.h"
#include "runtime-globals.h"
#include "heap.h"



#define UNSUPPORTED_SIGNAL 0

#ifndef SIGALRM
#define SIGALRM   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGABRT									// We favor "SIGABRT" over "SIGIOT" because the former has 10X more Google hits.
#ifdef  SIGIOT
#define SIGABRT   SIGIOT
#else
#define SIGABRT   UNSUPPORTED_SIGNAL
#endif
#endif

#ifndef SIGBUS
#define SIGBUS    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGCHLD
#ifdef  SIGCLD
#define SIGCHLD   SIGCLD
#else
#define SIGCHLD   UNSUPPORTED_SIGNAL
#endif
#endif

#ifndef SIGCONT
#define SIGCONT   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGHUP
#define SIGHUP    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGILL
#define SIGILL    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGINT
#define SIGINT    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGIO								// "SIGIO" has 10X more Google hits than "SIGPOLL".
#ifdef  SIGPOLL
#define SIGIO     SIGPOLL
#else
#define SIGIO     UNSUPPORTED_SIGNAL
#endif
#endif

#ifndef SIGKILL
#define SIGKILL   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGPIPE
#define SIGPIPE   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGPROF
#define SIGPROF   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGPWF
#define SIGPWF    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGQUIT
#define SIGPQUIT  UNSUPPORTED_SIGNAL
#endif

#ifndef SIGSTKFLT
#define SIGSTKFLT UNSUPPORTED_SIGNAL
#endif

#ifndef SIGSTOP
#define SIGSTOP   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGSYS
#define SIGSYS    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGTERM
#define SIGTERM   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGTRAP
#define SIGTRAP   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGTSTP
#define SIGTSTP   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGTTIN
#define SIGTTIN   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGTTOU
#define SIGTTOU   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGURG
#define SIGURG    UNSUPPORTED_SIGNAL
#endif

#ifndef SIGUSR1
#define SIGUSR1   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGUSR2
#define SIGUSR2   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGVTALRM
#define SIGVTALRM UNSUPPORTED_SIGNAL
#endif

#ifndef SIGWINCH							// "SIGWINCH" gets 10X more Google hits than "SIGWINDOW".
#ifdef  SIGWINDOW
#define SIGWINCH  SIGWINDOW
#else
#define SIGWINCH  UNSUPPORTED_SIGNAL
#endif
#endif

#ifndef SIGXCPU
#define SIGXCPU   UNSUPPORTED_SIGNAL
#endif

#ifndef SIGXFSZ
#define SIGXFSZ  UNSUPPORTED_SIGNAL
#endif



typedef struct {
    //
    int   host_os_id_for_signal;						// Numeric value of SIGQUIT or such.
    char* signal_h__name_for_signal;					// The name #define'd for the signal in <signal.h>, e.g. "SIGQUIT".
    //
} Signal_Descriptor;



//////////////////////////////////////////////
// The POSIX/ANSI/BSD/Linux signals we support.
//
//    KEEP THIS TABLE IN SYNC WITH Signal in src/lib/std/src/nj/interprocess-signals-guts.pkg
//
// The intention here is that we use the row						// It would be nice to have readable symbolic names for these row numbers, say PORTABLE_SIGNAL_NAME_SIGHUP ; I don't think we do as yet.
// number in the following table as the stable
// Mythryl-side name for a signal, to make the
// Mythryl-side world signal naming independent
// of the particular int signal names used by
// the current host OS kernel, and indeed
// independent of whether they are supported
// by the current kernel.
//
// To that end, it is probably best that any signals
// added to this table GO AT THE END, rather than
// in alphabetical order.
//											// signal_table__local is used in this file and also in   src/c/machine-dependent/interprocess-signals.c
static Signal_Descriptor   signal_table__local[ SIGNAL_TABLE_SIZE_IN_SLOTS ]   = {	// SIGNAL_TABLE_SLOTS	is #defined in   src/c/h/runtime-base.h
    //                     ====================
    //
    { 0,	"SIGZERO"},		// 			//  Totally bogus nonsignal. The point of this is to make row-number == host_os_id_for_signal, at least on stock Linux.
    { SIGHUP,	"SIGHUP"},		// POSIX		//  1 Hangup.
    { SIGINT,	"SIGINT"},		// ANSI			//  2 Interrupt.
    { SIGQUIT,	"SIGQUIT"},		// POSIX		//  3 Quit.
    { SIGILL,	"SIGILL"},		// ANSI			//  4 Illegal instruction
    { SIGTRAP,	"SIGTRAP"},		// POSIX		//  5 Trace trap
    { SIGABRT,	"SIGABRT"},		// ANSI			//  6 Abort.     On Linux == BSD4.2 SIGIOT.
    { SIGBUS,	"SIGBUS"},		// BSD 4.2		//  7 BUS error.
    { SIGFPE,	"SIGFPE"},		// ANSI			//  8 Floating-point exception.
    { SIGKILL,	"SIGKILL"},		// POSIX		//  9 Kill, unblockable.
    { SIGUSR1,	"SIGUSR1"},		// POSIX		// 10 User-defined signal 1.
    { SIGSEGV,	"SIGSEGV"},		// ANSI			// 11 Segmentation violation. (Typically due to use of an invalid C pointer.)
    { SIGUSR2,	"SIGUSR2"},		// POSIX		// 12 User-defined signal 2.
    { SIGPIPE,	"SIGPIPE"},		// POSIX		// 13 Broken pipe.
    { SIGALRM,	"SIGALRM"},		// POSIX		// 14 Alarm.  See also SIGVTALRM.
    { SIGTERM,	"SIGTERM"},		// POSIX		// 15 Polite (catchable) request to terminate. http://en.wikipedia.org/wiki/SIGTERM
    { SIGSTKFLT, "SIGSTKFLT"},		// Linux		// 16 Stack fault.
    { SIGCHLD,	"SIGCHLD"},		// POSIX		// 17 Child status has changed.
    { SIGCONT,	"SIGCONT"},		// POSIX		// 18 Continue.
    { SIGSTOP,	"SIGSTOP"},		// POSIX		// 19 Stop, unblockable.
    { SIGTSTP,	"SIGTSTP"},		// POSIX		// 20 Keyboard stop.
    { SIGTTIN,	"SIGTTIN"},		// POSIX		// 21 Background read from TTY.
    { SIGTTOU,	"SIGTTOU"},		// POSIX		// 22 Backround write to TTY.
    { SIGURG,	"SIGURG"},		// BSD 4.2		// 23 Urgent condition on socket.
    { SIGXCPU,	"SIGXCPU"},		// BSD 4.2		// 24 CPU limit exceeded.
    { SIGXFSZ,	"SIGXFSZ"},		// BSD 4.2		// 25 File size limit exceeded.
    { SIGVTALRM, "SIGVTALRM"},		// BSD 4.2		// 26 Alarm.  See also SIGALRM.
    { SIGPROF,	"SIGPROF"},		// BSD 4.2		// 27 Profiling alarm clock.
    { SIGWINCH,	"SIGWINCH"},		// BSD 4.3		// 28 Window size change.
    { SIGIO,	"SIGIO"},		// BSD4.2		// 29 I/O now possible.
    { SIGPWR,	"SIGPWR"},		// SYS V		// 30 Power failure restart.
    { SIGSYS,	"SIGSYS"},		// Linux		// 31 Bad system call.
};

// This is now statically defined in runtime-base.h
// #define SIGNAL_TABLE_SIZE_IN_SLOTS	(sizeof(signal_table__local) / sizeof(Signal_Descriptor))


// Trying to eliminate:  signal_sysconsts_table_guts

// SELF_HOSTTHREAD is used in
//
//     arithmetic_fault_handler					// arithmetic_fault_handler	def in   src/c/machine-dependent/posix-arithmetic-trap-handlers.c
//
// to supply the SELF_HOSTTHREAD->task
// and           SELF_HOSTTHREAD->executing_mythryl_code
// values for handling
// a divide-by-zero or whatever.
//
// This is probably pretty BOGUS ON LINUX -- I think
// it means a divide-by-zero in any thread will always
// be reported as being in thread zero.
//
// (Can't we just map our pid to our hostthread_table__global entry,
// by linear scan if nothing else?  -- 2011-11-03 CrT)
// 
// 
#define SELF_HOSTTHREAD	(pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() ))			// Note that we still have   #define SELF_HOSTTHREAD	(hostthread_table__global[ 0 ])   in   src/c/machine-dependent/posix-arithmetic-trap-handlers.c


#ifdef USE_ZERO_LIMIT_PTR_FN
Punt		SavedPC;
extern		Zero_Heap_Allocation_Limit[];								// Actually a pointer, not an array.
#endif


static void   c_signal_handler   (/* int sig,  Signal_Handler_Info_Arg info,  Signal_Handler_Context_Arg* scp */);


void   pause_until_signal   (Hostthread* hostthread) {
    // ==================
    //
    // Suspend the given Hostthread
    // until a signal is received:
    pause ();												// pause() is a clib function, see pause(2).
}

void   set_signal_state   (Hostthread* hostthread,  int sig_num,  int signal_state) {			// This fn is called (only) from   src/c/lib/signal/setsigstate.c
    // ================											// Gets bound as   set_signal_state   in   src/lib/std/src/nj/interprocess-signals-guts.pkg
    //
    // QUESTIONS:
    //
    // If we disable a signal that has pending signals,
    // should the pending signals be discarded?


    // Docs specify that SIGKILL and SIGSTOP are not allowed:

    #ifdef SIGKILL
    if (sig_num == SIGKILL)		return;								// This saves special-casing at the Mythryl level.
    #endif

    #ifdef SIGSTOP
    if (sig_num == SIGSTOP)		return;								// "						".
    #endif

    // We handle SIGFPE and SIGSEGV at the C level, so silently ignore
    // attempts to fiddle with them from the Mythryl level:
    //
    #ifdef SIGFPE
    if (sig_num == SIGFPE)		return;								// "						".
    #endif
    //
    #ifdef SIGSEGV
    if (sig_num == SIGSEGV)		return;								// "						".
    #endif

    if (!signal_table__local[ sig_num ].host_os_id_for_signal)  return;					// Signal is not implemented by host operating system, so just silently ignore it.

    switch (signal_state) {
	//
    case LIB7_SIG_IGNORE:
	SELECT_SIG_IGN_HANDLING_FOR_SIGNAL ( portable_signal_id_to_host_os_signal_id( sig_num ));
	break;

    case LIB7_SIG_DEFAULT:
	SELECT_SIG_DFL_HANDLING_FOR_SIGNAL( portable_signal_id_to_host_os_signal_id( sig_num ));
	break;

    case LIB7_SIG_ENABLED:
	SET_SIGNAL_HANDLER(  portable_signal_id_to_host_os_signal_id( sig_num ),   c_signal_handler  );	// SET_SIGNAL_HANDLER 	#define in   src/c/machine-dependent/system-dependent-signal-get-set-etc.h
	break;												// SET_SIGNAL_HANDLER	expands to sigaction() or sigvec().

    default:
	die ("bogus signal state: sig = %d, state = %d\n",
	    sig_num, signal_state);
    }

}


int   get_signal_state   (Hostthread* hostthread,  int sig_num)   {					// Called from   src/c/lib/signal/getsigstate.c
    //================											// Called from   src/c/machine-dependent/interprocess-signals.c
    //													// Called form   src/c/machine-dependent/win32-signal.c

    if ((!signal_table__local[ sig_num ].host_os_id_for_signal)) {					// If signal is not supported by host operating system.
	//
	return LIB7_SIG_DEFAULT;									// This saves special-casing at the Mythryl level.
    }

    //
    {   void    (*handler)();
	//

	// Docs specify that SIGKILL and SIGSTOP are not allowed:
	//
	#ifdef SIGKILL
	if (sig_num == SIGKILL)		return LIB7_SIG_DEFAULT;					// This saves special-casing at the Mythryl level.
	#endif
	//
	#ifdef SIGSTOP
	if (sig_num == SIGSTOP)		return LIB7_SIG_DEFAULT;					// "						".
	#endif

	// We handle SIGFPE and SIGSEGV at the C level, so silently ignore
	// attempts to fiddle with them from the Mythryl level:
	//
	#ifdef SIGFPE
	if (sig_num == SIGFPE)		return LIB7_SIG_DEFAULT;					// "						".
	#endif
	//
	#ifdef SIGSEGV
	if (sig_num == SIGSEGV)		return LIB7_SIG_DEFAULT;					// "						".
	#endif

	GET_SIGNAL_HANDLER(   portable_signal_id_to_host_os_signal_id( sig_num ),   handler   );								// Store it into 'handler'.
	//												// GET_SIGNAL_HANDLER	is from   src/c/h/system-dependent-signal-get-set-etc.h
	if      (handler == SIG_IGN)	return LIB7_SIG_IGNORE;						// GET_SIGNAL_HANDLER	expands to sigaction() or sigvec().
	else if (handler == SIG_DFL)	return LIB7_SIG_DEFAULT;
	else				return LIB7_SIG_ENABLED;
    }
}


#if defined(HAS_POSIX_SIGS) && defined(HAS_UCONTEXT)							// This is the version which is used on Linux and consequently is well-tested.

// In this case    src/c/h/system-dependent-signal-get-set-etc.h
// set c_signal_handler up to be registered as a signal handler
// via sigaction with the SA_SIGINFO flag set, which according to
// man sigaction(2) means:
//
//     If  SA_SIGINFO  is  specified in sa_flags, then sa_sigaction (instead
//     of sa_handler) specifies the signal-handling function for signum.
//     This function receives the signal number as its first argument,
//     a pointer to a siginfo_t as its second argument and
//     a pointer to a ucontext_t (cast to void *) as its third argument.
//
// where
//     The siginfo_t argument to sa_sigaction is a struct with the following elements:
//
//         siginfo_t {
//             int      si_signo;    /* Signal number */
//             int      si_errno;    /* An errno value */
//             int      si_code;     /* Signal code */
//             int      si_trapno;   /* Trap number that caused hardware-generated signal (unused on most architectures) */
//             pid_t    si_pid;      /* Sending process ID */
//             uid_t    si_uid;      /* Real user ID of sending process */
//             int      si_status;   /* Exit value or signal */
//             clock_t  si_utime;    /* User time consumed */
//             clock_t  si_stime;    /* System time consumed */
//             sigval_t si_value;    /* Signal value */
//             int      si_int;      /* POSIX.1b signal */
//             void    *si_ptr;      /* POSIX.1b signal */
//             int      si_overrun;  /* Timer overrun count; POSIX.1b timers */
//             int      si_timerid;  /* Timer ID; POSIX.1b timers */
//             void    *si_addr;     /* Memory location which caused fault */
//             long     si_band;     /* Band event (was int in glibc 2.3.2 and earlier) */
//             int      si_fd;       /* File descriptor */
//             short    si_addr_lsb; /* Least significant bit of address since kernel 2.6.32) */
//           }
//
// ucontext_t is defined in <ucontext.h>
// man getcontext(2) says:


static int milliseconds_between_ramlog_and_syslog_dumps = -1;		// -1 == not initialized, 0 == don't dump.
static int milliseconds_since_last_ramlog_and_syslog_dump = 0;

static void   c_signal_handler   (int host_os_signal_id,  siginfo_t* si,  void* c)   {
    //        ================
    //
    // This is the C signal handler for
    // signals that are to be passed to
    // the Mythryl level via signal_handler in
    //
    //     src/lib/std/src/nj/interprocess-signals-guts.pkg
    //

    int sig =  host_os_signal_id_to_portable_signal_id( host_os_signal_id ); 

    ucontext_t* scp		/* This variable is unused on some platforms, so suppress 'unused var' compiler warning: */   __attribute__((unused))
        =
        (ucontext_t*) c;

    Hostthread* hostthread = SELF_HOSTTHREAD;

// Commented out because repeat entries were flooding the syscall_log.
// Should either include a dup count or just ignore consecutive
// identical syscall_log entries.
										Task* task =  hostthread->task;
//										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    // Sanity check:  We compile in a SIGNAL_TABLE_SIZE_IN_SLOTS value but
    // have no way to ensure that we don't wind up getting run
    // on some custom kernel supporting more than SIGNAL_TABLE_SIZE_IN_SLOTS,
    // so we check here to be safe:
    //
    if ((unsigned)sig >= SIGNAL_TABLE_SIZE_IN_SLOTS)    die ("interprocess-signals.c: c_signal_handler: sig d=%d >= SIGNAL_TABLE_SIZE_IN_SLOTS %d\n", sig, SIGNAL_TABLE_SIZE_IN_SLOTS ); 


if (host_os_signal_id == SIGINT)  ramlog_printf("#%d c_signal_handler(%d==SIGINT)\n", syscalls_seen, host_os_signal_id );


    /////////////////////////////////// begin kludge ////////////////////////////////////
    // This is a little kludge because I'm getting unexpected compiler lockups
    // during the serial -> concurrent programming paradigm transition, and I'd
    // like to look at the ramlog and syscall log but attaching gdb doesn't work.
    // (It may be confused by our 8K Mythryl stackframe.)  The idea is just to
    // dump the syscall log and ramlog every five seconds or so, which should be
    // frequent enough to quell impatience but infrequent enough not to add
    // significant overhead to compiles:  -- 2012-10-15 CrT
    //
    // Upgraded to allow control via the environment var
    //     MILLISECONDS_BETWEEN_RAMLOG_AND_SYSLOG_DUMPS
    //   -- 2012-10-18 CrT
    //
    if (milliseconds_between_ramlog_and_syslog_dumps < 0) {
	//
	char* t;
	if (!(t = getenv("MILLISECONDS_BETWEEN_RAMLOG_AND_SYSLOG_DUMPS"))) {
	    //
	    milliseconds_between_ramlog_and_syslog_dumps = 0;					// Env var not set, so let's not do this stuff.
	} else {
	    int ms = atoi(t);
	    if (ms < 0)   	    milliseconds_between_ramlog_and_syslog_dumps = 0;
	    else if (ms < 100)	    milliseconds_between_ramlog_and_syslog_dumps = 100;		// Let's not try dumps every millisecond.
	    else		    milliseconds_between_ramlog_and_syslog_dumps = ms;
	}
    }
    if (milliseconds_between_ramlog_and_syslog_dumps > 0) {
	//
	milliseconds_since_last_ramlog_and_syslog_dump += 20;					// I'm assuming the usual 50Hz SIGALRM. Yes, this is fragile and hacky,
												// but this is just a nonce debugging hack anyhow. Feel free to improve it.
	if (milliseconds_since_last_ramlog_and_syslog_dump > milliseconds_between_ramlog_and_syslog_dumps) {
	    milliseconds_since_last_ramlog_and_syslog_dump = 0;
	    //
	    dump_ramlog     (task,"c_signal_handler");
	    dump_syscall_log(task,"c_signal_handler");
	}
    }
    //
    ///////////////////////////////////   end kludge ////////////////////////////////////



    // Remember that we have seen signal number 'sig'.
    //
    // This will eventually get noticed by  choose_signal()  (below)
    //
    ++ hostthread->posix_signal_counts[ sig ].seen_count;
    ++ hostthread->all_posix_signals.seen_count;

//    log_if(
//        "interprocess-signals.c/c_signal_handler: signal d=%d  seen_count d=%d  done_count d=%d   diff d=%d",
//        sig,
//        hostthread->posix_signal_counts[sig].seen_count,
//        hostthread->posix_signal_counts[sig].done_count,
//        hostthread->posix_signal_counts[sig].seen_count - hostthread->posix_signal_counts[sig].done_count
//    );

    #ifdef SIGNAL_DEBUG
    debug_say ("c_signal_handler: sig = %d, pending = %d, inHandler = %d\n", sig, hostthread->interprocess_signal_pending, hostthread->mythryl_handler_for_interprocess_signal_is_running);
    #endif

    // The following line is needed only when
    // currently executing "pure" C code, but
    // doing it anyway in all other cases will
    // not hurt:
    //
    hostthread->ccall_limit_pointer_mask = 0;

    if (  hostthread->executing_mythryl_code
    &&  ! hostthread->interprocess_signal_pending
    &&  ! hostthread->mythryl_handler_for_interprocess_signal_is_running
    ){
	hostthread->interprocess_signal_pending = TRUE;

	// The purpose of the following logic is to ensure that the TRUE
	//    hostthread->interprocess_signal_pending
	// flag set above gets noticed as quickly as practical, by making
	// the heap-memory-low probe fire prematurely:
	//
	#ifdef USE_ZERO_LIMIT_PTR_FN
	    //									// We don't use this approach currently; if we start using it again we need to check for possiblity of different hostthreads clobbering shared global storage. -- 2012-10-11 CrT
	    SIG_SavePC( hostthread->task, scp );
	    SET_SIGNAL_PROGRAM_COUNTER( scp,  );
	#else
	    ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER( scp );		// OK to adjust the heap limit directly.
	#endif									// ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER	is from   src/c/h/system-dependent-signal-get-set-etc.h
    }
//										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
}

#else										// This version is not used on Linux and consequently may have bit-rot at this point.   -- 2012-12-22 CrT

static void   c_signal_handler   (
    //
    int		    host_os_signal_id,
    #if (defined(TARGET_PWRPC32) && defined(OPSYS_LINUX))
	Signal_Handler_Context_Arg*   scp
    #else
	Signal_Handler_Info_Arg	info,
	Signal_Handler_Context_Arg*   scp
    #endif
){
    int sig =  host_os_signal_id_to_portable_signal_id( host_os_signal_id ); 

    #if defined(OPSYS_LINUX)  &&  defined(TARGET_INTEL32)  &&  defined(USE_ZERO_LIMIT_PTR_FN)
	//
	Signal_Handler_Context_Arg*  scp =  &sc;			// I find no trace of 'sc'; this looks like some ancient hack mostly bitrotted away.  -- 2012-10-11 CrT
    #endif

    Hostthread*  hostthread =  SELF_HOSTTHREAD;
    Task* task =  hostthread->task;
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    ++hostthread->posix_signal_counts[ sig ].seen_count;
    ++hostthread->all_posix_signals.seen_count;

    #ifdef SIGNAL_DEBUG
    debug_say ("c_signal_handler: sig = %d, pending = %d, inHandler = %d\n",
    sig, hostthread->interprocess_signal_pending, hostthread->mythryl_handler_for_interprocess_signal_is_running);
    #endif

    // The following line is needed only when
    // currently executing "pure" C code, but
    // doing it anyway in all other cases will
    // not hurt:
    //
    hostthread->ccall_limit_pointer_mask = 0;

    if (  hostthread-> executing_mythryl_code
    && (! hostthread-> interprocess_signal_pending)
    && (! hostthread-> mythryl_handler_for_interprocess_signal_is_running)
    ){
        //
	hostthread->interprocess_signal_pending =  TRUE;

	// The purpose of the following logic is to ensure that the TRUE
	//    hostthread->interprocess_signal_pending
	// flag set above gets noticed as quickly as practical, by making
	// the heap-memory-low probe fire prematurely:
	//
	#ifdef USE_ZERO_LIMIT_PTR_FN	
	    //													// We don't use this approach currently; if we start using it again we need to check for possiblity of different hostthreads clobbering shared global storage. -- 2012-10-11 CrT
	    SIG_SavePC( hostthread->task, scp );
	    SET_SIGNAL_PROGRAM_COUNTER( scp, Zero_Heap_Allocation_Limit );
	#else
	    ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER( scp );						// OK to adjust the heap limit directly.
	#endif
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
}

#endif


// I'm inclined to think it would faster, simpler and more portable
// to implement signal masking at the application level via an
// task->hostthread.signal_masks[ SIGNAL_TABLE_SIZE_IN_SLOTS ]
// bytevector. -- 2012-12-16 CrT  XXX SUCKO FIXME

void   set_signal_mask   (Task* task, Val arg)   {								// We are called (only) by   src/c/lib/signal/setsigmask.c
    // ===============												// We get bound as   set_sig_mask   in   src/lib/std/src/nj/interprocess-signals-guts.pkg
    // 
    // Set the signal mask to the list of signals given by 'arg'.
    // The signal_list has the type
    //
    //     Null_Or( List( Int ) )
    //
    // with the following semantics -- see src/lib/std/src/nj/interprocess-signals.pkg
    //
    //	NULL	-- the empty mask
    //	THE[]	-- mask all signals
    //	THE l	-- the signals in l are the mask
    //

    //									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Signal_Set	mask;												// Signal_Set			is from   src/c/h/system-dependent-signal-get-set-etc.h

    CLEAR_SIGNAL_SET(mask);											// CLEAR_SIGNAL_SET		is from   src/c/h/system-dependent-signal-get-set-etc.h
														// On posix it is a wrapper for sigemptyset() from   SIGSETOPS(3)
    Val signal_list  = arg;
    if (signal_list != OPTION_NULL) {
	signal_list  = OPTION_GET( signal_list );

	if (LIST_IS_NULL(signal_list)) {
	    //
	    // THE [] -- mask all signals
            //
	    for (int portable_signal_id = 0;  portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;  ++portable_signal_id) {
	        //
;		ADD_SIGNAL_TO_SET( mask, signal_table__local[ portable_signal_id ].host_os_id_for_signal );	// ADD_SIGNAL_TO_SET			is from   src/c/h/system-dependent-signal-get-set-etc.h
	    }													// On posix it is a wrapper for sigaddset() from   SIGSETOPS(3)

	} else {

	    while (signal_list != LIST_NIL) {
		//
		Val car =  LIST_HEAD( signal_list );
		//
		int portable_signal_id =  TAGGED_INT_TO_C_INT( car );
		//
		ADD_SIGNAL_TO_SET( mask, portable_signal_id_to_host_os_signal_id( portable_signal_id ) );
		//
		signal_list = LIST_TAIL( signal_list );
	    }
	}
    }

    // Do the actual host OS syscall
    // to change the signal mask.
    // This is our only invocation of this syscall:
    //
//  log_if("interprocess-signals.c/set_signal_mask: setting host signal mask for process to x=%x", mask );	// Commented out because it floods mythryl.compile.log -- 2011-10-10 CrT
    //
    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	SET_PROCESS_SIGNAL_MASK( mask );									// SET_PROCESS_SIGNAL_MASK		is from   src/c/h/system-dependent-signal-get-set-etc.h
	//													// On posix it is a wrapper for sigprocmask() -- see  SIGPROGMASK(2)
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
    //									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
}


Val   get_signal_mask__may_heapclean   (Task* task,   Val arg,   Roots* extra_roots)   {			// Called from src/c/lib/signal/getsigmask.c
    //==============================
    // 
    // 'arg' is unused.
    // 
    // Return the current signal mask (only those signals supported by Mythryl);
    // like set_signal_mask, the result has the following semantics:
    //	NULL	-- the empty mask
    //	THE[]	-- mask all signals
    //	THE l	-- the signals in l are the mask

    //									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Signal_Set	mask;
    int		portable_signal_id;
    int		n;
    int		total_signals = 0;

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	GET_PROCESS_SIGNAL_MASK( mask );									// GET_PROCESS_SIGNAL_MASK		is from   src/c/h/system-dependent-signal-get-set-etc.h
	//													// On posix it is a wrapper for sigprocmask() -- see  SIGPROGMASK(2)
	// Count the number of masked signals:
	//
	for (portable_signal_id = 0, n = 0;  portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;  portable_signal_id++) {
	    //
	    if (signal_table__local[ portable_signal_id ].host_os_id_for_signal) {							// If signal is implemented on host OS.
		//
		if (SIGNAL_IS_IN_SET(mask, signal_table__local[ portable_signal_id ].host_os_id_for_signal))   n++;

		++total_signals;
	    }
	}
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (n == 0)   return OPTION_NULL;

    Val	signal_list;

    if (n == total_signals) {
	//
	signal_list = LIST_NIL;
	//
    } else {
	//
        Roots roots1 = { &signal_list, extra_roots };
	//
	for (portable_signal_id = 0, signal_list = LIST_NIL;   portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;   portable_signal_id++) {

	    // If our agegroup0 buffer is more than half full,
	    // empty it by doing a heapcleaning.  This is very
	    // conservative -- which is the way I like it. :-)
	    //
	    if (agegroup0_freespace_in_bytes( task )
	      < agegroup0_usedspace_in_bytes( task )
	    ){
		call_heapcleaner_with_extra_roots( task,  0, &roots1 );
	    }

	    if (signal_table__local[ portable_signal_id ].host_os_id_for_signal) {						// If signal is implemented on host OS.
		//
		if (SIGNAL_IS_IN_SET(mask, signal_table__local[ portable_signal_id ].host_os_id_for_signal)) {
		    //
		    signal_list = LIST_CONS(task, TAGGED_INT_FROM_C_INT( portable_signal_id ), signal_list);
		}
	    }
	}
    }

    Val result =  OPTION_THE( task, signal_list );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


void   choose_signal   (Hostthread* hostthread)   {				// We are called (only) from   src/c/main/run-mythryl-code-and-runtime-eventloop.c
    // =============
    // 
    // Caller guarantees that at least one Unix signal has been
    // seen at the C level but not yet handled at the Mythryl
    // level.  Our job is to find and return the number of
    // that signal plus the number of times it has fired at
    // the C level since last being handled at the Mythryl level.
    //
    // Choose which signal to pass to the Mythryl-level handler
    // and set up the Mythryl state vector accordingly.
    //
    // This function gets called (only) from
    //
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //
    // WARNING: This should be called with signals masked
    // to avoid race conditions.

    int delta;

    // Scan the signal counts looking for
    // a signal that needs to be handled.
    //
    // The 'seen_count' field for a signal gets
    // incremented once for each incoming signal
    // in   c_signal_handler()   in
    //
    //     src/c/machine-dependent/interprocess-signals.c
    //
    // Here we increment the matching 'done_count' field
    // each time we invoke appropriate handling for that
    // signal;  thus, the difference between the two
    // gives the number of pending instances of that signal
    // currently needing to be handled.
    //
    // For fairness we scan for signals round-robin style, using
    //
    //     hostthread->posix_signal_rotor
    //
    // to remember where we left off scanning, so we can pick
    // up from there next time:	

    #if NEED_TO_EXECUTE_ASSERTS
    int j = 0;
    #endif

    int portable_signal_id = hostthread->posix_signal_rotor;
    do {
	ASSERT (j++ < SIGNAL_TABLE_SIZE_IN_SLOTS);				// At worst we have to search all the way through hostthread->posix_signal_counts[SIGNAL_TABLE_SIZE_IN_SLOTS].

	++portable_signal_id;

	// Wrap circularly around the signal vector:
	//
	if (portable_signal_id == SIGNAL_TABLE_SIZE_IN_SLOTS)			// SIGNAL_TABLE_SIZE_IN_SLOTS		is from   src/c/h/runtime-base.h
	    portable_signal_id =  0;						// 

	// Does this signal have pending work? (Nonzero == "yes"):
	//
	delta = hostthread->posix_signal_counts[ portable_signal_id ].seen_count
              - hostthread->posix_signal_counts[ portable_signal_id ].done_count;

    } while (delta == 0);

    hostthread->posix_signal_rotor = portable_signal_id;			// Next signal to scan on next call to this fn.

    // Record the signal to process
    // and how many times it has fired
    // since last being handled at the
    // Mythryl level:
    //
    hostthread->next_posix_signal_id    = portable_signal_id;
    hostthread->next_posix_signal_count = delta;
if (portable_signal_id) ramlog_printf("#%d choose_signal:  portable_signal_id == SIGINT\n", syscalls_seen );


//    log_if(
//        "signal-stuff.c/choose_signal: signal d=%d  seen_count d=%d  done_count d=%d   diff d=%d",
//        i,
//        hostthread->posix_signal_counts[portable_signal_id].seen_count,
//        hostthread->posix_signal_counts[portable_signal_id].done_count,
//        hostthread->posix_signal_counts[portable_signal_id].seen_count - hostthread->posix_signal_counts[portable_signal_id].done_count
//    );

    // Mark this signal as 'done':
    //
    hostthread->posix_signal_counts[ portable_signal_id ].done_count  += delta;
    hostthread->all_posix_signals.done_count                          += delta;

    #ifdef SIGNAL_DEBUG
        debug_say ("choose_signal: sig = %d, count = %d\n", hostthread->next_posix_signal_id, hostthread->next_posix_signal_count);
    #endif
}

void   clear_signal_counts   (Hostthread* hostthread)   {			// We are called (only) from   src/c/main/runtime-state.c
    // ===================

    for (int portable_signal_id = 0;  portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;  ++portable_signal_id) {
	//
	hostthread->posix_signal_counts[ portable_signal_id ].seen_count =  0;
	hostthread->posix_signal_counts[ portable_signal_id ].done_count =  0;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// Conversion between portable and host-os signal ids
//
// The idea here is to promote portability by as much as possible using
// a cross-platform-consistent set of signal numbers defined by both
//     signal_to_int / int_to_signal	in   src/lib/std/src/nj/interprocess-signals-guts.pkg
// and signal_table__local above (as the row numbers).
//
// This way things will work portabily if signal ids get stored in
// a Mythryl heap image or (godz forbid) even a Mythryl sourcefile.
//
// Also, anyone who blithely assumes that SIGKILL == 9 will get the
// expected behavior even on mutant platforms/kernels where it does not.
//
// And Mythryl source code will see exactly the same set of signals
// defined on every platform, independent of the set of signals
// actually supported by the host OS.  This reduces the need for
// icky platform-dependent conditional-compilation hacks at the
// Mythryl source level. (It also avoids consequent portability
// problems with coders who might otherwise forget to do such hacks).
//
// Coders who want to know at the Mythryl level if a particular signal
// is supported by the current host OS can call
//
//     interprocess_signals::signal_is_supported_by_host_os
//
// We translate from our portable signal ids to platform- and
// kernel-specific signal ids only at the point of actually
// making a system call.  This minimizes the chances of bugs
// creeping in due to platform- or kernel-dependent differences
// in signal ids.  For example if some crazy kernel assigns
// SIGHUP a value of 10009, we won't over-index some array
// and clobber RAM.


int    portable_signal_id_to_host_os_signal_id   (int portable_signal_id)   {
    // =======================================
    //
    return   signal_table__local[ portable_signal_id ].host_os_id_for_signal;
}


int    host_os_signal_id_to_portable_signal_id   (int host_os_signal_id)   {
    // =======================================
    //
    for (int portable_signal_id = 0;  portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;  ++portable_signal_id) {
	//
	if (signal_table__local[ portable_signal_id ].host_os_id_for_signal == host_os_signal_id) {
	    //
	    return portable_signal_id;
	}
    }

    return  0;												// 0 will never be a valid portable signal id, so this is unambiguously an error return.
}


int    ascii_signal_name_to_portable_signal_id   (char* ascii_signal_name)   {				// 
    // =======================================								// This fn is intended purely as 'make check' support for use by   src/lib/std/src/nj/interprocess-signals-unit-test.pkg
    //
    for (int portable_signal_id = 0;  portable_signal_id < SIGNAL_TABLE_SIZE_IN_SLOTS;  ++portable_signal_id) {
	//
	if (!strcmp( signal_table__local[ portable_signal_id ].signal_h__name_for_signal, ascii_signal_name)) {
	    //
	    return portable_signal_id;
	}
    }

    return  0;												// 0 will never be a valid portable signal id, so this is unambiguously an error return.
}

int    maximum_valid_portable_signal_id   (void)   {							// 
    // ================================									// This fn is intended purely as 'make check' support for use by   src/lib/std/src/nj/interprocess-signals-unit-test.pkg
    //
    return   SIGNAL_TABLE_SIZE_IN_SLOTS - 1;
}


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.


