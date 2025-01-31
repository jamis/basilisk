#include "bskenv.h"
#include "bsktypes.h"
#include "bskvar.h"

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

BSKVariable* BSKAddNewVariable( BSKVariable* list,
                                BSKVariable* newVar )
{
  newVar->next = list;
  return newVar;
}


BSKVariable* BSKFindVariable( BSKVariable* list, BSKUI32 id ) {
  BSKVariable* i;

  for( i = list; i != 0; i = i->next ) {
    if( i->id == id ) {
      return i;
    }
  }

  return 0;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */
