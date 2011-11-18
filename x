#!/usr/bin/mythryl

foo =   fn () = printf "Parent:  thread id d=%d   process id d=%d\n" (pthread::get_pthread_id()) (posix_1003_1b::get_process_id());

foo ();

bar =   fn () = { printf "child:  thread_id d=%d      process id d=%d\n" (pthread::get_pthread_id()) (posix_1003_1b::get_process_id()); pthread::pthread_exit (); };

pthread = pthread::spawn_pthread bar;


