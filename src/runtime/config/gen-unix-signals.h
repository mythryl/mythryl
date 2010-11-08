/* gen-unix-signals.h
 *
 */

typedef struct {
    int		    sig;	/* the UNIX signal code */
    char	    *sigName;	/* the symbolic name of the signal (i.e., */
				/* the #define name). */
    char	    *shortName;	/* the short name of the signal passed to Lib7 */
} sig_desc_t;

typedef struct {
    sig_desc_t	    **sigs;	/* an ordered vector of signal descriptions */
    int		    numSysSigs;	/* the number of system signals */
    int		    numRunSigs; /* the number of runtime signals */
    int		    minSysSig;	/* the minimum system signal number. */
    int		    maxSysSig;	/* the maximum system signal number. */
} sig_info_t;

extern sig_info_t *SortSignalTable ();



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
