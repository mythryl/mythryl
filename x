#!/usr/bin/mythryl

print "Hello!\n";

# include threadkit;

# package md = maildrop;
# 
# print "Boo!\n";
# thread_scheduler .{
#     sleep_for  time::from_seconds 2;
# };
# print "Hiss!\n";
# 
# 

	       s = (internet_socket::tcp::socket());
	       socket::bind(s, internet_socket::any(30000));
	       socket::listen(s, 1000);

exit 0;



