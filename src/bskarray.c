/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskarray.h"

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

  /* -------------------------------------------------------------------- *
   * s_growArrayToFit
   *
   * The given array is grown to the requested length.  If the array is
   * already greater than or equal to the requested length, this routine
   * does nothing.  All new elements that are created by the resizing of
   * the array are initialized to VT_NULL (see bskvalue.h).
   * -------------------------------------------------------------------- */
static void s_growArrayToFit( BSKArray* array, BSKUI16 newLength );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKArray* BSKNewArray( BSKUI16 initLength ) {
  BSKArray* array;
  BSKUI16   i;

  /* allocate and initialize the array object */

  array = (BSKArray*)BSKMalloc( sizeof( BSKArray ) );
  BSKMemSet( array, 0, sizeof( *array ) );

  array->length = initLength;

  /* if the requested length is greater than 0, allocate the list of 
   * values and initialize them all. */

  if( initLength > 0 ) {
    array->elements = (BSKValue*)BSKMalloc( sizeof( BSKValue ) * initLength );
    for( i = 0; i < initLength; i++ ) {
      BSKInitializeValue( &(array->elements[i]) );
    }
  }

  return array;
}


void BSKDestroyArray( BSKArray* array ) {
  BSKUI16 i;

  /* invalidate all values in the array */

  if( array->length > 0 ) {
    for( i = 0; i < array->length; i++ ) {
      BSKInvalidateValue( &(array->elements[i]) );
    }
    BSKFree( array->elements );
  }

  /* free the memory allocated to the array */

  BSKMemSet( array, 0, sizeof( *array ) );
  BSKFree( array );
}


BSKValue* BSKGetElement( BSKArray* array, BSKUI16 element ) {
  
  /* if the requested element is out of the bounds of the array, resize
   * the array to include it. */

  if( element >= array->length ) {
    s_growArrayToFit( array, (BSKUI16)(element+1) );
  }

  /* return the address of the requested element */

  return &( array->elements[ element ] );
}


void BSKPutElement( BSKArray* array, BSKUI16 index, BSKValue* value ) {
  
  /* if the requested element is out of the bounds of the array, resize
   * the array to include it. */

  if( index >= array->length ) {
    s_growArrayToFit( array, (BSKUI16)(index+1) );
  }

  /* invalidate the value that already exists at that element, and
   * dereference the new value into that element.  We use the dereference
   * here so that references to variables, attributes, etc., actually
   * reference what those objects contain.  I.e., you don't want an array
   * element to point to a variable object that points to a thing, you just
   * want the element to point to the thing. */

  BSKInvalidateValue( &(array->elements[index]) );
  BSKDereferenceValue( &(array->elements[index]), value );
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static void s_growArrayToFit( BSKArray* array, BSKUI16 newLength ) {
  BSKValue* temp;
  BSKUI16   i;

  /* don't bother, if the array is already larger than the requested
   * size. */
  if( array->length >= newLength ) {
    return;
  }

  /* allocate and initialize a new array of values that is the requested
   * size. If there were any elements in the existing array, copy them
   * into the new array. */

  temp = (BSKValue*)BSKMalloc( newLength * sizeof( BSKValue ) );

  if( array->length > 0 ) {
    BSKMemCpy( temp, array->elements, sizeof( BSKValue ) * array->length );
    BSKFree( array->elements );
  }

  for( i = array->length; i < newLength; i++ ) {
    BSKInitializeValue( &(temp[i]) );
  }

  /* set the new array of elements to be the newly created array.  set
   * the length to be the requested length. */
     
  array->elements = temp;
  array->length = newLength;
}
