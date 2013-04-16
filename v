#!/usr/bin/mythryl

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

fun test n
    =
    printf "is_perfect_number %d  ==  %B\n" n (is_perfect_number n);

test 1;
test 6;
test 10;
test 28;

# src/lib/src/lib/thread-kit/src/core-thread-kit/microthread.api
#
# Next we need to:
# Create a new task "Find Perfect Numbers" containing one thread which
#     spins off one thread per number between 1 and n, then exits.
# Each thread calls is_perfect_number and if it is perfect registers it
#     with Scoreboard, then calls thread_exit TRUE.
# Create a "Collect Perfect Numbers" thread which collects results via a mailqueue
#     and also (via  task_done__mailop) waits on "Find Perfect Numbers" task to exit,
#     then publishes result via a maildrop.
# Have main thread read result from maildrop.


exit(0);

