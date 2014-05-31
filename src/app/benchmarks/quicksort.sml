structure Main: BMARK =
struct

open Array

fun qsort lo hi (a : int array) =
  if lo < hi then
    let val pivot = sub(a, hi)
        fun loop1 i j = if i >= j then i else
          let fun loop2 i =
                if i < hi andalso sub(a,i) <= pivot then loop2(i+1) else i
              val i = loop2 i
              fun loop3 j =
                if j > lo andalso sub(a,j) >= pivot then loop3(j-1) else j
              val j = loop3 j
          in
            if i < j then
              let val temp = sub(a,i) in
                update(a,i,sub(a,j)); update(a,j,temp)
              end
            else ();
            loop1 i j
          end
        val i = loop1 lo hi
    in
      let val temp = sub(a,i) in
        update(a,i,sub(a,hi)); update(a,hi,temp)
      end;
      qsort lo (i-1) a;
      qsort (i+1) hi a
    end
  else ()

fun cmp (i : int) (j : int) = i - j

fun qsort2 lo hi (a : int array) =
  if lo < hi then
    let val pivot = sub(a, hi)
        fun loop1 i j = if i >= j then i else
          let fun loop2 i =
                if i < hi andalso cmp (sub(a,i)) pivot <= 0
                then loop2(i+1) else i
              val i = loop2 i
              fun loop3 j =
                if j > lo andalso cmp (sub(a,j)) pivot >= 0
                then loop3(j-1) else j
              val j = loop3 j
          in
            if i < j then
              let val temp = sub(a,i) in
                update(a,i,sub(a,j)); update(a,j,temp)
              end
            else ();
            loop1 i j
          end
        val i = loop1 lo hi
    in
      let val temp = sub(a,i) in
        update(a,i,sub(a,hi)); update(a,hi,temp)
      end;
      qsort lo (i-1) a;
      qsort (i+1) hi a
    end
  else ()

val seed = ref 0

fun random () =
  (seed := (!seed * 25173 + 17431) mod 4096; !seed)

fun test_sort sort_fun size =
  let val a = array (size, 0)
      val check = array (4096, 0)
      fun loop i = if i >= size then () else
            let val n = random() in
              update(a, i, n);
              update(check, n, sub(check, n) + 1);
              loop (i+1)
            end
  in
    loop 0;
    sort_fun 0 (size-1) a;
    let fun loop2 i = if i >= size then true else
                      if sub(a,i-1) > sub(a,i) then false else
                      (update(check,sub(a,i), sub(check, sub(a,i)) - 1);
                       loop2 (i+1))
        fun loop3 i = if i >= 4096 then true else
                      if sub(check,i) <> 0 then false else
                      loop3 (i+1)
    in
      update(check,sub(a,0), sub(check, sub(a,0)) - 1);
      loop2 1 andalso loop3 0
    end
  end

fun testit outstream =
  (output(outstream, if test_sort qsort 50000 then "OK\n" else "Failed\n");
   output(outstream, if test_sort qsort2 50000 then "OK\n" else "Failed\n"))

fun doit () =
  (test_sort qsort 50000; test_sort qsort2 50000; ())

end;
