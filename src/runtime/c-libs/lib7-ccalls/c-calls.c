/* c-calls.c
 *
 * C-side support for calling user C functions from lib7.
 *
 */

#include "../../config.h"

#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#if defined(INDIRECT_CFUNC) 
#  include "c-library.h"
#endif
#include "lib7-c.h"
#include "c-calls.h"


/* assumptions:
 *
 *     Word_t fits in a machine word
 *
 *     restrictions:
 *	   C function args must fit in Word_t
 *         C's double is the largest return value from a function 
 */

lib7_val_t	dummyRoot = LIB7_void;  /* empty root for GC */

/* visible_lib7_state used to expose lib7_state to C code */
lib7_state_t	*visible_lib7_state = NULL;

#define CONS_SZB (3*WORD_SZB)     /* desc + car + cdr */
#define CADDR_SZB (2*WORD_SZB)    /* string desc + ptr */

#define MK_SOME(lib7_state,v) recAlloc1(lib7_state,v)

#define NULLARY_DATACON  INT_CtoLib7(0)

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


/* This enumeration must match the tags on the cdata enum */
/* See c-calls.pkg */

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
/* #define LIB7VOID_TAG   not used  */

/* map from enum tags to single char descriptor (aka code) */
char typeMap[] = {LIB7ADDR_CODE,
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
		  LIB7VOID_CODE};

/* utility functions */

#define CHAR_RANGE 255   /* must agree with CharRange in c-calls.pkg */

static int extractUnsigned(unsigned char **s,int bytes)
{
    int r = 0;
    
    while (bytes--)
	r = r * CHAR_RANGE + (int) *((*s)++) - 1;
    return r;
}



/* could (should) use stdlib's strdup instead of this */
static char *mk_strcpy(char *s)
{
    char *p;

    if ((p = (char *) MALLOC(strlen(s)+1)) == NULL)
	Die("couldn't make string copy during C call\n");
    return strcpy(p,s);
}

Word_t *checked_memalign(int n,int align)
{
    Word_t *p;

    if (align < sizeof(Word_t))
	align = sizeof(Word_t);
    if ((p = (Word_t *)MALLOC(n)) == NULL)
	Die("couldn't alloc memory for C call\n");

    ASSERT(((Word_t)p & (Word_t)(align-1)) != 0);

    return p;
}

static lib7_val_t recAlloc1(lib7_state_t *lib7_state,lib7_val_t v)
{
    lib7_val_t ret;

    REC_ALLOC1(lib7_state,ret,v);
    return ret;
}

static lib7_val_t mkWord32(lib7_state_t *lib7_state, Word_t p)
{
    LIB7_AllocWrite(lib7_state, 0, MAKE_DESC(sizeof(Word_t), DTAG_string));
    LIB7_AllocWrite(lib7_state, 1, (lib7_val_t)p);
    return LIB7_Alloc(lib7_state, sizeof(Word_t));
}

static Word_t getWord32(lib7_val_t v)
{
    return (Word_t) REC_SEL(v,0);
}

#define MK_CADDR(lib7_state,p) mkWord32(lib7_state,(Word_t) (p))
#define GET_CADDR(v)    (Word_t *)getWord32(v)

static lib7_val_t double_CtoLib7(lib7_state_t *lib7_state,double g)
{
    lib7_val_t res;

#ifdef DEBUG_C_CALLS
SayDebug("double_CtoLib7: building an Lib7 double %l.15f\n", g);
#endif
    /* Force REALD_SZB alignment */
    lib7_state->lib7_heap_cursor = (lib7_val_t *)((Addr_t)(lib7_state->lib7_heap_cursor) | WORD_SZB);
    LIB7_AllocWrite(lib7_state,0,DESC_reald);
    res = LIB7_Alloc(lib7_state,(sizeof(double)>>2));
    memcpy (res, &g, sizeof(double));
    return res;
}

/* ptrs to storage alloc'd by the interface. */
typedef struct ptr_desc {
    Word_t *ptr;
    struct ptr_desc *next;
} ptrlist_t;

static ptrlist_t *ptrlist = NULL;

#ifdef DEBUG_C_CALLS
static int ptrlist_len()
{
    int i = 0;
    ptrlist_t *p = ptrlist;

    while (p != NULL) {
	i++;
	p = p->next;
    }
    return i;
}
#endif
    
    
static void keep_ptr(Word_t *p)
{
    ptrlist_t *q = (ptrlist_t *) checked_alloc(sizeof(ptrlist_t));

#ifdef DEBUG_C_CALLS
    SayDebug("keeping ptr %x, |ptrlist|=%d\n", p, ptrlist_len());
#endif

    q->ptr = p;
    q->next = ptrlist;
    ptrlist = q;
}

static void free_ptrlist()
{
    ptrlist_t *p;

#ifdef DEBUG_C_CALLS
    SayDebug("freeing ptr list, |ptrlist|=%d\n",ptrlist_len());
#endif
    p = ptrlist;
    while (p != NULL) {
	ptrlist = ptrlist->next;
	FREE(p->ptr);               /* the block */
	FREE(p);                    /* the block's descriptor */
	p = ptrlist;
    }
}

static lib7_val_t ptrlist_to_LIB7list(lib7_state_t *lib7_state)
{
    lib7_val_t lp = LIST_nil;
    lib7_val_t v;
    ptrlist_t *p;

#ifdef DEBUG_C_CALLS
    int i = 0;
    SayDebug("converting ptrlist (|ptrlist|=%d) to Lib7 list ",ptrlist_len());
#endif
    p = ptrlist;
    while (p != NULL) {
#ifdef DEBUG_C_CALLS
	i++;
#endif
	ptrlist = p->next;
	v = MK_CADDR(lib7_state,p->ptr);
	LIST_cons(lib7_state, lp, v, lp);
	FREE(p);
	p = ptrlist;
    }
#ifdef DEBUG_C_CALLS
    SayDebug("of length %d\n", i);
#endif
    return lp;
}

/* return the number of bytes the ptrlist will occupy in the Lib7 heap */
static int ptrlist_space()
{
    int n = 0;
    ptrlist_t *p;

    p = ptrlist;
    while (p != NULL) {
	p = p->next;
	n += CONS_SZB + CADDR_SZB;
    }
#ifdef DEBUG_C_CALLS
    SayDebug("space for ptrlist is %d, |ptrlist|=%d\n",n,ptrlist_len());
#endif
    return n;
}

static void save_ptrlist(ptrlist_t **save)
{
#ifdef DEBUG_C_CALLS
    SayDebug("saving ptrlist, |ptrlist|=%d\n", ptrlist_len());
#endif
    *save = ptrlist;
    ptrlist = NULL;
}

static void restore_ptrlist(ptrlist_t *save)
{
    ptrlist = save;
#ifdef DEBUG_C_CALLS
    SayDebug("restoring ptrlist, |ptrlist|=%d\n", ptrlist_len());
#endif
}

lib7_val_t revLib7List(lib7_val_t l,lib7_val_t r)
{
    if (l == LIST_nil)
	return r;
    else {
	lib7_val_t tmp = LIST_tl(l);

	LIST_tl(l) = r;
	return revLib7List(tmp,l);
    }
}

	
#define SMALL_SPACE 0    /* size to 'need_to_collect_garbage' for a small chunk, say <10 words */

static void spaceCheck(lib7_state_t *lib7_state, int bytes, lib7_val_t *one_root)
{
    /* assume the ONE_K buffer will absorb descriptors, '\0' terminators */
    if (need_to_collect_garbage(lib7_state,bytes + ONE_K)) {
#ifdef DEBUG_C_CALLS
SayDebug("spaceCheck: invoking GC\n");
#endif
	collect_garbage_with_extra_roots(lib7_state,0,one_root,NULL);
	if (need_to_collect_garbage(lib7_state,bytes + ONE_K))
	    Error("spaceCheck: cannot alloc Lib7 space for Lib7-C conversion\n");
    }
}


/* interface functions */

static char*   too_many_args   = "c-calls with more than 15 args not supported\n";

static Word_t   call_word_g   (Word_t (*f)(),int n,Word_t *args)
{
    /* Used when the return type fits into a machine word (Word_t): */
    Word_t ret = 0;

    switch(n) {
      case 0: 
	ret = (*f)();
	break;
      case 1:
	ret = (*f)(args[0]);
	break;
      case 2:
	ret = (*f)(args[0],args[1]);
	break;
      case 3:
	ret = (*f)(args[0],args[1],args[2]);
	break;
      case 4:
	ret = (*f)(args[0],args[1],args[2],args[3]);
	break;
      case 5:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;
      case 6:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5]);
	break;
      case 7:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6]);
	break;
      case 8:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7]);
	break;
      case 9:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8]);
	break;
      case 10:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9]);
	break;
      case 11:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9],
		   args[10]);
	break;
      case 12:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9],
		   args[10],args[11]);
	break;
      case 13:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9],
		   args[10],args[11],args[12]);
	break;
      case 14:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9],
		   args[10],args[11],args[12],args[13]);
	break;
      case 15:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9],
		   args[10],args[11],args[12],args[13],args[14]);
	break;
      default:
	/* shouldn't happen; Lib7 side assures this */
	Error(too_many_args);
    }

#ifdef DEBUG_C_CALLS
    SayDebug("call_word_g: return=0x%x\n",ret);
#endif
    return ret;
}    

/* call_double_g
 */
static double call_double_g(double (*f)(),int n,Word_t *args)
{
    double ret;

    switch(n) {
      case 0: 
	ret = (*f)();
	break;
      case 1:
	ret = (*f)(args[0]);
	break;
      case 2:
	ret = (*f)(args[0],args[1]);
	break;
      case 3:
	ret = (*f)(args[0],args[1],args[2]);
	break;
      case 4:
	ret = (*f)(args[0],args[1],args[2],args[3]);
	break;
      case 5:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;
      case 6:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5]);
	break;
      case 7:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6]);
	break;
      case 8:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7]);
	break;
      case 9:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8]);
	break;
      case 10:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9]);
	break;
      default:
	/* shouldn't happen; Lib7 side assures this */
	Error(too_many_args);
    }
    return ret;
}    

/* call_float_g
 */
static float call_float_g(float (*f)(),int n,Word_t *args)
{
    float ret;

    switch(n) {
      case 0: 
	ret = (*f)();
	break;
      case 1:
	ret = (*f)(args[0]);
	break;
      case 2:
	ret = (*f)(args[0],args[1]);
	break;
      case 3:
	ret = (*f)(args[0],args[1],args[2]);
	break;
      case 4:
	ret = (*f)(args[0],args[1],args[2],args[3]);
	break;
      case 5:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4]);
	break;
      case 6:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5]);
	break;
      case 7:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6]);
	break;
      case 8:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7]);
	break;
      case 9:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8]);
	break;
      case 10:
	ret = (*f)(args[0],args[1],args[2],args[3],args[4],
		   args[5],args[6],args[7],args[8],args[9]);
	break;
      default:
	/* shouldn't happen; Lib7 side assures this */
	Error(too_many_args);
    }
    return ret;
}    

/* error handling */

#define NO_ERR                   0
#define ERR_TYPEMISMATCH         1
#define ERR_EMPTY_AGGREGATE      2
#define ERR_SZ_MISMATCH    3
#define ERR_WRONG_ARG_COUNT      4
#define ERR_TOO_MANY_ARGS        5

static char *errtable[] = {
    "no error",
    "type mismatch",
    "empty aggregate",
    "array/vector size does not match registered size",
    "wrong number of args in C call",
    "current max of 10 args to C fn",
    };

static char errbuf[100];

static lib7_val_t RaiseError(lib7_state_t *lib7_state,int err)
{
    sprintf(errbuf,"Lib7-C-Interface: %s",errtable[err]);
    return RAISE_ERROR(lib7_state, errbuf);
}


/* char *nextdatum(char *t)
 *
 * must match typeToCtl in c-calls.pkg
 */
static char *nextdatum(char *t)
{
    int level = 0;

    do {
	switch(*t) {
	  case LIB7FUNCTION_CODE: {
	      int nargs, i;

	      t++;   /* skip code */
	      nargs = extractUnsigned((unsigned char **)&t,1);
	      /* skip arg types AND return type */
	      for (i = 0; i < nargs+1; i++) {
		  t = nextdatum(t);
	      }
	    }
	    break;
	  case LIB7PTR_CODE:
	    /* can fall through as long as Cptr has 4 bytes of size info */
	  case LIB7ARRAY_CODE:
	  case LIB7VECTOR_CODE:
	    t = nextdatum(t+5);    /* skip 4 bytes of size info & code */
	    break;
	  case LIB7OPENUNION_CODE:
	    t++;                   /* skip 1 byte of size info ; fall through */
	  case LIB7OPENSTRUCT_CODE:
	    t++;                   /* skip code */
	    level++;
	    break;
	  case LIB7CLOSEUNION_CODE:
	  case LIB7CLOSESTRUCT_CODE:
	    t++;                   /* skip code */
	    level--;
	    break;
	  case LIB7INT_CODE:
	  case LIB7SHORT_CODE:
	  case LIB7LONG_CODE:
	    /* skip 1 byte of size; fall through */
	    t++;
	  default:
	    t++;                   /* skip simple type */
	    break;
	}
    } while (level);
    return t;
}

static void mkCint(Word_t src,Word_t **dst,int bytes)
{
#ifdef DEBUG_C_CALLS
    SayDebug("mkCint: placing integer into %d bytes at %x\n", bytes, *dst);
#endif

#ifdef BYTE_ORDER_BIG
    src <<= (sizeof(Word_t) - bytes)*8;
#endif
    memcpy (*dst, &src, bytes);
    (*(Byte_t **)dst) += bytes;
}

static void mkLIB7int(Word_t **src,Word_t *dst,int bytes)
{
#ifdef DEBUG_C_CALLS
    SayDebug("mkLIB7int: reading integer from %x into %d bytes\n", *src, bytes);
#endif
    
    memcpy (dst, *src, bytes);
#ifdef BYTE_ORDER_BIG
    *dst >>= (sizeof(Word_t) - bytes)*8;
#endif
    *(Byte_t **)src += bytes;
}


#define DO_PAD(p,t)         (*(Byte_t **)(p) += extractUnsigned((unsigned char **)(t),1))
#define IF_PAD_DO_PAD(p,t)  {if (**t == LIB7PAD_CODE) {++(*t); DO_PAD(p,t);}}

int datumLib7toC(lib7_state_t *lib7_state,char **t,Word_t **p,lib7_val_t datum)
{
    int tag = REC_SELINT(datum,0);
    lib7_val_t val = REC_SEL(datum,1);
    int err = NO_ERR;
    int size = 0;

    while (**t == LIB7PAD_CODE) {
	++(*t);  /* advance past code */
#ifdef DEBUG_C_CALLS
	SayDebug("datumLib7toC: adding pad from %x ", *p);
#endif
	DO_PAD(p,t);
#ifdef DEBUG_C_CALLS
	SayDebug(" to %x\n", *p);
#endif
    }
    if (typeMap[tag] != **t) {
#ifdef DEBUG_C_CALLS
       SayDebug("datumLib7toC: type mismatch %c != %d\n",**t,tag);
#endif
	return ERR_TYPEMISMATCH;
    }
    switch(*(*t)++) {
      case LIB7FUNCTION_CODE: {
	  char *argtypes[N_ARGS], *rettype;
	  char *this_arg, *next_arg;
	  int nargs, len, i;

	  nargs = extractUnsigned((unsigned char **)t,1);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: function with %d args\n", nargs);
#endif
	  this_arg = *t;
	  for (i = 0; i < nargs; i++) {
	      next_arg = nextdatum(this_arg);
	      len = next_arg - this_arg;
	      argtypes[i] = (char *)checked_alloc(len+1);  /* len plus null */
	      strncpy(argtypes[i],this_arg,len);
	      argtypes[i][len] = '\0';
	      this_arg = next_arg;
#ifdef DEBUG_C_CALLS
	      SayDebug("datumLib7toC: function arg[%d] is \"%s\"\n", 
		     i,argtypes[i]);
#endif
	  }
	  /* get the return type */
	  next_arg = nextdatum(this_arg);
	  len = next_arg - this_arg;
	  rettype = (char *)checked_alloc(len+1);  /* len plus null */
	  strncpy(rettype,this_arg,len);
	  rettype[len] = '\0';
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: function returns \"%s\"\n", 
		 rettype);
#endif
	  *t = next_arg;
	  *(*p)++ = mk_C_function(lib7_state,val,nargs,argtypes,rettype);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: made C function\n");
#endif
        }
        break;
      case LIB7PTR_CODE: {
	  int szb, align;
	  Word_t *q;

	  szb = extractUnsigned((unsigned char **)t,4);
	  align = extractUnsigned((unsigned char **)t,1);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: ptr szb=%d, align=%d\n", szb, align);
#endif
	  q = checked_memalign(szb,align);
	  keep_ptr(q);
	  *(*p)++ = (Word_t) q;
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: ptr substructure at %x\n", q);
#endif
	  if (err = datumLib7toC(lib7_state,t,&q,val))
	      return err;
        }
	break;
      case LIB7CHAR_CODE:
	*(*(Byte_t **)p)++ = (Byte_t) INT_LIB7toC(val);
	break;
      case LIB7FLOAT_CODE: 
	size = sizeof(float);
	/* fall through */
      case LIB7DOUBLE_CODE: {
	  double g;

	  if (!size) {
	      /* came in through LIB7DOUBLE_CODE */
	      size = sizeof(double);
	  }
	  memcpy (&g, (Word_t *)val, sizeof(double));
#ifdef DEBUG_C_CALLS
SayDebug("datumLib7toC: Lib7 real %l.15f:%l.15f %.15f\n", *(double *)val, g, (float) g);
#endif
	  if (size == sizeof(float))
	      *(*(float **)p)++ = (float) g;
	  else
	      *(*(double **)p)++ = g;
        }
	break;
      case LIB7INT_CODE:
      case LIB7SHORT_CODE:
      case LIB7LONG_CODE:
#ifdef DEBUG_C_CALLS
	SayDebug("datumLib7toC: integer %d\n", getWord32(val));
#endif
	mkCint(getWord32(val),p,extractUnsigned((unsigned char **)t,1));
	break;
      case LIB7ADDR_CODE:
#ifdef DEBUG_C_CALLS
       SayDebug("datumLib7toC: addr %x\n", GET_CADDR(val));
#endif
	*(*p)++ = (Word_t) GET_CADDR(val);
	break;
      case LIB7STRING_CODE: {
	  char *r, *s;

	  s = PTR_LIB7toC(char,val);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: string \"%s\"\n",s);
#endif
	  r = (char *) checked_alloc(strlen(s)+1);
	  strcpy(r,s);
	  keep_ptr((Word_t *) r);
	  *(*p)++ =  (Word_t) r;
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: copied string \"%s\"=%x\n",r,r);
#endif
        }
	break;
      case LIB7OPENSTRUCT_CODE: {
	  lib7_val_t lp = val;
	  lib7_val_t hd;

#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: struct\n");
#endif
	  while (**t != LIB7CLOSESTRUCT_CODE) {
	      hd = LIST_hd(lp);
	      if (err = datumLib7toC(lib7_state,t,p,hd))
		  return err;
	      lp = LIST_tl(lp);
	      IF_PAD_DO_PAD(p,t);
	  }
	  (*t)++;  /* advance past LIB7CLOSESTRUCT_CODE */
        }
	break;
      case LIB7OPENUNION_CODE: {
	  Byte_t *init_p = (Byte_t *) *p;
	  char *next_try;

	  size = extractUnsigned((unsigned char **)t,1);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: union of size %d\n", size);
#endif
	  if ((**t) == LIB7CLOSEUNION_CODE)
	      return ERR_EMPTY_AGGREGATE;
	  next_try = nextdatum(*t);
	  /* try union types until one matches or all fail */
	  while ((err = datumLib7toC(lib7_state,t,p,val)) == ERR_TYPEMISMATCH) {
	      *t = next_try;
	      if ((**t) == LIB7CLOSEUNION_CODE) {
		  err = ERR_TYPEMISMATCH;
		  break;
	      }
	      next_try = nextdatum(*t);
	      *p = (Word_t *) init_p;
	  }
	  if (err)
	      return err;
	  while (**t != LIB7CLOSEUNION_CODE)
	      *t = nextdatum(*t);
	  (*t)++; /* advance past LIB7CLOSEUNION_CODE */
	  *p = (Word_t *) (init_p + size);
        }
	break;
      case LIB7ARRAY_CODE: 
      case LIB7VECTOR_CODE: {
	  int nelems,elem_size, i;
	  char *saved_t;

	  nelems = extractUnsigned((unsigned char **)t,2);
	  elem_size = extractUnsigned((unsigned char **)t,2);
#ifdef DEBUG_C_CALLS
	 SayDebug("datumLib7toC: array/vector of %d elems of size %d\n", 
		nelems, elem_size);
#endif
	  i = size = CHUNK_LEN(val);
#ifdef DEBUG_C_CALLS
	  SayDebug("datumLib7toC: array/vector size is %d\n", size);
#endif

	  if (size != nelems)
	      return ERR_SZ_MISMATCH;
	  saved_t = *t;
	  while (!err && i--) {
	      *t = saved_t;
	      err = datumLib7toC(lib7_state,t,p,*(lib7_val_t *)val++);
	  }
	  if (err)
	      return err;
        }
	break;
      case LIB7CLOSESTRUCT_CODE:
      case LIB7CLOSEUNION_CODE:
	return ERR_EMPTY_AGGREGATE;
	break;
      default:
	Die("datumLib7toC: cannot yet handle type\n");
    }
    return err;
}

/* Lib7 entry point for 'datumLib7toC' */
lib7_val_t lib7_datumLib7toC(lib7_state_t *lib7_state, lib7_val_t arg)
{
    /* NB: No garbage collection can occur since no allocation is done on Lib7 heap. */
    /* NB: We are guaranteed that datum is a pointer (Cptr or Cstring). */
    char *type = REC_SELPTR(char,arg,0);
    lib7_val_t datum = REC_SEL(arg,1);
    int err = 0;
    Word_t p, *q = &p;
    lib7_val_t lp, ret;
    ptrlist_t *saved_pl;

    save_ptrlist(&saved_pl);
    err = datumLib7toC(lib7_state,&type,&q,datum);
    if (err) {
      free_ptrlist();
      restore_ptrlist(saved_pl);
      return RaiseError(lib7_state,err);
    }
    /* return (result,list of pointers to alloc'd C chunks) */
    spaceCheck(lib7_state,ptrlist_space(),&dummyRoot);
    lp = ptrlist_to_LIB7list(lib7_state);   /* this frees the ptr descriptors */
    restore_ptrlist(saved_pl);
    ret = MK_CADDR(lib7_state,(Word_t *)p);
    REC_ALLOC2(lib7_state, ret, ret, lp);
    return ret;
}

static lib7_val_t word_CtoLib7(lib7_state_t *lib7_state,char **t,Word_t **p, lib7_val_t *root)
{
    lib7_val_t ret = LIB7_void;
    lib7_val_t mlval = LIB7_void;
    int tag;
    char code;

    switch(code = *(*t)++) {
      case LIB7PAD_CODE:
#ifdef DEBUG_C_CALLS
	SayDebug("word_CtoLib7: skipping pad %x ", *p);
#endif
	DO_PAD(p,t);
#ifdef DEBUG_C_CALLS
	SayDebug(" to %x\n", *p);
#endif
	return word_CtoLib7(lib7_state,t,p,root);
      case LIB7VOID_CODE:
	return NULLARY_DATACON;
      case LIB7CHAR_CODE:
	tag = LIB7CHAR_TAG;
	mlval = INT_CtoLib7(**(Byte_t **)p);
	(*(Byte_t **)p)++;
	break;
      case LIB7PTR_CODE: {
	  Word_t q;
#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: ptr %x\n", **(Word_t ****)p);
#endif
	  tag = LIB7PTR_TAG;
#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: size is %d\n", 
		   extractUnsigned((unsigned char **)t,4));
	  SayDebug("word_CtoLib7: align is %d\n", 
		   extractUnsigned((unsigned char **)t,1));
#else
	  *t += 5;  /* 5 bytes of size */
#endif
	  q = **p;
	  mlval = word_CtoLib7(lib7_state,t,(Word_t **) &q,root);
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
	{
	    Word_t w;

	    mkLIB7int(p,&w,extractUnsigned((unsigned char **)t,1));
	    mlval = mkWord32(lib7_state,w);
	}
	break;
      case LIB7ADDR_CODE: {
	  Word_t *cp = ** (Word_t ***) p;

#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7:  C addr %x\n", cp);
#endif
	  tag = LIB7ADDR_TAG;
	  mlval = MK_CADDR(lib7_state,cp);
	  (*p)++;
        }
	break;
      case LIB7FLOAT_CODE: {
	  /* C floats become Lib7 reals, which are doubles... */
	  tag = LIB7FLOAT_TAG;
	  mlval = double_CtoLib7(lib7_state,(double) *(*(float **)p)++);
#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: made float %l.15f\n", *(double*)mlval);
#endif
        }
	break;
      case LIB7DOUBLE_CODE: {
	  tag = LIB7DOUBLE_TAG;
	  mlval = double_CtoLib7(lib7_state,*(*(double **)p)++);
#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: made double %l.15f\n", *(double*)mlval);
#endif
        }
	break;
      case LIB7STRING_CODE:
#ifdef DEBUG_C_CALLS
	SayDebug("word_CtoLib7:  string \"%s\"\n", (char *)**p);
#endif
	tag = LIB7STRING_TAG;
	spaceCheck(lib7_state,strlen((char *)**p),root);
	mlval = LIB7_CString(lib7_state,(char *) **p);
	(*p)++;
	break;
      case LIB7OPENSTRUCT_CODE: {
	  lib7_val_t local_root;

	  tag = LIB7STRUCT_TAG;
	  mlval = LIST_nil;

#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: open struct\n");
#endif
	  while (**t != LIB7CLOSESTRUCT_CODE) {
	      LIST_cons(lib7_state,local_root,mlval,*root);
	      ret = word_CtoLib7(lib7_state,t,p,&local_root);
	      mlval = LIST_hd(local_root);
	      *root = LIST_tl(local_root);
	      LIST_cons(lib7_state,mlval,ret,mlval);
	      IF_PAD_DO_PAD(p,t);
	  }
	  (*t)++;  /* advance past LIB7CLOSESTRUCT_CODE */
	  mlval = revLib7List(mlval,LIST_nil);
        }
	break;
      case LIB7CLOSESTRUCT_CODE:
	Die("word_CtoLib7: found lone LIB7CLOSESTRUCT_CODE");
      case LIB7ARRAY_CODE: 
      case LIB7VECTOR_CODE: {
	  int szb;
	  char *saved_t;
	  lib7_val_t res,local_root;
	  int n,i;
	  Word_t dtag;

	  tag = (code == LIB7ARRAY_CODE) ? LIB7ARRAY_TAG : LIB7VECTOR_TAG;
	  dtag = (code == LIB7ARRAY_CODE) ? DTAG_array : DTAG_vector;
	  n = extractUnsigned((unsigned char **)t,2);  /* number of elements */
	  szb = extractUnsigned((unsigned char **)t,2);/* element size (bytes)*/
#ifdef DEBUG_C_CALLS
	  SayDebug("word_CtoLib7: array/vector with %d elems of size %d\n", n, szb);
#endif
	  saved_t = *t;
	  spaceCheck(lib7_state,szb*n,root);
	  /* LIB7_AllocArray isn't used here since it might call GC */
	  LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(n,dtag));
	  mlval = LIB7_Alloc (lib7_state, n);
	  /* clear the array/vector so that it can be GC'd if necessary */
	  for (i = 0; i < n; i++) {
	      PTR_LIB7toC(lib7_val_t,mlval)[i] = LIB7_void;
	  }
	  for (i = 0; i < n; i++) {
	      *t = saved_t;
	      LIST_cons(lib7_state,local_root,mlval,*root);
	      res = word_CtoLib7(lib7_state,t,p,&local_root);
	      mlval = LIST_hd(local_root);
	      *root = LIST_tl(local_root);
	      PTR_LIB7toC(lib7_val_t,mlval)[i] = res;
	  }
        }
	break;
      default:
#ifdef DEBUG_C_CALLS
	SayDebug("word_CtoLib7: bad type is '%c'\n", *(*t-1));
#endif
	Die("word_CtoLib7: cannot yet handle type\n");
    }
    REC_ALLOC2(lib7_state,ret,INT_CtoLib7(tag),mlval);
    return ret;
}

/* static  c-calls-fns.c needs to see this */
lib7_val_t datumCtoLib7(lib7_state_t *lib7_state, char *type, Word_t p, lib7_val_t *root)
{
    lib7_val_t ret;

#ifdef DEBUG_C_CALLS
   SayDebug("datumCtoLib7: C address is %x\n", p);
#endif

#ifdef DEBUG_C_CALLS
   SayDebug("datumCtoLib7: type is %s\n", type);
#endif

    switch (*type) {
      case LIB7DOUBLE_CODE:
	ret = double_CtoLib7(lib7_state, *(double *)p);
	REC_ALLOC2(lib7_state,ret,INT_CtoLib7(LIB7DOUBLE_TAG),ret);
	break;
      case LIB7FLOAT_CODE:
	ret = double_CtoLib7(lib7_state, (double) (*(float *)p));
	REC_ALLOC2(lib7_state,ret,INT_CtoLib7(LIB7FLOAT_TAG),ret);
	break;
      default: {
	  Word_t *q = &p;
	  ret = word_CtoLib7(lib7_state,&type,&q,root);
      }
      break;
    }
#ifdef DEBUG_C_CALLS
    SayDebug("datumCtoLib7: returning\n");
#endif
    return ret;
}


/* Lib7 entry point for 'datumCtoLib7' */
lib7_val_t lib7_datumCtoLib7(lib7_state_t *lib7_state, lib7_val_t arg)
{
    /* make copies of things that GC may move */
    char *type = mk_strcpy(REC_SELPTR(char,arg,0));
    Word_t *caddr = GET_CADDR(REC_SEL(arg,1));
    lib7_val_t ret;

    ret = datumCtoLib7(lib7_state,type,(Word_t) caddr,&arg);
    FREE(type);
    return ret;
}


/* Lib7 entry point for 'c_call' */
lib7_val_t lib7_c_call(lib7_state_t *lib7_state, lib7_val_t arg)
{
#if !defined(INDIRECT_CFUNC)
    Word_t (*f)() = (Word_t (*)())
                      REC_SELPTR(Word_t,arg,0);
#else
    Word_t (*f)() = (Word_t (*)())
                      ((cfunc_naming_t *)REC_SELPTR(Word_t,arg,0))->cfunc;
#endif
    int n_cargs = REC_SELINT(arg,1);
    lib7_val_t carg_types = REC_SEL(arg,2);                   /* string list */
    char *cret_type = REC_SELPTR(char,arg,3);
    lib7_val_t cargs = REC_SEL(arg,4);                        /* cdata list */
    bool_t auto_free = REC_SELINT(arg,5);
    ptrlist_t *saved_pl;

    lib7_val_t p,q;
    lib7_val_t ret;
    int i;
    Word_t vals[N_ARGS];
    Word_t w;
    int err = NO_ERR;

    if (n_cargs > N_ARGS)  /* shouldn't see this; Lib7 side insures this */
	return RaiseError(lib7_state,ERR_TOO_MANY_ARGS);
	
    /* save the ptrlist since C can call Lib7 can call C ... */
    save_ptrlist(&saved_pl);

    p = carg_types;
    q = cargs;
    i = 0;
    while (p != LIST_nil && q != LIST_nil) {
	char *carg_type = PTR_LIB7toC(char,LIST_hd(p));
	Word_t *vp;

#ifdef DEBUG_C_CALLS
	SayDebug("lib7_c_call: arg %d:\"%s\"\n",i,carg_type);
#endif

	vp = &vals[i];
	if (err = datumLib7toC(lib7_state,&carg_type,&vp,LIST_hd(q)))
	    break;
	i++;
	p = LIST_tl(p);
	q = LIST_tl(q);
    }
#ifdef DEBUG_C_CALLS
    SayDebug("lib7_c_call: rettype is \"%s\"\n", cret_type);
#endif

    /* within lib7_c_call, no Lib7 allocation occurs above this point */

    if (!err && (i != n_cargs))
	err = ERR_WRONG_ARG_COUNT;
    if (err) {
	free_ptrlist();
	restore_ptrlist(saved_pl);
	return RaiseError(lib7_state,err);
    }
#ifdef DEBUG_C_CALLS
    SayDebug("lib7_c_call: calling C function at %x\n", f);
#endif

    /* expose lib7_state so C has access to it */
    visible_lib7_state = lib7_state;
    switch (*cret_type) {
      case LIB7DOUBLE_CODE:
	ret = double_CtoLib7(lib7_state,call_double_g((double (*)())f,n_cargs,vals));
	REC_ALLOC2(lib7_state,ret,INT_CtoLib7(LIB7DOUBLE_TAG),ret);
	break;
      case LIB7FLOAT_CODE:
	ret = double_CtoLib7(lib7_state,
			   (double) call_float_g((float(*)())f,n_cargs,vals));
	REC_ALLOC2(lib7_state,ret,INT_CtoLib7(LIB7FLOAT_TAG),ret);
	break;
      case LIB7CHAR_CODE: {
	  Byte_t b = (Byte_t) call_word_g(f,n_cargs,vals);
	  Byte_t *bp = &b;
	
	  ret = word_CtoLib7(lib7_state,&cret_type,(Word_t **)&bp,&dummyRoot);
        }
	break;
      default: {
	Word_t w = call_word_g(f,n_cargs,vals);
	Word_t *wp = &w;
	
	ret = word_CtoLib7(lib7_state,&cret_type,&wp,&dummyRoot);
      }
    }
#ifdef DEBUG_C_CALLS
    SayDebug("lib7_c_call: returned from C function\n");
#endif

#ifdef DEBUG_C_CALLS
    SayDebug("lib7_c_call: auto_free is %d\n",auto_free);
#endif

    /* setup the return value, always a pair */
    {
	lib7_val_t lp = LIST_nil;

	if (auto_free) {
#ifdef DEBUG_C_CALLS
	    SayDebug("lib7_c_call: performing auto-free\n");
#endif

	    free_ptrlist();
	} else {
	    /* return (result,list of pointers to alloc'd C chunks) */
#ifdef DEBUG_C_CALLS
	    SayDebug("lib7_c_call: returning list of caddrs\n");
#endif
	    spaceCheck(lib7_state,ptrlist_space(),&ret);
	    lp = ptrlist_to_LIB7list(lib7_state); /* this frees the ptr descriptors */
	}
	REC_ALLOC2(lib7_state, ret, ret, lp);
    }
    restore_ptrlist(saved_pl);     /* restore the previous ptrlist */
    return ret;
}


/* end of c-calls.c */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

