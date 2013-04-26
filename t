#! /usr/bin/mythryl
#
# Stimulus case for a memory leak I'm investigating.    -- 2013-04-05 CrT

stipulate
    include threadkit;									# threadkit						is from   src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit.pkg
    #
    package hth =  hostthread;								# hostthread						is from   src/lib/std/src/hostthread.pkg
    package io  =  io_bound_task_hostthreads;						# io_bound_task_hostthreads				is from   src/lib/std/src/hostthread/io-bound-task-hostthreads.pkg
    package mps =  microthread_preemptive_scheduler;					# microthread_preemptive_scheduler			is from   src/lib/src/lib/thread-kit/src/core-thread-kit/microthread-preemptive-scheduler.pkg
    #

herein



    maildrop =   make_empty_maildrop (): Maildrop(Int);

    mutex = hostthread::make_mutex (); 

    io::do  .{

	hostthread::acquire_mutex mutex; 

	i = *runtime::microthread_switch_lock_refcell__global;

	hostthread::release_mutex mutex;

	mps::do .{
	    put_in_maildrop   (maildrop,   i);
	};
    }; 

    hostthread::free_mutex mutex;

    i = take_from_maildrop  maildrop;

    printf "i = %d\n" i;

    exit(0);
end;
