#include "bskenv.h"
#include "bsktypes.h"
#include "bskutil.h"

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

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKUI32 BSKRollDice( BSKI32 count, BSKUI32 type ) {
  BSKUI32 total = 0;
  BSKUI32 i;
  BSKI8   sign;

  if( type == 0 ) {
    return total;
  }

  /* if count is  negative, store the sign so we can make the result
   * negative. */

  if( count < 0 ) {
    count = -count;
    sign = -1;
  } else {
    sign = 1;
  }

  for( i = 0; i < count; i++ ) {
    total += ( BSKRand() % type ) + 1;
  }

  return ( sign * total );
}


BSKCHAR* BSKStringReplace( BSKCHAR* primary,
                           BSKCHAR* target,
                           BSKCHAR* replacement,
                           BSKUI32  from )
{
  BSKCHAR* p;
  BSKUI32  plen;
  BSKUI32  tlen;
  BSKUI32  rlen;

  if( ( primary == 0 ) || ( target == 0 ) || ( replacement == 0 ) ) {
    return 0;
  }

  /* compute the lengths of the respective strings */

  plen = BSKStrLen( primary );
  tlen = BSKStrLen( target );
  rlen = BSKStrLen( replacement );

  /* if from is not a valid index for the given string and target, then
   * return 0. */

  if( from > ( plen - tlen ) ) {
    return 0;
  }

  /* if 'target' does not appear in 'primary', return 0 */

  p = BSKStrStr( &(primary[from]), target );
  if( p == 0 ) {
    return 0;
  }

  /* perform the replacement */

  BSKMemMove( (p+rlen), (p+tlen), ( plen - (BSKUI32)( p - primary ) - tlen ) + 1 );
  BSKMemCpy( p, replacement, rlen );

  return p;
}


BSKCHAR* BSKStringDup( BSKCHAR* str ) {
  BSKUI32 len;
  BSKCHAR* c;

  len = BSKStrLen( str );
  c = (BSKCHAR*)BSKMalloc( len + 1 );
  BSKStrCpy( c, str );

  return c;
}


BSKI32 BSKStrCmp_NoCase( const BSKCHAR* str1, const BSKCHAR* str2 ) {
  BSKUI32 i;
  BSKCHAR c1;
  BSKCHAR c2;

  for( i = 0; str1[i] != 0 && str2[i] != 0; i++ ) {
    c1 = BSKToLower( str1[i] );
    c2 = BSKToLower( str2[i] );

    if( c1 < c2 ) {
      return -1;
    } else if( c1 > c2 ) {
      return 1;
    }
  }

  if( str1[i] == 0 && str2[i] == 0 ) {
    return 0;
  }

  if( str1[i] != 0 ) {
    return 1;
  }

  return -1;
}


/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
