#!/usr/bin/mythryl

{
    file::set_logger_to  (file::LOG_TO_FILE  "x.log");
    file::log_if file::compiler_logging .{ "Foo!\n"; };

    mutex = file::mutex;
    with_mutex_do = pthread::with_mutex_do;

    with_mutex_do mutex .{  printf "This is not a test 1\n"; };
    with_mutex_do mutex .{  printf "This is not a test 2\n"; };
    with_mutex_do mutex .{  printf "This is not a test 3\n"; };
};




