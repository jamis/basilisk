/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskvalue.h
 *
 * Values form the meat of any Basilisk database.  Every string, every
 * number, even every thing, category, and rule, is a value.  There are
 * other value types that are used internally, like CATEGORY_ENTRY and
 * ATTRIBUTE.
 *
 * When dealing with values, ALWAYS ALWAYS ALWAYS invalidate a value when
 * you are done with it!
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKVALUE_H__
#define __BSKVALUE_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * These are the standard value types used by Basilisk
   * -------------------------------------------------------------------- */
#define VT_NULL                  ( 0x0000 )
#define VT_DICE                  ( 0x0001 )
#define VT_BYTE                  ( 0x0002 )
#define VT_WORD                  ( 0x0004 )
#define VT_DWORD                 ( 0x0008 )
#define VT_FLOAT                 ( 0x0010 )
#define VT_STRING                ( 0x0020 )
#define VT_THING                 ( 0x0040 )
#define VT_CATEGORY              ( 0x0080 )
#define VT_RULE                  ( 0x0100 )
#define VT_VALUE                 ( 0x0200 )
#define VT_CATEGORY_ENTRY        ( 0x0400 )
#define VT_VARIABLE              ( 0x0800 )
#define VT_ATTRIBUTE             ( 0x1000 )
#define VT_ARRAY                 ( 0x2000 )
                                           
  /* -------------------------------------------------------------------- *
   * These are the combination values that define value groups.  If you
   * add a new value type, be sure to add it to the appropriate category
   * (or categories) below.
   * -------------------------------------------------------------------- */
#define VT_INTEGER               ( VT_BYTE | VT_WORD | VT_DWORD )
#define VT_NUMBER                ( VT_DICE | VT_BYTE | VT_WORD | VT_DWORD | VT_FLOAT )
#define VT_CATEGORIZABLE         ( VT_THING | VT_CATEGORY | VT_RULE )
#define VT_DEREFERENCABLE        ( VT_CATEGORY_ENTRY | VT_ATTRIBUTE | VT_VALUE | VT_VARIABLE )
#define VT_ALLOCATED             ( VT_NUMBER | VT_STRING )
#define VT_BOOLEAN               ( VT_NUMBER )

  /* -------------------------------------------------------------------- *
   * These flags are used for number and dice value types (VT_NUMBER).
   * -------------------------------------------------------------------- */
#define VF_NONE                  ( 0x00 )
#define VF_UNITS                 ( 0x01 )
#define VF_MODIFIER              ( 0x02 )
#define VF_MULTIPLICATIVE        ( 0x04 )

  /* -------------------------------------------------------------------- *
   * These are constants used for comparing values.  VC_EPSILON defines
   * "close enough to be zero to be zero", and VC_INFINITY defines "big
   * enough that it might as well be infinity".
   * -------------------------------------------------------------------- */
#define VC_EPSILON               ( 0.000001 )
#define VC_INFINITY              ( 1e12 )

/* ---------------------------------------------------------------------- *
 * Macros
 * ---------------------------------------------------------------------- */

  /* BSKBOOL BSKValueIsType( BSKValue* value, BSKUI16 type ) */
#define BSKValueIsType(x,y)    ( ( (x)->type == (y) ) || ( ( (x)->type & (y) ) != 0 ) )

  /* BSKUI16 BSKValueGetType( BSKValue* value ) */
#define BSKValueGetType(x)     ((x)->type)

  /* BSKUI8 BSKValueGetFlags( BSKValue* value ) */
#define BSKValueGetFlags(x)    ((x)->flags)

  /* BSKNOTYPE BSKValueGetData( BSKValue* value ) */
#define BSKValueGetData(x)     ((x)->datum)

  /* BSKCHAR* BSKValueGetString( BSKValue* value ) */
#define BSKValueGetString(x)   ((BSKCHAR*)(x)->datum)

  /* BSKThing* BSKValueGetThing( BSKValue* value ) */
#define BSKValueGetThing(x)   ((BSKThing*)(x)->datum)

  /* BSKCategory* BSKValueGetCategory( BSKValue* value ) */
#define BSKValueGetCategory(x)   ((BSKCategory*)(x)->datum)

  /* BSKRule* BSKValueGetRule( BSKValue* value ) */
#define BSKValueGetRule(x)   ((BSKRule*)(x)->datum)

  /* BSKArray* BSKValueGetArray( BSKValue* value ) */
#define BSKValueGetArray(x)   ((BSKArray*)(x)->datum)

  /* void BSKValueSetThing( BSKValue* value, BSKThing* thing ) */
#define BSKValueSetThing(x,y) (x)->type=VT_THING;(x)->datum=y

  /* void BSKValueSetCategory( BSKValue* value, BSKCategory* cat ) */
#define BSKValueSetCategory(x,y) (x)->type=VT_CATEGORY;(x)->datum=y

  /* void BSKValueSetRule( BSKValue* value, BSKRule* rule ) */
#define BSKValueSetRule(x,y) (x)->type=VT_RULE;(x)->datum=y

  /* void BSKValueSetArray( BSKValue* value, BSKArray* array ) */
#define BSKValueSetArray(x,y) (x)->type=VT_ARRAY;(x)->datum=y

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskvalue__ BSKValue;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskvalue__ {
  BSKUI16   type;  /* VT_xxx constant(s) */
  BSKUI8    flags; /* VF_xxx constant(s) */
  BSKNOTYPE datum;
};

struct __bskdatabase__;

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
   * BSKInitializeValue
   *
   * Initializes the given value to VT_NULL.
   * -------------------------------------------------------------------- */
void BSKInitializeValue( BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKInvalidateValue
   *
   * Invalidates the value.  If the value is one of the VT_ALLOCATED types,
   * then the 'datum' field of the value is also deallocated.  Either way,
   * 'value' will be VT_NULL when this function exits.
   * -------------------------------------------------------------------- */
void BSKInvalidateValue( BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKEvaluateNumber
   *
   * If value is VT_NUMBER, the numeric value is returned (and if it is
   * VT_DICE, the dice are rolled to determine the number).  If value is
   * VT_STRING, the string is interpreted as if representing a number, and
   * that number is returned (ie, via atof).  If value is anything else,
   * 0 is returned.
   * -------------------------------------------------------------------- */
BSKFLOAT BSKEvaluateNumber( BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKDereferenceValue
   *
   * The value 'value' is dereferenced into 'dest'.  If the value is not
   * VT_DEREFERENCABLE, then a simple copy (BSKCopyValue) is performed,
   * otherwise the value that value points to is copied into dest.
   * -------------------------------------------------------------------- */
void BSKDereferenceValue( BSKValue* dest, BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKCopyValue
   *
   * Copies 'src' into 'dest'.  If 'src' is VT_ALLOCATED, a copy of the
   * memory is made and assigned to dest, otherwise, the datum field is
   * shared between the two values.
   * -------------------------------------------------------------------- */
void BSKCopyValue( BSKValue* dest, BSKValue* src );

  /* -------------------------------------------------------------------- *
   * BSKSetValueString
   *
   * Makes a copy of the given string and puts it in value, making value
   * of type VT_STRING.
   * -------------------------------------------------------------------- */
void BSKSetValueString( BSKValue* value, BSKCHAR* string );

  /* -------------------------------------------------------------------- *
   * BSKSetValueNumber
   *
   * Determines the smallest possible number type (VT_BYTE, VT_WORD, 
   * VT_DWORD, and VT_FLOAT) that will accurately represent the given
   * number, and then makes value of that type and value.
   * -------------------------------------------------------------------- */
void BSKSetValueNumber( BSKValue* value, BSKFLOAT number );

  /* -------------------------------------------------------------------- *
   * BSKSetValueNumberU
   *
   * Same as BSKSetValueNumber, but also sets the units of the number.
   * (the VF_UNITS flag is set, in this case).
   * -------------------------------------------------------------------- */
void BSKSetValueNumberU( BSKValue* value, BSKFLOAT number, BSKUI32 units );

  /* -------------------------------------------------------------------- *
   * BSKSetValueDice
   *
   * Creates a new VT_DICE value using the given count and type of dice, the
   * given operator (must be '+' or '*'), and the given modifier (must be
   * 1 if not used).  This allows dice definitions of the type 1d8, 3d10,
   * 1d4+1 and 2d6*100.
   * -------------------------------------------------------------------- */
void BSKSetValueDice( BSKValue* value, 
                      BSKI16  count, 
                      BSKUI16 type,
                      BSKCHAR op,
                      BSKI16  modifier );

  /* -------------------------------------------------------------------- *
   * BSKSetValueDiceU
   *
   * Same as BSKSetValueDice but also sets the units (the VF_UNITS flag
   * is set).
   * -------------------------------------------------------------------- */
void BSKSetValueDiceU( BSKValue* value, 
                       BSKI16  count, 
                       BSKUI16 type,
                       BSKCHAR op,
                       BSKI16  modifier,
                       BSKUI32 units );

  /* -------------------------------------------------------------------- *
   * BSKValueOfAttributeType
   *
   * Asks the question "is the type of this value equivalent to the given
   * attribute type?"  The 'type' parameter must be one of the AT_xxx
   * constants defined in bskatdef.h.  If the types are equivalent, this
   * function returns BSKTRUE, otherwise it returns BSKFALSE.
   * -------------------------------------------------------------------- */
BSKBOOL BSKValueOfAttributeType( BSKValue* value, BSKUI32 type );

  /* -------------------------------------------------------------------- *
   * BSKCompareValues
   *
   * Tests to see if the two values are literally equivalent.  This is
   * done by actually comparing the memory buffers of the two values.  If
   * equivalence is dependant upon more values that simply the contents
   * of the buffer (comparing two things, for instance), do not use this
   * function.  Returns BSKTRUE if the two values are literally equivalent,
   * and BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
BSKBOOL BSKCompareValues( BSKValue* v1, BSKValue* v2 );

  /* -------------------------------------------------------------------- *
   * BSKValueUnits
   *
   * Returns the identifier for the units of the given value, or 0 if there
   * are no units for the value.  This only really makes sense for
   * VT_NUMBER values.
   * -------------------------------------------------------------------- */
BSKUI32 BSKValueUnits( BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKGetDiceParts
   *
   * Returns the different parts of the given dice value.  If the value
   * is not a dice value, but IS a number, the count is the number itself,
   * and the type is '1', and the modifier is 0.  If the value is not a
   * dice or a number, this function sets all parts to 0.
   * -------------------------------------------------------------------- */
void BSKGetDiceParts( BSKValue* value, BSKI16* count,
                      BSKUI16* type, BSKCHAR* op, BSKI16* modifier );

  /* -------------------------------------------------------------------- *
   * BSKSerializeValueOut
   *
   * Writes the given value to the given stream.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeValueOut( BSKValue* value, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeValueIn
   *
   * Reads a new value from the stream into the 'value' parameter, using
   * the database object as necessary to look up database members (things,
   * categories, and rules).
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeValueIn( struct __bskdatabase__* db, BSKValue* value, BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKVALUE_H__ */
