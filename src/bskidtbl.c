/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskidtbl.h"
#include "bskstream.h"
#include "bskutil.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

#define TABLE_GROW_RATE  ( 100 )

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKIdentifierTable* BSKNewIdentifierTable() {
  BSKIdentifierTable* table;

  /* allocate and initialize the table */

  table = (BSKIdentifierTable*)BSKMalloc( sizeof( BSKIdentifierTable ) );
  BSKMemSet( table, 0, sizeof( BSKIdentifierTable ) );

  /* set the initial capacity of the table to the rate at which a table
   * will grow when its bounds are exceeded. */

  table->capacity = TABLE_GROW_RATE;
  table->table = (BSKIdentifierTableEntry**)BSKMalloc( table->capacity * sizeof( BSKIdentifierTableEntry* ) );
  BSKMemSet( table->table, 0, table->capacity * sizeof( BSKIdentifierTableEntry* ) );

  /* skip the first element, so that element 0 is not a valid identifier */
  table->size = 1;

  return table;
}


void BSKDestroyIdentifierTable( BSKIdentifierTable* table ) {
  BSKUI32 i;

  /* destroy each non-null identifier */

  for( i = 1; i < table->size; i++ ) {
    if( table->table[ i ]->identifier != 0 ) {
      BSKFree( table->table[ i ]->identifier );
    }
    BSKFree( table->table[ i ] );
  }

  /* destroy the table */

  BSKFree( table->table );
  BSKFree( table );
}


BSKI32 BSKFindIdentifier( BSKIdentifierTable* table,
                          BSKCHAR* identifier )
{
  BSKUI32 i;

  /* don't bother looking for NULL identifiers */

  if( identifier == 0 ) {
    return 0;
  }

  /* search the table, doing case insensitive comparisons */

  for( i = 1; i < table->size; i++ ) {
    if( table->table[ i ]->identifier != 0 ) {
      if( BSKStrCaseCmp( identifier, table->table[ i ]->identifier ) == 0 ) {
        return i;
      }
    }
  }

  return 0;
}


BSKI32 BSKAddIdentifier( BSKIdentifierTable* table,
                         BSKCHAR* identifier )
{
  BSKIdentifierTableEntry* entry;
  BSKI32 i;

  /* first, check to make sure that the identifier doesn't already exist.
   * Note that because BSKFindIdentifier doesn't bother looking for NULL
   * identifiers, NULL identifiers will always be added.  This can be used
   * to return a new, unique identifying number for an anonymous entity. */

  i = BSKFindIdentifier( table, identifier );
  if( i > 0 ) {
    return i;
  }

  /* grow the table, if the size exceeds the capacity */

  if( table->size >= table->capacity ) {
    BSKIdentifierTableEntry** tmp;

    tmp = (BSKIdentifierTableEntry**)BSKMalloc( ( table->capacity + TABLE_GROW_RATE ) * sizeof( BSKIdentifierTableEntry* ) );
    BSKMemCpy( tmp, table->table, table->capacity * sizeof( BSKIdentifierTableEntry* ) );
    BSKFree( table->table );
    table->table = tmp;

    table->capacity += TABLE_GROW_RATE;
  }

  /* create and add the new identifier table entry */

  entry = (BSKIdentifierTableEntry*)BSKMalloc( sizeof( BSKIdentifierTableEntry ) );
  BSKMemSet( entry, 0, sizeof( *entry ) );

  if( identifier != 0 ) {
    entry->identifier = (BSKCHAR*)BSKMalloc( BSKStrLen( identifier ) + 1 );
    BSKStrCpy( entry->identifier, identifier );
  }

  table->table[ table->size ] = entry;

  return table->size++;
}


void BSKSetIdentifierData( BSKIdentifierTable* table,
                           BSKUI32 id,
                           BSKNOTYPE data )
{
  if( ( id < 1 ) || ( id >= table->size ) ) {
    return;
  }

  table->table[ id ]->data = data;
}


BSKUI32 BSKGetIdentifierLength( BSKIdentifierTable* table,
                                BSKUI32 id )
{
  if( ( id < 1 ) || ( id >= table->size ) ) {
    return BSKFALSE;
  }

  if( table->table[ id ]->identifier == 0 ) {
    return 0;
  }

  return BSKStrLen( table->table[ id ]->identifier );
}


void BSKGetIdentifier( BSKIdentifierTable* table,
                       BSKUI32 id,
                       BSKCHAR* buffer,
                       BSKUI32 length )
{
  if( ( id < 1 ) || ( id >= table->size ) ) {
    *buffer = 0;
    return;
  }

  if( table->table[ id ]->identifier == 0 ) {
    *buffer = 0;
    return;
  }

  /* copy length bytes of the identifier into the buffer */

  BSKStrNCpy( buffer, table->table[ id ]->identifier, length );
  buffer[ length-1 ] = 0;
}


BSKNOTYPE BSKGetIdentifierData( BSKIdentifierTable* table,
                                BSKUI32 id )
{
  if( ( id < 1 ) || ( id >= table->size ) ) {
    return 0;
  }

  return table->table[ id ]->data;
}


BSKI32 BSKSerializeIdentifierTableOut( BSKIdentifierTable* table,
                                       BSKStream* stream )
{
  BSKUI32 i;
  BSKUI8  len;

  /* the identifier table is serialized as follows:
   *   BSKUI32      table size
   *     BSKUI8     identifier length
   *     BSKCHAR*   identifier
   *     ...
   */

  /* write the table size */

  stream->write( stream,
                 &(table->size),
                 sizeof( table->size ) );

  for( i = 1; i < table->size; i++ ) {
    if( table->table[ i ]->identifier != 0 ) {
      len = BSKStrLen( table->table[ i ]->identifier );
      stream->write( stream, &len, sizeof( len ) );
      stream->write( stream,
                     table->table[ i ]->identifier,
                     len );
    } else {
      len = 0;
      stream->write( stream, &len, sizeof( len ) );
    }
  }

  return 0;
}


BSKIdentifierTable* BSKSerializeIdentifierTableIn( BSKStream* stream ) {
  BSKIdentifierTable* newTable;
  BSKUI32 i;
  BSKUI8  len;

  /* create and initialize a new table */

  newTable = (BSKIdentifierTable*)BSKMalloc( sizeof( BSKIdentifierTable ) );
  BSKMemSet( newTable, 0, sizeof( *newTable ) );

  /* read the capacity, and allocate that many NULL entries in the table */

  stream->read( stream, &(newTable->capacity), sizeof( newTable->capacity ) );
  newTable->table = (BSKIdentifierTableEntry**)BSKMalloc( newTable->capacity * sizeof( BSKIdentifierTableEntry* ) );
  BSKMemSet( newTable->table, 0, newTable->capacity * sizeof( BSKIdentifierTableEntry* ) );

  /* for each entry, read its length and data from the stream */

  for( i = 1; i < newTable->capacity; i++ ) {
    BSKIdentifierTableEntry* entry;

    stream->read( stream, &len, sizeof( len ) );

    entry = (BSKIdentifierTableEntry*)BSKMalloc( sizeof( BSKIdentifierTableEntry ) );
    BSKMemSet( entry, 0, sizeof( *entry ) );

    if( len > 0 ) {
      entry->identifier = (BSKCHAR*)BSKMalloc( len + 1 );
      stream->read( stream, entry->identifier, len );
      entry->identifier[ len ] = 0;
    }

    newTable->table[ i ] = entry;
  }

  newTable->size = newTable->capacity;

  return newTable;
}


/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
