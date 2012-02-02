// ccalls.c
//
// C-side support for calling user C functions from Mythryl.


#include "../../mythryl-config.h"

#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#if defined(INDIRECT_CFUNC) 
#  include "mythryl-callable-c-libraries.h"
#endif
#include "lib7-c.h"
#include "ccalls.h"


// Assumptions:
//
//     Val_Sized_Unt fits in a machine word
//
// Restrictions:
//	C function args must fit in Val_Sized_Unt
//      C's double is the largest return value from a function 
//

static Val   dummy_root__local =   HEAP_VOID;			// Empty root for garbage collection.


#define LIST_CONS_CELL_BYTESIZE (3*WORD_BYTESIZE)	// desc + car + cdr
#define CPOINTER_BYTESIZE (2*WORD_BYTESIZE)	// string desc + ptr

#define MK_SOME(task,v) recAlloc1(task,v)

#define NULLARY_VALCON  TAGGED_INT_FROM_C_INT(0)

#define LIB7ADDR_CODE          '@'
#define LIB7ARRAY_CODE         'A'
#define LIB7CHAR_CODE          'C'
#define LIB7DOUBLE_CODE        'D'
#define LIB7FLOAT_CODE         'R'
#define LIB7FUNCTION_CODE      'F'
#define LIB7INT_CODE           'I'
#define LIB7LONG_CODE          'L'
#define LIB7PTR_CODE           'P'
#define LIB7SHORT_CODE         'i'
#define LIB7STRING_CODE        'S'
#define LIB7OPENSTRUCT_CODE    '('
#define LIB7CLOSESTRUCT_CODE   ')'
#define LIB7OPENUNION_CODE     '<'
#define LIB7CLOSEUNION_CODE    '>'
#define LIB7VECTOR_CODE        'B'
#define LIB7VOID_CODE          'V'
#define LIB7PAD_CODE           '#'

#define LIB7STRUCT_CODE LIB7OPENSTRUCT_CODE
#define LIB7UNION_CODE  LIB7OPENUNION_CODE


// This enumeration must match the tags on the cdata enum.
// See ccalls.pkg
//
#define LIB7ADDR_TAG      0
#define LIB7ARRAY_TAG     1
#define LIB7CHAR_TAG      2
#define LIB7DOUBLE_TAG    3
#define LIB7FLOAT_TAG     4
#define LIB7FUNCTION_TAG  5
#define LIB7INT_TAG       6
#define LIB7LONG_TAG      7
#define LIB7PTR_TAG       8
#define LIB7SHORT_TAG     9
#define LIB7STRING_TAG    10
#define LIB7STRUCT_TAG    11
#define LIB7UNION_TAG     12
#define LIB7VECTOR_TAG     13
// #define LIB7VOID_TAG   not used

// Map from enum tags to single char descriptor (aka code)
//
static char type_map__local[] = {
    //
    LIB7ADDR_CODE,
    LIB7ARRAY_CODE,
    LIB7CHAR_CODE,
    LIB7DOUBLE_CODE,
    LIB7FLOAT_CODE,
    LIB7FUNCTION_CODE,
    LIB7INT_CODE,
    LIB7LONG_CODE,
    LIB7PTR_CODE,
    LIB7SHORT_CODE,
    LIB7STRING_CODE,
    LIB7STRUCT_CODE,
    LIB7UNION_CODE,
    LIB7VECTOR_CODE,
    LIB7VOID_CODE
};

//////////////////////////////////////
// Utility functions

#define CHAR_RANGE 255   /* must agree with CharRange in ccalls.pkg */

static int   extract_unsigned   (unsigned char** s,  int bytes)   {
    //       ================
    //
    int r = 0;
    
    while (bytes--) {
        //
	r = r * CHAR_RANGE + (int) *((*s)++) - 1;
    }

    return r;
}



// Could (should!) use stdlib's strdup instead of this:
//
static char*   mk_strcpy   (char* s)   {
    //
    char* p;

    if ((p = (char*) MALLOC(strlen(s)+1)) == NULL) 	die("couldn't make string copy during C call\n");

    return strcpy(p,s);
}

Val_Sized_Unt*   checked_memalign   (int n, int align)   {
    //
    Val_Sized_Unt* p;

    if (align < sizeof(Val_Sized_Unt)) {
	align = sizeof(Val_Sized_Unt);
    }

    if ((p = (Val_Sized_Unt *)MALLOC(n)) == NULL)   die("couldn't alloc memory for C call\n");

    ASSERT(((Val_Sized_Unt)p & (Val_Sized_Unt)(align-1)) != 0);

    return p;
}

static Val  recAlloc1  (Task* task, Val v)   {
    //
    Val ret;
    //
    REC_ALLOC1(task,ret,v);
    //
    return ret;
}

static Val   mkWord32   (Task* task, Val_Sized_Unt p)   {
    //
    LIB7_AllocWrite(task, 0, MAKE_TAGWORD(sizeof(Val_Sized_Unt), DTAG_string));
    LIB7_AllocWrite(task, 1, (Val)p);
    //
    return LIB7_Alloc(task, sizeof(Val_Sized_Unt));
}

static Val_Sized_Unt   getWord32   (Val v)   {
    //
    return (Val_Sized_Unt) GET_TUPLE_SLOT_AS_VAL(v,0);
}

#define MK_CADDR(task,p) mkWord32(task,(Val_Sized_Unt) (p))
#define GET_CADDR(v)    (Val_Sized_Unt *)getWord32(v)

static Val   double_CtoLib7   (Task* task, double g)   {
    //
    Val result;

    #ifdef DEBUG_C_CALLS
    debug_say("double_CtoLib7: building an Lib7 double %l.15f\n", g);
    #endif

    // Force FLOAT64_BYTESIZE alignment:
    //
    task->heap_allocation_pointer = (Val *)((Punt)(task->heap_allocation_pointer) | WORD_BYTESIZE);
    LIB7_AllocWrite(task,0,FLOAT64_TAGWORD);
    result = LIB7_Alloc(task,(sizeof(double)>>2));
    memcpy (result, &g, sizeof(double));

    return result;
}

/* Pointers to storage alloc'd by the interface:
*/
typedef struct ptr_desc {
    //
    Val_Sized_Unt* ptr;
    struct ptr_desc *next;
} Pointerlist;

static Pointerlist* pointerlist =  NULL;

#ifdef DEBUG_C_CALLS
static int pointerlist_len()
{
    int i = 0;
    Pointerlist *p = pointerlist;

    while (p != NULL) {
	i++;
	p = p->next;
    }
    return i;
}
#endif
    
    
static void keep_ptr(Val_Sized_Unt *p)
{
    Pointerlist *q = (Pointerlist *) checked_alloc(sizeof(Pointerlist));

#ifdef DEBUG_C_CALLS
    debug_say("keeping ptr %x, |pointerlist|=%d\n", p, pointerlist_len());
#endif

    q->ptr = p;
    q->next = pointerlist;
    pointerlist = q;
}

static void free_pointerlist()
{
    Pointerlist *p;

#ifdef DEBUG_C_CALLS
    debug_say("freeing ptr list, |pointerlist|=%d\n",pointerlist_len());
#endif
    p = pointerlist;
    while (p != NULL) {
	pointerlist = pointerlist->next;
	FREE(p->ptr);               /* the block */
	FREE(p);                    /* the block's descriptor */
	p = pointerlist;
    }
}

static Val   Pointerlisto_LIB7list   (Task* task)
{
    Val lp = LIST_NIL;
    Val v;
    Pointerlist *p;

#ifdef DEBUG_C_CALLS
    int i = 0;
    debug_say("converting pointerlist (|pointerlist|=%d) to Lib7 list ",pointerlist_len());
#endif
    p = pointerlist;
    while (p != NULL) {
#ifdef DEBUG_C_CALLS
	i++;
#endif
	pointerlist = p->next;
	v = MK_CADDR(task,p->ptr);
	lp = LIST_CONS(task, v, lp);
	FREE(p);
	p = pointerlist;
    }
#ifdef DEBUG_C_CALLS
    debug_say("of length %d\n", i);
#endif
    return lp;
}

static int   pointerlist_space   ()   {
    //
    // Return the number of bytes the pointerlist
    // will occupy in the Mythryl heap
    //
    int n = 0;

    Pointerlist *p = pointerlist;
    while (p != NULL) {
	p = p->next;
	n += LIST_CONS_CELL_BYTESIZE + CADDR_BYTESIZE;
    }

    #ifdef DEBUG_C_CALLS
	debug_say("space for pointerlist is %d, |pointerlist|=%d\n",n,pointerlist_len());
    #endif

    return n;
}

static void   save_pointerlist   (Pointerlist **save)   {
    //
    #ifdef DEBUG_C_CALLS
	debug_say("saving pointerlist, |pointerlist|=%d\n", pointerlist_len());
    #endif

    *save = pointerlist;

    pointerlist = NULL;
}

static void   restore_pointerlist   (Pointerlist *save)   {
    //
    pointerlist = save;

    #ifdef DEBUG_C_CALLS
	debug_say("restoring pointerlist, |pointerlist|=%d\n", pointerlist_len());
    #endif
}

Val   revLib7List   (Val l,Val r)   {
    //
    if (l == LIST_NIL) {
	return r;
    } else {
	Val tmp = LIST_TAIL(l);

	LIST_TAIL(l) = r;
	return revLib7List(tmp,l);
    }
}

	
#define SMALL_SPACE 0    // Size to 'need_to_call_heapcleaner' for a small chunk, say <10 words.

static void   space_check   (Task* task, int bytes, Val *one_root) {
    //        =========== 
    // Assume the ONE_K_BINARY buffer will absorb descriptors, '\0' terminators
    //
    if (need_to_call_heapcleaner(task,bytes + ONE_K_BINARY)) {

	#ifdef DEBUG_C_CALLS
	    debug_say("space_check: Cleaning heap.\n");
	#endif

	call_heapcleaner_with_extra_roots(task,0,one_root,NULL);

	if (need_to_call_heapcleaner(task,bytes + ONE_K_BINARY)) {
	    //
	    say_error( "space_check: Cannot alloc Mythryl space for Mythryl-to-C conversion.\n" );	// Is it really OK to then return??? XXX BUGGO FIXME
        }
    }
}

//////////////////////////////////
// Interface functions

static char*   too_many_args   = "ccalls with more than 15 args not supported\n";

static Val_Sized_Unt   call_word_g   (Val_Sized_Unt (*f)(),int n,Val_Sized_Unt *args)   {
    //                 ===========
    //
    // Used when the return type fits into a machine word (Val_Sized_Unt):
    //
    Val_Sized_Unt result = 0;

    switch(n) {
	//
    case 0: 
	result = (*f)();
	break;

    case 1:
	result = (*f)(args[0]);
	break;

    case 2:
	result = (*f)(args[0],args[1]);
	break;

    case 3:
	result = (*f)(args[0],args[1],args[2]);
	break;

    case 4:
	result = (*f)(args[0],args[1],args[2],args[3]);
	break;

    case 5:
	result = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;

    case 6:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5]);
	break;

    case 7:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
	break;

    case 8:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
	break;

    case 9:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
	break;

    case 10:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
	break;

    case 11:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
	break;

    case 12:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
	break;

    case 13:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12]);
	break;

    case 14:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13]);
	break;

    case 15:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14]);
	break;
    default:
	// Shouldn't happen -- Mythryl side assures this.
	say_error( too_many_args );
    }

    #ifdef DEBUG_C_CALLS
	debug_say("call_word_g: return=0x%x\n",result);
    #endif

    return result;
}    



static double   call_double_g   (double (*f)(),int n,Val_Sized_Unt *args)   {
    //          =============
    //
    double result;

    switch(n) {
	//
    case 0: 
	result = (*f)();
	break;

    case 1:
	result = (*f)(args[0]);
	break;

    case 2:
	result = (*f)(args[0],args[1]);
	break;

    case 3:
	result = (*f)(args[0],args[1],args[2]);
	break;

    case 4:
	result = (*f)(args[0],args[1],args[2],args[3]);
	break;

    case 5:
	result = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;

    case 6:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5]);
	break;

    case 7:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
	break;

    case 8:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
	break;

    case 9:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
	break;

    case 10:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
	break;

    default:
	// Shouldn't happen -- Mythryl side assures this:	Shouldn't this (and above case) be a 'die'??? XXX BUGGO FIXME
	say_error(too_many_args);
    }
    return result;
}    



static float   call_float_g   (float (*f)(),int n,Val_Sized_Unt *args)   {
    //         ============
    //
    float result;

    switch(n) {
	//
    case 0: 
	result = (*f)();
	break;

    case 1:
	result = (*f)(args[0]);
	break;

    case 2:
	result = (*f)(args[0],args[1]);
	break;

    case 3:
	result = (*f)(args[0],args[1],args[2]);
	break;

    case 4:
	result = (*f)(args[0],args[1],args[2],args[3]);
	break;

    case 5:
	result = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;

    case 6:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5]);
	break;

    case 7:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
	break;

    case 8:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
	break;

    case 9:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
	break;

    case 10:
	result = (*f)(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
	break;

    default:
	// Shouldn't happen -- Mythryl side assures this.
	say_error(too_many_args);
    }
    return result;
}    

///////////////////////////////////////////
// Error handling.

#define NO_ERR                   0
#define ERR_TYPEMISMATCH         1
#define ERR_EMPTY_AGGREGATE      2
#define ERR_SZ_MISMATCH    3
#define ERR_WRONG_ARG_COUNT      4
#define ERR_TOO_MANY_ARGS        5

static char*   errtable   [] = {
    //
    "no error",
    "type mismatch",
    "empty aggregate",
    "array/vector size does not match registered size",
    "wrong number of args in C call",
    "current max of 10 args to C fn",
    };

static char   errbuf[ 100 ];

static Val   RaiseError   (Task* task,  int err)   {
    //       =========
    //
    sprintf(errbuf,"Lib7-C-Interface: %s",errtable[err]);
    return RAISE_ERROR(task, errbuf);
}


static char*   nextdatum   (char* t)   {
    //         =========
    //
    // Must match typeToCtl in ccalls.pkg

    int level = 0;

    do {
	switch(*t) {
	    //
	case LIB7FUNCTION_CODE:
	    {
		int nargs;

		t++;   // Skip code.

		nargs = extract_unsigned((unsigned char **)&t,1);

		// Skip arg types AND return type:
		//
		for (int i = 0; i < nargs+1; i++) {
		    //
		    t = nextdatum( t );
		}
	    }
	    break;

	case LIB7PTR_CODE:
	    // Can fall through as long as Cptr has 4 bytes of size info.
	case LIB7ARRAY_CODE:
	case LIB7VECTOR_CODE:
	    t = nextdatum(t+5);    // Skip 4 bytes of size info & code.
	    break;
	case LIB7OPENUNION_CODE:
	    t++;                   // Skip 1 byte of size info ; fall through.
	case LIB7OPENSTRUCT_CODE:
	    t++;                   // Skip code.
	    level++;
	    break;
	case LIB7CLOSEUNION_CODE:
	case LIB7CLOSESTRUCT_CODE:
	    t++;                   // Skip code.
	    level--;
	    break;
	case LIB7INT_CODE:
	case LIB7SHORT_CODE:
	case LIB7LONG_CODE:
	    // skip 1 byte of size; fall through.
	    t++;
	default:
	    t++;                   // Skip simple type.
	    break;
	}
    } while (level);

    return t;
}

static void   mkCint   (Val_Sized_Unt src,  Val_Sized_Unt** dst,  int bytes) {
    //        =====
    //
    #ifdef DEBUG_C_CALLS
	debug_say("mkCint: placing integer into %d bytes at %x\n", bytes, *dst);
    #endif

    #ifdef BYTE_ORDER_BIG
	src <<= (sizeof(Val_Sized_Unt) - bytes)*8;
    #endif

    memcpy (*dst, &src, bytes);
    (*(Unt8 **)dst) += bytes;
}

static void   mkLIB7int   (Val_Sized_Unt** src,  Val_Sized_Unt* dst,  int bytes) {
    //        =========
    //
    #ifdef DEBUG_C_CALLS
	debug_say("mkLIB7int: reading integer from %x into %d bytes\n", *src, bytes);
    #endif
    
    memcpy (dst, *src, bytes);

    #ifdef BYTE_ORDER_BIG
	*dst >>= (sizeof(Val_Sized_Unt) - bytes)*8;
    #endif

    *(Unt8 **)src += bytes;
}


#define DO_PAD(p,t)         (*(Unt8 **)(p) += extract_unsigned((unsigned char **)(t),1))
#define IF_PAD_DO_PAD(p,t)  {if (**t == LIB7PAD_CODE) {++(*t); DO_PAD(p,t);}}

int   convert_mythryl_value_to_c   (Task* task,  char** t,  Val_Sized_Unt** p,  Val datum) {
    //==========================
    //
    // Called many times in this file and once in   
    //
    int tag  =  GET_TUPLE_SLOT_AS_INT(datum,0);
    Val val  =  GET_TUPLE_SLOT_AS_VAL(datum,1);

    int err  = NO_ERR;
    int size = 0;

    while (**t == LIB7PAD_CODE) {
	//
	++(*t);  			// Advance past code.

	#ifdef DEBUG_C_CALLS
	    debug_say("convert_mythryl_value_to_c: adding pad from %x ", *p);
	#endif

	DO_PAD(p,t);

	#ifdef DEBUG_C_CALLS
	    debug_say(" to %x\n", *p);
	#endif
    }

    if (type_map__local[tag] != **t) {
	//
	#ifdef DEBUG_C_CALLS
	    debug_say("convert_mythryl_value_to_c: type mismatch %c != %d\n",**t,tag);
	#endif
	//
	return ERR_TYPEMISMATCH;
    }

    switch(*(*t)++) {
	//
    case LIB7FUNCTION_CODE:								// Defined above.
        {
	    char* argtypes[ N_ARGS ];
	    char* rettype;

	    char* this_arg;
	    char* next_arg;

	    int len;

	    int nargs = extract_unsigned((unsigned char **)t,1);

	    #ifdef DEBUG_C_CALLS
		debug_say("convert_mythryl_value_to_c: function with %d args\n", nargs);
	    #endif

	    this_arg = *t;

	    for (int i = 0; i < nargs; i++) {
	        //
		next_arg = nextdatum(this_arg);
		len = next_arg - this_arg;
		argtypes[i] = (char *)checked_alloc(len+1);  /* len plus null */
		strncpy(argtypes[i],this_arg,len);
		argtypes[i][len] = '\0';
		this_arg = next_arg;

		#ifdef DEBUG_C_CALLS
		    debug_say("convert_mythryl_value_to_c: function arg[%d] is \"%s\"\n", i,argtypes[i]);
		#endif
	    }

	    // Get the return type:
	    //
	    next_arg = nextdatum(this_arg);
	    len = next_arg - this_arg;
	    rettype = (char*) checked_alloc( len+1 );		// Len plus null
	    strncpy(rettype,this_arg,len);
	    rettype[len] = '\0';

    	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: function returns \"%s\"\n", rettype);
	    #endif

	    *t = next_arg;
	    *(*p)++ = make_c_function( task, val, nargs, argtypes, rettype );		// make_c_function	def in    src/c/lib/ccalls/ccalls-fns.c

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: made C function\n");
    	    #endif
	}
        break;

    case LIB7PTR_CODE:
	{
	    int szb, align;
	    Val_Sized_Unt *q;

	    szb = extract_unsigned((unsigned char **)t,4);
	    align = extract_unsigned((unsigned char **)t,1);

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: ptr szb=%d, align=%d\n", szb, align);
	    #endif

	    q = checked_memalign(szb,align);
	    keep_ptr(q);
	    *(*p)++ = (Val_Sized_Unt) q;

            #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: ptr substructure at %x\n", q);
            #endif

	    if (err =  convert_mythryl_value_to_c( task, t, &q, val )) {
		//
		return err;
	    }
	}
        break;

    case LIB7CHAR_CODE:
	*(*(Unt8 **)p)++ = (Unt8) TAGGED_INT_TO_C_INT(val);
	break;

    case LIB7FLOAT_CODE: 
	size = sizeof(float);

	/* FALL THROUGH */

    case LIB7DOUBLE_CODE:
	{
	    double g;

	    if (!size) {		// Came in through LIB7DOUBLE_CODE
		size = sizeof(double);
	    }

	    memcpy (&g, (Val_Sized_Unt *)val, sizeof(double));

	    #ifdef DEBUG_C_CALLS
		debug_say("convert_mythryl_value_to_c: Lib7 real %l.15f:%l.15f %.15f\n", *(double *)val, g, (float) g);
	    #endif

	    if (size == sizeof(float))  *(*(float **)p)++ = (float) g;
	    else		        *(*(double**)p)++ =         g;
	}
	break;

    case LIB7INT_CODE:
    case LIB7SHORT_CODE:
    case LIB7LONG_CODE:
	#ifdef DEBUG_C_CALLS
	    debug_say("convert_mythryl_value_to_c: integer %d\n", getWord32(val));
	#endif
	mkCint(getWord32(val),p,extract_unsigned((unsigned char **)t,1));
	break;

    case LIB7ADDR_CODE:
	#ifdef DEBUG_C_CALLS
            debug_say("convert_mythryl_value_to_c: addr %x\n", GET_CADDR(val));
	#endif
	*(*p)++ = (Val_Sized_Unt) GET_CADDR(val);
	break;

    case LIB7STRING_CODE:
	{
	    char* r;
	    char* s = PTR_CAST(char*,val);

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: string \"%s\"\n",s);
	    #endif

	    r = (char*) checked_alloc( strlen(s)+1 );
	    strcpy(r,s);
	    keep_ptr((Val_Sized_Unt*) r);
	    *(*p)++ =  (Val_Sized_Unt) r;

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: copied string \"%s\"=%x\n",r,r);
	    #endif
        }
	break;

    case LIB7OPENSTRUCT_CODE:
	{
	    Val lp = val;
	    Val hd;

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: struct\n");
	    #endif

	    while (**t != LIB7CLOSESTRUCT_CODE) {
		hd = LIST_HEAD(lp);
		if (err = convert_mythryl_value_to_c(task,t,p,hd))
		    return err;
		lp = LIST_TAIL(lp);
		IF_PAD_DO_PAD(p,t);
	    }
	    (*t)++;				// Advance past LIB7CLOSESTRUCT_CODE
        }
	break;

    case LIB7OPENUNION_CODE:
	{
	    Unt8 *init_p = (Unt8 *) *p;
	    char *next_try;

	    size = extract_unsigned((unsigned char **)t,1);

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: union of size %d\n", size);
	    #endif

	    if ((**t) == LIB7CLOSEUNION_CODE)
		return ERR_EMPTY_AGGREGATE;

	    next_try = nextdatum(*t);

	    // Try union types until one matches or all fail:
            //
	    while ((err = convert_mythryl_value_to_c(task,t,p,val)) == ERR_TYPEMISMATCH) {
		//
		*t = next_try;
		//
		if (**t == LIB7CLOSEUNION_CODE) {
		    err = ERR_TYPEMISMATCH;
		    break;
		}
		next_try = nextdatum( *t );
		//
		*p = (Val_Sized_Unt*) init_p;
	    }

	    if (err)   return err;

	    while (**t != LIB7CLOSEUNION_CODE) {
		*t = nextdatum(*t);
            }
	    (*t)++;					// Advance past LIB7CLOSEUNION_CODE
	    *p = (Val_Sized_Unt *) (init_p + size);
        }
	break;

    case LIB7ARRAY_CODE: 
    case LIB7VECTOR_CODE:
	{
	    int nelems;
	    int elem_size;
	    int i;

	    char* saved_t;

	    nelems    =  extract_unsigned((unsigned char**)t,2);
	    elem_size =  extract_unsigned((unsigned char**)t,2);

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: array/vector of %d elems of size %d\n", nelems, elem_size);
	    #endif

	    i = size = CHUNK_LENGTH(val);

	    #ifdef DEBUG_C_CALLS
	        debug_say("convert_mythryl_value_to_c: array/vector size is %d\n", size);
	    #endif

	    if (size != nelems)   return ERR_SZ_MISMATCH;

	    saved_t = *t;

	    while (!err && i--) {
		//
		*t = saved_t;
		err = convert_mythryl_value_to_c(task,t,p,*(Val *)val++);
	    }

	    if (err)   return err;
        }
	break;

    case LIB7CLOSESTRUCT_CODE:
    case LIB7CLOSEUNION_CODE:
	return ERR_EMPTY_AGGREGATE;
	break;

    default:
	die("convert_mythryl_value_to_c: cannot yet handle type\n");
    }

    return err;
}

Val   lib7_convert_mythryl_value_to_c   (Task* task,  Val arg) {
    //===============================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("lib7_convert_mythryl_value_to_c");

    // This is the Mythryl entry point for 'convert_mythryl_value_to_c'


    // NB: No garbage collection can occur since no allocation is done on Mythryl heap.
    // NB: We are guaranteed that datum is a pointer (Cptr or Cstring).

    char* type =  GET_TUPLE_SLOT_AS_PTR( char*, arg, 0);
    Val datum  =  GET_TUPLE_SLOT_AS_VAL(        arg, 1);

    int err = 0;

    Val_Sized_Unt  p;
    Val_Sized_Unt* q = &p;

    Val lp;

    Pointerlist* saved_pl;

    save_pointerlist(&saved_pl);
    err = convert_mythryl_value_to_c(task,&type,&q,datum);

    if (err) {
        free_pointerlist();
        restore_pointerlist(    saved_pl  );
        return RaiseError(  task, err );
    }

    // Return (result,list of pointers to alloc'd C chunks):
    //
    space_check( task,pointerlist_space(),&dummy_root__local );

    lp = Pointerlisto_LIB7list(task);   /* this frees the ptr descriptors */

    restore_pointerlist( saved_pl );

    Val result =  MK_CADDR(task,(Val_Sized_Unt *)p);
    //
    return make_two_slot_record( task, result, lp);
}

static Val   word_CtoLib7   (Task* task,  char** t,  Val_Sized_Unt** p,  Val* root)   {
    //       ============
    //
    Val result =  HEAP_VOID;
    Val mlval  =  HEAP_VOID;

    int  tag;
    char code;

    switch(code = *(*t)++) {
	//
    case LIB7PAD_CODE:
	#ifdef DEBUG_C_CALLS
		debug_say("word_CtoLib7: skipping pad %x ", *p);
	#endif
	DO_PAD(p,t);
	#ifdef DEBUG_C_CALLS
		debug_say(" to %x\n", *p);
	#endif
	return word_CtoLib7(task,t,p,root);

    case LIB7VOID_CODE:
	return NULLARY_VALCON;

    case LIB7CHAR_CODE:
	tag = LIB7CHAR_TAG;
	mlval = TAGGED_INT_FROM_C_INT(**(Unt8 **)p);
	(*(Unt8 **)p)++;
	break;

    case LIB7PTR_CODE:
	{
	    Val_Sized_Unt q;
	    #ifdef DEBUG_C_CALLS
		debug_say("word_CtoLib7: ptr %x\n", **(Val_Sized_Unt ****)p);
	    #endif
	    tag = LIB7PTR_TAG;
	    #ifdef DEBUG_C_CALLS
		debug_say("word_CtoLib7: size is %d\n", extract_unsigned((unsigned char **)t,4));
		debug_say("word_CtoLib7: align is %d\n",extract_unsigned((unsigned char **)t,1));
	    #else
		*t += 5;  // 5 bytes of size.
	    #endif
	    q = **p;
	    mlval = word_CtoLib7(task,t,(Val_Sized_Unt **) &q,root);
	    (*p)++;
        }
	break;

    case LIB7INT_CODE: 
	tag = LIB7INT_TAG;
	goto handle_int;

    case LIB7SHORT_CODE:
	tag = LIB7SHORT_TAG;
	goto handle_int;

    case LIB7LONG_CODE:
	tag = LIB7LONG_TAG;
handle_int:
	{   Val_Sized_Unt w;
	    //
	    mkLIB7int(p,&w,extract_unsigned((unsigned char **)t,1));
	    mlval = mkWord32(task,w);
	}
	break;

    case LIB7ADDR_CODE:
	{
	    Val_Sized_Unt *cp = ** (Val_Sized_Unt ***) p;
	
	    #ifdef DEBUG_C_CALLS
		debug_say("word_CtoLib7:  C addr %x\n", cp);
	    #endif

	    tag = LIB7ADDR_TAG;
	    mlval = MK_CADDR(task,cp);
	    (*p)++;
        }
	break;

    case LIB7FLOAT_CODE:
	{
	    // C floats become Mythryl "Float"s which are doubles.
	    //
	    tag = LIB7FLOAT_TAG;
	    mlval = double_CtoLib7(task,(double) *(*(float **)p)++);
	    #ifdef DEBUG_C_CALLS
	        debug_say("word_CtoLib7: made float %l.15f\n", *(double*)mlval);
	    #endif
	}
	break;

    case LIB7DOUBLE_CODE:
	{
	    tag = LIB7DOUBLE_TAG;
	    mlval = double_CtoLib7(task,*(*(double **)p)++);
	    #ifdef DEBUG_C_CALLS
	        debug_say("word_CtoLib7: made double %l.15f\n", *(double*)mlval);
            #endif
        }
	break;

    case LIB7STRING_CODE:
	#ifdef DEBUG_C_CALLS
	    debug_say("word_CtoLib7:  string \"%s\"\n", (char *)**p);
	#endif
	tag = LIB7STRING_TAG;
	space_check( task, strlen((char*)**p), root );
	mlval = make_ascii_string_from_c_string(task,(char *) **p);
	(*p)++;
	break;

    case LIB7OPENSTRUCT_CODE:
	{
	    Val local_root;
	    //
	    tag = LIB7STRUCT_TAG;
	    mlval = LIST_NIL;

	    #ifdef DEBUG_C_CALLS
		debug_say("word_CtoLib7: open struct\n");
	    #endif

	    while (**t != LIB7CLOSESTRUCT_CODE) {
		//
		local_root = LIST_CONS(task, mlval, *root);
		result = word_CtoLib7(task,t,p,&local_root);
		mlval = LIST_HEAD(local_root);
		*root = LIST_TAIL(local_root);
		mlval = LIST_CONS(task, result, mlval);
		IF_PAD_DO_PAD(p,t);
	    }
	    (*t)++;					// Advance past LIB7CLOSESTRUCT_CODE.
	    mlval = revLib7List(mlval,LIST_NIL);
        }
	break;

    case LIB7CLOSESTRUCT_CODE:
	//
	die("word_CtoLib7: found lone LIB7CLOSESTRUCT_CODE");

    case LIB7ARRAY_CODE: 
    case LIB7VECTOR_CODE:
	{
	    Val_Sized_Unt dtag;

	    tag  = (code == LIB7ARRAY_CODE) ? LIB7ARRAY_TAG : LIB7VECTOR_TAG;
	    dtag = (code == LIB7ARRAY_CODE) ? DTAG_array : DTAG_vector;

	    int n    = extract_unsigned((unsigned char **)t,2);	// Number of elements.
	    int szb  = extract_unsigned((unsigned char **)t,2);	// Element size-in-bytes.

	    #ifdef DEBUG_C_CALLS
	        debug_say("word_CtoLib7: array/vector with %d elems of size %d\n", n, szb);
	    #endif

	    char*  saved_t =  *t;

	    space_check( task, szb*n, root );

	    // make_nonempty_rw_vector isn't used here since it might call cleaner.

	    LIB7_AllocWrite (task, 0, MAKE_TAGWORD(n,dtag));

	    mlval = LIB7_Alloc (task, n);

	    // Clear the array/vector so that it
	    // won't confuse the cleaner:
	    //
	    for (int i = 0; i < n; i++) {
	        //
		PTR_CAST( Val*, mlval )[ i ] =   HEAP_VOID;
	    }

	    for (int i = 0; i < n; i++) {
		//
		*t = saved_t;

	        Val                                    local_root;
		local_root = LIST_CONS(task,           mlval, *root );
		Val result = word_CtoLib7(task, t, p, &local_root );
		mlval = LIST_HEAD(                     local_root );
		*root = LIST_TAIL(                     local_root );

		PTR_CAST( Val*, mlval )[ i ] =   result;
	    }
        }
	break;

    default:
	#ifdef DEBUG_C_CALLS
	    debug_say("word_CtoLib7: bad type is '%c'\n", *(*t-1));
	#endif
	//
	die("word_CtoLib7: Cannot yet handle type\n");
    }

    return make_two_slot_record( task, TAGGED_INT_FROM_C_INT(tag), mlval);
}


Val   convert_c_value_to_mythryl   (Task* task,   char* type,   Val_Sized_Unt p,   Val* root) {
    //==========================

    // NB:  ccalls-fns.c needs to see this fn.


    Val result;

    #ifdef DEBUG_C_CALLS
        debug_say("convert_c_value_to_mythryl: C address is %x\n", p);
    #endif

    #ifdef DEBUG_C_CALLS
        debug_say("convert_c_value_to_mythryl: type is %s\n", type);
    #endif

    switch (*type) {
	//
    case LIB7DOUBLE_CODE:
	result = double_CtoLib7(task, *(double *)p);
	result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(LIB7DOUBLE_TAG), result);
	break;

    case LIB7FLOAT_CODE:
	result = double_CtoLib7(task, (double) (*(float *)p));
	result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(LIB7FLOAT_TAG), result);
	break;

    default:
	{   Val_Sized_Unt *q = &p;
	    result = word_CtoLib7(task,&type,&q,root);
        }
        break;
    }

    #ifdef DEBUG_C_CALLS
	debug_say("convert_c_value_to_mythryl: returning\n");
    #endif

    return result;
}


Val   lib7_convert_c_value_to_mythryl   (Task* task,  Val arg) {
    //===============================
    //
    // Mythryl entry point for 'convert_c_value_to_mythryl'.
    // This gets exported as ccalls::convert_c_value_to_mythryl       in     src/c/lib/ccalls/cfun-list.h
    // This gets bound at the Mythryl level only        in    src/lib/c-glue-old/ccalls.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("lib7_convert_c_value_to_mythryl");

    // Make copies of things that cleaning may move:
    //
    char*          type  =  mk_strcpy( GET_TUPLE_SLOT_AS_PTR( char*, arg, 0 ));
    Val_Sized_Unt* caddr =  GET_CADDR( GET_TUPLE_SLOT_AS_VAL(        arg, 1 ));

    Val result =  convert_c_value_to_mythryl (task,type,(Val_Sized_Unt) caddr,&arg);
    FREE(type);
    return result;
}


Val   lib7_c_call   (Task* task,   Val arg) {
    //===========
    //
    // Mythryl entry point for 'ccall'.
    // We are exported as ccalls::ccall          in   src/c/lib/ccalls/cfun-list.h
    // which is bound at the Mythryl level (only) in   src/lib/c-glue-old/ccalls.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("lib7_c_call");

    #if !defined(INDIRECT_CFUNC)
	Val_Sized_Unt (*f)() = (Val_Sized_Unt (*)())   GET_TUPLE_SLOT_AS_PTR( Val_Sized_Unt*, arg, 0 );
    #else
	Val_Sized_Unt (*f)() = (Val_Sized_Unt (*)())   ((Mythryl_Name_With_C_Function*) GET_TUPLE_SLOT_AS_PTR( Val_Sized_Unt*, arg, 0 ))->cfunc;
    #endif

    int   n_cargs    =  GET_TUPLE_SLOT_AS_INT(        arg, 1 );
    Val   carg_types =  GET_TUPLE_SLOT_AS_VAL(        arg, 2 );			// List(String)
    char* cret_type  =  GET_TUPLE_SLOT_AS_PTR( char*, arg, 3 );
    Val   cargs      =  GET_TUPLE_SLOT_AS_VAL(        arg, 4 );			// List(Cdata)
    Bool  auto_free  =  GET_TUPLE_SLOT_AS_INT(        arg, 5 );

    Pointerlist* saved_pointerlist;

    Val p,q;
    Val result;
    int i;
    Val_Sized_Unt vals[N_ARGS];
    Val_Sized_Unt w;
    int err = NO_ERR;

    if (n_cargs > N_ARGS) {
	return RaiseError(task,ERR_TOO_MANY_ARGS);	// Mythryl side guarantees that this can't happen.
    }	
	
    // Save the pointerlist since C
    // can call Mythryl can call C ...
    //
    save_pointerlist( &saved_pointerlist );

    p = carg_types;
    q = cargs;
    i = 0;
    while (p != LIST_NIL && q != LIST_NIL) {
	//
	char* carg_type = PTR_CAST(char*,LIST_HEAD(p));
	Val_Sized_Unt *vp;

	#ifdef DEBUG_C_CALLS
	    debug_say("lib7_c_call: arg %d:\"%s\"\n",i,carg_type);
	#endif

	vp = &vals[i];

	if (err = convert_mythryl_value_to_c(task,&carg_type,&vp,LIST_HEAD(q))) {
	    break;
	}
	i++;
	p = LIST_TAIL(p);
	q = LIST_TAIL(q);
    }

    #ifdef DEBUG_C_CALLS
        debug_say("lib7_c_call: rettype is \"%s\"\n", cret_type);
    #endif

    // Within lib7_c_call, no Mythryl allocation occurs above this point.

    if (!err && (i != n_cargs))   err = ERR_WRONG_ARG_COUNT;
    //
    if (err) {
	free_pointerlist();
	restore_pointerlist(saved_pointerlist);
	return RaiseError(task,err);
    }

    #ifdef DEBUG_C_CALLS
        debug_say("lib7_c_call: calling C function at %x\n", f);
    #endif

    set_visible_task( task );		// Publish task to fns in   src/c/lib/ccalls/ccalls-fns.c

    switch (*cret_type) {
	//
    case LIB7DOUBLE_CODE:
	result = double_CtoLib7(task,call_double_g((double (*)())f,n_cargs,vals));
	//
	result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(LIB7DOUBLE_TAG), result);
	break;

    case LIB7FLOAT_CODE:
	result = double_CtoLib7(task,  (double) call_float_g((float(*)())f,n_cargs,vals));
	//
	result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(LIB7FLOAT_TAG), result );
	break;

    case LIB7CHAR_CODE:
	{
	    Unt8 b = (Unt8) call_word_g(f,n_cargs,vals);
	    Unt8 *bp = &b;
	    //
	    result = word_CtoLib7(task,&cret_type,(Val_Sized_Unt **)&bp,&dummy_root__local);
        }
	break;

      default:
	{
	    Val_Sized_Unt w = call_word_g(f,n_cargs,vals);
	    Val_Sized_Unt *wp = &w;
	    //
	    result = word_CtoLib7(task,&cret_type,&wp,&dummy_root__local);
	}
    }

    #ifdef DEBUG_C_CALLS
	debug_say("lib7_c_call: returned from C function\n");
    #endif

    #ifdef DEBUG_C_CALLS
	debug_say("lib7_c_call: auto_free is %d\n",auto_free);
    #endif

    // Set up the return value, always a pair:
    //
    {   Val lp =  LIST_NIL;
	//
	if (auto_free) {
	    //
	    #ifdef DEBUG_C_CALLS
		debug_say("lib7_c_call: performing auto-free\n");
	    #endif

	    free_pointerlist();

	} else {

	    // Return (result,list of pointers to alloc'd C chunks):
            //
	    #ifdef DEBUG_C_CALLS
		debug_say("lib7_c_call: returning list of caddrs\n");
	    #endif

	    space_check( task, pointerlist_space(), &result );

	    lp = Pointerlisto_LIB7list(task);		// This frees the pointer descriptors.
	}
	result = make_two_slot_record(task, result, lp);
    }

    restore_pointerlist( saved_pointerlist );		// Restore the previous pointerlist.

    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


