/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskctgry.h"
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

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKCategory* BSKNewCategory( BSKUI32 id ) {
  BSKCategory* category;

  category = (BSKCategory*)BSKMalloc( sizeof( BSKCategory ) );
  BSKMemSet( category, 0, sizeof( *category ) );

  category->reserved = OT_CATEGORY;
  category->id = id;

  return category;
}


void BSKDestroyCategory( BSKCategory* category ) {
  BSKCategoryEntry* i;
  BSKCategoryEntry* n;

  if( category->reserved != OT_CATEGORY ) {
    return;
  }

  i = category->members;
  while( i != 0 ) {
    n = i->next;
    BSKMemSet( i, 0, sizeof( *i ) );
    BSKFree( i );
    i = n;
  }

  BSKMemSet( category, 0, sizeof( *category ) );
  BSKFree( category );
}


void BSKAddToCategory( BSKCategory* category, 
                       BSKUI16 weight, 
                       BSKNOTYPE newMember ) 
{
  BSKCategoryMember* member;
  BSKCategoryEntry* entry;

  member = (BSKCategoryMember*)newMember;

  /* only allow null (0) members, or those that are of type BSKThing,
   * BSKCategory, or BSKRule.  All others fail silently. */

  if( member != 0 ) {
    if( member->reserved != OT_THING && 
        member->reserved != OT_CATEGORY &&
        member->reserved != OT_RULE ) {
      return;
    }

    /* don't allow duplicates in the same category.  if an item is added
     * to a category in which it already exists, this function silently
     * returns without adding the member again. */

    if( member->id != 0 ) {
      entry = category->members;
      while( entry != 0 ) {
        if( entry->member != 0 ) {
          if( entry->member->id == member->id ) {
            return;
          }
        }
        entry = entry->next;
      }
    }
  }

  /* create a new BSKCategoryEntry object to represent the member, add
   * the entry to the category, and increment the category's total weight
   * by the weight of the member. */

  entry = (BSKCategoryEntry*)BSKMalloc( sizeof( BSKCategoryEntry ) );
  entry->weight = weight;
  category->totalWeight += weight;
  entry->member = member;
  entry->next = 0;

  if( category->tail == 0 ) {
    category->members = entry;
  } else {
    category->tail->next = entry;
  }

  category->tail = entry;
}


BSKCategoryMember* BSKGetCategoryMember( BSKCategory* category, 
                                         BSKUI32 id )
{
  BSKCategoryEntry* entry;

  entry = category->members;
  while( entry != 0 ) {
    if( entry->member != 0 ) {
      if( entry->member->id == id ) {
        return entry->member;
      }
    }
    entry = entry->next;
  }

  return 0;
}


BSKCategory* BSKDuplicateCategory( BSKCategory* category ) {
  BSKCategory* newCategory;
  BSKCategoryEntry* i;

  if( category->reserved != OT_CATEGORY ) {
    return 0;
  }
  
  /* duplicate the category by creating a new category with no id, and
   * iteratively adding this category's members to the new one. */

  newCategory = BSKNewCategory( 0 );
  i = category->members;
  while( i != 0 ) {
    BSKAddToCategory( newCategory, i->weight, i->member );
    i = i->next;
  }

  return newCategory;
}


BSKCategory* BSKFindCategory( struct __bskdatabase__* db, BSKUI32 id ) {
  BSKCategory* i;

  i = (BSKCategory*)BSKGetIdentifierData( db->idTable, id );
  if( i != 0 ) {
    if( i->reserved != OT_CATEGORY ) {
      i = 0;
    }
  }

  return i;
}


BSKCategoryMember* BSKGetAnyCategoryMember( BSKCategory* category ) {
  BSKUI32 target;

  if( category->totalWeight < 1 ) {
    return 0;
  }

  target = ( BSKRand() % category->totalWeight ) + 1;
  return BSKGetCategoryMemberByWeight( category, target );
}


BSKCategoryMember* BSKGetCategoryMemberByWeight( BSKCategory* category, BSKUI32 weight ) {
  BSKUI32 i;
  BSKCategoryEntry* entry;

  if( category->totalWeight < 1 ) {
    return 0;
  }

  /* make the requested weight fit within the category's totalWeight */
  weight = ( weight % category->totalWeight ) + 1;

  i = 0;
  entry = category->members;
  while( entry != 0 ) {

    /* increment the iterator by the weight of the current entry */
    
    i += entry->weight;

    /* if the requested weight is less than the current weight, we've found
     * our man. */

    if( weight <= i ) {
      return entry->member;
    }

    entry = entry->next;
  }

  return 0;
}


BSKBOOL BSKCategoryContains( BSKCategory* category, BSKCategoryMember* member )
{
  /* a category contains an item if the item's index is non-negative */
  return ( BSKGetMemberIndex( category, member ) >= 0 );
}


void BSKRemoveFromCategory( BSKCategory* category, BSKCategoryMember* member )
{
  BSKCategoryEntry* entry;
  BSKCategoryEntry* prev;

  prev = 0;
  for( entry = category->members; entry != 0; entry = entry->next ) {
    if( entry->member == member ) {
      if( prev == 0 ) {
        category->members = entry->next;
      } else {
        prev->next = entry->next;
      }
      if( entry == category->tail ) {
        category->tail = prev;
      }
      category->totalWeight -= entry->weight;
      entry->next = 0;
      BSKFree( entry );
      break;
    }
    prev = entry;
  }
}


void BSKRemoveAllFromCategory( BSKCategory* category ) {
  BSKCategoryEntry* entry;
  BSKCategoryEntry* next;

  entry = category->members;
  while( entry != 0 ) {
    next = entry->next;
    BSKFree( entry );
    entry = next;
  }

  category->members = 0;
  category->tail = 0;
  category->totalWeight = 0;
}


BSKUI32 BSKCategoryMemberCount( BSKCategory* category ) {
  BSKCategoryEntry* entry;
  BSKUI32 count;

  for( count = 0, entry = category->members; entry != 0; count++, entry = entry->next )
    /* do loop */;

  return count;
}


BSKCategoryEntry* BSKGetEntryByIndex( BSKCategory* category, BSKUI32 idx ) {
  BSKCategoryEntry* entry;
  BSKUI32 count;

	count = 0;
  for( entry = category->members; entry != 0; entry = entry->next ) {
    if( count == idx ) {
      return entry;
    }
		count++;
  }
  
  return 0;  
}


BSKCategoryMember* BSKGetMemberByIndex( BSKCategory* category, BSKUI32 idx ) {
  BSKCategoryEntry* entry;

  entry = BSKGetEntryByIndex( category, idx );
  if( entry == 0 ) {
    return 0;
  }

  return entry->member;
}


BSKCategory* BSKCategoryUnion( BSKCategory* first, BSKCategory* second ) {
  BSKCategory* newCategory;
  BSKCategoryEntry* entry;

  newCategory = BSKNewCategory( 0 );

  /* since BSKAddToCategory already checks for duplicates, all we have
   * to do is add all entries from both categories to the new category,
   * and we have the union of the two categories. */

  if( first != 0 ) {
    for( entry = first->members; entry != 0; entry = entry->next ) {
      BSKAddToCategory( newCategory, entry->weight, entry->member );
    }
  }

  if( second != 0 ) {
    for( entry = second->members; entry != 0; entry = entry->next ) {
      BSKAddToCategory( newCategory, entry->weight, entry->member );
    }
  }

  return newCategory;
}


BSKCategory* BSKCategoryIntersection( BSKCategory* first, BSKCategory* second ) {
  BSKCategory* newCategory;
  BSKCategoryEntry* entry;

  newCategory = BSKNewCategory( 0 );

  /* an intersection of the two categories is the aggregate of all members
   * that exist in both the first and the second categories. */

  for( entry = first->members; entry != 0; entry = entry->next ) {
    if( BSKCategoryContains( second, entry->member ) ) {
      BSKAddToCategory( newCategory, entry->weight, entry->member );
    }
  }

  return newCategory;
}


BSKCategory* BSKCategorySubtract( BSKCategory* first, BSKCategory* second ) {
  BSKCategory* newCategory;
  BSKCategoryEntry* entry;

  newCategory = BSKNewCategory( 0 );

  /* the difference of the two categories is the aggregate of all members
   * that exist in the first category but not in the second. */

  for( entry = first->members; entry != 0; entry = entry->next ) {
    if( !BSKCategoryContains( second, entry->member ) ) {
      BSKAddToCategory( newCategory, entry->weight, entry->member );
    }
  }

  return newCategory;
}


BSKI32 BSKGetMemberIndex( BSKCategory* category, BSKCategoryMember* member ) {
  BSKCategoryEntry* entry;
  BSKI32 i = -1;

  for( entry = category->members; entry != 0; entry = entry->next ) {
    i++;
    if( entry->member == member ) {
      return i;
    }
  }

  return -1;
}


BSKThing* BSKFindThingInCategory( BSKCategory* category,
                                  BSKUI32 attrId,
                                  BSKValue* value )
{
  BSKCategoryEntry* entry;
  BSKAttribute* attribute;

  for( entry = category->members; entry != 0; entry = entry->next ) {

    /* first -- is the current member a "Thing"? */
    if( ( entry->member != 0 ) && ( entry->member->reserved == OT_THING ) ) {

      /* does the thing have the requested attribute? */
      attribute = BSKGetAttributeOf( (BSKThing*)entry->member, attrId );

      if( attribute != 0 ) {
        if( value != 0 ) {

          /* if the value given is not null, compare it to the attribute's
           * value.  A match means success. */
          if( BSKCompareValues( value, &(attribute->value) ) ) {
            return (BSKThing*)entry->member;
          }
        } else {

          /* otherwise, if the value IS null, then we just return the
           * current thing, because it has the requested attribute. */
          return (BSKThing*)entry->member;
        }
      }
    }
  }

  return 0;
}


BSKI32 BSKSerializeCategoryListOut( BSKCategory* list,
                                    BSKStream* stream )
{
  BSKUI8 byte;
  BSKCategoryEntry* e;

  /* a serialized category list is terminated by null byte (0).  it
   * consists of zero or more records of the following format:
   *    byte (1)
   *    dword (id)
   *    word (totalWeight)
   *    member-list
   * the serialized member list is terminated by a null byte (0).  it
   * consists of zero or more records of the following format:
   *    byte (1)
   *    if member is null then
   *      byte (0)
   *    else
   *      byte (member type)
   *      dword (member id)
   *    end
   */

  while( list != 0 ) {
    byte = 1;
    stream->write( stream, &byte, sizeof( byte ) );

    stream->write( stream, &(list->id), sizeof( list->id ) );
    stream->write( stream, &(list->totalWeight), sizeof( list->totalWeight ) );

    for( e = list->members; e != 0; e = e->next ) {
      byte = 1;
      stream->write( stream, &byte, sizeof( byte ) );

      stream->write( stream, &(e->weight), sizeof( e->weight ) );
      if( e->member != 0 ) {
        stream->write( stream, &(e->member->reserved), sizeof( e->member->reserved ) );
        stream->write( stream, &(e->member->id), sizeof( e->member->id ) );
      } else {
        byte = 0;
        stream->write( stream, &byte, sizeof( byte ) );
      }
    }

    /* write the list terminator */

    byte = 0;
    stream->write( stream, &byte, sizeof( byte ) );

    list = list->next;
  }

  /* write the list terminator */

  byte = 0;
  stream->write( stream, &byte, sizeof( byte ) );
  
  return 0;
}


void BSKSerializeCategoryListIn( struct __bskdatabase__* db, 
                                 BSKStream* stream ) 
{
  BSKCategory* category;
  BSKCategory* t;
  BSKCategoryEntry* e;
  BSKUI8 byte;
  BSKUI32 id;

  stream->read( stream, &byte, sizeof( byte ) );
  while( byte != 0 ) {
    category = (BSKCategory*)BSKMalloc( sizeof( BSKCategory ) );
    BSKMemSet( category, 0, sizeof( *category ) );

    category->reserved = OT_CATEGORY;
    stream->read( stream, &(category->id), sizeof( category->id ) );

    /* because categories may be forward referenced, check to make sure
     * that this category has not already been declared.  If it has,
     * free the currently allocated object and use the retrieved one. */

    t = BSKFindCategory( db, category->id );

    if( t != 0 ) {
      BSKFree( category );
      category = t;
    } else {
      BSKAddCategoryToDB( db, category );
    }
      
    stream->read( stream, &(category->totalWeight), sizeof( category->totalWeight ) );

    stream->read( stream, &byte, sizeof( byte ) );
    while( byte != 0 ) {
      e = (BSKCategoryEntry*)BSKMalloc( sizeof( BSKCategoryEntry ) );
      BSKMemSet( e, 0, sizeof( *e ) );

      stream->read( stream, &(e->weight), sizeof( e->weight ) );
      stream->read( stream, &byte, sizeof( byte ) );

      /* if the byte read is not zero, then it indicates the type of the
       * member.  Read the id next, and then look for the thing in the
       * appropriate database list.  Since categories are read after things
       * and rules are read, they must exist, but categories may be forward
       * referenced, so if a requested category is not found, create an
       * empty one with that id and store it in the database. */

      if( byte != 0 ) {
        stream->read( stream, &id, sizeof( id ) );
        switch( byte ) {
          case OT_THING:
            e->member = (BSKCategoryMember*)BSKFindThing( db, id );
            break;
          case OT_CATEGORY:
            e->member = (BSKCategoryMember*)BSKFindCategory( db, id );
            if( e->member == 0 ) {
              t = BSKNewCategory( id );
              BSKAddCategoryToDB( db, t );
              e->member = (BSKCategoryMember*)t;
            }
            break;
          case OT_RULE:
            e->member = (BSKCategoryMember*)BSKFindRule( db->rules, id );
            break;
        }
      }

      if( category->members == 0 ) {
        category->members = e;
      } else {
        category->tail->next = e;
      }

      category->tail = e;

      /* look for the list terminator */

      stream->read( stream, &byte, sizeof( byte ) );
    }

    /* look for the list terminator */

    stream->read( stream, &byte, sizeof( byte ) );
  }
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
