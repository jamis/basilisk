/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskdb.h
 *
 * A BSKDatabase object is a grouping of definitions, things, categories,
 * rules, and values.  It is used by the parser (bskparse.c) to store all
 * parsed objects, and by the VM (bskexec.c) to answer queries regarding
 * objects.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKDB_H__
#define __BSKDB_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskutdef.h"    /* unit definitions */
#include "bskatdef.h"    /* attribute definitions */
#include "bskidtbl.h"    /* identifier table */
#include "bskthing.h"    /* things */
#include "bskctgry.h"    /* categories */
#include "bsksymtb.h"    /* symbol table */
#include "bskrule.h"     /* rules */
#include "bskstream.h"   /* streams, for serialization */

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskdatabase__ BSKDatabase;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskdatabase__ {
  BSKIdentifierTable* idTable;     /* all unique identifiers */
  BSKUnitDefinition*  unitDef;     /* unit definitions */
  BSKAttributeDef*    attrDef;     /* attribute definitions */
  BSKThing*           things;      /* things */
  BSKRule*            rules;       /* rules */
  BSKCategory*        categories;  /* categories */
  BSKSymbolTable*     symbols;     /* all defined symbols */
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
   * BSKNewDatabase
   *
   * Creates and returns a new, empty database object.
   * -------------------------------------------------------------------- */
BSKDatabase* BSKNewDatabase();

  /* -------------------------------------------------------------------- *
   * BSKDestroyDatabase
   *
   * Destroys the database and ALL objects and values contained by it.
   * -------------------------------------------------------------------- */
void BSKDestroyDatabase( BSKDatabase* db );

  /* -------------------------------------------------------------------- *
   * BSKAddThingToDB
   *
   * Adds the given thing to the database.  Doing so marks the 'thing' as
   * persistant (the OF_PERSISTANT flag is set in thing->flags).  The
   * thing is returned.
   * -------------------------------------------------------------------- */
BSKThing* BSKAddThingToDB( BSKDatabase* db, BSKThing* thing );

  /* -------------------------------------------------------------------- *
   * BSKAddCategoryToDB
   *
   * Adds the given category to the database.  Doing so marks the object as
   * persistant (the OF_PERSISTANT flag is set in category->flags).  The
   * category is returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKAddCategoryToDB( BSKDatabase* db, BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKAddRuleToDB
   *
   * Adds the given rule to the database.  The rule is returned.
   * -------------------------------------------------------------------- */
BSKRule* BSKAddRuleToDB( BSKDatabase* db, BSKRule* rule );

  /* -------------------------------------------------------------------- *
   * BSKSerializeDatabaseOut
   *
   * Writes the entire database to the given stream by calling the
   * appropriate serialize routine for each of the database's components.
   * Returns 0 on success.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeDatabaseOut( BSKDatabase* db, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeDatabaseIn
   *
   * Creates a new database object and initializes its contents from the
   * contents of the stream by calling the appropriate "Serialize...In"
   * routine for each of the database's components.  Returns the new
   * database.
   * -------------------------------------------------------------------- */
BSKDatabase* BSKSerializeDatabaseIn( BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKDB_H__ */
