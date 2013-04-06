#! /usr/bin/mythryl
#
# An attempt to replicate the compiler hang I'm seeing in a simpler setting.
# 'osval' is about the simplest possible call down to the C level, so it
# should be informative to loop doing that and see if that is sufficient
# to reproduce the problem.

stipulate
    include threadkit;									# threadkit						is from   src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit.pkg
    #
    package ci  =  mythryl_callable_c_library_interface;				# mythryl_callable_c_library_interface			is from   src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg
    package io  =  io_bound_task_hostthreads;						# io_bound_task_hostthreads				is from   src/lib/std/src/hostthread/io-bound-task-hostthreads.pkg
    package mps =  microthread_preemptive_scheduler;					# microthread_preemptive_scheduler			is from   src/lib/src/lib/thread-kit/src/core-thread-kit/microthread-preemptive-scheduler.pkg
    package psx =  posixlib;								# posixlib						is from   src/lib/std/src/psx/posixlib.pkg
    #
    fun cfun  fun_name
	=
	ci::find_c_function'' { lib_name => "posix_io", fun_name };

    (cfun "osval")									# osval		def in    src/c/lib/posix-io/osval.c
	->
	(      osval2__syscall:    String -> Int,					# The '2's here are just because otherwise when this pkg gets included into the posix package we get complaints about duplicate osval defs.
	       osval2__ref,
	  set__osval2__ref
	);


    fun osval string
	=
	*osval2__ref  string;

#    redirected_calls_done =  REF 0;
herein

		fun redirect_one_io_call
			{
			   io_call:  Y -> Z,
			  lib_name:  String,
			  fun_name:  String
			}
		    =
		    {
#														call_number =  REF 0;

			fn (y: Y)
			    =
			    {   Result(Z) =  RESULT Z
					  |  EXCEPTION Exception
					  ;

#														n = *call_number;	call_number := n + 1;	# Yes, in principle we should use a mutex. In practice we're testing mono-calling-thread  and the window is small and a bad 'n' is no biggie anyhow.


				oneshot =   make_oneshot_maildrop ();
				#
				io::do .{
					    result =    RESULT (io_call y)
							except
							    x = EXCEPTION x;


					    mps::do .{

#							redirected_calls_done :=  *redirected_calls_done + 1;	# Purely for src/lib/std/src/psx/posix-io-unit-test.pkg
														# No mutex here -- the performance hit wouldn't be worth it and we only use it in a single-threaded context.
														# NB: It is important to do this before the following set(), otherwise posix_io_unit_test will probably miss the last increment.
							set_oneshot (oneshot, result);
							#
						    };
					};

														result =
				case (get_from_oneshot  oneshot)
				    #
				    RESULT    z =>  z;
				    EXCEPTION x =>  raise exception  x;
				esac;
														result;
			    }; 
		    };

#    psx::set__osval__ref					redirect_one_io_call;
#    psx::set__osval2__ref					redirect_one_io_call;				# set__osval2__ref	is from   src/lib/std/src/psx/posix-io.pkg
    set__osval2__ref					redirect_one_io_call;

    fun loop (i, c)
	=
#	{   posixlib::osval "F_GETLK";
	{   osval "F_GETLK";
	    #
	    if (i == 0) { printf "foo %d!\n" c;  loop (10000, c+1); };
	    else				     loop (i - 1,   c);
	    fi;
	};

    loop (1, 1);
end;
