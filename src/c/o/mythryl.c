// mythryl.c, a small shebang wrapper for mythryl scripts.

// Created 2007-03-12 CrT.
// First successful run:   Lib7.110.58 [built: Tue Mar 13 13:19:31 2007]     Do   bin/mythryld -h   for help, <Ctrl>-D to quit. */

//
// For security reasons, most *nix operating systems will not
// allow a script interpreter to itself be a script:  If the
// shebang line in the script is "#!/usr/bin/foo", then
// /usr/bin/foo must be a compiled executable, typically written
// in C.
//
// This poses a mild problem for us, because the Mythryl compiler
// is in fact not a host executable image, but rather a heap dump
// in a Mythryl-specific format the host OS knows nothing about,
// which gets invoked by a "#/usr/bin/mythryl-runtime-intel32" shebang line at
// the top.  This makes our compiler logically a script as far
// as the host OS is concerned, which means that we can't run
// mythryl scripts by having their shebang lines point directly
// to the compiler image.
//
// That's where the following code comes in.  It provides a small
// C wrapper which may be referenced by Mythryl scripts via a
// "#!/usr/bin/mythryl" shebang line at the top, and which
// will then invoke the compiler image proper.
//
// Here are two references on shebang invocation:
//
//     http://homepages.cwi.nl/~aeb/std/hashexclam.html
//     http://en.wikipedia.org/wiki/Shebang_(Unix)



/*
###			"The best way to solve a hard problem
###			 is to convert it into an easy problem."
*/



#include "../mythryl-config.h"

#include <stdio.h>		// For fopen() etc.
#include <stdlib.h>		// For exit().
#include <signal.h>		// For sigaction(), kill().

#if HAVE_SYS_SELECT_H
    #include <sys/select.h>	// For select().
#endif

#include <string.h>		// For strlen(), strerror().
#include <errno.h>		// For errno().

#if HAVE_SYS_WAIT_H
    #include <sys/wait.h>	// For waitpid().
#endif

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>	// For stat(), getuid(), fork(), waitpid(), kill().
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>	// For stat().
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>		// For stat(), getuid(), fork(), dup(), pipe(), sleep()
#endif

#include "mythryl-executable.h"

#ifndef TRUE
    #define TRUE (1)
#endif

#ifndef FALSE
    #define FALSE (0)
#endif


#define DEBUG 0

#if DEBUG
    static FILE* log_fd = NULL;

    static void   open_logfile   (void)   {
	//
	log_fd = fopen("mythryl.c.log~", "a");
	fprintf(log_fd,"open_logfile: STDIN_FILENO d=%d STDOUT_FILENO d=%d STDERR_FILENO d=%d\n",STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO); fflush(log_fd);
    }

    static void   close_logfile   (void)   {
	//
	fprintf(log_fd,"src/c/o/mythryl.c: close_logfile()\n");
	fclose(log_fd);
    }
#endif



#define SCRIPT_EXIT_BOILERPLATE "\n posix_1003_1b::kill (posix_1003_1b::K_PROC (posix_1003_1b::get_process_id' () ), posix_1003_1b::signal::term );;\n"
    //
    // #define SCRIPT_EXIT_BOILERPLATE "\n  winix::process::exit winix::process::success;\n"
    // #define SCRIPT_EXIT_BOILERPLATE "\n (winix::process::exit winix::process::success): Void;\n"
    //
    // See Note[2] at bottom of file.

//
static char* our_name          = "(unknown)";

// Once we have established communication with the
// compiler, we feed into its stdin the text which
// it is to compile and execute.
//
// The following hold that text until it is time
// to deliver it:
//
static char* text_for_compiler_buffer;			// Allocated below by    allocate_script_buffer().
static char* remaining_text_for_compiler;


//
static int   child_is_dead     = FALSE;
static int   child_exit_status = 0;
static pid_t child_pid         = 0;
//
static void   usage   (void)    {
    //
    fprintf( stderr, "%s should only be invoked via ''#!...'' line in a script!\n", our_name );
    //
    exit(1);
}


//
static int   get_script_size   (char* filename)   {
    //
    struct stat buf;
    //
    if (stat (filename, &buf)) {
	//
        fprintf( stderr, "%s: Unable to stat() %s: %s\n", our_name, filename, strerror( errno ) );
        exit(1);
    }
    //
    return buf.st_size;
}


//
static void
allocate_script_buffer   ( int bytesize ) {
    //
    if (!bytesize)         exit(0);
    //
    text_for_compiler_buffer
        =
        (char*) malloc( bytesize + strlen( SCRIPT_EXIT_BOILERPLATE ) + 2 );	// "+2" for a newline and nul.

    if (!text_for_compiler_buffer) {
	//
        fprintf( stderr, "%s: Unable to allocate %d bytes for script buffer: %s\n", our_name, bytesize, strerror( errno ) );
        exit(1);
    }

    remaining_text_for_compiler = text_for_compiler_buffer;

    *text_for_compiler_buffer = '\0';
}


//
static void   read_script_into_buffer   (char* scriptname)   {
    //
    // Read script contents into buffer:

    FILE* fd =   fopen( scriptname, "r" );

    if (!fd) {
	//
        fprintf( stderr, "%s: Unable to open script %s: %s\n", our_name, scriptname, strerror( errno ) );
        exit(1);
    }

    {   char* p = text_for_compiler_buffer;
        int c;

        while ((c = fgetc(fd)) != -1) {

	    *p++ = c;
        }
        fclose( fd );

	*p++ = '\n';

        strcpy( p, SCRIPT_EXIT_BOILERPLATE );		// This will append a nul.
    }
}


//
static void   set_up_compile   (char** argv)   {
    //
    allocate_script_buffer(
	//
        get_script_size( argv[0] )
    );
    //
    read_script_into_buffer( argv[0] );
}


//
static int   is_executable   (char* filename)   {
    //
    struct stat buf;
    //
    if (stat (filename, &buf))   return FALSE;

    // If file is world-executable, we're ok:
    //
    if (buf.st_mode & S_IXOTH)   return TRUE;

    // If we own the file and it is owner-executable, we're ok:
    //
    if ((geteuid() == buf.st_uid)
        &&
        (buf.st_mode & S_IXUSR)
    ){
        return TRUE;
    }

    // Since nowadays a user can be in more than one
    // group, this must not be the right way to test.
    // XXX BUGGO FIXME
    // If we're in the file's group and it is group-executable, we're ok:
    //
    if ((getegid() == buf.st_gid)
        &&
        (buf.st_mode & S_IXGRP)
    ){
        return TRUE;
    }

    return FALSE;
}


//
static int   is_valid_script   (char* filename)   {
    //
    // Here we try to figure out whether 'filename'
    // might be the script whose shebang line invoked us.
    //
    if (!is_executable( filename ))   return FALSE;

    {   FILE* fd = fopen( filename, "r" );
	//
	if (!fd)   return FALSE;
	//
	{   int result = (   (fgetc(fd)=='#')
			     &&
			     (fgetc(fd)=='!')
			 );
	    fclose(fd);
	    return result;
	}
    }
}

//
static int   args_to_skip   (int argc,  char** argv)   {
    //
    if (argc > 0)   our_name = argv[0];

    // Suppose we were invoked by the user typing
    //
    //     sh/myscript x y z
    //
    // at the prompt, where sh/myscript is an executable
    // script whose first line reads
    //
    //     #!/usr/bin/mythryl a b c
    //
    // Shebang invocation is not well standardized,
    // but at that point we're hoping that argv[]
    // will look like so:
    //
    //     argv[0] == "/usr/bin/mythryl"
    //     argv[1] == "a b c"
    //     argv[2] == "sh/myscript"
    //     argv[3] == "x"
    //     argv[4] == "y"
    //     argv[5] == "z"
    //
    // But if the shebang line in question had no args, just
    //
    //     #!/usr/bin/mythryl
    //
    // then at this point we should be looking at just
    //
    //     argv[0] == "/usr/bin/mythryl"
    //     argv[1] == "sh/myscript"
    //     argv[2] == "x"
    //     argv[3] == "y"
    //     argv[4] == "z"
    //
    // (Obviously, this design sucks. It would be better if
    // the script name was always argv[2], and argv[1] was
    // set to "" or NULL in this case.  Oh well.)
    //
    // We'll distinguish the two by attempting to open
    // both argv[1] and argv[2].

    if (argc < 2) {
	// 
         fprintf (stderr, "%s: To run mythryl interactively do:   mythryld\n", our_name );
         exit(1);
    }
    if (argc == 2) {
	//
	if (!is_valid_script( argv[1] )) {
	    //
	    fprintf (stderr, "%s: '%s' is not a valid script!\n", our_name, argv[1] );
	    usage();
	}
	return 1;
    }

    // argc > 2

    {   int arg1_ok = is_valid_script( argv[1] );
	int arg2_ok = is_valid_script( argv[2] );

	if (!(arg1_ok || arg2_ok)) {
	    //
	    fprintf (stderr, "%s: Neither '%s' nor '%s' is a valid script!\n", our_name, argv[1], argv[2] );
	    usage();
	}

	if (arg1_ok && arg2_ok) {
	    //
	    fprintf (stderr, "%s: Both '%s' and '%s' look like valid scripts -- don't know which one to run!\n", our_name, argv[1], argv[2] );
	    usage();
	}

	if (arg1_ok)   return 1;
	else           return 2;       
    }
}


//
static char**   process_argv   (int argc,  char** argv )   {
    //
    // The first one or two argv[] entries
    // pertain to us rather than the script
    // we are to run, so note and drop them
    // to avoid confusing the script:
    //
    {   int to_skip = args_to_skip( argc, argv );
        //
        argc -= to_skip;
        argv += to_skip;
    }

    return argv;
}


//
static void   close_redundant_fds   (void)   {
    //
    // We might have been exec'd by some program with dozens
    // of fds open, but all we want is stdin/stdout/stderr,
    // so we take a shot at closing everything else:


    #if DEBUG
	fclose(log_fd);
    #endif

    for (int fd = 3;   fd < 100;   ++fd)   close( fd );

    #if DEBUG
	open_logfile ();
    #endif
}

// A record to track the six ends of the three pipes
// connecting us with our subprocess:
//
typedef struct { int read_fd;					// fd to read() on.
                 int write_fd;					// fd to write() on.
               } Pipe_Pair;
//
typedef struct { Pipe_Pair stdin_;
                 Pipe_Pair stdout_;
                 Pipe_Pair stderr_;
               } Stdin_Stdout_Stderr_Pipes;

//
static Pipe_Pair   make_pipe_pair   (void)   {
    //
    // To communicate with our subprocess, we need
    // three pipes, one each for its stdin, stdout,
    // stderr.  We create these using the following
    // fn, for each pipe later handing one end to
    // our subprocess and keeping the other for
    // ourself.

    Pipe_Pair pp;

    int fd_pair[2];

    if (pipe(fd_pair)) {
        fprintf(stderr,"%s: Unable to create pipe: %s\n", our_name, strerror(errno) );
        exit(1);
    }

    // In practice, we could probably pass pipe()
    // a Pipe_Pair in place of the specified fd_pair,
    // but it is better to do it cleanly.

    pp.read_fd  = fd_pair[0];
    pp.write_fd = fd_pair[1];

    return pp;
}


//
static Stdin_Stdout_Stderr_Pipes   open_subprocess_pipes   (void)   {
    //
    // Create the three pipes (stdin/stdout/stderr)
    // needed to communicate with our subprocess:

    Stdin_Stdout_Stderr_Pipes  pipes;

    pipes.stdin_   = make_pipe_pair ();
    pipes.stdout_  = make_pipe_pair ();
    pipes.stderr_  = make_pipe_pair ();

    return pipes;
}


//
static void   copy_pipe   (int from, int to)   {
    //
    if (dup2( from, to ) != to) {
	//
        fprintf(stderr,"%s: Unable to copy pipe: %s\n", our_name, strerror(errno) );
        exit(1);
    }
}





//
static void   sigchld_handler   (int signal,  siginfo_t* info,  void* context)   {
    //
    // The SIGCHLD signal notifies us of a state change
    // in our child process, the compiler image proper.
    //
    // If our child has exited, we should too, with the
    // same exit status.
    //
    // We currently ignore all other cases.

    // Find out what the child state change was:

    int status  = 0;

//  pid_t child
//      =
        waitpid(
            child_pid,
            &status,
            0
                        #ifdef WNOHANG
            | WNOHANG
                        #endif
        );

    if (WIFEXITED(status)) {
	//
        // We can't exit now because there may be
        // stdout/stderr output from the child waiting
        // to be drained, so we merely set a flag:

        child_is_dead     =   TRUE;

        child_exit_status =   WEXITSTATUS (status);
    }
}


static int   kernel_thinks_child_is_dead   (void) {
    //       ===========================
    //
    for (;;) {
	//
	int                          child_status;
	int i = waitpid( child_pid, &child_status, WNOHANG );

	switch (i) {
	//
	case 0:	return FALSE;
			//
			// "If waitpid() was invoked with WNOHANG set in options,
			//  and there are children specified by pid for which
			// status is not available, waitpid() returns 0."
			//    -- http://www.mkssoftware.com/docs/man3/waitpid.3.asp  
	    
	case -1:
	    switch (errno) {
	    case EINTR:		continue;	// waitpid() was interrupted -- retry it.	
	    case ECHILD:	return TRUE;	// Child is dead:
						//     "The process or process group specified by pid does not exist
						//      or is not a child of the calling process."
						//         -- http://www.mkssoftware.com/docs/man3/waitpid.3.asp 

	    // These cases should not be possible:	
	    //
	    case EFAULT: fprintf(stderr,"%s: Received EFAULT   in waitpid(?!), exit(1)ing\n",our_name      );	exit(1);	// "stat_loc is not a writable address."
	    case EINVAL: fprintf(stderr,"%s: Received EINVAL   in waitpid(?!), exit(1)ing\n",our_name      );	exit(1);	// "The options argument is not valid."
	    case ENOSYS: fprintf(stderr,"%s: Received ENOSYS   in waitpid(?!), exit(1)ing\n",our_name      );	exit(1);	// "pid specifies a process group (0 or less than -1), which is not currently supported."
	    default:     fprintf(stderr,"%s: Received errno %d in waitpid(?!), exit(1)ing\n",our_name,errno);	exit(1);	// "pid specifies a process group (0 or less than -1), which is not currently supported."
	    };
	    break;


	default:
	    if (i == child_pid) {
		//
		if (WIFEXITED(   child_status))   return TRUE;				// "Evaluates to a non-zero value if status was returned for a child process that exited normally." 
		if (WIFSIGNALED( child_status))	  return TRUE;				// "Evaluates to a non-zero value if status was returned for a child process that terminated due to receipt of a signal that was not caught."
		if (WIFSTOPPED(  child_status))	  return FALSE;				// "Evaluates to a non-zero value if status was returned for a child process that is currently stopped."
		return FALSE;								// I do not think we are supposed to be able to get here; I think the above three tests are intended to be exhaustive.
											// Returning FALSE here will make sigterm_handler() attempt to kill child, which seems the safest default action.
	    } else {
		fprintf(stderr,"%s: Received unexpected return value %d from waitpid(?!), exit(1)ing\n", our_name, i);	exit(1);
	    }
	}
    }
}

// Handle ^C:
// 
static void   sigterm_handler   (int signal,  siginfo_t* info,  void* context)   {
    //        ===============
    //
    // We want to shut our child process down cleanly too,
    // preferably as though it had gotten a ^C:

    // Don't try to kill our child if it is already dead:
    //
    if (child_is_dead)                   exit(1);
    if (kernel_thinks_child_is_dead())   exit(1);

    // Ask child nicely to shut down:
    //
    kill( child_pid, SIGTERM );

    // Give child plenty of time to save to
    // disk and exit even on a busy system.
    // Normally, we expect to be woken from
    // this sleep by a SIGCHLD signal that
    // our child has terminated, and thus
    // to never return from this call:
    sleep( 30 );

    // Kill child, not nicely:
    //
    kill( child_pid, SIGKILL );
    sleep( 30 );

    // There may be some possible race condition which allows
    // us to arrive here with child dead, so recheck before
    // complaining to user:
    //
    if (kernel_thinks_child_is_dead())   exit(1);

    fprintf(stderr,"%s: Unable to kill child in response to SIGTERM(?!), exit(1)ing\n",our_name);

    exit(1);
}


//
static void   set_signal_handler   (int signum,  void (*handler)(int signum, siginfo_t* info, void* context))   {
    //
    // The classic signal() is deprecated
    // in favor of the newer sigaction():

    struct sigaction s;

    s.sa_sigaction = handler;

    // Signals to mask out during processing of this signal (none):
    //
    if (sigemptyset( &s.sa_mask )) {
	//
        fprintf(stderr,"%s: Unable to create empty signal set: %s\n", our_name, strerror(errno) );
        exit(1);
    }

    s.sa_flags   = SA_SIGINFO
					#ifdef SA_NOCLDSTOP
                 | SA_NOCLDSTOP		// Don't notify us when child gets start or stop signals.
					#endif

        ;


    if (sigaction( signum, &s, NULL )) {
	//
        fprintf(stderr,"%s: Unable to set handler for signal %d: %s\n", our_name, signum, strerror(errno) );
	exit(1); 
    }
}


//
static void   set_signal_handlers   (void)   {
    //
    // We catch SIGCHLD so as to know when/if our
    // subprocess (the compiler process proper) exits:
    //
    set_signal_handler( SIGCHLD, sigchld_handler );

    // For now, at least, handle the other major
    // signals like ^C, by killing child and exiting:
    //
    set_signal_handler( SIGTERM, sigterm_handler );
    set_signal_handler( SIGPIPE, sigterm_handler );
    set_signal_handler( SIGQUIT, sigterm_handler );
}

//
static void   exec_mythryld   (char** argv) {
    //
    extern char** environ;
    //
    if (execve( MYTHRYL_EXECUTABLE, argv, environ )) {
	//
	fprintf(stderr,"%s: Unable to execve( %s, ... ): %s\n", our_name, MYTHRYL_EXECUTABLE, strerror(errno) );
	exit(1); 

    } else {
	
	// Execution should never reach this point.
        //
	fprintf(stderr,"%s: execve( %s, ... ) RETURNED ZERO?!!\n", our_name, MYTHRYL_EXECUTABLE );
	exit(1);
    }
}


//
static void   start_subprocess (
    //
    Stdin_Stdout_Stderr_Pipes   subprocess_pipes,
    char**                      argv
){
    if (! is_executable( MYTHRYL_EXECUTABLE )) {
        //
        printf( "Mythryl: putative Mythryl executable isn't: '%s'\n", MYTHRYL_EXECUTABLE );
        exit(1);
    }

    // Fork() returns zero in the child, and the pid of the
    // child in the parent.  That makes the following 'if'
    // look backwards at first blush:

    child_pid = fork();

    if (child_pid) {
	//
	// We are in parent.

        // Close the child-process pipe ends:
	//
        close( subprocess_pipes.stdin_.read_fd   );
        close( subprocess_pipes.stdout_.write_fd );
        close( subprocess_pipes.stderr_.write_fd );

        set_signal_handlers();

    } else {

	// We are in child.  We need to set up stdin/stdio/stderr
	// properly and then exec() the compiler image proper:

        // Close the parent-process pipe ends:
        //
        close( subprocess_pipes.stdin_.write_fd  );
        close( subprocess_pipes.stdout_.read_fd  );
        close( subprocess_pipes.stderr_.read_fd  );

        // Close the original stdin/stdout/stderr file descriptors:
	//
	close( STDIN_FILENO  ); 
	close( STDOUT_FILENO ); 
	close( STDERR_FILENO );

        // Set up the pipes leading to us as the child's
        // new stdin/stdout/stderr:
        //
	copy_pipe( subprocess_pipes.stdin_.read_fd,   STDIN_FILENO   );
	copy_pipe( subprocess_pipes.stdout_.write_fd, STDOUT_FILENO  );
	copy_pipe( subprocess_pipes.stderr_.write_fd, STDERR_FILENO  );

	setenv( "MYTHRYL_SCRIPT", "<stdin>", TRUE );				// The 'TRUE' makes it overwrite any pre-existing value for "MYTHRYL_SCRIPT".
	//
	// This tells mythryld that it is running a script.
	// This is mainly used in
	//
	//     src/lib/core/internal/mythryld-app.pkg
	//
	// Also, if MYTHRYL_SCRIPT is set the global variable
	//
	//     running_script__global
	//
	// is set to TRUE by process_commandline_options() in
	//
	//     src/c/main/runtime-main.c
	//
	// This is intended to allow ad hoc debug logic to (say)
	// do voluminious logging only when a script is running,
	// not during compiles etc (when it might flood the disk).
	// This global variable is not currently used in production code.
	//     -- 2011-12-27 CrT 	

        exec_mythryld( argv );
    }
}


//
static void   sleep_10ms   (void)   {
    //
    // Sleep for 1/100 second.

    struct timeval tv;

    tv.tv_sec  = 0;
    tv.tv_usec = 10000;

    select( 0, NULL, NULL, NULL, &tv );
}


//
static int   send_byte_to   (int value,  int write_fd)   {
    //
    char buf[ 8 ];
    //
    buf[0] = value;
    //
    return   write(   write_fd,   buf,   1   );
}


//
static int   copy_from_to   (
    //
    int read_fd,
    int write_fd,
    int max_bytes_to_copy
){
    char buf[ 512 ];

    if (max_bytes_to_copy <=   0)   return 0;

    if (max_bytes_to_copy >= 512) {
        max_bytes_to_copy  = 512;
    }

    ssize_t   bytes_read
	=
	read(   read_fd,   buf,   max_bytes_to_copy   );

    if (bytes_read <= -1) {
	//
	fprintf( stderr,"%s: copy_from_to: unable to read from fd %d: %s\n", our_name, read_fd, strerror(errno) );
	exit(1);
    }

    if (bytes_read == 0)  return 0;

    char* rest_of_buf
	=
	buf;

    int bytes_left_to_write
	=
	bytes_read;

    // Loop until we've written all the bytes we read:
    //
    while (bytes_left_to_write > 0) {
	//
	ssize_t   bytes_written
	    = 
	    write(   write_fd,   rest_of_buf,   bytes_left_to_write   );

	if (bytes_written <= -1) {
	    //
	    fprintf( stderr,"%s: copy_from_to: unable to write to fd %d: %s\n", our_name, write_fd, strerror(errno) );
	    exit(1);
	}

	// A sane OS shouldn't put us in a busy-wait loop
	// by accepting zero bytes, but sane OSes are as
	// common as unicorns, so:
	//
	if (bytes_written == 0)   sleep_10ms ();

	rest_of_buf         += bytes_written;
	bytes_left_to_write -= bytes_written;
    }         

    return   bytes_read;
}


//
static void   run_subprocess_to_conclusion   (Stdin_Stdout_Stderr_Pipes  subprocess_pipes)   {
    //
    // At this point, the compiler subprocess
    // is supposed to be running, and all we
    // need to do is to make like a wire,
    // transparently forwarding input from
    // our stdin to the compiler subprocess,
    // and stdout/stderr output from the
    // compiler subprocess to our own stdout/stderr
    // (plus of course keeping one eye open for
    // SIGCHLD signals telling us that the subprocess
    // died).
    // 
    // There is a possibility of deadlock if we
    // attempt to write to the compiler's stdin
    // while it is attempting to write reams of
    // stuff to its stdout or stderr, and consequently
    // ignoring its input.
    // 
    // To avoid this, we will adopt a policy of
    // only writing to the compiler when select()
    // says it is ready for input, and only writing
    // one byte, since select() doesn't say how much
    // we can write without blocking. [1]

    int eof_on_childs_stdin  = FALSE;				// We'll set this TRUE when we send an EOF to   child on pipe.
    int eof_on_childs_stdout = FALSE;				// We'll set this TRUE when we see  an EOF from child on pipe.
    int eof_on_childs_stderr = FALSE;				// We'll set this TRUE when we see  an EOF from child on pipe.

    int max_fd = 0;

    if (max_fd <= STDIN_FILENO) {
	max_fd  = STDIN_FILENO+1;
    }
    if (max_fd <= STDOUT_FILENO) {
	max_fd  = STDOUT_FILENO+1;
    }
    if (max_fd <= STDERR_FILENO) {
	max_fd  = STDERR_FILENO+1;
    }
    if (max_fd <= subprocess_pipes.stdin_.write_fd) {
	max_fd  = subprocess_pipes.stdin_.write_fd+1;
    }
    if (max_fd <= subprocess_pipes.stdout_.read_fd) {
	max_fd  = subprocess_pipes.stdout_.read_fd+1;
    }
    if (max_fd <= subprocess_pipes.stderr_.read_fd) {
	max_fd  = subprocess_pipes.stderr_.read_fd+1;
    }

    for (;;) {
	//
	fd_set readable_file_descriptors;
	fd_set writable_file_descriptors;

	FD_ZERO( &readable_file_descriptors );
	FD_ZERO( &writable_file_descriptors );

	{                              FD_SET( STDIN_FILENO,                     &readable_file_descriptors ); }
        if (!eof_on_childs_stdout) {   FD_SET( subprocess_pipes.stdout_.read_fd, &readable_file_descriptors ); }
        if (!eof_on_childs_stderr) {   FD_SET( subprocess_pipes.stderr_.read_fd, &readable_file_descriptors ); }

	{			       FD_SET( STDOUT_FILENO,                    &writable_file_descriptors ); }
	{			       FD_SET( STDERR_FILENO,                    &writable_file_descriptors ); }
	if (!eof_on_childs_stdin)  {   FD_SET( subprocess_pipes.stdin_.write_fd, &writable_file_descriptors ); }

        int bytes_copied  = 0;

	// XXX SUCKO FIXME The Linux select() manpage explains why pselect() is better than select().
	//                 Should probably recode to use it, and also analyse to see if the
	//                 Mythryl POSIX interface should be recoded to use it or offer access to
	//                 it. (It is POSIX-standard.). Same manpage warns that Linux is slightly nonstandard.

	int bits_set = select(
			   max_fd,
			   &readable_file_descriptors,
			   &writable_file_descriptors,
			   NULL,
			   NULL
		       );

	if (bits_set == -1) {
	    //
	    // -1 indicates an abnormal return.
	    //
	    // If this is due to catching a
	    // signal during the call then
	    // we can just continue, otherwise
	    // we had best shut down:
	    //
	    if (errno == EINTR) {
		continue;
	    }

	    fprintf(stderr,"%s: select( ... ) returned -1: %s\n", our_name, strerror(errno) );
	    exit(1);
	}

	// If our subprocess is dead and there is
	// no output from it waiting to be processed,
	// then we're done:
	//
	if (child_is_dead
	&&  (eof_on_childs_stdout || !FD_ISSET( subprocess_pipes.stdout_.read_fd, &readable_file_descriptors ))
	&&  (eof_on_childs_stderr || !FD_ISSET( subprocess_pipes.stderr_.read_fd, &readable_file_descriptors ))
	){
	    exit( child_exit_status );
	}

	// Copying output from the
	// compiler to our user is
	// bog safe:
	//
	if (!eof_on_childs_stdout && FD_ISSET( subprocess_pipes.stdout_.read_fd, &readable_file_descriptors )) {
	    //
	    int   copied   = copy_from_to( subprocess_pipes.stdout_.read_fd, STDOUT_FILENO, 512 );
	    bytes_copied  += copied;
	    if (!copied) {
		eof_on_childs_stdout = TRUE;
	    }
	}
	if (!eof_on_childs_stderr && FD_ISSET( subprocess_pipes.stderr_.read_fd, &readable_file_descriptors )) {
	    //
	    int   copied   = copy_from_to( subprocess_pipes.stderr_.read_fd, STDERR_FILENO, 512 );
	    bytes_copied  += copied;
	    if (!copied)  eof_on_childs_stderr = TRUE;
	}

	// If there is pending queued output for compiler
	// and room to send it, send a byte:
	//
	if (*remaining_text_for_compiler
	    &&
	    !child_is_dead
	) {
	    //
	    if (FD_ISSET( subprocess_pipes.stdin_.write_fd, &writable_file_descriptors )) {
		//
		int bytes_written
		    =
		    send_byte_to(
			//
			*remaining_text_for_compiler,
			subprocess_pipes.stdin_.write_fd
		    );

		remaining_text_for_compiler += bytes_written;
		bytes_copied                += bytes_written;

	    } else {

		// As far as I can see, it is never safe
		// to attempt writing more than one byte
		// to the compiler stdin, since it might
		// block on output exactly when we do so,
		// potentially yielding deadlock.
		//
		// Since such input is presumably human-typed,
		// this should not be a performance issue.
		//
		// (If it becomes one, we can set up non-blocking writes.)
		//
		if (FD_ISSET( STDIN_FILENO,                     &readable_file_descriptors )                &&
		    FD_ISSET( subprocess_pipes.stdin_.write_fd, &writable_file_descriptors )
		){
		    bytes_copied  += copy_from_to( STDIN_FILENO, subprocess_pipes.stdin_.write_fd, 1 );
		}
	    }
	}


	if (!*remaining_text_for_compiler						// When we've sent all text-to-compile to compiler,
	    &&										// send an EOF by closing the fd.  I originally forgot
	    !child_is_dead								// to do this, which worked fine except it would just
	    &&										// hang when sent a script like
	    !eof_on_childs_stdin							//
	){										//     #!/usr/bin/mythryl
	    close( subprocess_pipes.stdin_.write_fd );					//     (
	    //										//
	    eof_on_childs_stdin = TRUE;							// -- the Mythryl parser would hang forever waiting
	}										// for the missing close paren.

	// Don't busy-wait:
	//
	if (bytes_copied == 0) {
	    //
	    sleep_10ms ();
	} 
    }
}


//
static void   run_compiler_subprocess   (char** argv)   {
    //
    close_redundant_fds ();
    //
    Stdin_Stdout_Stderr_Pipes   subprocess_pipes
	=
	open_subprocess_pipes ();

    start_subprocess( subprocess_pipes, argv );

    run_subprocess_to_conclusion( subprocess_pipes );
}

//
int   main   ( int argc, char** argv ) {
    //
    argv = process_argv( argc, argv );

    set_up_compile( argv );

    run_compiler_subprocess( argv );

    exit( 0 );
}

// Code by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.



// ============================================================
// Note[1]:
//
// http://home.gna.org/pysfst/tests/pipe-limit.html
//
//     The 64K pipe buffer limit
//     
//     The deadlock
//     
//     SFST itself uses files to return result lists for
//     methods like analyze_string, generate_string or
//     generate. To map them to string lists, pysfst uses
//     pipes for SFST to write to. On large or even
//     infinite result lists like e.g. from
//     xmor_transducer.generate(), this technique causes
//     python to freeze when the buffer size is exceeded.
//     
//     The writing end of the pipe blocks until the reading end begins to
//     read, but the reading end will not begin to read because it waits for
//     the blocked SFST method to accomplish its task.
//     
//     The 64K limit
//     
//     On standard 2.6 Linux kernels, the buffer size is 64
//     kilobytes. Although $ ulimit -a reports a pipe size of 8 blocks, the
//     buffer size is not 4K, because the kernel dynamically allocates
//     maximal 16 "buffer entries" which multiply out to 64K. These limits
//     are hardcoded in
//     
//       /usr/src/linux/include/linux/pipe_fs_i.h:6 #define PIPE_BUFFERS (16)
//     
//     The bad news
//     
//     There seems to be no way to directly circumvent that limitation
//     without patching the kernel.
//
//
//
//
// ============================================================
// Note[2]:
//
// Cynbe, circa 2008:
//
//     The thematic way to exit from a script should be to do 
//
//         winix::process::exit  winix::process::success;
//
//     but unfortunately the interactive compiler currently barfs on
//     this because 'exit' has a type with a free type variable.
//     So instead we send ourself the TERM signal, whose handler
//     consists of the above code.     -- CrT, circa 2008
//
// Cynbe, 2012-02-05:
//
//     But shouldn't casting to Void resolve this?
//     Changing to either of
//
//         #define SCRIPT_EXIT_BOILERPLATE "\n  winix::process::exit winix::process::success;\n"
//         #define SCRIPT_EXIT_BOILERPLATE "\n (winix::process::exit winix::process::success): Void;\n"
//
//     results in the script
//
//         #!/usr/bin/mythryl
//         print "Hello, world!\n";
//
//     just hanging.  But entering
//
//         winix::process::exit  winix::process::success;
//
//     at the Mythryl   eval:   prompt works fine.




/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/


