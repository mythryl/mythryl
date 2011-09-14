signature BMARK =
  sig
    val doit : unit -> unit
    val testit : IO.outstream -> unit
  end;

structure Timing =
  struct

    local
      open System.Timer System.Control.Runtime
      val timeofday : unit -> time = System.Unsafe.CInterface.c_function "timeofday"
      type timing = {usr:time, gc:time, sys:time, real:time, alloc:int}
      val pad10 = "          "
      fun pad (s, n) = let
	    val l = size s
	    in
	      if (n <= l)
		then s
	      else if ((n-l) >= 10)
		then pad (pad10^s, n)
		else substring(pad10, 0, n-l) ^ s
	    end
      fun start () = (
	    System.Unsafe.CInterface.gc 2;
	    System.Control.Runtime.collectedfrom := 0;
	    {realt = timeofday(), timer = start_timer()})
      fun stop {realt=rt, timer=t} = let
	    val t' = check_timer t
	    val ts = check_timer_sys t
	    val tg = check_timer_gc t
	    val rt' = sub_time(timeofday(),rt)
	    val _ = System.Unsafe.CInterface.gc 2
	    val alloc = !collectedfrom
	    in
	      {
	        usr = t',
	        gc = tg,
	        sys = ts,
	        real = rt',
	        alloc = alloc
	      }
	    end

  (* convert a time value to a string, padded on the left to 8 characters *)
    fun timeToStr (TIME{sec, usec}) = let
          val tenMS = (usec + 5000) quot 10000
	  val str = let val s = Integer.makestring tenMS
		in
		  if (tenMS < 10) then ".0"^s else "."^s
		end
	  val s = (Integer.makestring sec) ^ str
          in
	    pad (s, 6)
	  end

    (* convert an integer number of Kilobytes to a string *)
      fun allocStr n = let
	    val n = n + 512
	    val tenth = (10*(n rem 1024)) quot 1024
	    val mb = (n div 1024)
	    in
	      pad (implode[
		  Integer.makestring mb, ".", Integer.makestring tenth, "Mb"
		], 8)
	    end
    in 

    fun output (strm, {usr, gc, sys, real, alloc} : timing) =
	  IO.output (strm, implode[
	      "usr = ", timeToStr usr,
	      ", sys = ", timeToStr sys,
	      ", gc = ", timeToStr gc,
	      ", real = ", timeToStr real,
	      ", alloc = ", allocStr alloc, "\n"
	    ])

  (* Time the compilation of the benchmark *)
    fun compileIt (outstrm, fname) = let
	  val t0 = start()
	  in
	    use fname;
	    output (outstrm, stop t0)
	  end

  (* Time one run of the benchmark *)
    fun runIt doit = let
	  val t0 = start()
	  in
	    doit();
	    stop t0
	  end

    fun timeIt (outstrm, doit) = output (outstrm, runIt doit)

    end (* local *)

  end (* Main *)

