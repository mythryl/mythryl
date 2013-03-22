#! /usr/bin/mythryl
#
# An attempt to replicate the compiler hang I'm seeing in a simpler setting.
# 'osval' is about the simplest possible call down to the C level, so it
# should be informative to loop doing that and see if that is sufficient
# to reproduce the problem.

fun loop (i, c)
    =
    {   posixlib::osval "F_GETLK";
	if (i == 0) { printf "foo %d!\n" c;  loop (1000, c+1); };
	else				     loop (i - 1,   c);
	fi;
    };

loop (1, 1);

