/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include <stdarg.h>

#include "bskenv.h"
#include "bsktypes.h"
#include "bskbtset.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

#define BIT(x) ( 1 << ( x % 32 ) )

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKBitSet* BSKClearBitSet( BSKBitSet* set ) {
  BSKI8 i;

  for( i = 0; i < BIT_SET_SIZE; i++ ) {
    (*set)[ i ] = 0;
  }

  return set;
}


BSKBitSet* BSKSetBit( BSKBitSet* set, BSKUI32 bit ) {
  (*set)[ bit / 32 ] |= BIT( bit );

  return set;
}


BSKBitSet* BSKClearBit( BSKBitSet* set, BSKUI32 bit ) {
  (*set)[ bit / 32 ] &= ~BIT( bit );

  return set;
}


BSKBOOL BSKTestBit( BSKBitSet* set, BSKUI32 bit ) {
  return ( ( (*set)[ bit / 32 ] & BIT( bit ) ) != 0 );
}


BSKBitSet* BSKOrBitSets( BSKBitSet* dest, BSKBitSet* first, BSKBitSet* second ) {
  BSKI8 i;

  for( i = 0; i < BIT_SET_SIZE; i++ ) {
    (*dest)[ i ] = (*first)[ i ] | (*second)[ i ];
  }

  return dest;
}


BSKBitSet* BSKAndBitSets( BSKBitSet* dest, BSKBitSet* first, BSKBitSet* second ) {
  BSKI8 i;

  for( i = 0; i < BIT_SET_SIZE; i++ ) {
    (*dest)[ i ] = (*first)[ i ] & (*second)[ i ];
  }

  return dest;
}


BSKBitSet* BSKInvBitSet( BSKBitSet* dest, BSKBitSet* src ) {
  BSKI8 i;

  for( i = 0; i < BIT_SET_SIZE; i++ ) {
    (*dest)[ i ] = ~(*src)[ i ];
  }

  return dest;
}


BSKBitSet* BSKCopyBitSet( BSKBitSet* dest, BSKBitSet* src ) {
  BSKI8 i;

  for( i = 0; i < BIT_SET_SIZE; i++ ) {
    (*dest)[ i ] = (*src)[ i ];
  }

  return dest;
}


BSKBitSet* BSKSetBits( BSKBitSet* set, ... ) {
  va_list arg;
  BSKUI16 bit;

  va_start( arg, set );
  do {
    bit = va_arg( arg, int );
    if( bit > 0 ) {
      BSKSetBit( set, bit );
    }
  } while( bit > 0 );
  va_end( arg );

  return set;
}


BSKBitSet* BSKClearBits( BSKBitSet* set, ... ) {
  va_list arg;
  BSKUI16 bit;

  va_start( arg, set );
  do {
    bit = va_arg( arg, int );
    if( bit > 0 ) {
      BSKClearBit( set, bit );
    }
  } while( bit > 0 );
  va_end( arg );

  return set;
}


BSKBOOL BSKTestAnyBits( BSKBitSet* set, ... ) {
  va_list arg;
  BSKUI16 bit;

  va_start( arg, set );
  do {
    bit = va_arg( arg, int );
    if( bit > 0 ) {
      if( BSKTestBit( set, bit ) ) {
        va_end( arg );
        return BSKTRUE;
      }
    }
  } while( bit > 0 );
  va_end( arg );

  return BSKFALSE;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
