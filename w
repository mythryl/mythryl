#!/usr/bin/mythryl

{   string  =  "matched";  
#   rx     =  "(matched)";  
    rx     =  "(fail)";  
  
    {   matched_string = (regex::find_first_group  1  rx  string);
        print "It matched\n";
        print matched_string;
    }
    except 
        NOT_FOUND = print "It didn't match.\n";
};
