/* c-globals-table.c
 *
 * This implements a registry of global C symbols that may be referenced
 * in the Lib7 heap (e.g., references to C functions).
 */

#include "../config.h"

#include "runtime-base.h"
#include "tags.h"
#include "runtime-values.h"
#include "c-globals-table.h"

#define MAKE_EXTERN(index)	MAKE_DESC(index, DTAG_extern)

#define HASH_STRING(name, res)	{				\
	const char	*__cp = (name);				\
	int	__hash = 0, __n;				\
	for (;  *__cp;  __cp++) {				\
	    __n = (128*__hash) + (unsigned)*__cp;		\
	    __hash = __n - (8388593 * (__n / 8388593));		\
	}							\
	(res) = __hash;						\
    }

typedef struct item {		/* An item in the Symbol/Addr tables */
    lib7_val_t	addr;		    /* The address of the external reference */
    const char	*name;		    /* The name of the reference */
    int		stringHash;	    /* The hash sum of the name */
    struct item	*nextSymb;	    /* The next item the SymbolTable bucket */
    struct item	*nextAddr;	    /* The next item the AddrTable bucket */
} item_t;

typedef struct item_ref {	/* an item in an export table */
    item_t	*item;
    int		index;
    struct item_ref *next;
} item_ref_t;

struct export_table {		/* A table of C symbols mapping strings to items, */
				/* which is used when loading a heap image. */
    item_ref_t	**table;
    int		tableSize;
    int		vals_count;
    item_t	**itemMap;	    /* A map from item #s to items */
    int		itemMapSize;
};

/* hash key to index */
#define STRHASH_INDEX(h, size)	((h) & ((size)-1))
#define ADDRHASH_INDEX(a, size)	(((Word_t)(a) >> 3) & ((size)-1))


static item_t	**SymbolTable = NULL;	/* Maps names to items */
static item_t	**AddrTable = NULL;	/* Maps addresses to items */
static int		TableSize = 0;		/* The size of the tables; always */
					/* power of 2. */
static int		NumSymbols = 0;		/* The number of entries in the tables */

/* local routines */
static void GrowTable (export_table_t *table);


/* RecordCSymbol:
 *
 * Enter a global C symbol into the tables.
 */
void RecordCSymbol (const char *name, lib7_val_t addr)
{
    int			n, i, hash;
    item_t		*item, *p;

    ASSERT ((((Word_t)addr & ~TAG_boxed) & TAG_desc) == 0);

    if (TableSize == NumSymbols) {
      /* double the table size */
	int	newTableSz = (TableSize ? 2*TableSize : 64);
	item_t	**newSTable = NEW_VEC(item_t *, newTableSz);
	item_t	**newATable = NEW_VEC(item_t *, newTableSz);

	memset ((char *)newSTable, 0, sizeof(item_t *) * newTableSz);
	memset ((char *)newATable, 0, sizeof(item_t *) * newTableSz);

	for (i = 0;  i < TableSize;  i++) {
	    for (p = SymbolTable[i];  p != NULL; ) {
		item = p;
		p = p->nextSymb;
		n = STRHASH_INDEX(item->stringHash, newTableSz);
		item->nextSymb = newSTable[n];
		newSTable[n] = item;
	    }
	    for (p = AddrTable[i];  p != NULL; ) {
		item = p;
		p = p->nextAddr;
		n = ADDRHASH_INDEX(item->addr, newTableSz);
		item->nextAddr = newATable[n];
		newATable[n] = item;
	    }
	}

	if (SymbolTable != NULL) {
	    FREE (SymbolTable);
	    FREE (AddrTable);
	}
	SymbolTable = newSTable;
	AddrTable = newATable;
	TableSize = newTableSz;
    }

  /* compute the string hash function */
    HASH_STRING(name, hash);

  /* Allocate the item */
    item = NEW_CHUNK(item_t);
    item->name		= name;
    item->stringHash	= hash;
    item->addr		= addr;

    /* Insert the item into the symbol table: */
    n = STRHASH_INDEX(hash, TableSize);
    for (p = SymbolTable[n];  p != NULL;  p = p->nextSymb) {
	if ((p->stringHash == hash) && (strcmp(name, p->name) == 0)) {
	    if (p->addr != addr)
		Die ("global C symbol \"%s\" defined twice", name);
	    else {
		FREE (item);
		return;
	    }
	}
    }
    item->nextSymb	= SymbolTable[n];
    SymbolTable[n]	= item;

    /* Insert the item into the addr table. */
    n = ADDRHASH_INDEX(addr, TableSize);
    for (p = AddrTable[n];  p != NULL;  p = p->nextAddr) {
	if (p->addr == addr) {
	    if ((p->stringHash != hash) || (strcmp(name, p->name) != 0))
		Die ("address %#x defined twice: \"%s\" and \"%s\"",
		    addr, p->name, name);
	    else {
		FREE (item);
		return;
	    }
	}
    }
    item->nextAddr	= AddrTable[n];
    AddrTable[n]	= item;
    NumSymbols++;

} /* end of RecordCSymbol */

/* AddrToCSymbol:
 *
 * Return the name of the C symbol that
 * labels the given address, else NULL.
 */
const char *AddrToCSymbol (lib7_val_t addr)
{
    item_t	*q;

  /* Find the symbol in the AddrTable */
    for (q = AddrTable[ADDRHASH_INDEX(addr, TableSize)];
	 q != NULL;
	 q = q->nextAddr)
    {
	if (q->addr == addr)
	   return q->name;
    }

    return NULL;

} /* end of AddrToCSymbol */

/* NewExportTable:
 */
export_table_t *NewExportTable ()
{
    export_table_t	*table;

    table = NEW_CHUNK(export_table_t);
    table->table		= NULL;
    table->tableSize	= 0;
    table->vals_count	= 0;
    table->itemMap	= NULL;
    table->itemMapSize	= 0;

    return table;

} /* end of NewExportTable */

/* ExportCSymbol:
 *
 * Add an external address to an export table, returning its external reference
 * descriptor.
 */
lib7_val_t ExportCSymbol (export_table_t *table, lib7_val_t addr)
{
    Addr_t	a = PTR_LIB7toADDR(addr);
    item_ref_t	*p;
    item_t	*q;
    int		h, index;

/*SayDebug("ExportCSymbol: addr = %#x, ", addr);*/

    if (table->vals_count >= table->tableSize)
	GrowTable (table);

  /* First check to see if addr is already in table */
    h = ADDRHASH_INDEX(a, table->tableSize);
    for (p = table->table[h];  p != NULL;  p = p->next) {
	if (p->item->addr == addr) {
/*SayDebug("old name = \"%s\", index = %d\n", p->item->name, p->index);*/
	    return MAKE_EXTERN(p->index);
	}
    }

  /* Find the symbol in the AddrTable */
    for (q = AddrTable[ADDRHASH_INDEX(a, TableSize)];  q != NULL;  q = q->nextAddr) {
	if (q->addr == addr)
	   break;
    }
    if (q == NULL) {
	Error("external address %#x not registered\n", addr);
	return LIB7_void;
    }

  /* Insert the index into the address to index map. */
/*SayDebug("new name = \"%s\", index = %d\n", q->name, table->vals_count);*/
    index		= table->vals_count++;
    if (table->itemMapSize <= index) {
	int		newSz = ((table->itemMapSize == 0) ? 64 : 2*table->itemMapSize);
	item_t		**newMap = NEW_VEC(item_t *, newSz);
	int		i;

	for (i = 0;  i < table->itemMapSize;  i++)
	    newMap[i] = table->itemMap[i];
	if (table->itemMap != NULL)
	    FREE (table->itemMap);
	table->itemMap = newMap;
	table->itemMapSize = newSz;
    }
    table->itemMap[index]	= q;

  /* Insert the address into the export table */
    p			= NEW_CHUNK(item_ref_t);
    p->item		= q;
    p->index		= index;
    p->next		= table->table[h];
    table->table[h]	= p;

    return MAKE_EXTERN(index);

} /* end of ExportCSymbol */

/* AddrOfCSymbol:
 *
 * Given an external reference, return its address.
 */
lib7_val_t AddrOfCSymbol (export_table_t *table, lib7_val_t xref)
{
    int	index;

    index = GET_LEN(xref);

/*SayDebug("AddrOfCSymbol: %#x: %d --> %#x\n", xref, index, table->itemMap[index]->addr);*/
    if (index >= table->vals_count)
	Die ("bad external chunk index %d", index);
    else
	return table->itemMap[index]->addr;

} /* end of AddrOfCSymbol */

/* ExportedSymbols:
 */
void ExportedSymbols (export_table_t *table, int *numSymbs, export_item_t **symbs)
{
    int			i, n = table->vals_count;
    item_t		**p;
    export_item_t	*ep;

    *numSymbs = n;
    *symbs  = ep = NEW_VEC(export_item_t, n);
    for (p = table->itemMap, i = 0;  i < n;  i++) {
	*ep = (*p)->name;
	p++; ep++;
    }

} /* end of ExportedSymbols */


/* FreeExportTable:
 *
 * Free the storage used by a import/export table.
 */
void FreeExportTable (export_table_t *table)
{
    int		i;
    item_ref_t	*p, *q;

    for (i = 0;  i <  table->tableSize;  i++) {
	for (p = table->table[i];  p != NULL; ) {
	    q = p->next;
	    FREE (p);
	    p = q;
	}
    }

    if (table->itemMap != NULL)
	FREE (table->itemMap);

    FREE (table);

} /* end of FreeExportTable */


/* ImportCSymbol:
 */
lib7_val_t ImportCSymbol (const char *name)
{
    int		hash, index;
    item_t	*p;

    HASH_STRING(name, hash);

  /* insert the item into the symbol table. */
    index = STRHASH_INDEX(hash, TableSize);
    for (p = SymbolTable[index];  p != NULL;  p = p->nextSymb) {
	if ((p->stringHash == hash) && (strcmp(name, p->name) == 0)) {
	    return (p->addr);
	}
    }

    return LIB7_void;

} /* end of ImportCSymbol */


/* ExportTableSz:
 *
 * Return the number of bytes required to represent the strings in an exported
 * symbols table.
 */
Addr_t ExportTableSz (export_table_t *table)
{
    int		i;
    Addr_t	nbytes;

    for (nbytes = 0, i = 0;  i < table->vals_count;  i++) {
	nbytes += (strlen(table->itemMap[i]->name) + 1);
    }
    nbytes = ROUNDUP(nbytes, WORD_SZB);

    return nbytes;

} /* end of ExportTableSz */


/* GrowTable:
 */
static void GrowTable (export_table_t *table)
{
    int		newTableSz = (table->tableSize ? 2 * table->tableSize : 32);
    item_ref_t	**newTable = NEW_VEC(item_ref_t *, newTableSz);
    int		i, n;
    item_ref_t	*p, *q;

    memset ((char *)newTable, 0, newTableSz * sizeof(item_ref_t *));

    for (i = 0;  i < table->tableSize;  i++) {
	for (p = table->table[i];  p != NULL; ) {
	    q = p;
	    p = p->next;
	    n = ADDRHASH_INDEX(q->item->addr, newTableSz);
	    q->next = newTable[n];
	    newTable[n] = q;
	}
    }

    if (table->table != NULL) FREE (table->table);
    table->table		= newTable;
    table->tableSize	= newTableSz;

} /* end of GrowTable */


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
