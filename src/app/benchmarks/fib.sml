structure Main : BMARK =
struct
  fun fib n = if n < 2 then 1 else fib(n-1) + fib(n-2)
  fun doit() = (fib 30; ())
  fun testit outstream = output (outstream, makestring (fib 30) ^ "\n")
end;
