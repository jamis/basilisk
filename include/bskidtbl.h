/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskidtbl.h
 *
 * An identifier table maintains a list of all declared identifiers in a
 * database.  An identifier is uniquely identified by its index in this
 * list.
 *
 * TODO: this object could be optimized by using a binary search tree to
 * maintain the identifiers -- it wouldn't necessarily help the VM much,
 * but the parser would probably benefit from the increased efficiency.  A
 * hash table would be ideal, but might require too much memory.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKIDTBL_H__
#define __BSKIDTBL_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKIdentifierTableGetCapacity( BSKIdentifierTable* ) */
#define BSKIdentifierTableGetCapacity(x)       ((x)->capacity)

  /* BSKUI32 BSKIdentifierTableGetSize( BSKIdentifierTable* ) */
#define BSKIdentifierTableGetSize(x)           ((x)->size)

  /* BSKCHAR* BSKIdentifierGetIdentifier( BSKIdentifierTableEntry* entry ) */
#define BSKIdentifierGetIdentifier(x)          ((x)->identifier)

  /* BSKNOTYPE BSKIdentifierGetData( BSKIdentifierTableEntry* entry ) */
#define BSKIdentifierGetData(x)                ((x)->data)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskidtable__      BSKIdentifierTable;
typedef struct __bskidtableentry__ BSKIdentifierTableEntry;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskidtable__ {
  BSKUI32 capacity;                 /* current capacity of the list */
  BSKUI32 size;                     /* actual number of elements in list */

  BSKIdentifierTableEntry** table;  /* the list itself */
};


  /* we used a struct for this, instead of simply making the above list an
   * array of BSKCHAR*, so that if we ever need to add a property to an
   * identifier, we can. */

struct __bskidtableentry__ {
  BSKCHAR*  identifier;              /* the identifier */
  BSKNOTYPE data;                    /* identifier's data */
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
   * BSKNewIdentifierTable
   *
   * Creates and returns a new, empty, identifier table.
   * -------------------------------------------------------------------- */
BSKIdentifierTable* BSKNewIdentifierTable();

  /* -------------------------------------------------------------------- *
   * BSKDestroyIdentifierTable
   *
   * Destroys and deallocates all memory allocated to the given table and
   * it's identifiers.
   * -------------------------------------------------------------------- */
void BSKDestroyIdentifierTable( BSKIdentifierTable* table );

  /* -------------------------------------------------------------------- *
   * BSKFindIdentifier
   *
   * Searches the given table for the given identifier, returning either
   * 0 if the identifier was not found in the table, or the numeric id of
   * the identifier if it was found.
   * -------------------------------------------------------------------- */
BSKI32 BSKFindIdentifier( BSKIdentifierTable* table,
                          BSKCHAR* identifier );

  /* -------------------------------------------------------------------- *
   * BSKAddIdentifier
   *
   * Searches the given table for the given identifier, and if it is not
   * found it is added to the table.  Either way, the numeric id of the
   * identifier is returned.
   * -------------------------------------------------------------------- */
BSKI32 BSKAddIdentifier( BSKIdentifierTable* table,
                         BSKCHAR* identifier );

  /* -------------------------------------------------------------------- *
   * BSKSetIdentifierData
   *
   * Sets the identifier data for the given identifier.
   * -------------------------------------------------------------------- */
void BSKSetIdentifierData( BSKIdentifierTable* table,
                           BSKUI32 id,
                           BSKNOTYPE data );

  /* -------------------------------------------------------------------- *
   * BSKGetIdentifierLength
   *
   * Returns the length of the requested identifier in bytes, or 0 if the
   * identifier does not exist in the table.  The requested length does
   * NOT include the null-terminating byte at the end.
   * -------------------------------------------------------------------- */
BSKUI32 BSKGetIdentifierLength( BSKIdentifierTable* table,
                                BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKGetIdentifier
   *
   * Retrieves the requested identifier into the given buffer, but copies
   * no more than 'length' bytes.
   * -------------------------------------------------------------------- */
void BSKGetIdentifier( BSKIdentifierTable* table,
                       BSKUI32 id,
                       BSKCHAR* buffer,
                       BSKUI32 length );

  /* -------------------------------------------------------------------- *
   * BSKGetIdentifierData
   *
   * Retrieves the data for the requested identifier, or 0 if no such
   * identifier exists in the table.
   * -------------------------------------------------------------------- */
BSKNOTYPE BSKGetIdentifierData( BSKIdentifierTable* table,
                                BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKSerializeIdentifierTableOut
   *
   * Writes the given table to the given stream.  The serialization begins
   * with the number of elements in the table, followed by the length and
   * subsequent data of each individual element.  Returns 0 on success.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeIdentifierTableOut( BSKIdentifierTable* table,
                                       BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeIdentifierTableIn
   *
   * Creates a new BSKIdentifierTable object and creates it from the
   * contents of the given stream.
   * -------------------------------------------------------------------- */
BSKIdentifierTable* BSKSerializeIdentifierTableIn( BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKIDTBL_H__ */
