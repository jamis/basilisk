#include "bskenv.h"
#include "bsktypes.h"
#include "bsksymtb.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKSymbolTable* BSKNewSymbolTable() {
  BSKSymbolTable* table;

  table = (BSKSymbolTable*)BSKMalloc( sizeof( BSKSymbolTable ) );
  BSKMemSet( table, 0, sizeof( *table ) );

  return table;
}


void BSKDestroySymbolTable( BSKSymbolTable* table ) {
  BSKSymbolTableEntry* entry;
  BSKSymbolTableEntry* n;

  entry = table->list;
  while( entry != 0 ) {
    n = entry->next;
    BSKFree( entry );
    entry = n;
  }

  BSKFree( table );
}


void BSKAddSymbol( BSKSymbolTable* table, 
                   BSKUI8 type, 
                   BSKUI8 flags, 
                   BSKUI32 id ) 
{
  BSKSymbolTableEntry* entry;

  entry = (BSKSymbolTableEntry*)BSKMalloc( sizeof( BSKSymbolTableEntry ) );
  BSKMemSet( entry, 0, sizeof( *entry ) );

  entry->type = type;
  entry->flags = flags;
  entry->id = id;
  entry->next = table->list;
  table->list = entry;
}


BSKSymbolTableEntry* BSKGetSymbol( BSKSymbolTable* table, BSKUI32 id ) {
  BSKSymbolTableEntry* entry;

  entry = table->list;
  while( entry != 0 ) {
    if( entry->id == id ) {
      return entry;
    }
    entry = entry->next;
  }

  /* if the symbol was not found in this table, and this table has a
   * parent, then search the parent table for the symbol */

  if( ( entry == 0 ) && ( table->parent != 0 ) ) {
    entry = BSKGetSymbol( table->parent, id );
  }

  return entry;
}


BSKI32 BSKSerializeSymbolTableOut( BSKSymbolTable* table, 
                                   BSKStream* stream )
{
  BSKSymbolTableEntry* i;
  BSKUI8 byte;

  /* a serialized symbol table has the following format:
   *   byte:  1
   *   byte:  type
   *   byte:  flags
   *   dword: id
   *   ...
   *   byte:  0
   */

  i = table->list;
  while( i != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );

    stream->write( stream, &(i->type), sizeof( i->type ) );
    stream->write( stream, &(i->flags), sizeof( i->flags ) );
    stream->write( stream, &(i->id), sizeof( i->id ) );

    i = i->next;
  }

  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );

  return 0;
}


BSKSymbolTable* BSKSerializeSymbolTableIn( BSKStream* stream ) {
  BSKSymbolTable* table;
  BSKSymbolTableEntry* i;
  BSKUI8 byte;

  table = (BSKSymbolTable*)BSKMalloc( sizeof( BSKSymbolTable ) );
  BSKMemSet( table, 0, sizeof( *table ) );

  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    i = (BSKSymbolTableEntry*)BSKMalloc( sizeof( BSKSymbolTableEntry ) );
    BSKMemSet( i, 0, sizeof( *i ) );

    stream->read( stream, &(i->type), sizeof( i->type ) );
    stream->read( stream, &(i->flags), sizeof( i->flags ) );
    stream->read( stream, &(i->id), sizeof( i->id ) );

    i->next = table->list;
    table->list = i;

    stream->read( stream, &byte, sizeof( byte ) );
  }

  return table;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
