/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * Values are complex entities, and deserve a bit of documentation.  
 * Optional fields are shown in curly braces.  The presence or absence of
 * units in a numeric field is indicated by the presence of absence of the
 * VF_UNITS flag in the value's 'flags' field.
 *
 * VT_DICE
 *   A VT_DICE value has the following format.  If the modifier is present,
 *   the VF_MODIFIER flag is set in the value's 'flags' field, and if
 *   the modifier is multiplicative, rather than additive, the
 *   VF_MULTIPLICATIVE flag is also set.
 *      count    (BSKI16)
 *      type     (BSKUI16)
 *    { modifier (BSKI16) }
 *    { units    (BSKUI32) }
 *
 * VT_BYTE
 *   A VT_BYTE value has the following format:
 *      byte     (BSKI8)
 *    { units    (BSKUI32) }
 *
 * VT_WORD
 *   A VT_WORD value has the following format:
 *      word     (BSKI16)
 *    { units    (BSKUI32) }
 *
 * VT_DWORD
 *   A VT_DWORD value has the following format:
 *      dword    (BSKI32)
 *    { units    (BSKUI32) }
 *
 * VT_FLOAT
 *   A VT_FLOAT value has the following format:
 *      float    (BSKFLOAT)
 *    { units    (BSKUI32) }
 *
 * VT_STRING
 *   A VT_STRING value has the following format:
 *      string   (BSKCHAR*)
 *
 * VT_NULL
 *   A VT_NULL value has no data component.
 *
 * All other value types are simply points to the indicated entity (i.e.,
 * VT_THING values point to a BSKThing object, etc.).
 *
 * ---------------------------------------------------------------------- */

#include <math.h>

#include "bskenv.h"
#include "bsktypes.h"
#include "bskvalue.h"
#include "bskutil.h"
#include "bskvar.h"
#include "bskthing.h"
#include "bskctgry.h"
#include "bskrule.h"
#include "bskatdef.h"
#include "bskstream.h"
#include "bskdb.h"

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
   * s_determineNumberType
   *
   * Determines the smallest numeric type capable of accurately depicting
   * the specified value.  Returns the appropriate VT_xxx type.
   * -------------------------------------------------------------------- */
static BSKUI16 s_determineNumberType( BSKFLOAT num );

  /* -------------------------------------------------------------------- *
   * s_determineNumberSize
   *
   * Calculates the minimum number of bytes necessary to hold a numeric
   * value of the given type (not including units and modifiers, if
   * applicable).  VT_DICE values are not handled by this routine.
   * -------------------------------------------------------------------- */
static BSKUI8 s_determineNumberSize( BSKUI16 type );

  /* -------------------------------------------------------------------- *
   * s_setValueNumber
   *
   * Computes the smallest numeric type that will accurately represent
   * the given number, and sets the given number and units (if non-zero)
   * into the value.
   * -------------------------------------------------------------------- */
static void s_setValueNumber( BSKValue* value, BSKFLOAT num, BSKUI32 units );

  /* -------------------------------------------------------------------- *
   * s_setValueDice
   *
   * Creates a new VT_DICE value with the given count and type of dice,
   * and if modifier is non-zero, the given modifier and op ('+' or '*').
   * 'op' represents whether the modifier is additive or multiplicative.
   * If units is non-zero, the new value will have the given units as
   * well.
   * -------------------------------------------------------------------- */
static void s_setValueDice( BSKValue* value, 
                            BSKI16    count,
                            BSKUI16   type,
                            BSKCHAR   op,
                            BSKI16    modifier,
                            BSKUI32   units );

  /* -------------------------------------------------------------------- *
   * s_computeValueSize
   *
   * Computes the number of bytes occupied by the given value, including
   * units and modifiers for numeric types, and the null terminator for
   * VT_STRING.  Returns 0 for types that point to database entities
   * (VT_THING, etc.).
   * -------------------------------------------------------------------- */
static BSKUI32 s_computeValueSize( BSKValue* value );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

void BSKInitializeValue( BSKValue* value ) {
  BSKMemSet( value, 0, sizeof( *value ) );
}


void BSKInvalidateValue( BSKValue* value ) {
  if( BSKValueIsType( value, VT_ALLOCATED ) ) {
    BSKFree( value->datum );
  }
  value->type = VT_NULL;
}


BSKFLOAT BSKEvaluateNumber( BSKValue* value ) {
  BSKFLOAT n = 0;
  BSKI16   count;
  BSKUI16  type;
  BSKI16   modifier;

  if( BSKValueIsType( value, VT_NUMBER ) ) {
    switch( value->type ) {
      case VT_DICE:

        /* get the count and type of dice to roll */

        count = ((BSKI16*)value->datum)[0];
        type = ((BSKUI16*)value->datum)[1];
        n = BSKRollDice( count, type );

        /* if there is a modifier present, add or multiply it into the
         * result, as appropriate */

        if( ( value->flags & VF_MODIFIER ) != 0 ) {
          modifier = ((BSKI16*)value->datum)[2];
          if( ( value->flags & VF_MULTIPLICATIVE ) != 0 ) {
            n *= modifier;
          } else {
            n += modifier;
          }
        }
        break;
      case VT_BYTE:
				{
					BSKI8 tmp;
					BSKMemCpy( &tmp, value->datum, sizeof( BSKI8 ) );
					n = tmp;
				}
        break;
      case VT_WORD:
				{
					BSKI16 tmp;
					BSKMemCpy( &tmp, value->datum, sizeof( BSKI16 ) );
					n = tmp;
				}
        break;
      case VT_DWORD:
				{
					BSKI16 tmp;
					BSKMemCpy( &tmp, value->datum, sizeof( BSKI32 ) );
					n = tmp;
				}
        break;
      case VT_FLOAT:
				BSKMemCpy( &n, value->datum, sizeof( BSKFLOAT ) );
        break;
    }
  } else if( BSKValueIsType( value, VT_STRING ) ) {

    /* if we are evaluating a string as a number, convert the string to
     * a number and return the result. */

    n = BSKAtoF( (BSKCHAR*)value->datum );
  }

  return n;
}


void BSKDereferenceValue( BSKValue* dest, BSKValue* value ) {
  BSKValue* temp = 0;

  BSKInitializeValue( dest );

  /* if the value is dereferencable, get the value of the thing that the
   * 'value' object is pointing to (be it a variable, another value, an
   * attribute, or a category entry).  Then copy that value into the
   * destination.  Otherwise, just copy the 'value' into 'dest'. */

  if( BSKValueIsType( value, VT_DEREFERENCABLE ) ) {
    if( value->type == VT_VARIABLE ) {
      temp = &(((BSKVariable*)value->datum)->value);
    } else if( value->type == VT_VALUE ) {
      temp = (BSKValue*)value->datum;
    } else if( value->type == VT_ATTRIBUTE ) {
      temp = &(((BSKAttribute*)value->datum)->value);
    } else if( value->type == VT_CATEGORY_ENTRY ) {
      BSKCategoryEntry* entry;
      entry = (BSKCategoryEntry*)value->datum;
      if( entry->member == 0 ) {
        dest->type = VT_NULL;
        return;
      }
      
      /* use the 'reserved' member field to determine what the type of the
       * member is, so we can set the value to the correct type. */

      switch( entry->member->reserved ) {
        case OT_RULE:
          dest->type = VT_RULE;
          break;
        case OT_CATEGORY:
          dest->type = VT_CATEGORY;
          break;
        case OT_THING:
          dest->type = VT_THING;
          break;
      }
      dest->datum = entry->member;
      return;
    }
    BSKCopyValue( dest, temp );
  } else {
    BSKCopyValue( dest, value );
  }
}


void BSKCopyValue( BSKValue* dest, BSKValue* src ) {
  BSKUI32 size = 0;

  BSKInitializeValue( dest );

  dest->type = src->type;
  dest->flags = src->flags;

  /* if the value is VT_ALLOCATED, figure how much space the given value
   * occupies and allocate that much for the 'dest' value.  Then copy
   * 'src' into 'dest'. */

  if( BSKValueIsType( src, VT_ALLOCATED ) ) {
    switch( src->type ) {
      case VT_DICE:
        size = 2 * sizeof( BSKUI16 );
        if( ( src->flags & VF_MODIFIER ) != 0 ) {
          size += sizeof( BSKI16 );
        }
        break;
      case VT_BYTE:
        size = sizeof( BSKI8 );
        break;
      case VT_WORD:
        size = sizeof( BSKI16 );
        break;
      case VT_DWORD:
        size = sizeof( BSKI32 );
        break;
      case VT_FLOAT:
        size = sizeof( BSKFLOAT );
        break;
      case VT_STRING:
        size = BSKStrLen( (BSKCHAR*)src->datum ) + 1;
        break;
    }
    if( ( src->flags & VF_UNITS ) != 0 ) {
      size += sizeof( BSKUI32 );
    }
    dest->datum = BSKMalloc( size );
    BSKMemCpy( dest->datum, src->datum, size );
  } else {

    /* for non-VT_ALLOCATED values, simply share the pointer */

    dest->datum = src->datum;
  }
}


void BSKSetValueString( BSKValue* value, BSKCHAR* string ) {
  BSKInitializeValue( value );
  value->type = VT_STRING;
  value->datum = BSKStrDup( string );
}


void BSKSetValueNumber( BSKValue* value, BSKFLOAT number ) {
  s_setValueNumber( value, number, 0 );
}


void BSKSetValueNumberU( BSKValue* value, BSKFLOAT number, BSKUI32 units ) {
  s_setValueNumber( value, number, units );
}

void BSKSetValueDice( BSKValue* value, 
                      BSKI16  count, 
                      BSKUI16 type,
                      BSKCHAR op,
                      BSKI16  modifier )
{
  s_setValueDice( value, count, type, op, modifier, 0 );
}


void BSKSetValueDiceU( BSKValue* value, 
                       BSKI16  count, 
                       BSKUI16 type,
                       BSKCHAR op,
                       BSKI16  modifier,
                       BSKUI32 units )
{
  s_setValueDice( value, count, type, op, modifier, units );
}


BSKBOOL BSKValueOfAttributeType( BSKValue* value, BSKUI32 type ) {
  switch( type ) {
    case AT_NONE:
      return BSKValueIsType( value, VT_NULL );
    case AT_NUMBER:
      return BSKValueIsType( value, VT_NUMBER );
    case AT_STRING:
      return BSKValueIsType( value, VT_STRING );
    case AT_BOOLEAN:
      return BSKValueIsType( value, VT_BOOLEAN );
    case AT_THING:
      return BSKValueIsType( value, VT_THING );
    case AT_CATEGORY:
      return BSKValueIsType( value, VT_CATEGORY );
    case AT_RULE:
      return BSKValueIsType( value, VT_RULE );  
  }

  return BSKFALSE;
}


BSKBOOL BSKCompareValues( BSKValue* v1, BSKValue* v2 ) {
  BSKUI32 s1;
  BSKUI32 s2;

  s1 = s_computeValueSize( v1 );
  s2 = s_computeValueSize( v2 );

  /* if the sizes differ, then they can't be equivalent. */

  if( s1 != s2 ) {
    return BSKFALSE;
  }

  /* if they have the same size, and that size is 0, then they are pointers
   * to database objects and are literally equivalent only if the pointers
   * are the same. */

  if( s1 == 0 ) {
    return ( v1->datum == v2->datum );
  }

  /* otherwise, they are equivalent only if the buffers are equivalent,
   * byte-for-byte. */

  return ( BSKMemCmp( v1->datum, v2->datum, s1 ) == 0 );
}


BSKUI32 BSKValueUnits( BSKValue* value ) {
	BSKUI32  units;
	BSKUI32  offset;

  /* only numbers have units */

  if( !BSKValueIsType( value, VT_NUMBER ) ) {
    return 0;
  }

  /* units only exist if the VF_UNITS flag is set */

  if( ( value->flags & VF_UNITS ) == 0 ) {
    return 0;
  }

  /* determine the offset in the buffer where the units exist */

  switch( value->type ) {
    case VT_BYTE: offset = sizeof( BSKI8 ); break;
    case VT_WORD: offset = sizeof( BSKI16 ); break;
    case VT_DWORD: offset = sizeof( BSKI32 ); break;
    case VT_FLOAT: offset = sizeof( BSKFLOAT ); break;
    case VT_DICE: offset = sizeof( BSKI16 ) + sizeof( BSKUI16 ); break;
  }

  /* return the result as the units */

	BSKMemCpy( &units, (BSKCHAR*)( value->datum ) + offset +
			       ( ( value->flags & VF_MODIFIER ) ? sizeof( BSKI16 ) : 0 ),
						 sizeof( BSKUI32 ) );

  return units;
}


void BSKGetDiceParts( BSKValue* value, BSKI16* count,
                      BSKUI16* type, BSKCHAR* op, BSKI16* modifier )
{
  BSKCHAR* p;

  if( BSKValueIsType( value, VT_DICE ) ) {
    if( ( value->flags & VF_MULTIPLICATIVE ) != 0 ) {
      *op = '*';
      *modifier = 1;
    } else {
      *op = '+';
      *modifier = 0;
    }
    p = value->datum;
		BSKMemCpy( count, p, sizeof( BSKI16 ) );
    p += sizeof( BSKI16 );
		BSKMemCpy( type, p, sizeof( BSKUI16 ) );
    if( ( value->flags & VF_MODIFIER ) != 0 ) {
      p += sizeof( BSKUI16 );
			BSKMemCpy( modifier, p, sizeof( BSKI16 ) );
    }
  } else if( BSKValueIsType( value, VT_NUMBER ) ) {
    *count = BSKEvaluateNumber( value );
    *type = 1;
    *modifier = 0;
    *op = '+';
  } else {
    *count = 0;
    *type = 0;
    *modifier = 0;
    *op = 0;
  }
}


BSKI32 BSKSerializeValueOut( BSKValue* value, BSKStream* stream ) {
  BSKUI16 size;

  /* A serialized value has the following format:
   *   word:    type
   *   byte:    flags
   *
   *   for strings:
   *     word:  BSKStrLen (excluding null terminator)
   *     char*: string contents
   *
   *   for numbers:
   *     byte stream: the contents of the number buffer
   *
   *   for database objects:
   *     dword: id
   *
   */

  stream->write( stream, &(value->type), sizeof( value->type ) );
  stream->write( stream, &(value->flags), sizeof( value->flags ) );
  
  if( BSKValueIsType( value, VT_STRING ) ) {
    size = s_computeValueSize( value ) - 1;
    stream->write( stream, &size, sizeof( size ) );
    stream->write( stream, value->datum, size );
  } else if( BSKValueIsType( value, VT_NUMBER ) ) {
    size = s_computeValueSize( value );
    stream->write( stream, value->datum, size );
  } else {
    BSKUI32 id;

    switch( value->type ) {
      case VT_THING:
        id = ((BSKThing*)value->datum)->id;
        break;
      case VT_CATEGORY:
        id = ((BSKCategory*)value->datum)->id;
        break;
      case VT_RULE:
        id = ((BSKRule*)value->datum)->id;
        break;
    }

    stream->write( stream, &id, sizeof( id ) );
  }

  return 0;
}


BSKI32 BSKSerializeValueIn( struct __bskdatabase__* db, BSKValue* value, BSKStream* stream ) {
  BSKUI16 size;

  stream->read( stream, &(value->type), sizeof( value->type ) );
  stream->read( stream, &(value->flags), sizeof( value->flags ) );

  if( BSKValueIsType( value, VT_STRING ) ) {
    stream->read( stream, &size, sizeof( size ) );
    value->datum = BSKMalloc( size+1 );
    stream->read( stream, value->datum, size );
    ((BSKCHAR*)value->datum)[size] = 0;
  } else if( BSKValueIsType( value, VT_NUMBER ) ) {
    size = s_computeValueSize( value );
    value->datum = BSKMalloc( size );
    stream->read( stream, value->datum, size );
  } else {
    BSKUI32 id;

    stream->read( stream, &id, sizeof( id ) );
    switch( value->type ) {
      case VT_THING:
        value->datum = BSKFindThing( db, id );
        if( value->datum == 0 ) {
          value->datum = BSKAddThingToDB( db, BSKNewThing( id ) );
        }
        break;
      case VT_CATEGORY:
        value->datum = BSKFindCategory( db, id );
        if( value->datum == 0 ) {
          value->datum = BSKAddCategoryToDB( db, BSKNewCategory( id ) );
        }
        break;
      case VT_RULE:
        value->datum = BSKFindRule( db->rules, id );
        if( value->datum == 0 ) {
          value->datum = BSKAddRuleToDB( db, BSKNewRule( id ) );
        }
        break;
      default:
        /* FIXME: error handling */ ;
    }
  }

  if( BSKValueIsType( value, VT_STRING ) ) {
    ((BSKCHAR*)value->datum)[ size ] = 0;
  }

  return 0;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKUI16 s_determineNumberType( BSKFLOAT num ) {
  BSKFLOAT dif;

  /* check to see if the number has any digits after the decimal */
  
  dif = BSKFMod( num, 1.0 );

  /* if not, then it's an integer, and we just need to decide which *
   * size range it fits into.                                       */

  if( BSKFAbs( dif ) < VC_EPSILON ) {
    if( ( num >= -128 ) && ( num <= 127 ) ) {
      return VT_BYTE;
    } else if( ( num >= -32768 ) && ( num <= 32767 ) ) {
      return VT_WORD;
    }
    return VT_DWORD; 
  }
  
  /* if it has digits after the decimal, then it has to be a floating *
   * point number, regardless of how small it is.                     */

  return VT_FLOAT;
}


static BSKUI8 s_determineNumberSize( BSKUI16 type ) {
  switch( type ) {
    case VT_BYTE: return sizeof( BSKI8 );
    case VT_WORD: return sizeof( BSKI16 );
    case VT_DWORD: return sizeof( BSKI32 );
    case VT_FLOAT: return sizeof( BSKFLOAT );
  }

  return 0;
}


/* updated 21 Feb 2001 by Lydia Leong (lwl@sorcery.black-knight.org).  Previously,
 * typecasting the value->datum pointer and assigning values directly to it was
 * causing memory problems on some platforms.  This is fixed by using a memcpy
 * instead, and is probably a cleaner solution. */

static void s_setValueNumber( BSKValue* value, BSKFLOAT num, BSKUI32 units ) {
  BSKUI8    size;

  BSKInitializeValue( value );
  
  /* decide what type the number will be, and get the size of it */

  value->type = s_determineNumberType( num );
  size = s_determineNumberSize( value->type );

  /* allocate the number's buffer */

  value->datum = BSKMalloc( size + ( units > 0 ? sizeof( BSKUI32 ) : 0 ) );

  switch( value->type ) {
    case VT_BYTE:
			{
				BSKI8 tmp;
				tmp = (BSKI8)num;
				BSKMemCpy( value->datum, &tmp, size );
			}
      break;
    case VT_WORD:
			{
				BSKI16 tmp;
				tmp = (BSKI16)num;
				BSKMemCpy( value->datum, &tmp, size );
			}
      break;
    case VT_DWORD:
			{
				BSKI32 tmp;
				tmp = (BSKI32)num;
				BSKMemCpy( value->datum, &tmp, size );
      }
      break;
    case VT_FLOAT:
			BSKMemCpy( value->datum, &num, size );
      break;
  }

	/* if we have units, set a flag indicating this, and tack them on */

  if( units > 0 ) {
    value->flags |= VF_UNITS;
		BSKMemCpy( (BSKCHAR*)(value->datum) + size, &units, sizeof( BSKUI32 ) );
  }
}


/* updated 21 Feb 2001 by Lydia Leong (lwl@sorcery.black-knight.org).  Previously,
 * typecasting the value->datum pointer and assigning values directly to it was
 * causing memory problems on some platforms.  This is fixed by using a memcpy
 * instead, and is probably a cleaner solution. */

static void s_setValueDice( BSKValue* value, 
                            BSKI16    count,
                            BSKUI16   type,
                            BSKCHAR   op,
                            BSKI16    modifier,
                            BSKUI32   units )
{
  BSKInitializeValue( value );

  value->type = VT_DICE;
  value->flags = VF_NONE;

  /* allocate and populate the buffer */

	value->datum = BSKMalloc( sizeof( BSKI16 ) + sizeof( BSKUI16 ) +
			                      ( ( modifier != 0 ) ? sizeof( BSKI16 ) : 0 ) +
														( ( units > 0 ) ? sizeof( BSKUI32 ) : 0 ) );

	BSKMemCpy( value->datum, &count, sizeof( BSKI16 ) );
	BSKMemCpy( (BSKCHAR*)( value->datum ) + sizeof( BSKI16 ), &type, sizeof( BSKUI16 ) );

	/* if a modifier has been specified, set the appropriate flags */

  if( modifier != 0 ) {
		value->flags |= VF_MODIFIER;
		if( op == '*' ) {
			value->flags |= VF_MULTIPLICATIVE;
		}
		BSKMemCpy( (BSKCHAR*)( value->datum ) + sizeof( BSKI16 ) + sizeof( BSKUI16 ),
				       &modifier, sizeof( BSKI16 ) );
  }

  /* if there are units, add them in as well */

  if( units > 0 ) {
    value->flags |= VF_UNITS;
		BSKMemCpy( (BSKCHAR*)( value->datum ) + sizeof( BSKI16 ) + sizeof( BSKUI16 ) +
				       ( ( modifier != 0 ) ? sizeof( BSKI16 ) : 0 ),
							 &units, sizeof( BSKUI32 ) );
  }
}


static BSKUI32 s_computeValueSize( BSKValue* value ) {
  BSKUI32 size;

  switch( value->type ) {
    case VT_NULL: size = 0; break;
    case VT_BYTE: size = sizeof( BSKI8 ); break;
    case VT_WORD: size = sizeof( BSKI16 ); break;
    case VT_DWORD: size = sizeof( BSKI32 ); break;
    case VT_FLOAT: size = sizeof( BSKFLOAT ); break;
    case VT_STRING: size = BSKStrLen( (BSKCHAR*)value->datum ) + 1; break;
    case VT_DICE: size = 2 * sizeof( BSKI16 ); break;
    default:
      size = 0;
      break;
  }

  if( ( value->flags & VF_MODIFIER ) != 0 ) {
    size += sizeof( BSKI16 );
  }

  if( ( value->flags & VF_UNITS ) != 0 ) {
    size += sizeof( BSKUI32 );
  }

  return size;
}
