/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskvar.h
 *
 * The BSKVariable object represents a variable in a Basilisk rule during
 * the rule's execution (see bskexec.c).  
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKVAR_H__
#define __BSKVAR_H__

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

  /* BSKUI32 BSKVariableGetID( BSKVariable* var ) */
#define BSKVariableGetID(x)              ((x)->id)

  /* BSKValue* BSKVariableGetValue( BSKVariable* var ) */
#define BSKVariableGetValue(x)           (&(x)->value)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskvariable__ BSKVariable;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskvariable__ {
  BSKUI32      id;     /* the identifier of the variable */
  BSKValue     value;  /* the variable's value */
  BSKVariable* next;
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
   * BSKAddNewVariable
   *
   * Adds the given variable object to the list, and returns the new
   * list.
   * -------------------------------------------------------------------- */
BSKVariable* BSKAddNewVariable( BSKVariable* list,
                                BSKVariable* newVar );

  /* -------------------------------------------------------------------- *
   * BSKFindVariable
   *
   * Searches the given list for a variable with the given identifier, and
   * returns the variable object when it is found.  If not such variable
   * exists, this function returns 0.
   * -------------------------------------------------------------------- */
BSKVariable* BSKFindVariable( BSKVariable* list, BSKUI32 id );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKVAR_H__ */
