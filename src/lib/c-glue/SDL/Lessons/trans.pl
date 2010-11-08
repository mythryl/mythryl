#!/usr/bin/perl -p -i

{  s/\t/        /g;       # tabs
   s/\/\/(.*)/(* \1 *)/;  # C++ comments 
   s/\/\*/(*/g;           # C comments 
   s/\*\//*)/g;           # C comments 
   s/((gl|SDL)[_a-zA-Z0-9]+)(\s*)\(/\1.f\3(/g; # calls
   s/(\.[0-9]+)f/\1/g; # reals
}
