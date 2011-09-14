structure Main : BMARK =
  struct
    fun tak x y z =
      if x > y then tak(tak (x-1) y z) (tak (y-1) z x) (tak (z-1) x y)
               else z
    fun repeat n =
      if n <= 0 then 0 else tak 18 12 6 + repeat (n-1)
    fun doit() = (repeat 50; ())
    fun testit outstream = output(outstream, makestring (repeat 50) ^ "\n")
  end
;
