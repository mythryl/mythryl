#!/usr/bin/mythryl

stipulate
    package u1w =  one_word_unt;			# two_word_unt		is from   src/lib/std/src/two-word-unt.pkg
herein
#    (&)  =  two_word_unt::bitwise_and;
#    (<<) =  two_word_unt::(<<);

    one  = u1w::from_multiword_int  (the (multiword_int::from_string   "1"));
    zero = u1w::from_multiword_int  (the (multiword_int::from_string   "0"));

    fun testbit(value: u1w::Unt, bit) =  (value & (one << bit)) != zero; 

    a =  u1w::from_multiword_int  (the (multiword_int::from_string "255"));
    b =  unt::from_multiword_int  (the (multiword_int::from_string   "1"));
    
    printf "testbif(0xff,0x1) == %b\n" (testbit(a, b));
end;

