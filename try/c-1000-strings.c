/* c-1000-strings.c
 *
 * A simple benchmark that allocates and then frees a lot of strings
 *
 *    gcc c-1000-strings.c -o c-1000-strings;   ./c-1000-strings
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void malloc_and_free_1000_strings ( void ) {
    char* vec[ 1000 ];
    int i;
    for (i = 0;    i < 1000;   ++i) {
        vec[i] = "";
    }
    for (i = 0;    i < 1000;   ++i) {
        char buf[ 10 ];
        sprintf( buf, "%d" );
        vec[i] = (char*) malloc( strlen(buf)+1 );
        strcpy( vec[i], buf );
    }
    for (i = 0;    i < 1000;   ++i) {
        free( vec[i] );
    }
}

#define PASSES 100000

main () {
    time_t started_at = time(0);
    int i;
    for (i = PASSES;   i --> 0;  ) {
        malloc_and_free_1000_strings ();
    }

    printf(
        "c-make-strings: %d passes took %d seconds\n", 
        PASSES,
        time(0) - started_at
    );

    exit( 0 );
}
