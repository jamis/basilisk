/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskdb.h"
#include "bskatdef.h"
#include "bskutdef.h"
#include "bskidtbl.h"
#include "bskthing.h"
#include "bskctgry.h"
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

BSKDatabase* BSKNewDatabase() {
  BSKDatabase* db;

  db = (BSKDatabase*)BSKMalloc( sizeof( BSKDatabase ) );
  if( db == 0 ) {
    return 0;
  }

  /* ALWAYS initialize to zero!!! */
  BSKMemSet( db, 0, sizeof( *db ) );

  db->idTable = BSKNewIdentifierTable();
  if( db->idTable == 0 ) {
    BSKFree( db );
    return 0;
  }

  db->symbols = BSKNewSymbolTable();
  if( db->symbols == 0 ) {
    BSKDestroyIdentifierTable( db->idTable );
    BSKFree( db );
    return 0;
  }

  return db;
}


void BSKDestroyDatabase( BSKDatabase* db ) {
  BSKThing* it;
  BSKThing* nt;
  BSKCategory* ic;
  BSKCategory* nc;
  BSKRule* ir;
  BSKRule* nr;

  BSKDestroyIdentifierTable( db->idTable );
  BSKDestroySymbolTable( db->symbols );

  BSKDestroyAttributeDefList( db->attrDef );
  BSKDestroyUnitDefList( db->unitDef );

  it = db->things;
  while( it != 0 ) {
    nt = it->next;
    BSKDestroyThing( it );
    it = nt;
  }

  ic = db->categories;
  while( ic != 0 ) {
    nc = ic->next;
    BSKDestroyCategory( ic );
    ic = nc;
  }

  ir = db->rules;
  while( ir != 0 ) {
    nr = ir->next;
    BSKDestroyRule( ir );
    ir = nr;
  }

  BSKFree( db );
}


BSKThing* BSKAddThingToDB( BSKDatabase* db, BSKThing* thing ) {
  thing->next = db->things;
  thing->flags = OF_PERSISTANT;

  /* the ownerLevel is used by the VM to determine what stack frame
   * owns the object.  An ownerLevel of 0 indicates that the database
   * itself owns the object. */

  thing->ownerLevel = 0;
  db->things = thing;

  /* set the thing that was added to be the data for the identifier
   * that identifies the thing -- this will help to speed up the
   * BSKFindThing routine IMMENSELY. */

  if( thing->id > 0 ) {
    BSKSetIdentifierData( db->idTable, thing->id, thing );
  }

  return thing;
}


BSKCategory* BSKAddCategoryToDB( BSKDatabase* db, BSKCategory* category ) {  
  category->next = db->categories;
  category->flags = OF_PERSISTANT;

  /* the ownerLevel is used by the VM to determine what stack frame
   * owns the object.  An ownerLevel of 0 indicates that the database
   * itself owns the object. */

  category->ownerLevel = 0;
  db->categories = category;

  /* set the category that was added to be the data for the identifier
   * that identifies the category -- this will help to speed up the
   * BSKFindCategory routine IMMENSELY. */

  if( category->id > 0 ) {
    BSKSetIdentifierData( db->idTable, category->id, category );
  }

  return category;
}


BSKRule* BSKAddRuleToDB( BSKDatabase* db, BSKRule* rule ) {
  rule->next = db->rules;
  db->rules = rule;

  /* set the rule that was added to be the data for the identifier
   * that identifies the rule -- this will help to speed up the
   * BSKFindRule routine IMMENSELY. */

  if( rule->id > 0 ) {
    BSKSetIdentifierData( db->idTable, rule->id, rule );
  }

  return rule;
}


BSKI32 BSKSerializeDatabaseOut( BSKDatabase* db, BSKStream* stream ) {
  BSKSerializeIdentifierTableOut( db->idTable, stream );
  BSKSerializeUnitDefListOut( db->unitDef, stream );
  BSKSerializeAttrDefListOut( db->attrDef, stream );
  BSKSerializeThingListOut( db->things, stream );
  BSKSerializeRuleListOut( db->rules, stream );
  BSKSerializeCategoryListOut( db->categories, stream );
  BSKSerializeSymbolTableOut( db->symbols, stream );

  return 0;
}


BSKDatabase* BSKSerializeDatabaseIn( BSKStream* stream ) {
  BSKDatabase* db;
  BSKRule*     rule;

  db = (BSKDatabase*)BSKMalloc( sizeof( BSKDatabase ) );
  BSKMemSet( db, 0, sizeof( *db ) );

  db->idTable = BSKSerializeIdentifierTableIn( stream );
  db->unitDef = BSKSerializeUnitDefListIn( stream );
  db->attrDef = BSKSerializeAttrDefListIn( stream );
  
  BSKSerializeThingListIn( db, stream );
  BSKSerializeRuleListIn( db, stream );
  BSKSerializeCategoryListIn( db, stream );
  
  db->symbols = BSKSerializeSymbolTableIn( stream );

  /* make sure that the symbol table for each rule has, as its parent,
   * the top-level symbol table owned by the database. */

  for( rule = db->rules; rule != 0; rule = rule->next ) {
    rule->symbols->parent = db->symbols;
  }

  return db;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
