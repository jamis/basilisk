/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskthing.h
 *
 * A BSKThing object is what makes the whole Basilisk universe go round.
 * It represents a single entity in the database, and possesses a set of
 * attributes that describe it.
 *
 * A thing's attributes may be strings, numbers (including dice), booleans,
 * other things, categories, and even rules.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKTHING_H__
#define __BSKTHING_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskvalue.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKThingGetID( BSKThing* thing ) */
#define BSKThingGetID(x)                  ((x)->id)

  /* BSKAttribute* BSKThingGetAttributeList( BSKThing* thing ) */
#define BSKThingGetAttributeList(x)       ((x)->list)

  /* BSKUI32 BSKThingAttributeGetID( BSKAttribute* attr ) */
#define BSKThingAttributeGetID(x)         ((x)->id)

  /* BSKValue* BSKThingAttributeGetValue( BSKAttribute* attr ) */
#define BSKThingAttributeGetValue(x)      (&(x)->value)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskthing__      BSKThing;
typedef struct __bskattribute__  BSKAttribute;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskthing__ {
  BSKUI8        reserved;   /* reserved for internal use only */
  BSKUI32       id;         /* the thing's identifier */
  BSKUI8        flags;      /* flags denoting the thing's state */
  BSKUI8        ownerLevel; /* the frame level that owns this thing */
  BSKAttribute* list;       /* the attribute list */
  BSKThing*     next;       /* the next thing in the list */
};


struct __bskattribute__ {
  BSKUI32 id;               /* the attribute's identifier */
  BSKValue value;           /* the attribute's value */
  BSKAttribute* next;
};

struct __bskdatabase__;

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
   * BSKNewThing
   *
   * Creates and returns a new, empty thing with the given identifier.
   * -------------------------------------------------------------------- */
BSKThing* BSKNewThing( BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDestroyThing
   *
   * Destroys and deallocates the thing and all of its attributes, though
   * if an attribute is a thing, category, or rule, the thing, category,
   * or rule referenced is NOT deallocated.
   * -------------------------------------------------------------------- */
void BSKDestroyThing( BSKThing* thing );

  /* -------------------------------------------------------------------- *
   * BSKAddAttributeTo
   *
   * Adds the given attribute with the given value to the given thing.
   * If the thing already has an attribute with the given identifier, the
   * new one is added to the list *in addition to* the previously existing
   * attribute.
   * -------------------------------------------------------------------- */
void BSKAddAttributeTo( BSKThing* thing, 
                        BSKUI32 id, 
                        BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKDuplicateThing
   *
   * Duplicates the given thing by performing a shallow copy of all the
   * thing's attributes.  The new thing has an identifier of 0.
   * -------------------------------------------------------------------- */
BSKThing* BSKDuplicateThing( BSKThing* thing );

  /* -------------------------------------------------------------------- *
   * BSKGetAttributeOf
   *
   * Gets the first attribute of the given type in the thing's attribute
   * list.  If there is no such attribute, returns 0.
   * -------------------------------------------------------------------- */
BSKAttribute* BSKGetAttributeOf( BSKThing* thing, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKGetAttributeAt
   *
   * Gets the attribute at the given index in the thing's attribute list.
   * If the index is out of bounds, this function returns 0.
   * -------------------------------------------------------------------- */
BSKAttribute* BSKGetAttributeAt( BSKThing* thing, BSKUI32 idx );

  /* -------------------------------------------------------------------- *
   * BSKFindThing
   *
   * Searches the list of things in the given database for the first
   * thing with the given identifier and returns it.  If no such thing
   * exists, this function returns 0.
   * -------------------------------------------------------------------- */
BSKThing* BSKFindThing( struct __bskdatabase__* db, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKSerializeThingListOut
   *
   * Writes the list of things to the given stream.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeThingListOut( BSKThing* list,
                                 BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeThingListIn
   *
   * Reads a list of things from the given stream and adds them to the
   * given database object.
   * -------------------------------------------------------------------- */
void BSKSerializeThingListIn( struct __bskdatabase__* db,
                              BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKTHING_H__ */
