/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bsksymtb.h
 *
 * A symbol table stores information about a given identifier.  It is
 * *almost* the same as the identifier table (bskidtbl.h), but with the
 * difference that it can exist heirarchically, where a lower symbol table
 * has precedence over a higher one.
 *
 * The information stored with each symbol are its type, and whether it
 * has been fully defined or not.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKSYMTB_H__
#define __BSKSYMTB_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

#define ST_NONE                  (  0 )
#define ST_UNIT                  (  1 )
#define ST_ATTRIBUTE             (  2 )
#define ST_THING                 (  3 )
#define ST_CATEGORY              (  4 )
#define ST_VARIABLE              (  5 )
#define ST_RULE                  (  6 )
#define ST_BUILTIN               (  7 )

#define SF_NONE                  ( (BSKUI8)0x00 )
#define SF_FORWARD_DECL          ( (BSKUI8)0x01 )

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI8 BSKSymbolGetType( BSKSymbolTableEntry* entry ) */
#define BSKSymbolGetType(x)                ((x)->type)

  /* BSKUI8 BSKSymbolGetID( BSKSymbolTableEntry* entry ) */
#define BSKSymbolGetID(x)                  ((x)->id)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bsksymboltable__        BSKSymbolTable;
typedef struct __bsksymboltableentry__   BSKSymbolTableEntry;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bsksymboltable__ {
  BSKSymbolTable*      parent;  /* the parent table of this table */
  BSKSymbolTableEntry* list;    /* the list of symbols in this table */
};


struct __bsksymboltableentry__ {
  BSKUI8  type;                 /* the type of this symbol */
  BSKUI8  flags;                /* whether it has been fully defined or not */
  BSKUI32 id;                   /* this symbol's identifier */
  BSKSymbolTableEntry* next;    /* next symbol in the list */
};

/* ---------------------------------------------------------------------- *
 * C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- *
 * Functions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * BSKNewSymbolTable
   *
   * Creates and returns a new, empty symbol table.
   * -------------------------------------------------------------------- */
BSKSymbolTable* BSKNewSymbolTable();

  /* -------------------------------------------------------------------- *
   * BSKDestroySymbolTable
   *
   * Destroys and deallocates all symbols in the table, as well as the
   * table itself.
   * -------------------------------------------------------------------- */
void BSKDestroySymbolTable( BSKSymbolTable* table );

  /* -------------------------------------------------------------------- *
   * BSKAddSymbol
   *
   * Adds a new symbol to the given table, of the given type, with the
   * given flags, and with the given identifier.  This routine does not
   * check to see if the symbol already exists in the table.
   * -------------------------------------------------------------------- */
void BSKAddSymbol( BSKSymbolTable* table, 
                   BSKUI8 type, 
                   BSKUI8 flags, 
                   BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKGetSymbol
   *
   * Retrieves the symbol table entry for the given identifier, if one
   * exists.  If no such symbol exists in the table, this routine returns
   * null (0).
   * -------------------------------------------------------------------- */
BSKSymbolTableEntry* BSKGetSymbol( BSKSymbolTable* table, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKSerializeSymbolTableOut
   *
   * Writes this symbol table out to the given stream.  Returns 0.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeSymbolTableOut( BSKSymbolTable* table, 
                                   BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeSymbolTableIn
   *
   * Reads a new symbol table from the given stream and returns it.
   * -------------------------------------------------------------------- */
BSKSymbolTable* BSKSerializeSymbolTableIn( BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKSYMTB_H__ */
