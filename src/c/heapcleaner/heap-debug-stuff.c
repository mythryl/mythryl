// heap-debug-stuff.c
//
// Heap-centric debug-support code for Heisenbug hunting.

#include "../mythryl-config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "get-multipage-ram-region-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"



void   zero_agegroup0_overrun_tripwire_buffer( Task* task ) {
    // ==========================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we zero that out:
    //
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = 0;
             i < AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS;
             i++
    ){
	//
	p[i] = 0;
    }
//  log_if("zero_agegroup0_overrun_tripwire_buffer: Done zeroing %x -> %x", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1));	// Commented out because it spams the logfile with gigabytes of text.
}

static char*  val_sized_unt_as_ascii(  char* buf,  Val_Sized_Unt u ) {
    //        ======================
    //
    char* p = buf;
    //
    for (int i = 0;
             i < sizeof(Val_Sized_Unt);
             i++
    ){
	//
	char c =  u & 0xFF;
	u      =  u >> 8;
	*p++ = (c >= ' ' && c <= '~') ? c : '.';
    } 

    *p++ = '\0';

    return buf;
}

void   check_agegroup0_overrun_tripwire_buffer( Task* task, char* caller ) {
    // =======================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we verify that it is all zeros:
    //
#ifndef SOON
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS; i --> 0; ) {
	//
	if (p[i] != 0) {
	    //
	    log_if("check_agegroup0_overrun_tripwire_buffer:  While checking %x -> %x agegroup0 buffer overrun of %d words detected at %s", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1), i, caller);
	    //
	    for (int j = 0;
                     j <= i;
                     j++
            ){
		//
		char buf[ 132 ];
		log_if("check_agegroup0_overrun_tripwire_buffer: tripwire_buffer[%3d] x=%08x s='%s'", j, p[j], val_sized_unt_as_ascii(buf, (Val_Sized_Unt)(p[j])));
	    }
	    die( "check_agegroup0_overrun_tripwire_buffer:  Overran agegroup0 buffer by %d words -- see logfile for details.", i);
	    exit(1);										// die() should never return, so this should never execute. But gcc understands it better.
	}
    }
#endif
}

static FILE*   open_heapdump_logfile   (char* filename_buf, size_t bufsize, char* dumptype) {
    //         =====================
    //
    // The idea here is to write each heap snapshot to a separate file
    // to facilitate diff-ing between them and to avoid spamming the
    // main logfile for the process.
    //
    // We want to minimize the chance of two dumps clobbering
    // each other, so we make the snapshot filename depend on:
    //
    //     getpid()
    //     pthread_self()
    //     gettimeofday()
    //     sequentially assigned dump number.
    //
    struct timeval tv;
    //
    if (gettimeofday(&tv,NULL)) {
	fprintf(stderr,"open_heapdump_logfile: gettimeofday failed: %s\n", strerror(errno) );
	exit(1);
    }

    static int dump_number = 0;

    int pid    = getpid();
    int	c_sec  = tv.tv_sec;									// Current time to second accuracy.
    int	c_usec = tv.tv_usec;									// Fractional current time to nominal microsecond accuracy, usually actually good to roughly millisecond resolution.
    int ptid   = (int)(pthread_self());
    //
    snprintf(filename_buf, bufsize, "%sdump-%08x-%08x-%08d.%06d-%d.log", dumptype, pid, ptid, c_sec, c_usec, dump_number++);

    FILE* fd = fopen(filename_buf, "w");
    if (!fd) {
	fprintf(stderr,"open_heapdump_logfile: Unable to fopen(%s): %s\n", filename_buf, strerror(errno) );
	exit(1);
    }
    return fd;
}

static void   close_heapdump_logfile   (FILE* fd, char* filename) {
    //        ======================
    //
    if (fclose(fd)) {
	fprintf(stderr,"close_heapdump_logfile: Unable to fclose(%s): %s\n", filename, strerror(errno) );
	exit(1);
    }
}

static unsigned int   v2u   (Val v) { return (unsigned int) v; }		// More type-safe than using a cast:  Ensures that we really are converting a Val.
//                    ===

// Write to logfile contents of a Task record:
//
void   dump_task   (Task* task, char* caller) {
    // =========
    //
    char filename[ 1024 ];
    FILE* fd = open_heapdump_logfile( filename, 1024, "task" );
    //
    log_if("dump_task: Starting dump to '%s'", filename);

    fprintf(fd,"Task dump generated by dump_task in src/c/heapcleaner/heap-debug-stuff.c\n"	);
    fprintf(fd,"\n"										);
    fprintf(fd,"                     caller s=%s\n",  caller);
    fprintf(fd,"                       task p=%p\n",  task);
    fprintf(fd,"                       heap p=%p\n",  task->heap);
    fprintf(fd,"                    pthread p=%p\n",  task->pthread);
    fprintf(fd,"                pthread->id x=%x\n",  (unsigned int)(task->pthread->tid));
    fprintf(fd,"    heap_allocation_pointer p=%p\n",  task->heap_allocation_pointer);
    fprintf(fd,"      heap_allocation_limit p=%p\n",  task->heap_allocation_limit);
    fprintf(fd," real_heap_allocation_limit p=%p\n",  task->real_heap_allocation_limit);
    fprintf(fd,"                   argument x=%x\n",  v2u( task->argument ));
    fprintf(fd,"                       fate x=%x\n",  v2u( task->fate ));
    fprintf(fd,"            current_closure x=%x\n",  v2u( task->current_closure ));
    fprintf(fd,"              link_register px=%x\n", v2u( task->link_register ));
    fprintf(fd,"            program_counter x=%x\n",  v2u( task->program_counter ));
    fprintf(fd,"             exception_fate x=%x\n",  v2u( task->exception_fate ));
    fprintf(fd,"             current_thread x=%x\n",  v2u( task->current_thread ));
    fprintf(fd,"             heap_changelog x=%x\n",  v2u( task->heap_changelog ));
    fprintf(fd,"            fault_exception x=%x\n",  v2u( task->fault_exception ));
    fprintf(fd,"   faulting_program_counter x=%x\n",  (unsigned int) task->faulting_program_counter);	// Val_Sized_Unt.
    fprintf(fd,"            protected_c_arg p=%p\n",  task->protected_c_arg);
    fprintf(fd,"                  &heapvoid p=%p\n", &task->heapvoid);
    fprintf(fd," Following stuff is in task->heap:\n");
    fprintf(fd,"           agegroup0_buffer p=%p\n",  task->heap->agegroup0_buffer);
    fprintf(fd,"  agegroup0_buffer_bytesize x=%x\n",  (unsigned int) task->heap->agegroup0_buffer_bytesize);	// Punt
    fprintf(fd,"           sum of above two p=%p\n",  (char*)(task->heap->agegroup0_buffer) + task->heap->agegroup0_buffer_bytesize);
    fprintf(fd,"       multipage_ram_region p=%p\n",  task->heap->multipage_ram_region);
    fprintf(fd,"           active_agegroups d=%d\n",  task->heap->active_agegroups);
    fprintf(fd," oldest_agegroup_keeping_idle_fromspace_buffers d=%d\n",  task->heap->oldest_agegroup_keeping_idle_fromspace_buffers);
    fprintf(fd,"  hugechunk_ramregion_count d=%d\n",  task->heap->hugechunk_ramregion_count);
    fprintf(fd,"      total_bytes_allocated x=(%x,%x) (millions, 1s)\n",  (unsigned int)task->heap->total_bytes_allocated.millions, (unsigned int)task->heap->total_bytes_allocated.ones );

    for (int i = 0;
             i < task->heap->active_agegroups;
             i++
    ){
	//
	Agegroup* a = task->heap->agegroup[ i ];
	fprintf(fd,"           agegroup[%d] p=%p (holds agegroup %d)\n",  i, a, i+1);
	fprintf(fd,"           a->age       d=%d\n",  a->age);
	fprintf(fd,"           a->cleanings d=%d\n",  a->cleanings);
	fprintf(fd,"           a->ratio     x=%x (Desired number of collections of the previous agegroup for one collection of this agegroup)\n",  a->ratio);
	fprintf(fd,"           a->last_cleaning_count_of_younger_agegroup d=%d\n",  a->last_cleaning_count_of_younger_agegroup);

	for (int s = 0;
                 s < MAX_PLAIN_ILKS;
                 s++
        ){
	    //
	    fprintf(fd,"\n");
	    fprintf(fd,"          a->sib[%d]    p=%p\n",  s, a->sib[s]);
	    fprintf(fd,"          a->sib[%d].id d=%d\n",  s, a->sib[s]->id);
	    fprintf(fd,"\n");
	    fprintf(fd,"          a->sib[%d].tospace            p=%p\n",  s, a->sib[s]->tospace);
	    fprintf(fd,"          a->sib[%d].tospace_bytesize   p=%p\n",  s, (void*)a->sib[s]->tospace_bytesize);		// Punt
	    fprintf(fd,"          a->sib[%d].tospace_limit      p=%p\n",  s, a->sib[s]->tospace_limit);
	    fprintf(fd,"\n");
	    fprintf(fd,"          a->sib[%d].fromspace          p=%p\n",  s, a->sib[s]->fromspace);
	    fprintf(fd,"          a->sib[%d].fromspace_bytesize x=%x\n",  s, (unsigned int) a->sib[s]->fromspace_bytesize);	// Val_Sized_Unt
	    fprintf(fd,"          a->sib[%d].fromspace_used_end p=%p\n",  s, a->sib[s]->fromspace_used_end);
	    fprintf(fd,"\n");
	    fprintf(fd,"          a->sib[%d].next_tospace_word_to_allocate p=%p\n",  s, a->sib[s]->next_tospace_word_to_allocate);
	    fprintf(fd,"          a->sib[%d].next_word_to_sweep_in_tospace p=%p\n",  s, a->sib[s]->next_word_to_sweep_in_tospace);
	    fprintf(fd,"          a->sib[%d].repairlist p=%p\n",  s, a->sib[s]->repairlist);
	    fprintf(fd,"          a->sib[%d].end_of_fromespace_oldstuff p=%p\n",  s, a->sib[s]->end_of_fromspace_oldstuff);
	}
    }
    close_heapdump_logfile( fd, filename );
    log_if("dump_task: Dump to '%s' now complete.", filename);
}


static void   dump_records   (FILE* fd, Val* start, Val* stop) {
    //        ============
    //    
    int words_left_in_record = 0;
    //
    for (Val* p = start;
	      p < stop;
	      p++
    ){
	//
	if (words_left_in_record > 0
	||  !IS_TAGWORD(*p)
	){
	    if (!words_left_in_record)   fprintf(fd,"\n");
	    //
	    char buf[ 132 ];
	    char* as_ascii = val_sized_unt_as_ascii(buf,(Val_Sized_Unt)(*p));
	    fprintf(fd," %8p: %08x  %s",  p, v2u(*p), as_ascii);
	    //
	    if (!words_left_in_record
	    && (!(((int)p)&4))					// We use padwords to 64-bit align a record payload, which non-64-bits-aligns the record tagword itself -- so the padword itself is 64-bit aligned.  Unintuitive.
	    && IS_TAGWORD(p[1])
	    && GET_BTAG_FROM_TAGWORD(p[1])==EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG
	    ){
		fprintf(fd,"    (padword to 64-bit align payload of following record)");
	    }
	    //
	    fprintf(fd,"\n");
	    --words_left_in_record;

	} else {

	    int   words = GET_LENGTH_IN_WORDS_FROM_TAGWORD(*p);
	    Bool  words_is_bogus = FALSE;
	    Bool    tag_is_bogus = FALSE;
	    char* tag;

	    switch (GET_BTAG_FROM_TAGWORD(*p)) {
	    //
	    case PAIRS_AND_RECORDS_BTAG:				tag = "RECORD";					break;
	    //
	    case RW_VECTOR_HEADER_BTAG:
		words_is_bogus = TRUE;				// 'words' field is abused for additional tag info in this case -- length is implicitly fixed at 2 words.
		switch (words) {
		case VECTOR_OF_EIGHT_BYTE_FLOATS_CTAG:		tag = "FLOAT64_RW_VECTOR HEADER";		break;
		case TYPEAGNOSTIC_VECTOR_CTAG:			tag = "TYPEAGNOSTIC_RW_VECTOR HEADER";		break;
		case VECTOR_OF_ONE_BYTE_UNTS_CTAG:			tag = "UNT8_RW_VECTOR HEADER";			break;
		default:	tag_is_bogus = TRUE;			tag = "???_RW_VECTOR HEADER";			break;
		}
		words = 2;
		break;
	    //
	    case RO_VECTOR_HEADER_BTAG:
		words_is_bogus = TRUE;				// 'words' field is abused for additional tag info in this case -- length is implicitly fixed at 2 words.
		switch (words) {
		case TYPEAGNOSTIC_VECTOR_CTAG:			tag = "TYPEAGNOSTIC_RO_VECTOR HEADER";		break;
		case VECTOR_OF_ONE_BYTE_UNTS_CTAG:			tag = "STRING / UNT8_RO_VECTOR HEADER";		break;
		default:	tag_is_bogus = TRUE;			tag = "???_RO_VECTOR HEADER";			break;
		}
		words = 2;
		break;
	    //
	    case RW_VECTOR_DATA_BTAG:				tag = "RW_VECTOR_DATA";				break;
	    case FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:		tag = "NONPTR_DATA4";				break;
	    case EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:		tag = "NONPTR_DATA8";				break;
	    case EXTERNAL_REFERENCE_IN_EXPORTED_HEAP_IMAGE_BTAG:	tag = "EXTERNAL_REF";				break;
	    case FORWARDED_CHUNK_BTAG:				tag = "FORWARDED";				break;
	    //
	    case WEAK_POINTER_OR_SUSPENSION_BTAG:
		words_is_bogus = TRUE;				// 'words' field is abused for additional tag info in this case -- length is implicitly fixed at 2 words.
		switch (words) {
		case UNEVALUATED_LAZY_SUSPENSION_CTAG:		tag = "UNEVAL_SUSPENSION";			break;
		case   EVALUATED_LAZY_SUSPENSION_CTAG:		tag =   "EVAL_SUSPENSION";			break;
		case                WEAK_POINTER_CTAG:		tag = "WEAK_PTR";				break;
		case         NULLED_WEAK_POINTER_CTAG:		tag = "NULLED_WEAK_PTR";			break;
		default:	tag_is_bogus = TRUE;			tag = "??? (bogus WEAK/LAZY)";			break;
		}	
		words = 1;
		break;
	    //
	    default:	tag_is_bogus = TRUE;			tag = "???";					break;
	    }

	    if (!tag_is_bogus)	words_left_in_record = words;		// If the tag is bogus, the length probably is too, so ignore it.

	    if (words_is_bogus)	fprintf(fd,"\n %8p: %08x %s\n",          p, v2u(*p),        tag);
	    else			fprintf(fd,"\n %8p: %08x %d-word %s\n",  p, v2u(*p), words, tag);
	}
    }
}


// Write to logfile contents of the generation0
// buffer for a given Task.  No attempt is made
// to distinguish live from dead data.
//
void   dump_gen0   (Task* task, char* caller) {
    // =========
    //
    char filename[ 1024 ];
    FILE* fd = open_heapdump_logfile( filename, 1024, "gen0" );

    log_if("dump_gen0: Starting dump to '%s'", filename);

    fprintf(fd,"Generation-0 dump generated by dump_gen0 in src/c/heapcleaner/heap-debug-stuff.c\n"	);
    fprintf(fd,"\n"											);
    fprintf(fd," generation-0 heapbuffer dump for pthread->id x=%x, called by %s\n",  (unsigned int)(task->pthread->tid), caller);
    fprintf(fd,"           agegroup0_buffer p=%p\n",  task->heap->agegroup0_buffer);
    fprintf(fd,"  agegroup0_buffer_bytesize x=%x\n",  (unsigned int)task->heap->agegroup0_buffer_bytesize);				// Punt
    fprintf(fd,"           sum of above two p=%p\n",  (char*)(task->heap->agegroup0_buffer) + task->heap->agegroup0_buffer_bytesize);
    fprintf(fd,"    heap_allocation_pointer p=%p\n",  task->heap_allocation_pointer);
    fprintf(fd,"      heap_allocation_limit p=%p\n",  task->heap_allocation_limit);
    fprintf(fd," real_heap_allocation_limit p=%p\n",  task->real_heap_allocation_limit);
    fprintf(fd," gen0 buffer contents:\n\n");

    dump_records( fd, task->heap->agegroup0_buffer, task->heap_allocation_pointer );

    close_heapdump_logfile( fd, filename );
    log_if("dump_gen0: Dump to '%s' now complete.", filename);
}



static void   dump_sib   (Task* task, FILE* fd, Sib* sib) {
    //        ========
    fprintf(fd,"                     sib->id x= %04x  = id%01x/kind%02x/age%01x\n",	sib->id, GET_ID_FROM_SIBID(sib->id), GET_KIND_FROM_SIBID(sib->id), GET_AGE_FROM_SIBID(sib->id)	);
    fprintf(fd,"                     sib->next_tospace_word_to_allocate p= %p\n",	sib->next_tospace_word_to_allocate			);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->tospace          p= %p\n",				sib->tospace					);
    fprintf(fd,"                     sib->tospace_bytesize x= 0x%08x\n",	(unsigned int)  sib->tospace_bytesize				);
    fprintf(fd,"                     sib->tospace_limit    p= %p\n",				sib->tospace_limit				);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->next_word_to_sweep_in_tospace p= %p\n",		sib->next_word_to_sweep_in_tospace		);
    fprintf(fd,"                     sib->repairlist p= %p\n",					sib->repairlist					);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->fromspace          p= %p\n",				sib->fromspace					);
    fprintf(fd,"                     sib->fromspace_bytesize x= 0x%08x\n",(unsigned int)	sib->fromspace_bytesize				);
    fprintf(fd,"                     sib->fromspace_used_end p= %p\n",				sib->fromspace_used_end				);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->end_of_fromspace_oldstuff p= %p\n",		sib->end_of_fromspace_oldstuff				);
    fprintf(fd,"                     sib->sib_for_promoted_chunks   p= %p\n",		sib->sib_for_promoted_chunks				);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->heap_needs_repair b= %s\n",			sib->heap_needs_repair ? "TRUE" : "FALSE"		);
    fprintf(fd,"\n"																);
    fprintf(fd,"                     sib->requested_sib_buffer_bytesize x= %08x\n",	(unsigned int) sib->requested_sib_buffer_bytesize	);
    fprintf(fd,"                     sib->soft_max_bytesize             x= %08x\n",	(unsigned int) sib->soft_max_bytesize			);
}



static void   dump_record_sib   (Task* task, FILE* fd, Sib* sib) {
    //        ===============
    //
    dump_records   (fd, sib->tospace, sib->next_tospace_word_to_allocate);
}

static void   dump_pair_sib   (Task* task, FILE* fd, Sib* sib) {	// The pairs in the pair sib have no tag/length word -- avoiding that overhead is the main point of having a separate sib for pairs.
    //        =============
    for (Val* p = sib->tospace;
	      p < sib->next_tospace_word_to_allocate;
	      p += 2
    ){
	fprintf(fd,"\n");
	fprintf(fd," %8p: %08x\n",  p, v2u(*p));
	fprintf(fd," %8p: %08x\n",  p, v2u(*p));
    }    
}

static void   dump_string_sib   (Task* task, FILE* fd, Sib* sib) {
    //        ===============
    //
    for (Val* p = sib->tospace;
	      p < sib->next_tospace_word_to_allocate;
	      p++
    ){
	char buf[ 132 ];
	char* as_ascii = val_sized_unt_as_ascii(buf,(Val_Sized_Unt)(*p));
	fprintf(fd," %8p: %08x  %s\n",  p, v2u(*p), as_ascii);
    }    
}

static void   dump_vector_sib   (Task* task, FILE* fd, Sib* sib) {
    //        ===============
    //
    dump_records   (fd, sib->tospace, sib->next_tospace_word_to_allocate);

//    for (Val* p = sib->tospace;
//	      p < sib->next_tospace_word_to_allocate;
//	      p++
//    ){
//	fprintf(fd," %8p: %08x\n",  p, v2u(*p));
//    }    
}



// Write to logfile contents of the generation-1
// buffer for a given Task.  No attempt is made
// to distinguish live from dead data.
//
void   dump_gens   (Task* task, char* caller) {
    // =========
    //
    Heap*     heap =  task->heap;

    char filename[ 1024 ];
    FILE* fd = open_heapdump_logfile( filename, 1024, "gens" );

    log_if("dump_gens: Starting dump to '%s'", filename);

    fprintf(fd,"Generation-1 dump generated by dump_gens in src/c/heapcleaner/heap-debug-stuff.c\n"							);
    fprintf(fd,"\n"																	);

    for (int a = 0;
             a < task->heap->active_agegroups;
             a++
    ){
	fprintf(fd,"\n"																	);
	fprintf(fd,"\n"																	);
	fprintf(fd,"=====================\n"														);
	fprintf(fd,"Dump of generation %d\n", a+1);
	fprintf(fd,"\n"																	);

	Agegroup* ag  =  heap->agegroup[a];		// Get pointer to our agegroup.

	fprintf(fd,"                     task p= %p\n",					task								);
	fprintf(fd,"                     heap p= %p  (task->heap)\n",			heap								);
	fprintf(fd,"                      ag p= %p  (agegroup%d)\n", a+1,		ag								);
	fprintf(fd,"\n"																	);
	fprintf(fd,"                 ag->age d= %d\n",						ag->age							);
	fprintf(fd,"                 ag->cleanings d= %d\n",					ag->cleanings						);
	fprintf(fd,"                 ag->ratio     d= %d\n",					ag->ratio						);
	fprintf(fd,"                 ag->last_cleaning_count_of_younger_agegroup d= %d\n",	ag->last_cleaning_count_of_younger_agegroup		);
	fprintf(fd,"                 ag->tospace_ram_region p= %p\n",				ag->tospace_ram_region					);
	fprintf(fd,"                 ag->fromspace_ram_region p= %p\n",				ag->fromspace_ram_region				);
	fprintf(fd,"                 ag->saved_fromspace_ram_region p= %p\n",			ag->saved_fromspace_ram_region				);
	fprintf(fd,"                 ag->coarse_inter_agegroup_pointers_map p= %p\n",		ag->coarse_inter_agegroup_pointers_map			);


	fprintf(fd,"                 ag->hugechunks[0] p= %p   (CODE__HUGE_ILK)\n",		ag->hugechunks[0]					);

	fprintf(fd,"                 ag->sib[%d] p= %p    (RECORD_ILK)\n",			RECORD_ILK,	ag->sib[RECORD_ILK]			);
	fprintf(fd,"                 ag->sib[%d] p= %p    (  PAIR_ILK)\n",			  PAIR_ILK,	ag->sib[  PAIR_ILK]			);
	fprintf(fd,"                 ag->sib[%d] p= %p    (STRING_ILK)\n",			STRING_ILK,	ag->sib[STRING_ILK]			);
	fprintf(fd,"                 ag->sib[%d] p= %p    (VECTOR_ILK)\n",			VECTOR_ILK,	ag->sib[VECTOR_ILK]			);


	fprintf(fd,"\n------------------------\nRECORD_ILK sib details:\n");    dump_sib(task,fd, ag->sib[RECORD_ILK] );
	fprintf(fd,"\n------------------------\n  PAIR_ILK sib details:\n");    dump_sib(task,fd, ag->sib[  PAIR_ILK] );
	fprintf(fd,"\n------------------------\nSTRING_ILK sib details:\n");    dump_sib(task,fd, ag->sib[STRING_ILK] );
	fprintf(fd,"\n------------------------\nVECTOR_ILK sib details:\n");    dump_sib(task,fd, ag->sib[VECTOR_ILK] );


	fprintf(fd,"\n------------------------\nRECORD_ILK sib contents -- immutable pointerblocks:\n");		dump_record_sib( task, fd, ag->sib[RECORD_ILK] );
	fprintf(fd,"\n------------------------\n  PAIR_ILK sib contents -- immutable pointerpairs:\n");			  dump_pair_sib( task, fd, ag->sib[  PAIR_ILK] );
	fprintf(fd,"\n------------------------\nSTRING_ILK sib contents -- immutable nonpointer stuff:\n");		dump_string_sib( task, fd, ag->sib[STRING_ILK] );
	fprintf(fd,"\n------------------------\nVECTOR_ILK sib contents -- all mutable data goes in this sib:\n");	dump_vector_sib( task, fd, ag->sib[VECTOR_ILK] );
    }
    close_heapdump_logfile( fd, filename );
    log_if("dump_gens: Dump to '%s' now complete.", filename);
}

// Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

