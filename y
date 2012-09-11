#!/usr/bin/mythryl 
 

    stipulate
	package psx =  posix_1003_1b;								# posix_1003_1b			is from   src/lib/std/src/posix-1003.1b/posix-1003-1b.pkg
	package tim =  time;									# time				is from   src/lib/std/time.pkg
	package vu1 =  vector_of_one_byte_unts;							# vector_of_one_byte_unts	is from   src/lib/std/src/vector-of-one-byte-unts.pkg
	package wio =  winix::io;								# winix::io			is from   src/lib/std/src/posix/winix-io.pkg
	package wxp =  winix::process;								# winix::process		is from   src/lib/std/src/posix/winix-process.pkg
    herein

	Pipe    =  { infd:   psx::File_Descriptor,
		     outfd:  psx::File_Descriptor
		   };

	fun pipe_to_string (pipe: Pipe)
	    =
	    sprintf "pipe = { infd=%d, outfd=%d }"  (psx::fd_to_int pipe.infd)  (psx::fd_to_int pipe.outfd);

	fun default_wait_request_list (pipe: Pipe)
	    =
	    {   # Our minimal request list is to read
		# the pipe that clients use to wake us:
		#
		io_descriptor =  psx::fd_to_iod  pipe.infd;
		#
		[ { io_descriptor,
		    #
		    readable => TRUE,
		    writable => FALSE,
		    oobdable => FALSE
		  }
		]; 
	    };

	fun print_wait_requests ({ io_descriptor, readable, writable, oobdable } ! rest)	# A little debug-support function, not used in in production code.
		=>
		{   fd =  psx::iod_to_fd (psx::fd_to_int  io_descriptor);
		    #
		    printf "y:    fd %d readable %b writable %b oobdable %b\n"
			fd
			readable
			writable
			oobdable
		   ;
		};

	    print_wait_requests [] =>   ();
	end;

	fun write_to_private_pipe  (pipe: Pipe)								# We do this to wake the main server hostthread from its I/O wait sleep.
	    =
	    {
												printf "y:write_to_private_pipe/TOP\n";
		{   bytes_written									# Ignored.
			=
			psx::write_vector								# Write one byte into pipe.
			      (
				pipe.outfd,
				one_byte_slice_of_one_byte_unts
			      );
		}
		except
		    x = {
			    printf "y:write_to_private_pipe/XXX EXCEPTION THROWN BY WRITE_VECTOR!\n";
			    (exceptions::exception_message x) -> msg;
			    printf "y:write_to_private_pipe/XXXb: %s\n" msg;
			};

											printf "y:write_to_private_pipe/ZZZ\n";
	    }
	    where
		one_byte_slice_of_one_byte_unts							# Just anything one byte long to write into our internal pipe.
		    =
		    vector_slice_of_one_byte_unts::make_full_slice				# vector_slice_of_one_byte_unts	is from   src/lib/std/src/vector-slice-of-one-byte-unts.pkg
			#
			one_byte_vector_of_one_byte_unts
			where
			    one_byte_vector_of_one_byte_unts
				=
				vu1::from_list
				    #
				    [ (one_byte_unt::from_int  0) ];
			end;
	    end;

	fun process_io_ready_fd  { io_descriptor => iod, readable, writable, oobdable }
	    =
	    {
											printf "y:process_io_ready_fd/TOP: Doing read_as_vector()\n";
	       bytevector
		   =	
		   psx::read_as_vector								# Read and discard the byte that was sent to us.
		     {
		       file_descriptor   =>  iod,
		       max_bytes_to_read =>  1
		     };

											printf "y:process_io_ready_fd/AAA: Done read, (vu1::length bytevector) d=%d\n" (vu1::length bytevector);
	       if ((vu1::length bytevector) == 0)						# We expect to see a 1-byte result.
		   #
											printf "y:process_io_ready_fd/BBB: ZERO LENGTH RESULT UNEXPECTED!\n";
		   wxp::sleep 0.1;
											printf "y:process_io_ready_fd/CCC: slept 0.1.\n";
	       fi;
											printf "y:process_io_ready_fd/ZZZ\n";
	    };

	timeout  =   THE (tim::from_float_seconds 2.00);

											printf "\n\n\n###################################\ny:AAA: Making pipe...\n";
	pipe = psx::make_pipe ();
											printf "y:BBB: Pipe made: %s\n" (pipe_to_string pipe);

	wait_requests =  default_wait_request_list  pipe;					# By default we listen only on our private pipe.
											printf "y:CCC: wait_requests constructed:\n";
											print_wait_requests  wait_requests;

											printf "y:CCCa: Waiting for io opportunity:\n";
	fds_ready_for_io
	    =
	    wio::wait_for_io_opportunity { wait_requests, timeout };

											printf "y:CCCb: Back from waiting for io opportunity, %d fds_ready_for_io:\n" (list::length fds_ready_for_io);
											print_wait_requests  fds_ready_for_io;

											printf "y:DDD: Writing to pipe\n";
	write_to_private_pipe  pipe;
											printf "y:EEE: Back from writing to pipe\n";

											printf "y:FFF: Waiting for io opportunity:\n";
	fds_ready_for_io
	    =
	    wio::wait_for_io_opportunity { wait_requests, timeout };

											printf "y:GGG: Back from waiting for io opportunity, %d fds_ready_for_io:\n" (list::length fds_ready_for_io);
											print_wait_requests  fds_ready_for_io;


											printf "y:HHH:  Doing   apply  process_io_ready_fd   fds_ready_for_io\n";
	apply  process_io_ready_fd   fds_ready_for_io;						# Handle any new I/O opportunities.
											printf "y:III:  Done    apply  process_io_ready_fd   fds_ready_for_io\n";

											printf "y:ZZZ\n##################################\n\n\n\n";
    end;											# stipulate
