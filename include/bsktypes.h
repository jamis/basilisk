/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bsktypes.h
 *
 * This file contains various type definitions used extensively by the
 * Basilisk engine.  Whenever possible, the types defined in this file
 * should be used in any development using the Basilisk engine.
 *
 * Looking at the types, you may wonder why BSKFLOAT and BSKDBL are both
 * actually of the type 'double'.  It is because, when I began, I
 * assumed that a 'float' would offer precision enough for my purposes,
 * but I later discovered that it did not, after I had already written
 * a considerable mass of code.  Rather than change all the existing
 * type declarations, I simply made BSKFLOAT a 'double'.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKTYPES_H__
#define __BSKTYPES_H__

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef signed   char      BSKI8;
typedef unsigned char      BSKUI8;
typedef signed   short int BSKI16;
typedef unsigned short int BSKUI16;
typedef signed   long  int BSKI32;
typedef unsigned long  int BSKUI32;

typedef char               BSKCHAR;
typedef BSKI8              BSKBOOL;

typedef void*              BSKNOTYPE;

typedef double             BSKFLOAT;
typedef double             BSKDBL;

/* ---------------------------------------------------------------------- *
 * Constants
 * ---------------------------------------------------------------------- */

#define BSKFALSE        ( 0 )
#define BSKTRUE         ( 1 )

#endif /* __BSKTYPES_H__ */
