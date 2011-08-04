
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

struct stat s;

int main (int argc, char**argv) {
    printf( "sizeof(s.st_size) d=%d\n", sizeof(s.st_size) );
    exit(0);
}
