/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskarray.h
 *
 * The BSKArray type defines a dynamic array of BSKValue items (see
 * bskvalue.h).  The array will grow to accommodate items at whatever
 * indices they are placed or requested at.  The BSKArray type is used
 * primarily during the run-time of the Basilisk engine, as it executes
 * rules (see bskrule.h, bskexec.h).
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKARRAY_H__
#define __BSKARRAY_H__

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

  /* BSKUI16 BSKArrayGetLength( BSKArray* ) */
#define BSKArrayGetLength(x)        ((x)->length)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskarray__ BSKArray;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskarray__ {
  BSKUI16   length;       /* # of elements */
  BSKUI8    flags;        /* used by the BSKExec routines */
  BSKUI8    ownerLevel;   /* used by the BSKExec routines */
  BSKValue* elements;     /* array of elements */
  BSKArray* next;         /* the next BSKArray item in the list */
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
   * BSKNewArray
   *
   * Creates and initializes a new array with a beginning capacity of
   * 'initLength.'  All elements are initialized to VT_NULL (see 
   * bskvalue.h).  If initLength is 0, the array is initialized with a
   * length of 0.  The new array object is returned.
   * -------------------------------------------------------------------- */
BSKArray* BSKNewArray( BSKUI16 initLength );

  /* -------------------------------------------------------------------- *
   * BSKDestroyArray
   *
   * Destroys the array object pointed to by 'array'.  Each element of the
   * array is invalidated (see bskvalue.h, BSKInvalidateValue).  The
   * array object is not valid after calling this function, and should not
   * be accessed.
   * -------------------------------------------------------------------- */
void BSKDestroyArray( BSKArray* array );

  /* -------------------------------------------------------------------- *
   * BSKGetElement
   *
   * Returns a pointer to the value at index 'element' of the array pointed
   * to by 'array'.  If the array is smaller than the index indicated by
   * 'element', the array is grown to encompass that index.  Note that the
   * value returned is the actual value at that index, NOT a copy, so it
   * should not be freed or invalidated (see bskvalue.h, BSKInvalidateValue)
   * unless you really mean to invalidate that value.
   * -------------------------------------------------------------------- */
BSKValue* BSKGetElement( BSKArray* array, BSKUI16 element );

  /* -------------------------------------------------------------------- *
   * BSKPutElement
   *
   * If the array is smaller than the index indicated by 'index', the
   * array is grown to encompass that index.  The value at that index
   * is then invalidated (bskvalue.h, BSKInvalidateValue), and the new
   * value is dereferenced into that index (bskvalue.h, 
   * BSKDereferenceValue).
   * -------------------------------------------------------------------- */
void BSKPutElement( BSKArray* array, BSKUI16 index, BSKValue* value );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKARRAY_H__ */
