/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * A word here on unit lists and converting between units.
 *
 * Units can be thought of as a tree, where the "root" node of the tree
 * is the base unit, and the children of each node are all the units that
 * are defined relative to the the parent.  For example:
 *
 * unit cp;
 * unit sp = 10 cp;
 * unit gp = 10 sp;
 *
 * This forms a tree with three nodes.  'cp' forms the root, with one child
 * 'sp', and 'sp' has one child, 'gp'.  More complex trees are definately
 * possible, such as:
 *
 * unit cp;
 * unit sp = 10 cp;
 * unit gp = 10 sp;
 * unit ep = 15 cp;
 * unit wierdMoney = 3.5 sp;
 *
 * In this case, cp is still the root, but it has two children (sp and ep)
 * and sp has two children (gp and wierdMoney).
 *
 * Converting between two units requires that they both exist in the same
 * unit tree (ie, both have the same root node).  Converting from unit A
 * to unit B, you would first travel up the tree from unit A, collecting
 * the product of all the units between A and the root node (inclusive).
 * Then you would do the same for unit B, collecting the product of all
 * the units between B and the root node (inclusive).  Once you have the
 * two products, divide product(A) by product(B), and you'll have the
 * correct conversion, which you would then multiply by the quantity of
 * unit A you want converted to get the result.
 *
 * For example, suppose we were had 30 gp that we wanted to convert to ep
 * (using the tree in the example above).  The conversion product for gp is
 * 10 * 10, or 100.  The conversion product for ep is 15.  Divide 100 by
 * 15 and you get 20/3.  Multiply that by 30 (the number of gold pieces
 * to convert) and you get 200, which means that 30 gp equals 200 ep.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskutdef.h"

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
   * s_travelToParent
   *
   * Returns the ultimate parent of the given unit id in the list, and
   * puts the conversion value (the product of all the 'units' fields
   * for each unit definition in the path) in the 'conversion' parameter.
   * -------------------------------------------------------------------- */
static BSKUnitDefinition* s_travelToParent( BSKUnitDefinition* list,
                                            BSKUI32 id,
                                            BSKFLOAT* conversion );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKUnitDefinition* BSKAddUnitDef( BSKUnitDefinition* list,
                                  BSKUI32 id,
                                  BSKFLOAT units,
                                  BSKUI32 refUnit )
{
  BSKUnitDefinition* unit;

  unit = (BSKUnitDefinition*)BSKMalloc( sizeof( BSKUnitDefinition ) );
  BSKMemSet( unit, 0, sizeof( *unit ) );

  unit->id = id;
  unit->units = units;
  unit->referenceUnit = refUnit;
  unit->next = list;

  return unit;
}


BSKUnitDefinition* BSKFindUnitDef( BSKUnitDefinition* list,
                                   BSKUI32 id )
{
  BSKUnitDefinition* i;

  for( i = list; i != 0; i = i->next ) {
    if( i->id == id ) {
      return i;
    }
  }

  return 0;
}


void BSKDestroyUnitDefList( BSKUnitDefinition* list ) {
  BSKUnitDefinition* i;
  BSKUnitDefinition* j;

  i = list;
  while( i != 0 ) {
    j = i->next;
    BSKFree( i );
    i = j;
  }
}


BSKI32 BSKConvertUnits( BSKUnitDefinition* list,
                        BSKFLOAT quantity,
                        BSKUI32 from,
                        BSKUI32 to,
                        BSKFLOAT* result )
{
  BSKUnitDefinition *fromParent;
  BSKUnitDefinition *toParent;
  BSKFLOAT           numerator;
  BSKFLOAT           denominator;

  /* for a detailed description of the algorithm used here, see the
   * comment at the top of this file. */

  /* first, get the conversion factors for the two units */
  
  fromParent = s_travelToParent( list, from, &numerator );
  toParent = s_travelToParent( list, to, &denominator );

  /* if the top parent's match, and are not 0, then the units are
   * compatible, and the result can be obtained. */

  if( ( fromParent != 0 ) && ( fromParent == toParent ) ) {
    *result = quantity * numerator / denominator;
  } else {
    return UERR_INCOMPATIBLE_UNITS;
  }

  return 0;
}


BSKI32 BSKSerializeUnitDefListOut( BSKUnitDefinition* list,
                                   BSKStream* stream )
{
  BSKUI8 byte;

  /* A serialized unit definition list has the following format:
   *   byte:   1
   *   dword:  id
   *   float:  units
   *   dword:  referenceUnit
   *   ...
   *   byte:   0
   */

  while( list != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );
    stream->write( stream, &(list->id), sizeof( list->id ) );
    stream->write( stream, &(list->units), sizeof( list->units ) );
    stream->write( stream, &(list->referenceUnit), sizeof( list->referenceUnit ) );
    list = list->next;
  }
  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );

  return 0;
}


BSKUnitDefinition* BSKSerializeUnitDefListIn( BSKStream* stream ) {
  BSKUI8 byte;
  BSKUnitDefinition* unit;
  BSKUnitDefinition* list;

  list = 0;
  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    unit = (BSKUnitDefinition*)BSKMalloc( sizeof( BSKUnitDefinition ) );
    BSKMemSet( unit, 0, sizeof( *unit ) );
    stream->read( stream, &(unit->id), sizeof( unit->id ) );
    stream->read( stream, &(unit->units), sizeof( unit->units ) );
    stream->read( stream, &(unit->referenceUnit), sizeof( unit->referenceUnit ) );
    unit->next = list;
    list = unit;
    stream->read( stream, &byte, sizeof( byte ) );
  }

  return list;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKUnitDefinition* s_travelToParent( BSKUnitDefinition* list,
                                            BSKUI32 id,
                                            BSKFLOAT* conversion )
{
  BSKUnitDefinition* u;

  u = BSKFindUnitDef( list, id );
  if( u == 0 ) {
    return 0;
  }

  *conversion = u->units;
  while( u->referenceUnit > 0 ) {
    u = BSKFindUnitDef( list, u->referenceUnit );
    *conversion *= u->units;
  }

  return u;
}
