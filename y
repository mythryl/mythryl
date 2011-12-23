#!/usr/bin/mythryl

{
    include int_red_black_map;					# int_red_black_map		is from   src/lib/src/int-red-black-map.pkg

    fun assert bool_value
        =
	if (not bool_value)
	   #
	   print "Assertion failed.\n";
	   exit 1;
	fi;


    # When debugging uncomment the following lines and
    # add more log_if calls as appropriate:
    #
#   file::set_logger_to (file::LOG_TO_FILE "xyzzy.log");
#   log_if = file::log_if file::compiler_logging;
#   log_if .{ "Top of script"; }; 

    loops = 100;			# Originally 100, then 1100.

    limit = 10 * 1000;			# Originally 100.

    fun subpthread_fn id ()
	=
	{
	    for (loop = 0; loop < loops; ++loop) {

		# Create a map by successive appends:
		#
		my test_map
		    =
		    for (m = empty, i = 0;  i < limit;  ++i; m) {

			m = set (m, i, i);
			assert (all_invariants_hold   m);
			assert (not (is_empty m));
			assert (the (first_val_else_null m) == 0);
			assert (     vals_count m  == i+1);

			assert (#1 (the (first_keyval_else_null m)) == 0);
			assert (#2 (the (first_keyval_else_null m)) == 0);

		    };

		# Check resulting map's contents:
		#
		for (i = 0;  i < limit;  ++i) {
		    assert ((the (get (test_map, i))) == i);
		};

		# Try removing at all possible positions in map:
		#
		for (map' = test_map, i = 0;   i < limit;   ++i) {

		    my (map'', value) = drop (map', i);

		    assert (all_invariants_hold map'');
		};

		assert (is_empty empty);
	    };

	    pthread::pthread_exit ();
	};	

    subpthread0 = pthread::spawn_pthread  (subpthread_fn 0);
    subpthread1 = pthread::spawn_pthread  (subpthread_fn 1);
    subpthread2 = pthread::spawn_pthread  (subpthread_fn 2);
    subpthread3 = pthread::spawn_pthread  (subpthread_fn 3);
    subpthread4 = pthread::spawn_pthread  (subpthread_fn 4);
    subpthread5 = pthread::spawn_pthread  (subpthread_fn 5);

heap_debug::check_agegroup0_overrun_tripwire_buffer "y: About to join subthread0";
    pthread::join_pthread  subpthread0;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread0";
    pthread::join_pthread  subpthread1;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread1";
    pthread::join_pthread  subpthread2;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread2";
    pthread::join_pthread  subpthread3;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread3";
    pthread::join_pthread  subpthread4;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread4";
    pthread::join_pthread  subpthread5;
heap_debug::check_agegroup0_overrun_tripwire_buffer "y: Joined subthread5";


#    log_if .{ "Script DONE."; }; 		# printf "Script DONE\n";   file::flush file::stdout;   
};

