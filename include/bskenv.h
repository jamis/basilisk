/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskenv.h
 *
 * This file basically just defines macros and constants to allow more
 * flexibility in which routines are used to allocate, deallocate, and
 * reference memory.  Other platform-dependant routines are also defined
 * here.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKENV_H__
#define __BSKENV_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "bskdebug.h"

/* if we are in debug mode, we will use the BSKMallocDebug and BSKFreeDebug
 * routines, otherwise we'll just use malloc and free. */

#ifdef _BSKDEBUG
#  define BSKMalloc(x)       BSKMallocDebug(x,__FILE__,__LINE__)
#  define BSKFree(x)         BSKFreeDebug(x,__FILE__,__LINE__)
#else
#  define BSKMalloc          malloc
#  define BSKFree            free
#endif

/* string/number conversion routines:
 *   BSKFLOAT BSKAtoF( BSKCHAR* str ) - converts str to a float
 *   BSKUI32  BSKAtoI( BSKCHAR* str ) - converts str to an int
 */

#define BSKAtoF            atof
#define BSKAtoI            atoi

/* memory manipulation routines:
 *   void BSKMemSet( void* buffer, BSKCHAR byte, BSKUI32 len )
 *         - sets 'len' bytes of 'buffer' to 'byte'
 *   void BSKMemCpy( void* dest, void* src, BSKUI32 len )
 *         - copies 'len' bytes of 'src' into 'dest'
 *   void BSKMemMove( void* dest, void* src, BSKUI32 len )
 *         - copies 'len' bytes of 'src' into 'dest', and handles
 *           overlapping memory regions correctly.
 *   BSKI8 BSKMemCmp( void* buf1, void* buf2, BSKUI32 len )
 *         - compares 'len' bytes of 'buf1' to 'buf2', and returns
 *           -1 if less, 0 if equal, or 1 if greater.
 */

#define BSKMemSet          memset
#define BSKMemCpy          memcpy
#define BSKMemMove         memmove
#define BSKMemCmp          memcmp

/* string manipulation routines:
 *   BSKI8 BSKStrCaseCmp( BSKCHAR* s1, BSKCHAR* s2 )
 *          - compares 's1' and 's2' lexically, without regard for
 *            for case.  Returns -1 if s1 < s2, 0 if s1 == s2, and
 *            1 if s1 > s2.
 *   BSKI8 BSKStrCmp( BSKCHAR* s1, BSKCHAR* s2 )
 *          - compares 's1' and 's2' lexically, with regard for
 *            case.  Returns same as BSKStrCaseCmp.
 *   void BSKStrCpy( BSKCHAR* s1, BSKCHAR* s2 )
 *          - copies 's2' to 's1', up to and including the null byte
 *            in 's2'.
 *   void BSKStrNCpy( BSKCHAR* s1, BSKCHAR* s2, int len )
 *          - copies 's2' into 's1', up to and including the null byte
 *            in 's2', unless s2 is longer than 'len' bytes, in which
 *            case only 'len' bytes are copied (and the string is
 *            not null-teriminated).
 *   BSKCHAR* BSKStrDup( BSKCHAR* str )
 *          - allocates BSKStrLen(str)+1 bytes and copies 'str' into
 *            that buffer.  The new buffer is returned.
 *   void BSKStrCat( BSKCHAR* dest, BSKCHAR* src )
 *          - appends 'src' onto 'dest'.
 *   BSKUI32 BSKStrLen( BSKCHAR* str )
 *          - returns the number of bytes in 'str' up to (but not
 *            including) the null terminator.
 *   BSKCHAR* BSKStrStr( BSKCHAR* str, BSKCHAR* pattern )
 *          - returns a pointer to the first instance of 'pattern'
 *            in 'str', or NULL if the pattern does not exist in
 *            'str'.
 *   BSKCHAR* BSKStrChr( BSKCHAR* str, BSKCHAR ch )
 *          - returns a pointer to the first instance of 'ch' in
 *            'str', or NULL if 'ch' does not exist in 'str'.
 */

#define BSKStrCaseCmp      BSKStrCmp_NoCase 
#define BSKStrCmp          strcmp
#define BSKStrCpy          strcpy
#define BSKStrNCpy         strncpy
#define BSKStrDup          BSKStringDup
#define BSKStrCat          strcat
#define BSKStrLen          strlen
#define BSKStrStr          strstr
#define BSKStrChr          strchr

/* character manipulation routines:
 *   BSKCHAR BSKToLower( BSKCHAR c )
 *      - returns a lowercase version of the character in 'c'
 *   BSKCHAR BSKToUpper( BSKCHAR c )
 *      - returns an uppercase version of the character in 'c'
 *   BSKBOOL BSKIsAlpha( BSKCHAR c )
 *      - returns BSKTRUE if 'c' is one of [a-zA-Z].
 *   BSKBOOL BSKIsAlNum( BSKCHAR c )
 *      - returns BSKTRUE if 'c' is one of [0-9a-zA-Z].
 *   BSKBOOL BSKIsDigit( BSKCHAR c )
 *      - returns BSKTRUE if 'c' is one of [0-9].
 */

#define BSKToLower         tolower
#define BSKToUpper         toupper
#define BSKIsAlpha         isalpha
#define BSKIsAlNum         isalnum
#define BSKIsDigit         isdigit

/* random number routines:
 *   BSKUI32 BSKRand()
 *      - returns a random, non-negative integer between
 *        0 and MAX_INT (inclusive, where MAX_INT is at least
 *        2^16).
 *   void BSKSRand( BSKUI32 seed )
 *      - uses the given seed to start the random number
 *        generator.
 */

#define BSKRand            rand
#define BSKSRand           srand

/* i/o routines:
 *   void BSKsprintf( BSKCHAR* buf, BSKCHAR* format, ... )
 *      - formats the given parameters after the format given
 *        by 'format', and puts the result in 'buf'. (See
 *        'printf' documentation for ANSI C for description of
 *        format parameter).
 */

#define BSKsprintf         sprintf

/* math routines:
 *   BSKFLOAT BSKFMod( BSKFLOAT a, BSKFLOAT b )
 *     - returns 'a mod b'
 *   BSKFLOAT BSKFAbs( BSKFLOAT a )
 *     - returns the absolute value of a
 *   BSKFLOAT BSKPow( BSKFLOAT a, BSKFLOAT b )
 *     - returns a raised to the power of b
 *   BSKFLOAT BSKFloor( BSKFLOAT a )
 *     - returns the largest whole number not greater than a
 *   BSKFLOAT BSKSqrt( BSKFLOAT a )
 *     - returns the square root of a
 *   BSKFLOAT BSKLog( BSKFLOAT a )
 *     - returns the natural log of a
 */

#define BSKFMod            fmod
#define BSKFAbs            fabs
#define BSKPow             pow
#define BSKFloor           floor
#define BSKSqrt            sqrt
#define BSKLog             log

/* "object" types */
#define OT_THING         (  1 )
#define OT_CATEGORY      (  2 )
#define OT_RULE          (  3 )

/* type id's -- for use by the 'type-of' operator in rules */
#define TID_CATEGORY     (  1 )
#define TID_THING        (  2 )
#define TID_RULE         (  3 )
#define TID_NUMBER       (  4 )
#define TID_STRING       (  5 )
#define TID_BOOLEAN      (  6 )
#define TID_ARRAY        (  7 )

/* object flags for use by things and categories */
#define OF_NONE          (  0 )
#define OF_PERSISTANT    (  1 )
#define OF_TRANSIENT     (  2 )
#define OF_LOCAL         (  3 )
#define OF_VISITED       (  4 )

#endif /* __BSKENV_H__ */
