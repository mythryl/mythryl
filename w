#! /usr/bin/mythryl
#
# Stimulus case for a memory leak I'm investigating.    -- 2013-04-05 CrT

stipulate
    include threadkit;									# threadkit						is from   src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit.pkg
    #
    package ci  =  mythryl_callable_c_library_interface;				# mythryl_callable_c_library_interface			is from   src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg
    package hth =  hostthread;								# hostthread						is from   src/lib/std/src/hostthread.pkg
    package io  =  io_bound_task_hostthreads;						# io_bound_task_hostthreads				is from   src/lib/std/src/hostthread/io-bound-task-hostthreads.pkg
    package mps =  microthread_preemptive_scheduler;					# microthread_preemptive_scheduler			is from   src/lib/src/lib/thread-kit/src/core-thread-kit/microthread-preemptive-scheduler.pkg
    package psx =  posixlib;								# posixlib						is from   src/lib/std/src/psx/posixlib.pkg
    #

herein
    Result(Z) =  RESULT Z;

    fun io_do (task: Void -> Void) = {								hth::acquire_mutex  io::mutex; 		 	hth::increment_microthread_switch_lock ();
	io::external_request_queue :=  (io::DO_TASK task)  !  *io::external_request_queue; 
												hth::decrement_microthread_switch_lock ();	hth::release_mutex io::mutex;  
												hth::broadcast_condvar  io::condvar;  
    };           

    fun mps_do  (thunk: Void -> Void)
	= 
	{  											hth::acquire_mutex mps::mutex;  
		mps::request_queue :=  (mps::DO_THUNK thunk)  !  *mps::request_queue; 
												hth::release_mutex mps::mutex;  
												hth::broadcast_condvar mps::condvar;  
	};           


    maildrop =   make_empty_maildrop (): Maildrop(Int);

    fun loop (i, c)
	=
	{ 
	    io_do  .{
		mps_do .{
		    put_in_maildrop   (maildrop,   12);
		};
	    }; 

	    take_from_maildrop  maildrop;

	    #
	    if (i == 0) { printf "foo %d!\n" c;  if (c==50) heap_debug::dump_all "dump_all"; fi;
						 loop (10000, c+1); };
	    else				 loop (i - 1, c  );
	    fi;
	};

    loop (1, 1);
end;
