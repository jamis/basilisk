/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bskenv.h"
#include "bsktypes.h"
#include "bskdebug.h"

/* ---------------------------------------------------------------------- *
 * These macros are for use in maintaining and accessing the linked list
 * of allocated buffers.
 *   LAST(x) returns the previous pointer in the list
 *   NEXT(x) returns the next pointer in the list
 *   SIZE(x) returns a pointer to the number of bytes allocated
 *   FILENAME(x) returns the name of the file that allocated the buffer
 *   FILELINE(x) returns the line number at which the buffer was allocated
 *   BASE(x) returns the first byte of the buffer that was returned to
 *           the calling routine by BSKMallocDebug.
 * ---------------------------------------------------------------------- */

#define LAST(x)  (*(char**)x)
#define NEXT(x)  (*(char**)(x+sizeof(char**)))
#define START(x) (x+2*sizeof(char**))
#define SIZE(x)  START(x)
#define FILENAME(x) (START(x)+sizeof(BSKUI32))
#define FILELINE(x) (FILENAME(x)+BSKStrLen(FILENAME(x))+1)
#define BASE(x)  (FILELINE(x)+sizeof(BSKUI32)+sizeof(char**))

BSKUI32 g_currentAllocation = 0;
BSKUI32 g_maximumAllocation = 0;

/* ---------------------------------------------------------------------- *
 * These two variables are used to maintain the list of allocated
 * buffers.  's_head' points to the first allocated buffer, and 's_tail'
 * points to the last one.
 * ---------------------------------------------------------------------- */

static BSKCHAR* s_head = 0;
static BSKCHAR* s_tail = 0;

  /* -------------------------------------------------------------------- *
   * s_appendToList
   *
   * Appends the given buffer to the list of allocated memory.
   * -------------------------------------------------------------------- */
static void s_appendToList( BSKCHAR* ptr );

  /* -------------------------------------------------------------------- *
   * s_removeFromList
   *
   * Removes the given buffer from the list of allocated memory.
   * -------------------------------------------------------------------- */
static void s_removeFromList( BSKCHAR* ptr );

  /* -------------------------------------------------------------------- *
   * s_inList
   *
   * Returns BSKTRUE if the indicated buffer exists in the list of
   * allocated memory, and BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
static BSKBOOL s_inList( BSKCHAR* ptr );


void* BSKMallocDebug( BSKUI32 size, BSKCHAR* file, BSKUI32 line ) {
  char* ptr;
  char* original;
  BSKUI32 extra;

  /* warn when buffer's consisting of 0 bytes are allocated */
  if( size == 0 ) {
    fprintf( stdout, "!!! attempt to malloc 0 bytes at %s:%ld\n", file, line );
  }

  /* determine how much extra space we're going to need to store all the
   * information about this buffer.  This includes the size of the buffer,
   * the name of the calling file, the line number, the next pointer, the
   * previous pointer, and the pointer to the beginning of allocated
   * memory. */

  extra = 2 * sizeof(char**) + BSKStrLen( file ) + 1 + 2 * sizeof( BSKUI32 ) + sizeof( char** );

  original = ptr = (char*)malloc( size + extra );
  if( ptr == 0 ) {
    fprintf( stdout, "!!! out of memory at %s:%ld\n", file, line );
    exit(0);
  }

  /* append the buffer to the list */
  s_appendToList( ptr );

  /* set the values */
  ptr = START(ptr);
  *(BSKUI32*)ptr = size;
  ptr += sizeof( BSKUI32 );
  strcpy( ptr, file );
  ptr += BSKStrLen( file ) + 1;
  *(BSKUI32*)ptr = line;
  ptr += sizeof( BSKUI32 );
  *(char**)ptr = original;
  ptr += sizeof( char** );

  /* increment the number of bytes current allocated */
  g_currentAllocation += size;
  if( g_currentAllocation > g_maximumAllocation ) {
    /* if we've exceeded the previous maximum, set the current maximum to
     * the current number of bytes allocated. */
    g_maximumAllocation = g_currentAllocation;
  }

  /* return the memory requested */
  return (void*)(original+extra);
}


void BSKFreeDebug( void* ptr, BSKCHAR* file, BSKUI32 line ) {
  char* tptr;
  char* base;
  BSKUI32 size;

  /* calculate the base of the memory that was actually allocated by looking
   * at the four bytes below the buffer passed in.  These four bytes are
   * a pointer to the base of this buffer's allocated memory. */

  tptr = (char*)ptr;
  tptr = (char*)( tptr - sizeof( char** ) );
  base = tptr = *(char**)tptr;

  /* make sure that the buffer exists in the list of allocated memory */

  if( !s_inList( base ) ) {
    fprintf( stdout, "free of unallocated memory at %s:%ld!\n", file, line );
    return;
  }

  /* remove it from the list */

  s_removeFromList( tptr );

  /* get the number of bytes that the user requested for this buffer */
  tptr = START(tptr);
  size = *(BSKUI32*)tptr;
  tptr += sizeof( BSKUI32 );

  /* decrease the number of bytes currently allocated */
  g_currentAllocation -= size;

  /* free the memory */
  free( base );
}


  /* -------------------------------------------------------------------- *
   * printMem
   *
   * Print the requested number of bytes of 'buf' to the FILE f, displaying
   * binary characters in hexadecimal.
   * -------------------------------------------------------------------- */
static void printMem( FILE* f, BSKCHAR* buf, BSKUI32 len ) {
  BSKUI32 i;

  for( i = 0; i < len; i++ ) {
    if( ( buf[i] >= 32 ) && ( buf[i] < 127 ) ) {
      putc( buf[i], f );
    } else {
      BSKUI32 j;
      j = buf[i];
      fprintf( f, "[%02lX]", (BSKUI32)(unsigned char)j );
    }
  }
}


void BSKDumpAllocations( FILE* f ) {
  BSKCHAR* i;
  
  for( i = s_head; i != 0; i = NEXT(i) ) {
    fprintf( f, "not deallocated: %ld bytes at %p (%s:%ld) ", 
                *(BSKUI32*)SIZE(i),
                BASE(i),
                (BSKCHAR*)FILENAME(i),
                *(BSKUI32*)FILELINE(i) );
    printMem( f, BASE(i), *(BSKUI32*)SIZE(i) );
    printf( "\n" );
  }
}


static void s_appendToList( BSKCHAR* ptr ) {
  if( s_head == 0 ) {
    s_head = ptr;
  } else {
    NEXT(s_tail) = ptr;
  }

  LAST(ptr) = s_tail;
  NEXT(ptr) = 0;

  s_tail = ptr;
}


static void s_removeFromList( BSKCHAR* ptr ) {
  if( ptr == s_head ) {
    s_head = NEXT(ptr);
  }
  if( ptr == s_tail ) {
    s_tail = LAST(ptr);
  }

  if( LAST(ptr) != 0 ) {
    NEXT(LAST(ptr)) = NEXT(ptr);
  }

  if( NEXT(ptr) != 0 ) {
    LAST(NEXT(ptr)) = LAST(ptr);
  }
}


static BSKBOOL s_inList( BSKCHAR* ptr ) {
  BSKCHAR* i;
  
  for( i = s_head; i != 0; i = NEXT(i) ) {
    if( i == ptr ) {
      return BSKTRUE;
    }
  }

  return BSKFALSE;
}
