/* c-globals-table.h
 *
 */

#ifndef _C_GLOBALS_TABLE_
#define _C_GLOBALS_TABLE_

typedef struct export_table export_table_t;

/* info about an exported external reference */
typedef const char *export_item_t;

extern void RecordCSymbol (const char *name, lib7_val_t addr);
extern const char *AddrToCSymbol (lib7_val_t addr);

extern export_table_t *NewExportTable ();
extern void FreeExportTable (export_table_t *table);

extern lib7_val_t ExportCSymbol (export_table_t *table, lib7_val_t addr);
extern lib7_val_t AddrOfCSymbol (export_table_t *table, lib7_val_t xref);
extern void ExportedSymbols (export_table_t *table, int *numSymbs, export_item_t **symbs);

extern lib7_val_t ImportCSymbol (const char *name);

extern Addr_t ExportTableSz (export_table_t *table);

#endif /* !_C_GLOBALS_TABLE_ */


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
