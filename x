#!/usr/bin/mythryl

{
    file::set_logger_to (file::LOG_TO_FILE "x.log");
    foo = "This is a test";
    wk0 = weak_reference::make_weak_reference foo;
    wk1 = weak_reference::make_weak_reference foo;
    wk2 = weak_reference::make_weak_reference foo;
    wk3 = weak_reference::make_weak_reference foo;
    zot = "This is NOT a test";
    heap_debug::dump_gen0 "x";
    heap_debug::dump_gen0 "x";
    heap_debug::dump_gens "x";
    heap_debug::dump_hugechunk_stuff "x";
};




