/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskatdef.h"

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

BSKAttributeDef* BSKAddAttributeDef( BSKAttributeDef* list,
                                     BSKUI32 id,
                                     BSKUI8 type )
{
  BSKAttributeDef* attr;

  /* first, look for an attribute definition with the given id */

  attr = BSKFindAttributeDef( list, id );
  if( attr != 0 ) {
    if( attr->type != type ) {
      return 0;
    }
    return attr;
  }

  /* create and populate the new attribute definition, adding it to the
   * front of the list. */

  attr = (BSKAttributeDef*)BSKMalloc( sizeof( BSKAttributeDef ) );
  BSKMemSet( attr, 0, sizeof( *attr ) );

  attr->id = id;
  attr->type = type;
  attr->next = list;

  return attr;
}


BSKAttributeDef* BSKFindAttributeDef( BSKAttributeDef* list,
                                      BSKUI32 id )
{
  BSKAttributeDef* i;

  /* look at each item in the list until one is found with the given
   * id, then return that item. */

  for( i = list; i != 0; i = i->next ) {
    if( i->id == id ) {
      return i;
    }
  }

  return 0;
}


void BSKDestroyAttributeDefList( BSKAttributeDef* list ) {
  BSKAttributeDef* i;
  BSKAttributeDef* n;

  i = list;
  while( i != 0 ) {
    n = i->next;
    BSKFree( i );
    i = n;
  }
}


BSKI32 BSKSerializeAttrDefListOut( BSKAttributeDef* list, 
                                   BSKStream* stream )
{
  BSKUI8 byte;

  /* the serialized stream consists of a '1' if an attribute definition
   * follows, followed by the id and type of the definition, or a '0' if 
   * there are no more attribute definitions. */

  while( list != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );
    stream->write( stream, &(list->id), sizeof( list->id ) );
    stream->write( stream, &(list->type), sizeof( list->type ) );
    list = list->next;
  }
  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );

  return 0;
}


BSKAttributeDef* BSKSerializeAttrDefListIn( BSKStream* stream ) {
  BSKUI8 byte;
  BSKAttributeDef* list;
  BSKAttributeDef* attr;

  list = 0;
  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    attr = (BSKAttributeDef*)BSKMalloc( sizeof( BSKAttributeDef ) );
    BSKMemSet( attr, 0, sizeof( *attr ) );
    stream->read( stream, &(attr->id), sizeof( attr->id ) );
    stream->read( stream, &(attr->type), sizeof( attr->type ) );
    attr->next = list;
    list = attr;
    stream->read( stream, &byte, sizeof( byte ) );
  }

  return list;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
