structure Main : BMARK =
  struct
    fun sumlist [] = 0
      | sumlist (a::L) = a + sumlist L
    fun interval n =
      if n <= 0 then [] else n :: interval (n-1)
    fun repeat n =
      if n <= 0 then 0 else (repeat (n-1); sumlist(interval 10000))
    fun doit() = (repeat 100; ())
    fun testit outstream = output(outstream, makestring (repeat 100) ^ "\n")
  end
;
