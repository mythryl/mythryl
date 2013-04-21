#!/usr/bin/mythryl
#
# A microthread system stress test.
#
# The idea here is to find (small) perfect numbers by
# brute-force testing Int values in the range (say) 1 -> 100,000
# using one worker microthread per Int, each brute-force testing
# all possible factors (and then some :-) and reporting to one
# central boss microthread if the number turns out to be perfect.
#
# A sentinel microthread detects completion of worker thread
# computation and signals the boss thread to return the result
# list via a oneshot maildrop.  
#
# The main microthread fires things up and then prints the
# result read from the maildrop.

stipulate
    package cmd =  commandline;								# commandline						is from   src/lib/std/commandline.pkg
    package cpu =  cpu_bound_task_hostthreads;						# cpu_bound_task_hostthreads				is from   src/lib/std/src/hostthread/cpu-bound-task-hostthreads.pkg
    package hth =  hostthread;								# hostthread						is from   src/lib/std/src/hostthread.pkg
    package io  =  io_bound_task_hostthreads;						# io_bound_task_hostthreads				is from   src/lib/std/src/hostthread/io-bound-task-hostthreads.pkg
    package mps =  microthread_preemptive_scheduler;					# microthread_preemptive_scheduler			is from   src/lib/src/lib/thread-kit/src/core-thread-kit/microthread-preemptive-scheduler.pkg

#  10000 =  1.040u 0.072s 0:01.11 100.0%	0+0k 0+12208io 0pf+0w
#  20000 =  3.904u 0.156s 0:04.05 100.0%	0+0k 0+36896io 0pf+0w
#  30000 =  9.884u 0.264s 0:10.16  99.8%	0+0k 8+90472io 0pf+0w
#  40000 = 20.181u 0.552s 0:20.72 100.0%	0+0k 0+187520io 0pf+0w
#  50000 = 36.762u 1.124s 0:37.86 100.0%	0+0k 0+350104io 0pf+0w
# 500000 = 

    fun io_do (task: Void -> Void) = {
log::note_in_ramlog .{ sprintf "io_do/AAA -- acquiring mutex"; };
												hth::acquire_mutex  io::mutex;
log::note_in_ramlog .{ sprintf "io_do/BBB -- acquired mutex, incrementing microthread_switch_lock"; };
 		 										hth::increment_microthread_switch_lock ();
log::note_in_ramlog .{ sprintf "io_do/CCC -- incremented microthread_switch_lock, updating queue"; };
	io::external_request_queue :=  (io::DO_TASK task)  !  *io::external_request_queue; 
log::note_in_ramlog .{ sprintf "io_do/DDD -- decrementing microthread_switch_lock"; };
												hth::decrement_microthread_switch_lock ();
log::note_in_ramlog .{ sprintf "io_do/EEE -- decremented microthread_switch_lock, releasing mutex"; };
												hth::release_mutex io::mutex;  
log::note_in_ramlog .{ sprintf "io_do/FFF -- decremented microthread_switch_lock, released mutex, broadcasting condvar"; };
result =
												hth::broadcast_condvar  io::condvar;
log::note_in_ramlog .{ sprintf "io_do/ZZZ"; };
result;
    };           

    fun mps_do  (thunk: Void -> Void)
	= 
	{
log::note_in_ramlog .{ sprintf "mps_do/AAA"; };
  												hth::acquire_mutex mps::mutex;  
		mps::request_queue :=  (mps::DO_THUNK thunk)  !  *mps::request_queue; 
												hth::release_mutex mps::mutex;  
result =
												hth::broadcast_condvar mps::condvar;
log::note_in_ramlog .{ sprintf "mps_do/ZZZ"; };
result;
	};           
herein

    commandline_args =  cmd::get_commandline_arguments ();

    fun usage ()
	=
	die_x (sprintf "Usage: %s worker_thread_count\n" (cmd::get_program_name ()));
    

    if (list::length commandline_args  !=  1)   usage();   fi;


    # We'll check all Int values in this range:
    #
    lower_number_to_check =  1;

    upper_number_to_check =  (atoi (list::nth (commandline_args, 0)))  except _ = usage();
    

    fun is_perfect_number n
	=
	{
log::note_in_ramlog .{ sprintf "is_perfect_number(%d)/AAA" n; };
	    sum_of_nonself_factors
		=
		for (i = 1, sum = 0;  i < n;  ++i;  sum) {
		    #
		    sum =   (n % i == 0)   ??   (sum + i)   ::   sum;
		};

result =
	    n == sum_of_nonself_factors;
log::note_in_ramlog .{ sprintf "is_perfect_number(%d)/ZZZ" n; };
result;
	};

    fun is_perfect_number' (n: Int)
	=
{
log::note_in_ramlog .{ sprintf "is_perfect_number'(%d)/AAA" n; };
result =
	case (take_from_oneshot  oneshot)
	    #
	    RESULT    z =>  z;
	    EXCEPTION x =>  raise exception  x;
	esac
	where
	    Result(Z) =  RESULT Z
		      |  EXCEPTION Exception
		      ;

	    oneshot =   make_oneshot_maildrop ();

	    io_do .{
log::note_in_ramlog .{ sprintf "is_perfect_number'(%d)/io_do/AAA" n; };
			result =    RESULT (is_perfect_number n)
				    except
					x = EXCEPTION x;

log::note_in_ramlog .{ sprintf "is_perfect_number'(%d)/io_do/BBB -- calling mps_do" n; };
			mps_do .{
log::note_in_ramlog .{ sprintf "is_perfect_number'/mps_do(%d)/AAA" n; };
result =
				    put_in_oneshot (oneshot, result);
log::note_in_ramlog .{ sprintf "is_perfect_number'/mps_do(%d)/ZZZ" n; };
result;
				};
log::note_in_ramlog .{ sprintf "is_perfect_number'(%d)/io_do/ZZZ -- done mps_do call." n; };
		    };
	end; 
log::note_in_ramlog .{ sprintf "is_perfect_number'(%d)/ZZZ" n ; };
result;
};


    Queue_Entry = PERFECT_NUMBER Int
		| DONE
		;

    result_queue =  make_mailqueue ():  Mailqueue( Queue_Entry );

    taskref = REF (NULL:  Null_Or(Apptask));

    fun worker_thread n
	=
{
	if (is_perfect_number' n)
	    #
	    put_in_mailqueue (result_queue, PERFECT_NUMBER n);

	fi;
log::note_in_ramlog .{ sprintf "thread %d done live_tasks d=%d" n case *taskref   NULL => -1;   THE task => get_task's_alive_threads_count task; esac; };
	thread_exit { success => TRUE };
};

    result_maildrop =   make_oneshot_maildrop ():   Oneshot_Maildrop( List(Int) );

    fun result_loop (result_list: List(Int))
	=
	case (take_from_mailqueue result_queue)
	    #
	    PERFECT_NUMBER i => result_loop (i ! result_list);

	    DONE	     => {   put_in_oneshot (result_maildrop, result_list);
				    thread_exit { success => TRUE };
				};
	esac;


    fun initialize__find_perfect_numbers__task ()
	=
	{
	    for (i = lower_number_to_check;  i < upper_number_to_check;  ++i) {
		#
log::note_in_ramlog .{ sprintf "initialize__find_perfect_numbers__task loop %d/AAA" i; };
		make_thread "worker thread" .{ worker_thread  i; };
log::note_in_ramlog .{ sprintf "initialize__find_perfect_numbers__task loop %d/BBB" i; };
	    };
	    #
log::note_in_ramlog .{ sprintf "initialize__find_perfect_numbers__task DONE"; };
	    thread_exit { success => TRUE };
	};

    make_thread "boss thread" .{ result_loop []; };

    task =  make_task "Find Perfect Numbers"  [ ("startup_thread", initialize__find_perfect_numbers__task) ];
    taskref := THE task;

    fun sentinel ()
	=
	{   task_finished' =  task_done__mailop  task;
	    #
	    block_until_mailop_fires  task_finished';
log::note_in_ramlog .{ sprintf "sentinel firing"; };
	    #
	    put_in_mailqueue (result_queue, DONE);
	};

    fun narration ()
	=
	{   log::note_on_stderr .{ sprintf "Live threads left = %d\n" (get_task's_alive_threads_count task); };
	    sleep_for 0.5;
	};

    make_thread "sentinel thread" .{ sentinel (); };

    make_thread "narration" .{ narration (); };

    printf "Perfect numbers between %d and %d inclusive (in binary):\n" lower_number_to_check upper_number_to_check;
    apply (printf "%20b\n") (take_from_oneshot result_maildrop);


    exit(0);
end;
