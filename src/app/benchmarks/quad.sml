structure Main : BMARK =
  struct
    fun double f = fn x => f(f x)
    val quad = double double
    fun interval n = if n <= 0 then [] else n :: interval (n-1)
    fun map f [] = [] | map f (a::L) = f a :: map f L
    fun succ x = x+1
    fun doit() = (map (quad quad succ) (interval 1000); ())
    fun do_list f =
      let fun do_rec [] = ()
            | do_rec (a::L) = (f a; do_rec L)
      in
        do_rec
      end
    fun testit outstream =
      do_list (fn (n:int) => (output(outstream, makestring n);
                              output(outstream, " ")))
              (map (quad quad succ) (interval 1000))
  end;

