/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskctgry.h
 *
 * A BSKCategory object contains zero or more "null" objects and objects
 * of type BSKRule (bskrule.h), BSKThing (bskthing.h) and BSKCategory.  It
 * is used in the database to categorize things.  Items in a BSKCategory
 * object may also be obtained by index (as an array) or randomly, using
 * a weighting system where each item has a weight that determines its
 * likelihood of being chosen.
 *
 * Internally, a BSKCategory object contains a list of BSKCategoryEntry
 * objects, each of which contains either a null (0), or a pointer to
 * a BSKCategory, BSKThing, or BSKRule.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKCTGRY_H__
#define __BSKCTGRY_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskthing.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKCategoryGetID( BSKCategory* ) */
#define BSKCategoryGetID(x)             ((x)->id)

  /* BSKUI32 BSKCategoryGetTotalWeight( BSKCategory* ) */
#define BSKCategoryGetTotalWeight(x)    ((x)->totalWeight)

  /* BSKCategoryEntry* BSKCategoryGetFirstMember( BSKCategory* ) */
#define BSKCategoryGetFirstMember(x)    ((x)->members)

  /* BSKUI16 BSKCategoryEntryGetWeight( BSKCategoryEntry* ) */
#define BSKCategoryEntryGetWeight(x)    ((x)->weight)

  /* BSKCategoryMember* BSKCategoryEntryGetMember( BSKCategoryEntry* ) */
#define BSKCategoryEntryGetMember(x)    ((x)->member)

  /* BSKUI8 BSKCategoryMemberGetType( BSKCategoryMember* ) */
#define BSKCategoryMemberGetType(x)     ((x)->reserved)

  /* BSKUI32 BSKCategoryMemberGetID( BSKCategoryMember* ) */
#define BSKCategoryMemberGetID(x)       ((x)->id)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskcategory__       BSKCategory;
typedef struct __bskcategoryentry__  BSKCategoryEntry;
typedef struct __bskcategorymember__ BSKCategoryMember;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskcategory__ {
  BSKUI8  reserved;              /* OT_CATEGORY */
  BSKUI32 id;                    /* id of the category */
  BSKUI8  flags;                 /* one of the OF_xxx flags in bskenv.h */
  BSKUI8  ownerLevel;            /* used during run-time (bskexec.c) */
  BSKCategory* next;             /* the next category in the list */

  BSKUI32 totalWeight;           /* total weight of all members */
  BSKCategoryEntry* members;     /* list of members */
  BSKCategoryEntry* tail;        /* tail of the list of members */
};


struct __bskcategoryentry__ {
  BSKUI16 weight;                /* weight of this member */
  BSKCategoryMember* member;     /* 0, or a pointer to a thing, rule, or category */
  BSKCategoryEntry* next;        /* the next member */
};


  /* -------------------------------------------------------------------- *
   * This structure is used to represent BSKThing, BSKCategory, and BSKRule
   * objects.  The first two fields of all three objects begin with
   * 'reserved' and 'id' fields, and this is used to extract those fields
   * and determine the type and id of each member in the category.
   * -------------------------------------------------------------------- */
struct __bskcategorymember__ {
  BSKUI8  reserved;
  BSKUI32 id;
};

  /* -------------------------------------------------------------------- *
   * Because the category serialization routines require access to the
   * database, we need to declare the database structure here, even though
   * it is formally defined in bskdb.h.
   * -------------------------------------------------------------------- */
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
   * BSKNewCategory
   *
   * Creates a new category with the given id, and returns the new object.
   * -------------------------------------------------------------------- */
BSKCategory* BSKNewCategory( BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDestroyCategory
   *
   * Destroys and deallocates the given category.  All BSKCategoryEntry
   * objects for this category are deallocated as well, though the objects
   * they point to are NOT deallocated.
   * -------------------------------------------------------------------- */
void BSKDestroyCategory( BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKAddToCategory
   *
   * Adds the given member to the given category with the given weight.
   * 'newMember' must either be null (0), or it must point to a BSKThing,
   * BSKRule, or BSKCategory object.  The object is added to the END of the
   * category contents list, contrary to the behavior of most other lists in
   * Basilisk.
   * -------------------------------------------------------------------- */
void BSKAddToCategory( BSKCategory* category, 
                       BSKUI16 weight, 
                       BSKNOTYPE newMember );

  /* -------------------------------------------------------------------- *
   * BSKGetCategoryMember
   *
   * Finds the BSKCategoryMember with the requested id, or null (0) if
   * no such category member exists.
   * -------------------------------------------------------------------- */
BSKCategoryMember* BSKGetCategoryMember( BSKCategory* category, 
                                         BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDuplicateCategory
   *
   * Duplicates the requested category, creating a new category with an id
   * of 0.  The BSKCategoryEntry objects are all duplicated as well, though
   * the members pointed to by the BSKCategoryEntry objects are NOT
   * duplicated.  The new BSKCategory object is returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKDuplicateCategory( BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKFindCategory
   *
   * Searches the list of BSKCategory objects in the database for a 
   * BSKCategory object with the given id.  If found, the BSKCategory object
   * is returned, otherwise 0 is returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKFindCategory( struct __bskdatabase__* db, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKGetCategoryMemberByWeight
   *
   * Searches the given BSKCategory object for the BSKCategoryMember at
   * the given "total" weight.  A member item's 'total weight' is the range
   * of numbers starting at the sum of all previous item's weights to that
   * value plus the member item's weight.  If no such member exists for the
   * given weight value, 0 is returned.
   * -------------------------------------------------------------------- */
BSKCategoryMember* BSKGetCategoryMemberByWeight( BSKCategory* category, 
                                                 BSKUI32 weight );

  /* -------------------------------------------------------------------- *
   * BSKGetAnyCategoryMember
   *
   * A number from 1 to the category's 'totalWeight' is randomly selected
   * and used as the weight value passed to BSKGetCategoryMemberByWeight.
   * The category member returned by that function is also returned by this
   * one.
   * -------------------------------------------------------------------- */
BSKCategoryMember* BSKGetAnyCategoryMember( BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKCategoryContains
   *
   * Returns BSKTRUE if the category object contains the requested member,
   * and BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
BSKBOOL BSKCategoryContains( BSKCategory* category, 
                             BSKCategoryMember* member );

  /* -------------------------------------------------------------------- *
   * BSKRemoveFromCategory
   *
   * Removes the requested member from the given category.  If the member
   * does not exist in the category, nothing happens.
   * -------------------------------------------------------------------- */
void BSKRemoveFromCategory( BSKCategory* category, 
                            BSKCategoryMember* member );

  /* -------------------------------------------------------------------- *
   * BSKRemoveAllFromCategory
   *
   * Removes all members from the given category, leaving the category
   * empty.
   * -------------------------------------------------------------------- */
void BSKRemoveAllFromCategory( BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKCategoryMemberCount
   *
   * Returns the number of members in the given category.
   * -------------------------------------------------------------------- */
BSKUI32 BSKCategoryMemberCount( BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * BSKGetEntryByIndex
   *
   * Returns the BSKCategoryEntry object at the given index 'idx', or 0
   * if the index is out of bounds.
   * -------------------------------------------------------------------- */
BSKCategoryEntry* BSKGetEntryByIndex( BSKCategory* category, BSKUI32 idx );

  /* -------------------------------------------------------------------- *
   * BSKGetMemberByIndex
   *
   * Returns the BSKCategoryMember object at the given index 'idx', or 0
   * if the index is out of bounds.  Note that 0 will also be returned for
   * a valid index if the member itself is null (0).
   * -------------------------------------------------------------------- */
BSKCategoryMember* BSKGetMemberByIndex( BSKCategory* category, 
                                        BSKUI32 idx );

  /* -------------------------------------------------------------------- *
   * BSKCategoryUnion
   *
   * Creates a new category that is the union of the given 'first' and
   * 'second' categories -- in other words, it contains all the items from
   * both the 'first' and 'second' categories.  The new category is
   * returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKCategoryUnion( BSKCategory* first, BSKCategory* second );

  /* -------------------------------------------------------------------- *
   * BSKCategoryIntersection
   *
   * Creates a new category that is the intersection of the given 'first' 
   * and 'second' categories -- in other words, it contains only the items
   * that exist in both the 'first' and 'second' categories.  The new 
   * category is returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKCategoryIntersection( BSKCategory* first, 
                                      BSKCategory* second );

  /* -------------------------------------------------------------------- *
   * BSKCategorySubtract
   *
   * Creates a new category that is the difference of the given 'first' 
   * and 'second' categories -- in other words, it contains only the items
   * in the 'first' category that do not exist in the 'second' category.  
   * The new category is returned.
   * -------------------------------------------------------------------- */
BSKCategory* BSKCategorySubtract( BSKCategory* first, BSKCategory* second );

  /* -------------------------------------------------------------------- *
   * BSKGetMemberIndex
   *
   * Returns the index of the requested member in the given category, or
   * -1 if the member does not exist in the category.
   * -------------------------------------------------------------------- */
BSKI32 BSKGetMemberIndex( BSKCategory* category, BSKCategoryMember* member );

  /* -------------------------------------------------------------------- *
   * BSKFindThingInCategory
   *
   * Searches the given category for a BSKThing member that has an
   * attribute with the given id.  If the 'value' parameter is not null,
   * the value of the attribute must also match.  The member is returned
   * if one is found, otherwise 0 is returned.
   * -------------------------------------------------------------------- */
BSKThing* BSKFindThingInCategory( BSKCategory* category,
                                  BSKUI32 attrId,
                                  BSKValue* value );

  /* -------------------------------------------------------------------- *
   * BSKSerializeCategoryListOut
   *
   * Writes the given category list to the given stream.  Returns '0' if
   * successful.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeCategoryListOut( BSKCategory* list,
                                    BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeCategoryListIn
   *
   * Reads BSKCategory object from the stream and adds them to the given
   * database.
   * -------------------------------------------------------------------- */
void BSKSerializeCategoryListIn( struct __bskdatabase__* db,
                                 BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKCTGRY_H__ */
