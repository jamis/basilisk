/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskrule.h"
#include "bsksymtb.h"
#include "bskstream.h"
#include "bskdb.h"

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

BSKRule* BSKNewRule( BSKUI32 id ) {
  BSKRule* rule;

  rule = (BSKRule*)BSKMalloc( sizeof( BSKRule ) );
  BSKMemSet( rule, 0, sizeof( *rule ) );

  rule->reserved = OT_RULE;
  rule->id = id;

  return rule;
}


void BSKDestroyRule( BSKRule* rule ) {
  if( rule->parameterCount > 0 ) {
    BSKFree( rule->parameters );
    rule->parameterCount = 0;
  }

  if( rule->codeLength > 0 ) {
    BSKFree( rule->code );
    rule->code = 0;
  }

  if( rule->symbols != 0 ) {
    BSKDestroySymbolTable( rule->symbols );
  }

  if( rule->file != 0 ) {
    BSKFree( rule->file );
  }
  rule->startLine = 0;

  BSKFree( rule );
}


BSKRule* BSKFindRule( BSKRule* list, BSKUI32 id ) {
  BSKRule* i;

  for( i = list; i != 0; i = i->next ) {
    if( i->id == id ) {
      return i;
    }
  }

  return 0;
}


BSKI32 BSKSerializeRuleListOut( BSKRule* list, BSKStream* stream ) {
  BSKUI8 byte;

  /* rules are serialized as follows:
   *   byte: 1
   *   dword: id
   *   dword: parameter count
   *   array of dword: parameters
   *   dword: code length
   *   array of byte: code
   *   if file == 0 then 0 else len(file), file (excludes null)
   *   dword: startLine
   *   local symbol table
   *   ...
   *   byte: 0 (at end of list)
   */

  while( list != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );

    stream->write( stream, &(list->id), sizeof( list->id ) );
    stream->write( stream, &(list->parameterCount), sizeof( list->parameterCount ) );
    if( list->parameterCount > 0 ) {
      stream->write( stream, list->parameters, sizeof( list->parameters[0] ) * list->parameterCount );
    }
    stream->write( stream, &(list->codeLength), sizeof( list->codeLength ) );
    stream->write( stream, list->code, list->codeLength );
    
    if( list->file == 0 ) {
      byte = 0;
      stream->write( stream, &byte, sizeof( byte ) );
    } else {
      byte = BSKStrLen( list->file );
      stream->write( stream, &byte, sizeof( byte ) );
      stream->write( stream, list->file, byte );
    }

    stream->write( stream, &(list->startLine), sizeof( list->startLine ) );

    BSKSerializeSymbolTableOut( list->symbols, stream );

    list = list->next;
  }

  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );

  return 0;
}


void BSKSerializeRuleListIn( struct __bskdatabase__* db, BSKStream* stream ) {
  BSKRule* rule;
  BSKRule* t;
  BSKUI8   byte;

  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    rule = (BSKRule*)BSKMalloc( sizeof( BSKRule ) );
    BSKMemSet( rule, 0, sizeof( *rule ) );

    rule->reserved = OT_RULE;
    stream->read( stream, &(rule->id), sizeof( rule->id ) );
   
    /* check to see if the given rule does not already exist in the
     * database.  If it does, use it, otherwise add this newly created
     * rule to the database. */
      
    t = BSKFindRule( db->rules, rule->id );
    if( t != 0 ) {
      BSKFree( rule );
      rule = t;
    } else {
      BSKAddRuleToDB( db, rule );
    }

    stream->read( stream, &(rule->parameterCount), sizeof( rule->parameterCount ) );
    if( rule->parameterCount > 0 ) {
      rule->parameters = (BSKUI32*)BSKMalloc( sizeof( BSKUI32 ) * rule->parameterCount );
      stream->read( stream, rule->parameters, sizeof( BSKUI32 ) * rule->parameterCount );
    }
    stream->read( stream, &(rule->codeLength), sizeof( rule->codeLength ) );
    rule->code = (BSKUI8*)BSKMalloc( rule->codeLength );
    stream->read( stream, rule->code, rule->codeLength );

    stream->read( stream, &byte, sizeof( byte ) );
    if( byte > 0 ) {
      rule->file = (BSKCHAR*)BSKMalloc( byte + 1 );
      stream->read( stream, rule->file, byte );
      rule->file[ byte ] = 0;
    }

    stream->read( stream, &(rule->startLine), sizeof( rule->startLine ) );
    rule->symbols = BSKSerializeSymbolTableIn( stream );

    stream->read( stream, &byte, sizeof( byte ) );
  }
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
