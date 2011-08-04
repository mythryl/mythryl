#!/usr/bin/mythryl

show_control "mythryl_parser::lazy_is_a_keyword";
set_control "mythryl_parser::lazy_is_a_keyword" "TRUE";
show_control "mythryl_parser::lazy_is_a_keyword";

include lazy;					# lazy		is from   src/lib/std/src/nj/lazy.pkg




lazy Stream(X) = CONS(X,Stream(X));

recursive val lazy ones = CONS ((1, ones));

my CONS(h,t) = ones;

fun shd (CONS (x, _)) = x;
fun stl (CONS (_, s)) = s;
fun lazy lstl (CONS (_, s)) = s;		# The CMU tutorial incorrectly omits the "lazy" in this fun.



recursive val lazy s = { print "."; CONS (1, s); };
s' = stl s;                               # prints "."
my CONS _ = s';                           # silent

recursive val lazy s = { print "."; CONS (1, s); };
s'' = lstl s;                             # silent
my CONS _ = s'';                          # prints "."

fun smap f = { fun lazy loop (CONS (x, s)) = CONS (f x, loop s); loop; };

one_plus = smap (fn n = n+1);

recursive val lazy nats = CONS (0, one_plus nats);

fun sfilter pred = { fun lazy loop (CONS (x, s)) = if (pred x)  CONS (x, loop s);  else loop s;  fi;   loop; };

infix val mod ;
fun m mod n = m - n * (m / n);
fun divides m n = (n mod m) == 0;
fun lazy sieve (CONS (x, s)) = CONS (x, sfilter (not o (divides x)) s);

nats2 = stl (stl nats);          # Might as well be eager.
primes = sieve nats2;

fun take 0 _ => [];    take n (CONS (x, s)) =>  x ! (take (n - 1) s);  end;

recursive val lazy s = CONS ({ print "."; 1; }, s);
my CONS (h, _) = s;                       # Prints ".", binds h to 1
my CONS (h, _) = s;                       # Silent, binds h to 1



