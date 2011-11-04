// import-heap.c
//
// Routines to import a Mythryl heap image.

#include "../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include "runtime-base.h"
#include "architecture-and-os-names-system-dependent.h"
#include "get-multipage-ram-region-from-os.h"
#include "flush-instruction-cache-system-dependent.h"
#include "task.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "runtime-heap-image.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "address-hashtable.h"
#include "import-heap-stuff.h"
#include "heap-io.h"

#if HAVE_DLFCN_H
    #include <dlfcn.h>
#endif

#ifdef DEBUG
    static void   print_region_map   (Hugechunk_Region_Relocation_Info* r)   {
        //
	Hugechunk_Relocation_Info* dp;
	Hugechunk_Relocation_Info* dq;

	debug_say ("region @%#x: |", r->first_ram_quantum);

	for (int i = 0, dq = r->chunkmap[0];  i < r->page_count;  i++) {
	    //
	    dp = r->chunkmap[i];

	    if (dp != dq) {
		debug_say ("|");
		dq = dp;
	    }

	    if (dp == NULL)	debug_say ("_");
	    else		debug_say ("X");
	}
	debug_say ("|\n");
    }
#endif



static void         read_heap			(Inbuf* bp,  Heap_Header* header,  Task* task,  Val* externs );
static Hugechunk*   allocate_a_hugechunk	(Hugechunk*, Hugechunk_Header*, Hugechunk_Region_Relocation_Info* );
static void         repair_heap			(Heap*, Sibid*, Punt [MAX_AGEGROUPS][MAX_PLAIN_ILKS], Addresstable*, Val*);
static Val          repair_word			(Val w,   Sibid* oldBOOK2SIBID,   Punt addrOffset[MAX_AGEGROUPS][MAX_PLAIN_ILKS],   Addresstable* boRegionTable,   Val* externs);

// static int   RepairBORef   (Sibid* book2sibid,  Sibid id,  Val* ref,  Val oldChunk);

static Hugechunk_Relocation_Info*   address_to_relocation_info   (Sibid*,  Addresstable*,  Sibid,  Punt);

#define READ(bp,chunk)	heapio__read_block(bp, &(chunk), sizeof(chunk))


Task*   import_heap_image   (const char* fname, Heapcleaner_Args* params) {
    //  =================
    //
    Task*		task;
    Heapfile_Header	image_header;
    Heap_Header	heap_header;
    Val		*externs;
    Pthread_Image	image;
    Inbuf		inbuf;

    if (fname != NULL) {
	//
        // Resolve the name of the image.
        //  If the file exists use it, otherwise try the
        // pathname with the machine ID as an extension.

	if ((inbuf.file = fopen(fname, "rb"))) {
	    //
	    if (verbosity > 0)   say("loading %s ", fname);

	} else {
	    //
	    if ((inbuf.file = fopen(fname, "rb"))) {
		//
	        if (verbosity > 0)   say("loading %s ", fname);

	    } else {

		die ("unable to open heap image \"%s\"\n", fname);
	    }
	}

	inbuf.needs_to_be_byteswapped = FALSE;
	inbuf.buf	    = NULL;
	inbuf.nbytes    = 0;

    } else {
	//
	// fname == NULL, so try to find
	// an in-core heap image:

  	#if defined(DLOPEN) && !defined(OPSYS_WIN32)
	    //
	    void *lib = dlopen (NULL, RTLD_LAZY);
	    void *vimg, *vimglenptr;

	    if ((vimg       = dlsym(lib,HEAP_IMAGE_SYMBOL    )) == NULL)      die("no in-core heap image found\n");
	    if ((vimglenptr = dlsym(lib,HEAP_IMAGE_LEN_SYMBOL)) == NULL)      die("unable to find length of in-core heap image\n");

	    inbuf.file      = NULL;
	    inbuf.needs_to_be_byteswapped = FALSE;

	    inbuf.base      = vimg;
	    inbuf.buf       = inbuf.base;
	    inbuf.nbytes    = *(long*)vimglenptr;
        #else
	    die("in-core heap images not implemented\n");
        #endif
    }

    READ(&inbuf, image_header);

    if (image_header.byte_order != ORDER)						die ("incorrect byte order in heap image\n");
    if (image_header.magic != IMAGE_MAGIC)						die ("bad magic number (%#x) in heap image\n", image_header.magic);
    if ((image_header.kind != EXPORT_HEAP_IMAGE) && (image_header.kind != EXPORT_FN_IMAGE))	die ("bad image kind (%d) in heap image\n", image_header.kind);

    READ(&inbuf, heap_header);

    // Check for command-line overrides of heap parameters:
    //
    if (params->agegroup0_buffer_bytesize == 0) {
        params->agegroup0_buffer_bytesize = heap_header.agegroup0_buffer_bytesize;
    }
    if (params->active_agegroups < heap_header.active_agegroups) {
        params->active_agegroups = heap_header.active_agegroups;
    }
    if (params->oldest_agegroup_keeping_idle_fromspace_buffers < 0) {
        params->oldest_agegroup_keeping_idle_fromspace_buffers = heap_header.oldest_agegroup_keeping_idle_fromspace_buffers;
    } 

    task = make_task( FALSE, params );						// make_task		def in   src/c/main/runtime-state.c

    // Get the run-time pointers into the heap:
    //
    *PTR_CAST( Val*, PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL )
        =
        heap_header.pervasive_package_pickle_list;

    // This carefully constructed fake looks like a normal
    // compiled package from the Mythryl side but actually
    // links to compile C code -- see the hack in
    //	
    //     src/c/main/load-compiledfiles.c
    //
    runtime_package__global =  heap_header.runtime_pseudopackage;

    #ifdef ASM_MATH
	mathvec__global = heap_header.math_package;
    #endif


    externs = heapio__read_externs_table (&inbuf);		// Read the externals table.

    READ(&inbuf, image);				// Read and initialize the Mythryl state info.
    //
    if (image_header.kind == EXPORT_HEAP_IMAGE) {

        // Load the live registers:
        //
	ASSIGN( POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL, image.posix_interprocess_signal_handler );
	//
	task->argument	= image.stdArg;
	task->fate		= image.stdCont;
	task->closure	= image.stdClos;
	task->program_counter= image.pc;
	task->exception_fate	= image.exception_fate;
	task->thread	= image.current_thread;
	task->callee_saved_registers[0]	= image.calleeSave[0];
	task->callee_saved_registers[1]	= image.calleeSave[1];
	task->callee_saved_registers[2]	= image.calleeSave[2];

	read_heap (&inbuf, &heap_header, task, externs);			// Read the Mythryl heap.

	/* cleaner_messages_are_enabled__global = TRUE; */					// Cleaning messages are on by default for interactive images.

    } else { 								// EXPORT_FN_IMAGE

        Val function_to_run;
	Val program_name;
	Val args;

        // Restore the signal handler:
        //
	ASSIGN( POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL, image.posix_interprocess_signal_handler );

        // Read the Mythryl heap:
        //
	task->argument		= image.stdArg;
	read_heap (&inbuf, &heap_header, task, externs);

        // Initialize the calling context (taken from run_mythryl_function):					// run_mythryl_function		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
        //
	function_to_run		= task->argument;
	//
	task->exception_fate	= PTR_CAST( Val,  handle_uncaught_exception_closure_v + 1 );
	task->thread		= HEAP_VOID;
	//
	task->fate		= PTR_CAST( Val,  return_to_c_level_c );
	task->closure		= function_to_run;
	//
	task->program_counter	=
	task->link_register	= GET_CODE_ADDRESS_FROM_CLOSURE( function_to_run );

        // Set up the arguments to the imported function:
        //
	program_name = make_ascii_string_from_c_string(task, mythryl_program_name__global);
	args = make_ascii_strings_from_vector_of_c_strings (task, commandline_arguments);
	REC_ALLOC2(task, task->argument, program_name, args);

	// debug_say("arg = %#x : [%#x, %#x]\n", task->argument, GET_TUPLE_SLOT_AS_VAL(task->argument, 0), GET_TUPLE_SLOT_AS_VAL(task->argument, 1));

        // Cleaner messages are off by
        // default for spawn_to_disk images:
        //
	cleaner_messages_are_enabled__global =  FALSE;
    }

    FREE( externs );

    if (inbuf.file)   fclose (inbuf.file);

    if (verbosity > 0)   say(" done\n");

    return task;
}								// fun import_heap_image



static void   read_heap   (
    //        =========
    //
    Inbuf*       bp,
    Heap_Header* header,
    Task*        task,
    Val*         externs
){
    Heap*		heap =  task->heap;

    Sib_Header*	sib_headers;
    Sib_Header*	p;
    Sib_Header*	q;

    int			sib_headers_bytesize;
    int			i, j, k;

    long		prevSzB[MAX_PLAIN_ILKS], size;
    Sibid*		oldBOOK2SIBID;
    Punt		addrOffset[MAX_AGEGROUPS][MAX_PLAIN_ILKS];

    Hugechunk_Region_Relocation_Info*	boRelocInfo;

    Addresstable*	boRegionTable;

    // Allocate a book_to_sibid__global for the imported
    // heap image's address space:
    //
    #ifdef TWO_LEVEL_MAP
        #error two level map not supported
    #else
	oldBOOK2SIBID = MALLOC_VEC (Sibid, BOOK2SIBID_TABLE_SIZE_IN_SLOTS);
    #endif

    // Read in the hugechunk region descriptors
    // for the old address space:
    //
    {
	int		  size;
	Hugechunk_Region_Header* boRgnHdr;

	boRegionTable = make_address_hashtable(LOG2_BOOK_BYTESIZE+1, header->hugechunk_ramregion_count);

	size = header->hugechunk_ramregion_count * sizeof(Hugechunk_Region_Header);

	boRgnHdr = (Hugechunk_Region_Header*) MALLOC (size);

	heapio__read_block( bp, boRgnHdr, size );

	boRelocInfo = MALLOC_VEC(Hugechunk_Region_Relocation_Info, header->hugechunk_ramregion_count);

	for (i = 0;  i < header->hugechunk_ramregion_count;  i++) {

	    set_book2sibid_entries_for_range(oldBOOK2SIBID,
		(Val*)(boRgnHdr[i].base_address),
		BOOKROUNDED_BYTESIZE(boRgnHdr[i].bytesize),
		HUGECHUNK_DATA_SIBID(1)
            );

	    oldBOOK2SIBID[GET_BOOK_CONTAINING_POINTEE(boRgnHdr[i].base_address)] = HUGECHUNK_RECORD_SIBID(MAX_AGEGROUPS);

	    boRelocInfo[i].first_ram_quantum = boRgnHdr[i].first_ram_quantum;

	    boRelocInfo[i].page_count
                =
                (boRgnHdr[i].bytesize - (boRgnHdr[i].first_ram_quantum - boRgnHdr[i].base_address))
                >>
                LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

	    boRelocInfo[i].hugechunk_page_to_hugechunk = MALLOC_VEC(Hugechunk_Relocation_Info*, boRelocInfo[i].page_count);

	    for (j = 0;  j < boRelocInfo[i].page_count;  j++) {
	        //
		boRelocInfo[i].hugechunk_page_to_hugechunk[j] = NULL;
            } 
	    addresstable_insert (boRegionTable, boRgnHdr[i].base_address, &(boRelocInfo[i]));
	}
	FREE (boRgnHdr);
    }

    // Read the sib headers:
    //
    sib_headers_bytesize = header->active_agegroups * TOTAL_ILKS * sizeof( Sib_Header );
    //
    sib_headers = (Sib_Header*) MALLOC( sib_headers_bytesize );
    //
    heapio__read_block( bp, sib_headers, sib_headers_bytesize );

    for (i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	prevSzB[i] = heap->agegroup0_buffer_bytesize;
    }

    // Allocate the sib buffers and read in the heap image:
    //
    for (p = sib_headers, i = 0;  i < header->active_agegroups;  i++) {
        //
	Agegroup*  age =  heap->agegroup[ i ];

	// Compute the space required for this agegroup,
	// and mark the oldBOOK2SIBID to reflect the old address space:
	//
	for (q = p, j = 0;  j < MAX_PLAIN_ILKS;  j++) {

	    set_book2sibid_entries_for_range (
		//
		oldBOOK2SIBID,

		(Val*) q->info.o.base_address,

		BOOKROUNDED_BYTESIZE( q->info.o.bytesize ),

		age->sib[ j ]->id
	    );

	    size = q->info.o.bytesize + prevSzB[j];

	    if (j == PAIR_ILK
            &&  size > 0
            ){
		size += 2*WORD_BYTESIZE;
	    }

	    age->sib[ j ]->tospace_bytesize
		=
		BOOKROUNDED_BYTESIZE( size );

	    prevSzB[ j ] =  q->info.o.bytesize;

	    q++;
	}

	if (allocate_and_partition_an_agegroup(age) == FAILURE) {
	    die ("unable to allocated space for agegroup %d\n", i+1);
        } 
	if (sib_is_active( age->sib[ VECTOR_ILK ] )) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    make_new_coarse_inter_agegroup_pointers_map_for_agegroup (age);
        }

	// Read in the sib buffers for this agegroup
	// and initialize the address offset table:
	//
	for (int j = 0;  j < MAX_PLAIN_ILKS;  j++) {
	    //
	    Sib* ap = age->sib[ j ];

	    if (p->info.o.bytesize > 0) {

		addrOffset[i][j] = (Punt)(ap->tospace) - (Punt)(p->info.o.base_address);

		heapio__seek( bp, (long) p->offset );

		heapio__read_block( bp, (ap->tospace), p->info.o.bytesize );

		ap->next_tospace_word_to_allocate  = (Val *)((Punt)(ap->tospace) + p->info.o.bytesize);
		ap->end_of_fromspace_oldstuff = ap->tospace;

	    } else if (sib_is_active(ap)) {

		ap->end_of_fromspace_oldstuff =  ap->tospace;
	    }

	    if (verbosity > 0)   say(".");

	    p++;
	}

        // Read in the hugechunk sib buffers (currently just codechunks):
        //
	for (int ilk = 0;  ilk < MAX_HUGE_ILKS;  ilk++) {			// MAX_HUGE_ILKS		def in    src/c/h/sibid.h
	    //	
	    Punt	 totSizeB;

	    Hugechunk* freeChunk;
	    Hugechunk* bdp = NULL;		// Without this initialization, gcc -Wall gives a 'possible uninitialized use' warning.

	    Hugechunk_Region*	 free_region;
	    Hugechunk_Header*	 boHdrs;

	    int			 boHdrSizeB;
	    int			 index;

	    Hugechunk_Region_Relocation_Info*  region;

	    if (p->info.bo.hugechunk_quanta_count > 0) {
		//
		totSizeB = p->info.bo.hugechunk_quanta_count << LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

		freeChunk = allocate_hugechunk_region( heap, totSizeB );

		free_region = freeChunk->region;

		free_region->age_of_youngest_live_chunk_in_region
		    =
                    i;

		set_book2sibid_entries_for_range (
		    //
		    book_to_sibid__global,
                    (Val*) free_region,
		    BYTESIZE_OF_MULTIPAGE_RAM_REGION( free_region->ram_region ),
		    HUGECHUNK_DATA_SIBID( i )
		);

		book_to_sibid__global[ GET_BOOK_CONTAINING_POINTEE( free_region ) ]
		    =
		    HUGECHUNK_RECORD_SIBID( i );

	        // Read in the hugechunk headers:
                //
		boHdrSizeB = p->info.bo.hugechunk_count * sizeof(Hugechunk_Header);
		//
		boHdrs = (Hugechunk_Header*) MALLOC (boHdrSizeB);
		//
		heapio__read_block (bp, boHdrs, boHdrSizeB);

	        // Read in the hugechunks:
                //
		heapio__read_block( bp, (void *)(freeChunk->chunk), totSizeB );
		//
		if (ilk == CODE__HUGE_ILK) {					// ilk = 0 == CODE__HUGE_ILK	def in    src/c/h/sibid.h
		    //
		    flush_instruction_cache ((void *)(freeChunk->chunk), totSizeB);
		}

	        // Set up the hugechunk descriptors 
                // and per-chunk relocation info:
                //
		for (k = 0;  k < p->info.bo.hugechunk_count;  k++) {
		    //
		    // Find the region relocation info for the
		    // chunk's region in the exported heap:
		    //
		    for (index = GET_BOOK_CONTAINING_POINTEE(boHdrs[k].base_address);
			!SIBID_ID_IS_BIGCHUNK_RECORD(oldBOOK2SIBID[index]);
			index--)
			continue;

		    region = LOOK_UP_HUGECHUNK_REGION (boRegionTable, index);

		    // Allocate the hugechunk record for
		    // the chunk and link it into the list
                    // of hugechunks for its agegroup.
		    //
		    bdp = allocate_a_hugechunk( freeChunk, &(boHdrs[k]), region );

		    bdp->next = age->hugechunks[ ilk ];

		    age->hugechunks[ ilk ] = bdp;

		    ASSERT( bdp->gen == i+1 );

		    if (codechunk_comment_display_is_enabled__global
                    &&  ilk == CODE__HUGE_ILK
                    ){
		        // Dump the comment string of the code chunk.

			Unt8* namestring;
			//
			if ((namestring = get_codechunk_comment_string_else_null( bdp ))) {
			    debug_say ("[%6d bytes] %s\n", bdp->bytesize, (char*)namestring);
                        }
		    }
		}

		if (freeChunk != bdp) {					// if p->info.bo.hugechunk_count can be zero, 'bdp' value here may be bogus. XXX BUGGO FIXME.
		    //
		    // There was some extra space left in the region:
		    //
		    insert_hugechunk_in_doubly_linked_list( heap->hugechunk_freelist, freeChunk);						// insert_hugechunk_in_doubly_linked_list	def in   src/c/h/heap.h
		}

		FREE (boHdrs);
	    }

	    if (verbosity > 0)   say(".");

	    p++;
	}
    }

    repair_heap (heap, oldBOOK2SIBID, addrOffset, boRegionTable, externs);

    // Adjust the run-time globals
    // that point into the heap:
    //
    *PTR_CAST( Val*, PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL )
        =
        repair_word(
            *PTR_CAST( Val*, PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL ),
	    oldBOOK2SIBID,
            addrOffset,
            boRegionTable,
            externs
        );

    runtime_package__global = repair_word( runtime_package__global, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

#ifdef ASM_MATH
    mathvec__global = repair_word (mathvec__global, oldBOOK2SIBID, addrOffset, boRegionTable, externs);
#endif

    // Adjust the Mythryl registers
    // to the new address space:
    //
    ASSIGN(
        POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL,
	//
        repair_word (
	    //
	    DEREF( POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL ),
	    oldBOOK2SIBID,
	    addrOffset,
	    boRegionTable,
            externs
	)
    );

    task->argument
	=
	repair_word( task->argument, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->fate
	=
	repair_word( task->fate, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->closure
	=
	repair_word( task->closure, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->program_counter
	=
	repair_word(  task->program_counter, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->link_register
	=
	repair_word (task->link_register, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->exception_fate
	=
	repair_word( task->exception_fate, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->thread
	=
	repair_word( task->thread, oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->callee_saved_registers[0]
	=
	repair_word( task->callee_saved_registers[0], oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->callee_saved_registers[1]
	=
	repair_word( task->callee_saved_registers[1], oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    task->callee_saved_registers[2]
	=
	repair_word( task->callee_saved_registers[2], oldBOOK2SIBID, addrOffset, boRegionTable, externs );

    // Release storage:
    //
    for (i = 0; i < header->hugechunk_ramregion_count;  i++) {
      //
	Hugechunk_Relocation_Info*	p;
	for (p = NULL, j = 0;  j < boRelocInfo[i].page_count;  j++) {
	    if ((boRelocInfo[i].hugechunk_page_to_hugechunk[j] != NULL)
	    && (boRelocInfo[i].hugechunk_page_to_hugechunk[j] != p)) {
		FREE (boRelocInfo[i].hugechunk_page_to_hugechunk[j]);
		p = boRelocInfo[i].hugechunk_page_to_hugechunk[j];
	    }
	}
    }

    free_address_table( boRegionTable, FALSE );

    FREE( boRelocInfo    );
    FREE( sib_headers  );
    FREE( oldBOOK2SIBID       );

    // Reset the next_word_to_sweep_in_tospace pointers:
    //
    for (int i = 0;  i < heap->active_agegroups;  i++) {
        //
	Agegroup*	age =  heap->agegroup[i];
        //
	for (int j = 0;  j < MAX_PLAIN_ILKS;  j++) {
	    //
	    Sib* ap =  age->sib[ j ];
	    //
	    if (sib_is_active(ap)) {							// sib_is_active	def in    src/c/h/heap.h
		//
		ap->next_word_to_sweep_in_tospace
		    =
		    ap->next_tospace_word_to_allocate;
	    }
	}
    }
}                                                       // fun read_heap



static Hugechunk*   allocate_a_hugechunk   (
    //              ====================
    //
    Hugechunk*                          free,
    Hugechunk_Header*                   header,
    Hugechunk_Region_Relocation_Info*   old_region
) {
    Hugechunk*  new_chunk;

    Hugechunk_Relocation_Info* reloc_info;

    int	 first_ram_quantum;

    int total_bytesize
	=
	ROUND_UP_TO_POWER_OF_TWO(
	    //
	    header->bytesize,
	    HUGECHUNK_RAM_QUANTUM_IN_BYTES
	);

    int npages =   total_bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    Hugechunk_Region* region =  free->region;

    if (free->bytesize == total_bytesize) {

        // Allocate the whole
        // free area to the chunk:
        //
	new_chunk = free;

    } else {

        // Split the free chunk:
        //
	new_chunk	  =  MALLOC_CHUNK( Hugechunk );
	new_chunk->chunk  =  free->chunk;
	new_chunk->region =  region;
	//
	free->chunk	     = (Punt)(free->chunk) + total_bytesize;
	free->bytesize -= total_bytesize;
        //
	first_ram_quantum =  GET_HUGECHUNK_FOR_POINTER_PAGE( region, new_chunk->chunk );

	for (int i = 0;  i < npages;  i++) {
	    //
	    region->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
		=
		new_chunk;
        }
    }

    new_chunk->bytesize   =  header->bytesize;
    new_chunk->hugechunk_state =  YOUNG_HUGECHUNK;

    new_chunk->age	       =  header->age;
    new_chunk->huge_ilk        =  header->huge_ilk;

    region->free_pages -=  npages;

    // Set up the relocation info:
    //
    reloc_info = MALLOC_CHUNK( Hugechunk_Relocation_Info );
    reloc_info->old_address = header->base_address;
    reloc_info->new_chunk = new_chunk;
    //
    first_ram_quantum = GET_HUGECHUNK_FOR_POINTER_PAGE(old_region, header->base_address);
    //
    for (int i = 0;  i < npages;  i++) {
	//
	old_region->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
	    =
	    reloc_info;
    }

    return new_chunk;
}									// fun allocate_a_hugechunk



static void   repair_heap   (
    //        ===========
    //
    Heap*             heap,
    Sibid*         oldBOOK2SIBID,
    //
    Punt addrOffset  [ MAX_AGEGROUPS ][ MAX_PLAIN_ILKS ],
    //
    Addresstable*     hugechunk_region_table,
    Val*              externs
){
    // Scan the heap replacing external references
    // with their addresses and adjusting pointers.
    //
    for (int i = 0;  i < heap->active_agegroups;  i++) {
        //
	Agegroup* ag =  heap->agegroup[ i ];

	#define MARK(cm, p, g)	MAYBE_UPDATE_CARD_MIN_AGE_PER_POINTER(cm, p, g)

	#define REPAIR_SIB(index)	{						\
	    Sib*  __ap = ag->sib[ index ];						\
	    Val	*__p, *__q;								\
	    __p = __ap->tospace;							\
	    __q = __ap->next_tospace_word_to_allocate;					\
	    while (__p < __q) {								\
		Val	__w = *__p;							\
		int		__gg, __chunkc;						\
		if (IS_POINTER(__w)) {							\
		    Punt	__chunk = HEAP_POINTER_AS_UNT(__w);		\
		    Sibid __aid = SIBID_FOR_POINTER(oldBOOK2SIBID, __chunk);			\
		    if (SIBID_KIND_IS_CODE(__aid)) {					\
			Hugechunk_Relocation_Info*	__dp;				\
			__dp = address_to_relocation_info (oldBOOK2SIBID, 			\
				hugechunk_region_table, __aid, __chunk);			\
			*__p = PTR_CAST( Val, (__chunk - __dp->old_address) 		\
				+ __dp->new_chunk->chunk);				\
			__gg = __dp->new_chunk->age-1;					\
		    }									\
		    else {								\
			__gg = GET_AGE_FROM_SIBID(__aid)-1;					\
			__chunkc = GET_KIND_FROM_SIBID(__aid)-1;				\
			*__p = PTR_CAST( Val, __chunk + addrOffset[__gg][__chunkc]);	\
		    }									\
		    if (((index) == VECTOR_ILK) && (__gg < i)) {			\
			MARK(ag->coarse_inter_agegroup_pointers_map, __p, __gg+1);	/** **/				\
		    }									\
		}									\
		else if (IS_EXTERNAL_TAG(__w)) {					\
		    *__p = externs[EXTERNAL_ID(__w)];					\
		}									\
		__p++;									\
	    }										\
	}

	REPAIR_SIB( RECORD_ILK );
	REPAIR_SIB(   PAIR_ILK );
	REPAIR_SIB( VECTOR_ILK );

	#undef REPAIR_SIB
    }
}										// fun repair_heap



static Val   repair_word   (
    //       ===========
    //
    Val                w,
    Sibid*          oldBOOK2SIBID,
    //
    Punt  addrOffset  [ MAX_AGEGROUPS ][ MAX_PLAIN_ILKS ],
    //
    Addresstable*      hugechunk_region_table,
    Val*               externs
) {
    //
    if (IS_POINTER(w)) {
	//
	Punt	chunk = HEAP_POINTER_AS_UNT(w);
	Sibid	aid = SIBID_FOR_POINTER(oldBOOK2SIBID, chunk);

	if (SIBID_KIND_IS_CODE(aid)) {

	    Hugechunk_Relocation_Info* dp = address_to_relocation_info (oldBOOK2SIBID, hugechunk_region_table, aid, chunk);

	    return PTR_CAST( Val, (chunk - dp->old_address) + dp->new_chunk->chunk);

	} else {

	    int	g = GET_AGE_FROM_SIBID(aid)-1;
	    int	chunkc = GET_KIND_FROM_SIBID(aid)-1;
	    return PTR_CAST( Val,  PTR_CAST(char*, w) + addrOffset[g][chunkc]);
	}

    } else if (IS_EXTERNAL_TAG(w)) {
	//
	return externs[ EXTERNAL_ID( w ) ];

    } else {

	return w;
    }
}



static Hugechunk_Relocation_Info*   address_to_relocation_info   (
    //                             ========================== 
    //
    Sibid*            oldBOOK2SIBID, 
    Addresstable*     hugechunk_region_table, 
    Sibid             id, 
    Punt oldchunk
) {
    int  index;
    for (index = GET_BOOK_CONTAINING_POINTEE(oldchunk);
        !SIBID_ID_IS_BIGCHUNK_RECORD(id);
        id = oldBOOK2SIBID[--index]
    );

    Hugechunk_Region_Relocation_Info* region
	=
	LOOK_UP_HUGECHUNK_REGION( hugechunk_region_table, index );		// Find the old region descriptor.

    if (!region)   die ("unable to map hugechunk @ %#x; index = %#x, id = %#x\n", oldchunk, index, (unsigned)id);

//    return GET_HUGECHUNK_FOR_POINTER(region, oldchunk);
    return   get_hugechunk_holding_pointee_via_reloc_info( region, (Val)oldchunk );

}


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.







/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/
