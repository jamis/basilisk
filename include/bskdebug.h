/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskdebug.h
 *
 * These routines are provided to help with bug tracking, especially memory
 * leaks caused by accessing unallocated memory, deallocating unallocated
 * memory, or neglecting to deallocate memory.  Note that for these routines
 * to be effective, all calls to allocate or deallocate memory MUST use the
 * BSKMallocDebug and BSKFreeDebug routines.  Other allocation and
 * deallocation routines should NOT be used in conjuction with these
 * routines, lest you incur the wrath of the great General Protection
 * Fault.
 *
 * One final note:  using these routines will SIGNIFICANTLY reduce the
 * performance of your application, and so should never be used for the
 * final version.  They are for debugging only.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKDEBUG_H__
#define __BSKDEBUG_H__

#include "bskenv.h"
#include "bsktypes.h"

  /* -------------------------------------------------------------------- *
   * g_currentAllocation
   *
   * Always tells the number of bytes currently allocated by BSKMallocDebug
   * that have not yet been deallocated with BSKFreeDebug.
   * -------------------------------------------------------------------- */
extern BSKUI32 g_currentAllocation;

  /* -------------------------------------------------------------------- *
   * g_maximumAllocation
   *
   * Always tells the maximum number of bytes that have been allocated
   * concurrently.
   * -------------------------------------------------------------------- */
extern BSKUI32 g_maximumAllocation;


  /* -------------------------------------------------------------------- *
   * BSKMallocDebug
   *
   * Allocates a buffer of at least the given size and returns it.  The
   * buffer is added internally to a list of allocated buffers, and marked
   * with the given file and line number information for debugging
   * purposes.  (The file and line number should be the name of the file
   * and the line number at which BSKMallocDebug is being called.
   * -------------------------------------------------------------------- */
void* BSKMallocDebug( BSKUI32 size, BSKCHAR* file, BSKUI32 line );

  /* -------------------------------------------------------------------- *
   * BSKFreeDebug
   *
   * Attempts to free the given buffer.  If the buffer does not exist in
   * the list of allocated buffers, an error message is printed to stderr
   * indicating the file and line # at which BSKFreeDebug was called.
   * Otherwise, the buffer is removed from the list and freed.
   * -------------------------------------------------------------------- */
void BSKFreeDebug( void* ptr, BSKCHAR* file, BSKUI32 line );

  /* -------------------------------------------------------------------- *
   * BSKDumpAllocations
   *
   * Writes a list of all unfreed allocations to the given FILE.  The
   * output includes the number of bytes and the file and line number at
   * which the buffer was allocated.  The contents of the buffer are also
   * displayed, with binary bytes displayed in hexidecimal.
   * -------------------------------------------------------------------- */
void BSKDumpAllocations( FILE* f );

#endif
