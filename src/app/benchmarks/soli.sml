structure Main : BMARK = struct

open Array
infix 3 sub

datatype peg = Out | Empty | Peg;

val board = Array.fromList [
 Array.fromList [ Out, Out, Out, Out, Out, Out, Out, Out, Out],
 Array.fromList [ Out, Out, Out, Peg, Peg, Peg, Out, Out, Out],
 Array.fromList [ Out, Out, Out, Peg, Peg, Peg, Out, Out, Out],
 Array.fromList [ Out, Peg, Peg, Peg, Peg, Peg, Peg, Peg, Out],
 Array.fromList [ Out, Peg, Peg, Peg, Empty, Peg, Peg, Peg, Out],
 Array.fromList [ Out, Peg, Peg, Peg, Peg, Peg, Peg, Peg, Out],
 Array.fromList [ Out, Out, Out, Peg, Peg, Peg, Out, Out, Out],
 Array.fromList [ Out, Out, Out, Peg, Peg, Peg, Out, Out, Out],
 Array.fromList [ Out, Out, Out, Out, Out, Out, Out, Out, Out]
];

val moves = array (31, (Array.fromList [] : int array array));

val dir =  Array.fromList [
  Array.fromList [0, 1],
  Array.fromList [1, 0],
  Array.fromList [0, ~1],
  Array.fromList [~1, 0]
];

val counter = ref 0;

exception Found;;

fun solve m =
 (counter := !counter + 1;
  if m = 31 then
    (case board sub 4 sub 4 of Peg => true | _ => false)
  else
   (((if !counter mod 500 = 0 then
        (print (!counter); print "\n")
      else
        ());
     let fun loop_i i =
       if i > 7 then () else
        (let fun loop_j j =
          if j > 7 then () else
           ((case board sub i sub j of
               Peg =>
                 let fun loop_k k =
                   if k > 3 then () else
                    (let val d1 = dir sub k sub 0;
                         val d2 = dir sub k sub 1;
                         val i1 = i+d1;
                         val i2 = i1+d1;
                         val j1 = j+d2;
                         val j2 = j1+d2
                     in
                       case board sub i1 sub j1 of
                         Peg =>
                          (case board sub i2 sub j2 of
                             Empty =>
                              (update (board sub i, j, Empty);
                               update (board sub i, j,  Empty);
                               update (board sub i1, j1, Empty);
                               update (board sub i2, j2, Peg);
                               (if solve(m+1) then
                                 (update(moves, m,
                                         Array.fromList [Array.fromList[i,j],
                                                      Array.fromList[i2,j2]]);
                                  raise Found)
                                else
                                 (update (board sub i, j, Peg);
                                  update (board sub i1, j1, Peg);
                                  update (board sub i2, j2, Empty))))
                           | _ =>
                               ())
                         | _ =>
                              ()
                       end;
                       loop_k (k+1))
                 in loop_k 0
                 end
               | _ =>
                 ());
            loop_j (j + 1))
         in loop_j 1
         end;
         loop_i (i + 1))
       in loop_i 1
       end;
       false) handle Found => true));

fun print_peg outstream Out = output(outstream, ".")
  | print_peg outstream Empty = output(outstream, " ")
  | print_peg outstream Peg = output(outstream, "$")
;

fun print_board outstream board =
  let fun loop_i i =
    if i > 8 then () else
     (let fun loop_j j =
        if j > 8 then () else
         (print_peg outstream (board sub i sub j);
          loop_j (j+1))
      in
        loop_j 0;
        output(outstream, "\n")
      end;
      loop_i (i+1))
  in
    loop_i 0
  end;

fun testit outstream =
  if solve 0 then (output(outstream, "\n"); print_board outstream board)
             else output(outstream, "No solution.\n")

fun doit () = testit std_out

end;
