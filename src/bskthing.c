/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskthing.h"
#include "bskatdef.h"
#include "bskvalue.h"
#include "bskdb.h"
#include "bskidtbl.h"

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

BSKThing* BSKNewThing( BSKUI32 id ) {
  BSKThing* thing;

  thing = (BSKThing*)BSKMalloc( sizeof( BSKThing ) );
  BSKMemSet( thing, 0, sizeof( *thing ) );

  thing->reserved = OT_THING;
  thing->id = id;

  return thing;
}


void BSKDestroyThing( BSKThing* thing ) {
  BSKAttribute* attr;
  BSKAttribute* n;

  attr = thing->list;
  while( attr != 0 ) {
    n = attr->next;
    BSKInvalidateValue( &(attr->value) );
    BSKMemSet( attr, 0, sizeof( *attr ) );
    BSKFree( attr );
    attr = n;
  }

  BSKMemSet( thing, 0, sizeof( *thing ) );
  BSKFree( thing );
}


void BSKAddAttributeTo( BSKThing* thing, 
                        BSKUI32 id, 
                        BSKValue* value )
{
  BSKAttribute* attr;

  attr = (BSKAttribute*)BSKMalloc( sizeof( BSKAttribute ) );
  BSKMemSet( attr, 0, sizeof( *attr ) );

  attr->id = id;
  BSKCopyValue( &(attr->value), value );

  attr->next = thing->list;
  thing->list = attr;
}


BSKThing* BSKDuplicateThing( BSKThing* thing ) {
  BSKThing*     newThing;
  BSKAttribute* attr;
  BSKValue      newValue;

  newThing = BSKNewThing( 0 );
  
  attr = thing->list;
  while( attr != 0 ) {
    BSKCopyValue( &newValue, &(attr->value) );
    BSKAddAttributeTo( newThing, attr->id, &(newValue) );
    BSKInvalidateValue( &newValue );
    attr = attr->next;
  }

  return newThing;
}


BSKAttribute* BSKGetAttributeOf( BSKThing* thing, BSKUI32 id ) {
  BSKAttribute* attr;

  for( attr = thing->list; attr != 0; attr = attr->next ) {
    if( attr->id == id ) {
      return attr;
    }
  }

  return 0;
}


BSKAttribute* BSKGetAttributeAt( BSKThing* thing, BSKUI32 idx ) {
  BSKAttribute* attr;
  BSKUI32 i;

  i = 0;
  for( attr = thing->list; attr != 0; attr = attr->next ) {
    if( i == idx ) {
      return attr;
    }
    i++;
  }

  return 0;
}


BSKThing* BSKFindThing( struct __bskdatabase__* db, BSKUI32 id ) {
  BSKThing* i;

  i = (BSKThing*)BSKGetIdentifierData( db->idTable, id );
  if( i != 0 ) {
    if( i->reserved != OT_THING ) {
      i = 0;
    }
  }

  return i;
}


BSKI32 BSKSerializeThingListOut( BSKThing* list,
                                 BSKStream* stream )
{
  BSKUI8 byte;
  BSKAttribute* attr;

  /* a serialized list of things looks like this:
   *   byte:  1
   *   dword: id
   *     byte:  1
   *     dword: attr-id
   *     value: attr-value
   *     ...
   *     byte: 0
   *   ...
   *   byte: 0
   */

  while( list != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );

    stream->write( stream, &(list->id), sizeof( list->id ) );

    for( attr = list->list; attr != 0; attr = attr->next ) {
      byte = 1;
      stream->write( stream, &byte, sizeof( byte ) );
      stream->write( stream, &(attr->id), sizeof( attr->id ) );
      BSKSerializeValueOut( &(attr->value), stream );
    }

    byte = 0;
    stream->write( stream, &byte, sizeof( byte ) );

    list = list->next;
  }
  
  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );

  return 0;
}


void BSKSerializeThingListIn( struct __bskdatabase__* db, BSKStream* stream ) {
  BSKThing* thing;
  BSKThing* t;
  BSKAttribute* attrList;
  BSKAttribute* attr;
  BSKUI8 byte;

  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    thing = (BSKThing*)BSKMalloc( sizeof( BSKThing ) );
    BSKMemSet( thing, 0, sizeof( *thing ) );

    thing->reserved = OT_THING;
    stream->read( stream, &(thing->id), sizeof( thing->id ) );

    /* make sure that the given thing has not already been read referred to
     * somewhere else.  If it has, use the previous reference.  Otherwise,
     * add it to the database. */

    t = BSKFindThing( db, thing->id );
    if( t != 0 ) {
      BSKFree( thing );
      thing = t;
    } else {
      BSKAddThingToDB( db, thing );
    }

    attrList = 0;
    stream->read( stream, &byte, sizeof( byte ) );
    while( byte != 0 ) {
      attr = (BSKAttribute*)BSKMalloc( sizeof( BSKAttribute ) );
      BSKMemSet( attr, 0, sizeof( *attr ) );

      stream->read( stream, &(attr->id), sizeof( attr->id ) );
      BSKSerializeValueIn( db, &(attr->value), stream );

      stream->read( stream, &byte, sizeof( byte ) );

      attr->next = attrList;
      attrList = attr;
    }

    thing->list = attrList;

    stream->read( stream, &byte, sizeof( byte ) );
  }
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
