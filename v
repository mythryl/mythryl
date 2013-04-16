#!/usr/bin/mythryl
#
# Preliminary sketch for a microthread system stress test.
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
    # We'll check all Int values in this range:
    #
    lower_number_to_check =  1;
    upper_number_to_check =  10000;
herein
    fun is_perfect_number n
	=
	{   sum_of_nonself_factors
		=
		for (i = 1, sum = 0;  i < n;  ++i;  sum) {
		    #
		    sum =   (n % i == 0)   ??   (sum + i)   ::   sum;
		};

	    n == sum_of_nonself_factors;
	};

    Queue_Entry = PERFECT_NUMBER Int
		| DONE
		;

    result_queue =  make_mailqueue ():  Mailqueue( Queue_Entry );

    fun worker_thread n
	=
	if (is_perfect_number n)
	    #
	    put_in_mailqueue (result_queue, PERFECT_NUMBER n);

	    thread_exit { success => TRUE };
	fi;

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
		make_thread "worker thread" .{ worker_thread  i; };
	    };
	    #
	    thread_exit { success => TRUE };
	};

    make_thread "boss thread" .{ result_loop []; };

    task =  make_task "Find Perfect Numbers"  [ ("startup_thread", initialize__find_perfect_numbers__task) ];

    fun sentinel ()
	=
	{   task_finished' =  task_done__mailop  task;
	    #
	    block_until_mailop_fires  task_finished';
	    #
	    put_in_mailqueue (result_queue, DONE);
	};

    make_thread "sentinel thread" .{ sentinel (); };


    printf "Perfect numbers found:\n";
    apply (printf "   %b\n") (take_from_oneshot result_maildrop);


    exit(0);
end;
