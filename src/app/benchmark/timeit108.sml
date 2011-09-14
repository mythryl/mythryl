signature BMARK =
  sig
    val doit : unit -> unit
    val testit : IO.outstream -> unit
  end;

structure Timing =
  struct
    open Time
    fun timeappl outstream funct arg =
      let val cpu_timer = Timer.startCPUTimer() in
        funct arg;
        case Timer.checkCPUTimer cpu_timer of
          {usr = u, gc = g, sys = s} =>
            (output(outstream, (toString(u + g + s)));
             output(outstream, "s  (usr "); output(outstream, toString u);
             output(outstream, "s, gc "); output(outstream, toString g);
             output(outstream, "s, sys "); output(outstream, toString s);
             output(outstream, "s)\n");
             flush_out outstream)
      end
    fun compileIt outstream filename = timeappl outstream use filename
    fun runIt outstream doit = timeappl outstream doit ()
  end

          
    