/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskbtset.h
 *
 * The BSKBitSet type defines a set of bits (up to 2^(4*BIT_SET_SIZE) bits)
 * that may be manipulated either individually or as a set.  This type is
 * used extensively by the parser (bskparse.c).
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKBTSET_H__
#define __BSKBTSET_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

 /* a bit-set size of 4 gives us 32*4=128 possible tokens.  If this ever
  * becomes insufficient, this number should be incremented to increase
  * the number of possible tokens in increments of 32. */
#define BIT_SET_SIZE     ( 4 )

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef BSKUI32 BSKBitSet[ BIT_SET_SIZE ];

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
   * BSKClearBitSet
   *
   * Initializes the given bit set to 0.  The set is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKClearBitSet( BSKBitSet* set );

  /* -------------------------------------------------------------------- *
   * BSKSetBit
   *
   * Sets the indicated bit in the set, where bit is the 'index' of the
   * bit to set.  The set is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKSetBit( BSKBitSet* set, BSKUI32 bit );

  /* -------------------------------------------------------------------- *
   * BSKClearBit
   *
   * Clears the indicated bit in the set, where bit is the 'index' of the
   * bit to clear.  The set is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKClearBit( BSKBitSet* set, BSKUI32 bit );

  /* -------------------------------------------------------------------- *
   * BSKTestBit
   *
   * Returns BSKTRUE if the indicated bit is set, or BSKFALSE if it is
   * not, where bit is the 'index' of the bit to test.
   * -------------------------------------------------------------------- */
BSKBOOL BSKTestBit( BSKBitSet* set, BSKUI32 bit );

  /* -------------------------------------------------------------------- *
   * BSKOrBitSets
   *
   * Combines sets 'first' and 'second' into 'dest', using a bitwise OR
   * operation.  The set 'dest' is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKOrBitSets( BSKBitSet* dest, BSKBitSet* first, BSKBitSet* second );

  /* -------------------------------------------------------------------- *
   * BSKAndBitSets
   *
   * Combines sets 'first' and 'second' into 'dest', using a bitwise AND
   * operation.  The set 'dest' is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKAndBitSets( BSKBitSet* dest, BSKBitSet* first, BSKBitSet* second );

  /* -------------------------------------------------------------------- *
   * BSKInvBitSet
   *
   * Takes the twos-complement of the 'src' set, placing it in the 'dest'
   * set.  The 'dest' set is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKInvBitSet( BSKBitSet* dest, BSKBitSet* src );

  /* -------------------------------------------------------------------- *
   * BSKCopyBitSet
   *
   * Copies the set 'src' into the set 'dest'.  The set 'dest' is returned.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKCopyBitSet( BSKBitSet* dest, BSKBitSet* src );

  /* -------------------------------------------------------------------- *
   * BSKSetBits
   *
   * Sets the indicated bits in the given set, and returns the set.  The
   * list of bits to set must be terminated by '0'.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKSetBits( BSKBitSet* set, ... );

  /* -------------------------------------------------------------------- *
   * BSKClearBits
   *
   * Clears the indicated bits in the given set, and returns the set.  The
   * list of bits to clear must be terminated by '0'.
   * -------------------------------------------------------------------- */
BSKBitSet* BSKClearBits( BSKBitSet* set, ... );

  /* -------------------------------------------------------------------- *
   * BSKTestAnyBits
   *
   * Returns BSKTRUE if any of the bits in the list are set, and BSKFALSE
   * otherwise.  The list of bits to test must be terminated by 0.
   * -------------------------------------------------------------------- */
BSKBOOL BSKTestAnyBits( BSKBitSet* set, ... );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKBTSET_H__ */
