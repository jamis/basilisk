/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskutdef.h
 *
 * A "unit" is a descriptor for a number.  The BSKUnitDefinition
 * structure stores the definition of a single unit in a Basilisk database.
 * This file also provides routines for converting between units (pounds to
 * kilograms, for instance).
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKUTDEF_H__
#define __BSKUTDEF_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * Return code for BSKConvertUnits if the units to convert are
   * incompatible (feet to pounds, for instance).
   * -------------------------------------------------------------------- */
#define UERR_INCOMPATIBLE_UNITS    ( -1 )

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKUnitGetID( BSKUnitDefinition* unit ) */
#define BSKUnitGetID(x)               ((x)->id)

  /* BSKFLOAT BSKUnitGetUnits( BSKUnitDefinition* unit ) */
#define BSKUnitGetUnits(x)            ((x)->units)

  /* BSKUI32 BSKUnitGetReference( BSKUnitDefinition* unit ) */
#define BSKUnitGetReference(x)        ((x)->referenceUnit)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskunitdefinition__ BSKUnitDefinition;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskunitdefinition__ {
  BSKUI32 id;               /* unit identifier */
  BSKFLOAT units;           /* number of relative units, or 1 */
  BSKUI32 referenceUnit;    /* unit to which this unit is relative */
  BSKUnitDefinition* next;
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
   * BSKAddUnitDef
   *
   * Adds the given unit definition (id, units, and refUnit) to the 
   * list.  Although there is no checking done to ensure this, refUnit
   * should be an identifier of an existing unit.  Returns the new
   * list of unit definitions.
   * -------------------------------------------------------------------- */
BSKUnitDefinition* BSKAddUnitDef( BSKUnitDefinition* list,
                                  BSKUI32 id,
                                  BSKFLOAT units,
                                  BSKUI32 refUnit );

  /* -------------------------------------------------------------------- *
   * BSKFindUnitDef
   *
   * Looks for the requested unit definition in the list, and returns it
   * if it is found.  Otherwise, returns 0.
   * -------------------------------------------------------------------- */
BSKUnitDefinition* BSKFindUnitDef( BSKUnitDefinition* list,
                                   BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDestroyUnitDefList
   *
   * Destroys and deallocates the given list of unit definitions.
   * -------------------------------------------------------------------- */
void BSKDestroyUnitDefList( BSKUnitDefinition* list );

  /* -------------------------------------------------------------------- *
   * BSKConvertUnits
   *
   * Converts the given quantity (in 'from' units) to the units given by
   * 'to', and stores the new number in 'result'.  If the units 'from'
   * and 'to' are not compatible, returns UERR_INCOMPATIBLE_UNITS.  Units
   * are incompatible if, by following the path of 'referenceUnit' from
   * one unit to it's parent, a common unit definition cannot be found for
   * both 'from' and 'to'.
   * -------------------------------------------------------------------- */
BSKI32 BSKConvertUnits( BSKUnitDefinition* list,
                        BSKFLOAT quantity,
                        BSKUI32 from,
                        BSKUI32 to,
                        BSKFLOAT* result );

  /* -------------------------------------------------------------------- *
   * BSKSerializeUnitDefListOut
   *
   * Writes the given unit definition list to the given stream.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeUnitDefListOut( BSKUnitDefinition* list,
                                   BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeUnitDefListIn
   *
   * Reads a new list of unit definitions from the stream and returns
   * the list.
   * -------------------------------------------------------------------- */
BSKUnitDefinition* BSKSerializeUnitDefListIn( BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKUTDEF_H__ */
