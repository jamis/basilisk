/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskutil.h
 *
 * This file defines a few utility functions that are used in various
 * places in the Basilisk API.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKUTIL_H__
#define __BSKUTIL_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

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
   * BSKRollDice
   *
   * Roll a 'type'-sided die 'count' times, and return the sum of the
   * results.  If count is negative, the result will be negative.
   * -------------------------------------------------------------------- */
BSKUI32 BSKRollDice( BSKI32 count, BSKUI32 type );

  /* -------------------------------------------------------------------- *
   * BSKStringReplace
   *
   * Searches 'primary' for the first appearance of 'target', at or after
   * 'from', and replaces 'target' with 'replacement.'  The string is
   * returned.  If 'target' is not found in 'primary', this function 
   * returns 0.
   * -------------------------------------------------------------------- */
BSKCHAR* BSKStringReplace( BSKCHAR* primary,
                           BSKCHAR* target,
                           BSKCHAR* replacement,
                           BSKUI32  from );

  /* -------------------------------------------------------------------- *
   * BSKStringDup
   *
   * Duplicates the given string and returns the copy.  This function
   * must be used (rather than the string library function strdup) because
   * some platforms may need to redefine BSKMalloc and BSKFree (bskenv.h)
   * to meet their own memory allocation/deallocation needs, and the
   * string duplication function must use their routines.
   * -------------------------------------------------------------------- */
BSKCHAR* BSKStringDup( BSKCHAR* str );

  /* -------------------------------------------------------------------- *
   * BSKStrCmp_NoCase
   *
   * Compares the two strings within worrying about what case the
   * characters are (ie, "HELLO" is the same as "hello"), and returns -1
   * if the first is lexically less than the second, 0 if they are
   * equivalent, and 1 if the second is greater than the first.
   * -------------------------------------------------------------------- */
BSKI32 BSKStrCmp_NoCase( const BSKCHAR* str1, const BSKCHAR* str2 );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKUTIL_H__ */
