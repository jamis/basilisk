/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskatdef.h
 *
 * The BSKAttributeDef object represents an attribute definition in a
 * Basilisk database.  An attribute is defined by its identifier and a
 * type.  An attribute is then instantiated by a BSKThing object (see
 * bskthing.h, and bskparse.c and bskexec.c for examples on how they 
 * BSKAttributeDef objects are used in practice).
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKATDEF_H__
#define __BSKATDEF_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

#define AT_NONE           ( 0 )
#define AT_NUMBER         ( 1 )
#define AT_STRING         ( 2 )
#define AT_BOOLEAN        ( 3 )
#define AT_THING          ( 4 )
#define AT_CATEGORY       ( 5 )
#define AT_RULE           ( 6 )

/* ---------------------------------------------------------------------- *
 * Macro Definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKAttributeGetID( BSKAttributeDef* ) */
#define BSKAttributeGetID(x)         ((x)->id)

  /* BSKUI8 BSKAttributeGetType( BSKAttributeDef* ) */
#define BSKAttributeGetType(x)       ((x)->type)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskattributedef__ BSKAttributeDef;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskattributedef__ {
  BSKUI32 id;              /* the identifier of the attribute */
  BSKUI8  type;            /* the type of the attribute (one of the AT_xxx
                            *   types, defined above) */

  BSKAttributeDef* next;
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
   * BSKAddAttributeDef
   *
   * Creates a new BSKAttributeDef object with the given id and type, and
   * adds it to the front of the given list.  The new object is returned.
   * If an attribute definition already exists in the list with the given
   * id and type, that object is returned and no new object is added.  If
   * the attribute already exists with the same id but a different type,
   * this function returns 0.
   * -------------------------------------------------------------------- */
BSKAttributeDef* BSKAddAttributeDef( BSKAttributeDef* list,
                                     BSKUI32 id,
                                     BSKUI8 type );

  /* -------------------------------------------------------------------- *
   * BSKFindAttributeDef
   *
   * Looks in the list for an attribute definition with the given id.  If a
   * definition is found, it is returned, otherwise, 0 is returned.
   * -------------------------------------------------------------------- */
BSKAttributeDef* BSKFindAttributeDef( BSKAttributeDef* list,
                                      BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDestroyAttributeDefList
   *
   * Deallocates and destroys all attribute definitions in the indicated
   * list.
   * -------------------------------------------------------------------- */
void BSKDestroyAttributeDefList( BSKAttributeDef* list );

  /* -------------------------------------------------------------------- *
   * BSKSerializeAttrDefListOut
   *
   * Writes the given list of attribute definitions to the given stream.
   * Returns '0' on success.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeAttrDefListOut( BSKAttributeDef* list, 
                                   BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeAttrDefListIn
   *
   * Creates a new list of attribute definitions from the data contained
   * in the indicated stream.  If the stream contains no attribute
   * definitions, this function returns 0.
   * -------------------------------------------------------------------- */
BSKAttributeDef* BSKSerializeAttrDefListIn( BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKATDEF_H__ */
