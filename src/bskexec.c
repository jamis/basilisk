/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskthing.h"
#include "bskatdef.h"
#include "bskutdef.h"
#include "bskvalue.h"
#include "bskexec.h"
#include "bskctgry.h"
#include "bsksymtb.h"
#include "bskvar.h"
#include "bskutil.h"
#include "bskarray.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

#define VT_CATENATABLE            ( VT_NUMBER | VT_STRING )
#define VT_MATHABLE               ( VT_NUMBER )

#define CHECKPARMCNTIS(x,y)       if( x != y ) { \
                                    s_error( env, RTE_WRONG_PARAM_COUNT, buffer ); \
                                    return RTE_WRONG_PARAM_COUNT; \
                                  }

#define CHECKPARMCNTIS2(x,y,z)    if( ( x < y ) || ( x > z ) ) { \
                                    s_error( env, RTE_WRONG_PARAM_COUNT, buffer ); \
                                    return RTE_WRONG_PARAM_COUNT; \
                                  }

#define ENFORCETYPE(x,y,z,w)      if( ( x & y ) == 0 ) { \
                                    s_error( env, RTE_INVALID_OPERANDS, "wrong type for parameter #" z " in " w ); \
                                    return RTE_INVALID_OPERANDS; \
                                  }

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * s_execCode
   *
   * Executes the instructions for the indicated rule, under the indicated
   * environment.  Returns RTE_SUCCESS if successful, or one of the other
   * RTE_xxx constants otherwise.  Also sets the retVal field of the
   * topmost stack frame.  The stack frame stack should contain at least one
   * stack frame when this function is called.
   * -------------------------------------------------------------------- */
static BSKI32 s_execCode( BSKExecutionEnvironment* env, BSKRule* rule );

  /* -------------------------------------------------------------------- *
   * s_newStackFrame
   *
   * Creates, initializes, and returns a new BSKStackFrame object.  Also
   * pushes the new stack frame onto the top of the stack of stack frame
   * objects owned by the environment.
   * -------------------------------------------------------------------- */
static BSKStackFrame* s_newStackFrame( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_popStackFrame
   *
   * Deallocates and destroys the topmost stack frame object of the
   * environment.  The next top-most stack frame object becomes the current
   * one, and is returned.  All locally instantiated variables, arrays,
   * things, and categories are freed after first protecting any value
   * being returned as the return value from the called rule.
   * -------------------------------------------------------------------- */
static BSKStackFrame* s_popStackFrame( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_newStackItem
   *
   * Creates, initializes, and returns a new stack item in the given stack
   * frame.
   * -------------------------------------------------------------------- */
static BSKExecutionStackItem* s_newStackItem( BSKStackFrame* frame );

  /* -------------------------------------------------------------------- *
   * s_popStackItem
   *
   * The topmost stack item in the frame is deallocated after first
   * invalidating its 'value' member.
   * -------------------------------------------------------------------- */
static void s_popStackItem( BSKStackFrame* frame );

  /* -------------------------------------------------------------------- *
   * s_addOperands
   *
   * Adds the topmost two operands on the stack of the current stack
   * frame.  If the operands are numeric, they are added as numbers.  If
   * one or both are strings, they are catenated.  Otherwise, an error
   * condition is raised (RTE_INVALID_OPERANDS).  Both parameters are
   * popped from the stack and the new value is pushed onto it.
   *
   * Also, if the values are both numeric, they must also share the same
   * units or an RTE_WRONG_UNITS error is raised.  The result will be of
   * the same units as the original values.
   * -------------------------------------------------------------------- */
static BSKI32 s_addOperands( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_doOperands
   *
   * Executes the given math operation on the topmost two operands of the
   * stack of the current stack frame.  Both operands must be numeric or
   * an RTE_INVALID_OPERANDS error will result.  For a subtraction
   * (OP_SUB) operation, both numbers must have the same units.  For other
   * operations, either both numbers must have the same units or one of
   * them must have NO units specified.
   *
   * Both operands are popped from the stack and the new value is pushed
   * on.  The new value will have the same units as the operands.
   * -------------------------------------------------------------------- */
static BSKI32 s_doOperands( BSKExecutionEnvironment* env, BSKUI32 op );

  /* -------------------------------------------------------------------- *
   * s_ensureOpCount
   *
   * Returns BSKTRUE if there are at least 'count' items on the value
   * stack of the given stack frame, or BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
static BSKBOOL s_ensureOpCount( BSKStackFrame* frame, BSKUI32 count );

  /* -------------------------------------------------------------------- *
   * s_findVariable
   *
   * Searches the list of locally instantiated variables of the given
   * stack frame for a variable with the given id.  If one is found, it
   * is returned, otherwise a new variable is instantiated with the given
   * id, added to the stack frame's list, and returned.
   * -------------------------------------------------------------------- */
static BSKVariable* s_findVariable( BSKStackFrame* frame, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * s_compareable
   *
   * Returns BSKTRUE if the two stack items are of the same type, or at
   * least both matching the VT_NUMERIC mask.
   * -------------------------------------------------------------------- */
static BSKBOOL s_compareable( BSKValue* v1, BSKValue* v2 );

  /* -------------------------------------------------------------------- *
   * s_ultimateTypeOf
   *
   * If the value of the given item is dereferencable (ie, a VALUE,
   * VARIABLE, or ATTRIBUTE), the value is dereferenced and THAT value's
   * type is returned.  Otherwise, the type of the given stack item is
   * returned.
   * -------------------------------------------------------------------- */
static BSKUI32 s_ultimateTypeOf( BSKExecutionStackItem* i1 );

  /* -------------------------------------------------------------------- *
   * s_evaluateValue
   *
   * Evaluates the value by recursively dereferencing it as necessary and
   * returning a pointer to the value that the value holds, i.e., string,
   * floating point, integer, rule, thing, category, etc.  The pointer to
   * the value is returned in the buffer pointed to by 'ptr'.
   * -------------------------------------------------------------------- */
static void s_evaluateValue( BSKValue* value, BSKNOTYPE ptr );

  /* -------------------------------------------------------------------- *
   * s_callSub
   *
   * The top 'parmCount' items on the stack are popped off and made into
   * an array of BSKValue objects.  The next item on the stack is the
   * value to call, which must evaluate to either a rule or the identifier
   * of a built-in function, otherwise an RTE_CALL_OF_NONFUNCTION error
   * is generated.  The value to call is popped off the stack, the call is
   * made, and the return value is pushed onto the stack.  Returns
   * RTE_SUCCESS on success, and one of the other RTE_xxx codes if it
   * fails.
   * -------------------------------------------------------------------- */
static BSKI32 s_callSub( BSKExecutionEnvironment *env,
                         BSKUI32 parmCount );

  /* -------------------------------------------------------------------- *
   * s_execBuiltin
   *
   * Attempts to execute the built-in function identified by 'funcId',
   * with 'parmCount' 'parameters', and puts the return value in 'retVal.'
   * Returns RTE_SUCCESS on success, or one of the other RTE_xxx codes if
   * it fails.  It can fail if the funcId is not a valid built-in function,
   * or if the number or type of parameters are not correct.
   * -------------------------------------------------------------------- */
static BSKI32 s_execBuiltin( BSKExecutionEnvironment* env,
                             BSKUI32 funcId,
                             BSKUI32 parmCount,
                             BSKValue** parameters,
                             BSKValue* retVal );

  /* -------------------------------------------------------------------- *
   * s_doNegative
   *
   * Pops the topmost value (which must be, ultimately, NUMERIC) from the 
   * stack and negates it, pushing the result on the stack.  Returns 
   * RTE_SUCCESS on success or one of the other RTE_xxx codes on failure.
   * -------------------------------------------------------------------- */
static BSKI32 s_doNegative( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_doDereference
   *
   * The topmost stack item is considered to be an attribute of the
   * second topmost stack item.  The two stack items are popped off,
   * the requested attribute is extracted from the 'parent' item, and
   * pushed onto the stack as an ATTRIBUTE value.  Returns 
   * RTE_SUCCESS on success or one of the other RTE_xxx codes on failure.
   * -------------------------------------------------------------------- */
static BSKI32 s_doDereference( BSKExecutionEnvironment* env, 
                               BSKStackFrame* frame );

  /* -------------------------------------------------------------------- *
   * s_doLogicalNegation
   *
   * The topmost stack item must be either dereferencable (VARIABLE,
   * ATTRIBUTE, or VALUE) or NUMERIC.  It is popped off, logically
   * negated (a NOT operation), and the result pushed back onto the stack.
   * Returns RTE_SUCCESS on success, or one of the other RTE_xxx codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doLogicalNegation( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_testTopValue
   *
   * The topmost stack item must be either dereferencable or NUMERIC.
   * It is popped off and tested as to whether it is non-zero (TRUE)
   * or zero (FALSE), and the result placed in the 'result' parameter.
   * No value is subsequently pushed onto the stack. Returns RTE_SUCCESS on
   * success, or one of the other RTE_xxx codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_testTopValue( BSKExecutionEnvironment* env, 
                              BSKBOOL* result );

  /* -------------------------------------------------------------------- *
   * s_doPut
   *
   * Stores a value into a dereferencable object (VALUE, VARIABLE, or
   * ATTRIBUTE).  The topmost stack item is the value to store, and the
   * second is the object to store it in.  Both are popped from the stack,
   * the previous value in the dereferencable object is invalidated
   * (bskvalue.h, BSKInvalidateValue), and the new value is dereferenced
   * and put into the dereferencable object.  Returns RTE_SUCCESS on
   * success, or one of the other RTE_xxx codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doPut( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_doBooleanComparison
   *
   * Compares the two topmost stack items according to the given
   * operation (eq, lt, le, gt, ge, ne, or typeof).  The two topmost items
   * are popped off and the boolean result of the comparison (0 or 1) is
   * pushed onto the stack.  Returns RTE_SUCCESS on success or one of the
   * other RTE_xxx codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doBooleanComparison( BSKExecutionEnvironment* env, BSKUI32 op );

  /* -------------------------------------------------------------------- *
   * s_doBooleanConnector
   *
   * Performs the given logical operation (and/or) on the two topmost
   * stack items, which MUST be numeric.  If they are not, a value of
   * RTE_INVALID_OPERANDS is returned.  The two items are popped off and
   * the boolean result of the comparison (0 or 1) is pushed onto the
   * stack. Returns RTE_SUCCESS on success or one of the other RTE_xxx 
   * codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doBooleanConnector( BSKExecutionEnvironment* env, BSKUI32 op );

  /* -------------------------------------------------------------------- *
   * s_doFirst
   *
   * Performs a FIRST operation on the topmost stack item.  The topmost
   * stack item must be or point to a CATEGORY, otherwise an 
   * RTE_INVALID_OPERANDS error results.  The item is popped off, and its
   * first member is pushed onto the stack as a CATEGORY_ENTRY value, unless
   * the category is empty, in which case a NULL value is placed on the
   * stack.  Returns RTE_SUCCESS on success or one of the other RTE_xxx 
   * codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doFirst( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_doNext
   *
   * Performs a NEXT operation on the topmost stack item.  The topmost
   * stack item must be or point to a CATEGORY_ENTRY or NULL, otherwise an 
   * RTE_INVALID_OPERANDS error results.  The item is popped off, and the
   * next member in that category is pushed onto the stack as a 
   * CATEGORY_ENTRY value, unless there are no more entries, in which case a
   * NULL value is placed on the stack.  Returns RTE_SUCCESS on success or 
   * one of the other RTE_xxx codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doNext( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_addThingToFrame
   *
   * The given thing is set to be LOCAL (OF_LOCAL flag).  If the thing
   * already exists in the given frame's list of things, the function exits.
   * Otherwise, if the thing's ownerLevel is greater than the frame's level,
   * the thing is added to the frame's thing list.  The thing is always
   * returned.
   * -------------------------------------------------------------------- */
static BSKThing* s_addThingToFrame( BSKStackFrame* frame, BSKThing* thing );

  /* -------------------------------------------------------------------- *
   * s_addCategoryToFrame
   *
   * The given category is set to be LOCAL (OF_LOCAL flag).  If the category
   * already exists in the given frame's list of categories, the function 
   * exits.  Otherwise, if the category's ownerLevel is greater than the 
   * frame's level, the category is added to the frame's category list.  The
   * category is always returned.
   * -------------------------------------------------------------------- */
static BSKCategory* s_addCategoryToFrame( BSKStackFrame* frame, 
                                          BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * s_addArrayToFrame
   *
   * The given array is set to be LOCAL (OF_LOCAL flag).  If the array
   * already exists in the given frame's list of arrays, the function 
   * exits.  Otherwise, if the arrays's ownerLevel is greater than the 
   * frame's level, the array is added to the frame's array list.  The
   * array is always returned.
   * -------------------------------------------------------------------- */
static BSKArray* s_addArrayToFrame( BSKStackFrame* frame, BSKArray* array );

  /* -------------------------------------------------------------------- *
   * s_convertObjectFlag
   *
   * This function is used to recursively convert flags of THINGS,
   * CATEGORIES, and ARRAYS that are set to 'from', to 'to'.
   * -------------------------------------------------------------------- */
static void s_convertObjectFlag( BSKValue* value, BSKUI8 from, BSKUI8 to );

  /* -------------------------------------------------------------------- *
   * s_convertThingFlag
   *
   * This function is used to recursively convert flags of THINGS
   * that are set to 'from', to 'to'.  Each of the thing's attributes are
   * then passed iteratively to s_convertObjectFlag.
   * -------------------------------------------------------------------- */
static void s_convertThingFlag( BSKThing* thing, BSKUI8 from, BSKUI8 to );

  /* -------------------------------------------------------------------- *
   * s_convertCategoryFlag
   *
   * This function is used to recursively convert flags of CATEGORIES
   * that are set to 'from', to 'to'.  Each of the category's members that
   * are of type CATEGORY are passed to s_convertCategoryFlag, and each
   * of the members that are THINGS are passed to s_convertThingFlag.
   * -------------------------------------------------------------------- */
static void s_convertCategoryFlag( BSKCategory* category, BSKUI8 from, 
                                   BSKUI8 to );

  /* -------------------------------------------------------------------- *
   * s_collectTransientObjects
   *
   * Recursively descends the tree of values represented by 'value' and
   * adds all objects (THINGS, CATEGORIES, and ARRAYS) that have the
   * OF_TRANSIENT flag set to the appropriate list of the given stack frame.
   * See also s_collectTransientThing and s_collectTransientCategory.
   * -------------------------------------------------------------------- */
static void s_collectTransientObjects( BSKStackFrame* frame, 
                                       BSKValue* value );

  /* -------------------------------------------------------------------- *
   * s_collectTransientThing
   *
   * If the given thing has the OF_TRANSIENT flag set, it is added to the
   * given frame (s_addThingToFrame).  Each of the thing's attributes are
   * then passed in order to s_collectTransientObjects.
   * -------------------------------------------------------------------- */
static void s_collectTransientThing( BSKStackFrame* frame, 
                                     BSKThing* thing );

  /* -------------------------------------------------------------------- *
   * s_collectTransientCategory
   *
   * If the given category has the OF_TRANSIENT flag set, it is added to the
   * given frame (s_addCategoryToFrame).  Each of the category's 'thing'
   * members are passed to s_collectTransientThing and each of the 
   * category's 'category' members are passed to s_collectTransientCategory.
   * -------------------------------------------------------------------- */
static void s_collectTransientCategory( BSKStackFrame* frame, 
                                        BSKCategory* category );

  /* -------------------------------------------------------------------- *
   * s_cleanupValue
   *
   * Recursively descends the given value and destroys all objects that have
   * the given flag ('type') set.
   * -------------------------------------------------------------------- */
static void s_cleanupValue( BSKValue* value, BSKUI8 type );

  /* -------------------------------------------------------------------- *
   * s_cleanupThing
   *
   * Each of the thing's attributes are passed in order to s_cleanupValue.
   * Then, if the thing has the given flag set ('type'), it is destroyed.
   * -------------------------------------------------------------------- */

static void s_cleanupThing( BSKThing* thing, BSKUI8 type );

  /* -------------------------------------------------------------------- *
   * s_cleanupCategory
   *
   * The category's members are iterated through, and all things are
   * passed to s_cleanupThing, and all categories are passed to
   * s_cleanupCategory.  Then, if the category has the given flag set
   * ('type'), it is destroyed.
   * -------------------------------------------------------------------- */
static void s_cleanupCategory( BSKCategory* category, BSKUI8 type );

  /* -------------------------------------------------------------------- *
   * s_doGetElement
   *
   * The topmost stack item must be NUMERIC (or dereferencable to NUMERIC)
   * and the second stack item must be an ARRAY.  Both are popped off
   * and the element of ARRAY inidicated by NUMBER is pushed onto the
   * stack.  Returns RTE_SUCCESS on success, or one of the other RTE_xxx
   * codes.
   * -------------------------------------------------------------------- */
static BSKI32 s_doGetElement( BSKExecutionEnvironment* env );

  /* -------------------------------------------------------------------- *
   * s_doSort
   *
   * Sorts the given array, using the given rule as the comparison
   * operator.  The rule must accept two parameters, and return -1 if the
   * first parameter should be sorted before the second, 0 if they are 
   * equivelent, or 1 of the first parameter should be sorted after the
   * second.  The sort is done in place, so the array is actually modified
   * by this operation.
   * -------------------------------------------------------------------- */
static void s_doSort( BSKExecutionEnvironment* env, 
                      BSKArray* array, 
                      BSKRule* rule );

  /* -------------------------------------------------------------------- *
   * s_error
   *
   * If the "errorHandler" for the 'env' object is not null, it is called,
   * passing the given code and prefix to the handler.  The 'userData'
   * field for the 'env' object is also passed to the handler. This function
   * does nothing else.
   * -------------------------------------------------------------------- */
static void s_error( BSKExecutionEnvironment* env, BSKI32 code, 
                     BSKCHAR* pfx );

  /* -------------------------------------------------------------------- *
   * s_newThing
   *
   * If the 'model' parameter is null, a new thing is created (BSKNewThing),
   * otherwise, the given thing is duplicated (BSKDuplicateThing).  The
   * ownerLevel of the thing is set to be the level of the current stack
   * frame, and the thing is added to the stack frame using 
   * s_addThingToFrame.  The new thing is returned.
   * -------------------------------------------------------------------- */
static BSKThing* s_newThing( BSKExecutionEnvironment* env, 
                             BSKThing* model );

  /* -------------------------------------------------------------------- *
   * s_newCategory
   *
   * If the 'model' parameter is null, a new category is created 
   * (BSKNewCategory), otherwise, the given category is duplicated 
   * (BSKDuplicateCategory).  The ownerLevel of the category is set to be 
   * the level of the current stack frame, and the category is added to the 
   * stack frame using s_addCategoryToFrame.  The new category is returned.
   * -------------------------------------------------------------------- */
static BSKCategory* s_newCategory( BSKExecutionEnvironment* env, 
                                   BSKCategory* model );

  /* -------------------------------------------------------------------- *
   * s_newArray
   *
   * A new array object is created (BSKNewArray) using the 'init' parameter
   * as the initial size of the array.  The ownerLevel of the array is set 
   * to be the level of the current stack frame, and the array is added to 
   * the stack frame using s_addArrayToFrame.  The new array is returned.
   * -------------------------------------------------------------------- */
static BSKArray* s_newArray( BSKExecutionEnvironment* env, BSKUI16 init );

  /* -------------------------------------------------------------------- *
   * s_initializeNewAttribute
   *
   * Creates a new attribute for the given Thing of the given type, and
   * initializes it to an appropriate value.
   * -------------------------------------------------------------------- */
static BSKAttribute* s_initializeNewAttribute( BSKExecutionEnvironment* env,
                                               BSKThing* thing,
                                               BSKUI32   attribute );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKI32 BSKExec( BSKExecOpts* opts )
{
  BSKExecutionEnvironment env;
  BSKRule* rule;
  BSKI32 rc;
  BSKUI32 i;

  /* clear the value */
  
  BSKInitializeValue( opts->rval );

  /* initialize the environment object */
  
  BSKMemSet( &env, 0, sizeof( env ) );
  env.db = opts->db;
  env.errorHandler = opts->errorHandler;
  env.console = opts->console;
  env.userData = opts->userData;
  env.opts = opts;

  /* lookup the requested rule in the list of rules */
  
  rule = BSKFindRule( opts->db->rules, opts->ruleId );
  if( rule == 0 ) {
    s_error( &env, RTE_INVALID_RULE, "Invalid rule passed to BSKExec" );
    return RTE_INVALID_RULE;
  }

  /* create a new stack frame object and initialize its list of parameters
   * to those passed it.  Set the return value to use to be the one passed
   * in as well.  Set the currently executing rule to be the one passed in.
   */

  s_newStackFrame( &env );
  env.stackFrame->parameterCount = opts->parameterCount;
  env.stackFrame->parameters = opts->parameters;
  env.stackFrame->retVal = opts->rval;
  env.stackFrame->ruleId = opts->ruleId;

  /* For each parameter passed in, map it to a variable corresponding to
   * the positional parameter of the rule.  If more parameters are passed
   * in than are expected, make sure we don't count higher than the number
   * of parameters expected. */

  rc = ( opts->parameterCount < rule->parameterCount ? opts->parameterCount : rule->parameterCount );
  for( i = 0; i < rc; i++ ) {
    BSKVariable* v;
    v = s_findVariable( env.stackFrame, rule->parameters[i] );
    BSKCopyValue( &(v->value), opts->parameters[i] );
  }

  /* execute the rule's code */

  rc = s_execCode( &env, rule );

  /* convert all newly created values that are to be kept to OF_TRANSIENT,
   * so that s_popStackFrame will not deallocate them. (popStackFrame will
   * automatically convert the return value, so we don't need to do it
   * here. */

  for( i = 0; i < opts->parameterCount; i++ ) {
    s_convertObjectFlag( opts->parameters[i], OF_LOCAL, OF_TRANSIENT );
  }

  /* pop the topmost stack frame.  this will also deallocate all objects
   * that are not in the database and are not marked OF_TRANSIENT. */

  s_popStackFrame( &env );

  /* convert all OF_TRANSIENT objects back to OF_LOCAL, so that we can keep
   * track of them in the calling routine. Check the list of parameters
   * because a rule might use them to return values. */

  for( i = 0; i < opts->parameterCount; i++ ) {
    s_convertObjectFlag( opts->parameters[i], OF_TRANSIENT, OF_LOCAL );
  }

  s_convertObjectFlag( opts->rval, OF_TRANSIENT, OF_LOCAL );

  if( *(env.opts->halt) ) {
    rc = RTE_HALTED;
  }

  return rc;
}


void BSKCleanupReturnValue( BSKValue* value ) {
  s_cleanupValue( value, OF_LOCAL );

  /* always invalidate values that are no longer being used! */

  BSKInvalidateValue( value );
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKI32 s_execCode( BSKExecutionEnvironment* env, BSKRule* rule ) {
  BSKUI32 pc;
  BSKExecutionStackItem* top;
  BSKSymbolTableEntry* sym;
  BSKUI32 id;
  BSKI32 rc;
  BSKBOOL b;
  BSKVariable* var;

  for( pc = 0; pc < rule->codeLength; pc++ ) {

    /* check to see if the VM has been asked to halt execution */
    if( *(env->opts->halt) ) {
      break;
    }

    switch( rule->code[ pc ] ) {
      /* NOOP: NO OPeration.  Not really used, but here for completeness. */
      case OP_NOOP: break;

      /* LSTR: Load STRing.  Reads all subsequent bytes up to a 0 and
       * creates a new string value on the top of the stack with them. */
      case OP_LSTR: 
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueString( &(top->value), (BSKCHAR*)&(rule->code[pc]) );
        while( rule->code[pc] != 0 ) {
          pc++;
        }
        break;

      /* LBYTE: Load BYTE.  Reads the next byte and creates a new numeric
       * value on the top of the stack with it. */
      case OP_LBYTE:
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueNumber( &(top->value), (BSKI8)(rule->code[pc]) );
        break;

      /* LWORD: Load WORD:  Reads the next two bytes and creates a new
       * numeric value on the top of the stack with them.  Note that this 
       * will be dependent on the architecture of the host machine. */
      case OP_LWORD:
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueNumber( &(top->value), *(BSKI16*)(&rule->code[pc]) );
        pc++;
        break;

      /* LDWORD: Load Double WORD:  Reads the next four bytes and creates a
       * new numeric value on the top of the stack with them.  Note that 
       * this will be dependent on the architecture of the host machine. */
      case OP_LDWORD:
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueNumber( &(top->value), *(BSKI32*)(&rule->code[pc]) );
        pc += 3;
        break;

      /* LFLOAT: Load FLOATing point:  Reads the next sizeof(BSKFLOAT) bytes
       * and creates a new numeric value on the top of the stack with them.
       * Note that this will be dependent on the architecture of the host 
       * machine. */
      case OP_LFLOAT:
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueNumber( &(top->value), *(BSKFLOAT*)(&rule->code[pc]) );
        pc += sizeof( BSKFLOAT ) - 1;
        break;

      /* LDICE: Load DICE:  Reads the next four bytes and creates a new 
       * numeric value on the top of the stack with them. Note that this 
       * will be dependent on the architecture of the host machine.  See
       * bskvalue.c for more on the format of the dice type. */
      case OP_LDICE:
        pc++;
        top = s_newStackItem( env->stackFrame );
        BSKSetValueDice( &(top->value), *(BSKI16*)&(rule->code[pc]),
                                        *(BSKUI16*)&(rule->code[pc+sizeof(BSKUI16)]),
                                        0, 0 );
        pc += 2*sizeof( BSKUI16 )-1;
        break;

      /* LID: Load IDentifier:  Reads the next four bytes and treats them
       * as a DWORD value representing an identifier.  Depending on the
       * symbol type of the identifier, the appropriate object is loaded
       * onto the top of the stack. */
      case OP_LID:
        pc++;
        BSKMemCpy( &id, &(rule->code[pc]), sizeof( id ) );
        pc += sizeof( id )-1;
        sym = BSKGetSymbol( rule->symbols, id );
        if( sym == 0 ) {
          s_error( env, RTE_BUG, "loading identifier [symbol does not exist]" );
          return RTE_BUG;
        }
        top = s_newStackItem( env->stackFrame );
        switch( sym->type ) {
          case ST_THING:
            top->value.type = VT_THING;
            top->value.datum = BSKFindThing( env->db, id );
            break;
          case ST_CATEGORY:
            top->value.type = VT_CATEGORY;
            top->value.datum = BSKFindCategory( env->db, id );
            break;
          case ST_RULE:
            top->value.type = VT_RULE;
            top->value.datum = BSKFindRule( env->db->rules, id );
            break;
          case ST_VARIABLE:
            top->value.type = VT_VARIABLE;
            top->value.datum = s_findVariable( env->stackFrame, id );
            break;
          case ST_ATTRIBUTE:
          case ST_BUILTIN:
            BSKSetValueNumber( &(top->value), id );
            break;
          case ST_UNIT:
            s_error( env, RTE_INVALID_OPERANDS, "cannot load a unit identifier in this context" );
            return RTE_INVALID_OPERANDS;
          default:
            s_error( env, RTE_BUG, "loading identifier [unknown symbol type]" );
            return RTE_BUG;
        }
        break;

      /* NULL: load NULL:  Creates a null value on the top of the stack. */
      case OP_NULL: 
        top = s_newStackItem( env->stackFrame );
        break;

      /* CALL: CALL subroutine:  the next byte is the number of parameters
       * to this call. The return value is then loaded onto the stack. */
      case OP_CALL:
        pc++;
        rc = s_callSub( env, rule->code[ pc ] );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* ADD: adds (or catenates, for strings) the two topmost stack items.
       */
      case OP_ADD: 
        rc = s_addOperands( env );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* SUB: subtract
       * MUL: multiply
       * DIV: divide
       * MOD: modulus division
       * POW: exponentiation
       *   all five of these work on the two topmost stack items and push
       *   the result onto the top of the stack. */
      case OP_SUB:
      case OP_MUL:
      case OP_DIV:
      case OP_MOD:
      case OP_POW:
        rc = s_doOperands( env, rule->code[pc] );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* NEG: negates the topmost stack item. (see s_doNegative). */
      case OP_NEG:
        rc = s_doNegative( env );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* DREF: DeREFerence: dereferences an attribute of a 'thing'
       * (see s_doDereference) */
      case OP_DREF:
        rc = s_doDereference( env, env->stackFrame );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* LT: Less Than
       * GT: Greater Than
       * LE: Less than or Equal to
       * GE: Greater than or Equal to
       * EQ: EQuivilent
       * NE: Not Equivilent
       * TYPEOF: x is TYPEOF y
       * (see s_doBooleanComparison)
       */
      case OP_LT:
      case OP_GT:
      case OP_LE:
      case OP_GE:
      case OP_EQ:
      case OP_NE:
      case OP_TYPEOF:
        rc = s_doBooleanComparison( env, rule->code[pc] );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* NOT: does logical negation (see s_doLogicalNegation) */
      case OP_NOT: 
        rc = s_doLogicalNegation( env ); 
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* JUMP: moves the program counter to the specified address. */
      case OP_JUMP:
        pc++;
        BSKMemCpy( &pc, &(rule->code[pc]), sizeof( pc ) );
        pc--;
        break;

      /* JMPT: JUMp if True: tests the topmost stack item to see if it
       * is true, and if it is, moves the program counter to the specified
       * address. */
      case OP_JMPT:
        pc++;
        BSKMemCpy( &id, &(rule->code[pc]), sizeof( id ) );
        rc = s_testTopValue( env, &b );
        if( rc != 0 ) {
          return rc;
        }
        if( b ) {
          pc = id - 1;
        } else {
          pc += sizeof( id ) - 1;
        }
        break;

      /* JMPF: JUMp if False: tests the topmost stack item to see if it
       * is false, and if it is, moves the program counter to the specified
       * address. */
      case OP_JMPF:
        pc++;
        BSKMemCpy( &id, &(rule->code[pc]), sizeof( id ) );
        rc = s_testTopValue( env, &b );
        if( rc != 0 ) {
          return rc;
        }
        if( !b ) {
          pc = id - 1;
        } else {
          pc += sizeof( id ) - 1;
        }
        break;

      /* END: terminates execution of the current rule by moving the
       * program counter to the end of the code. */
      case OP_END:  
        pc = rule->codeLength;
        break;

      /* GET: dereferences the topmost stack item and puts the result
       * on the top of the stack */
      case OP_GET: 
        {
          BSKValue temp;
          if( !s_ensureOpCount( env->stackFrame, 1 ) ) {
            s_error( env, RTE_STACK_UNDERFLOW, "GET" );
            return RTE_STACK_UNDERFLOW;
          }
          BSKDereferenceValue( &temp, &(env->stackFrame->stack->value) );
          s_popStackItem( env->stackFrame );
          s_newStackItem( env->stackFrame );
          BSKCopyValue( &(env->stackFrame->stack->value), &temp );
          BSKInvalidateValue( &temp );
        }
        break;

      /* PUT: stores the topmost value into the value specified by the
       * second topmost value.  (See s_doPut) */
      case OP_PUT:
        rc = s_doPut( env );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* AND: x AND y
       * OR: x OR y
       * (see s_doBooleanConnector) */
      case OP_AND:
      case OP_OR:
        rc = s_doBooleanConnector( env, rule->code[pc] );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* FIRST: gets the first member of a category.
       * (See s_doFirst). */
      case OP_FIRST: 
        rc = s_doFirst( env ); 
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* NEXT: gets the next member of a category.
       * (See s_doNext). */
      case OP_NEXT:
        rc = s_doNext( env );
        if( rc != 0 ) {
          return rc;
        }
        break;

      /* DUP: DUPlicate: duplicates the topmost stack item and DOES NOT pop
       * the stack. */
      case OP_DUP: 
        if( !s_ensureOpCount( env->stackFrame, 1 ) ) {
          s_error( env, RTE_STACK_UNDERFLOW, "DUP" );
          return RTE_STACK_UNDERFLOW;
        }
        s_newStackItem( env->stackFrame );
        BSKCopyValue( &(env->stackFrame->stack->value), &(env->stackFrame->stack->next->value) );
        break;

      /* POP: pops the topmost stack item from the stack. */
      case OP_POP:
        s_popStackItem( env->stackFrame );
        break;

      /* ELEM: get ELEMent: gets an indexed element from an array (see
       * s_doGetElement). */
      case OP_ELEM:
        rc = s_doGetElement( env );
        break;

      /* SWAP: swaps the two topmost stack items on the stack */
      case OP_SWAP:
        {
          BSKExecutionStackItem* temp;
          if( !s_ensureOpCount( env->stackFrame, 2 ) ) {
            s_error( env, RTE_STACK_UNDERFLOW, "SWAP" );
            return RTE_STACK_UNDERFLOW;
          }
          temp = env->stackFrame->stack->next;
          env->stackFrame->stack->next = temp->next;
          temp->next = env->stackFrame->stack;
          env->stackFrame->stack = temp;
        }
        break;

      /* LINE: sets the current source line being executed to the DWORD
       * indicated by the next four bytes. */
      case OP_LINE:
        pc++;
        env->line = *(BSKUI32*)(&rule->code[pc]);
        pc += sizeof( BSKUI32 ) - 1;
        break;

      /* Otherwise, the instruction is unknown (which is almost definately
       * a bug, since nothing the user does in Basilisk script can cause
       * this) */
      default:
        s_error( env, RTE_UNKNOWN_INSTRUCTION, 0 );
        return RTE_UNKNOWN_INSTRUCTION;
    }
  }

  /* upon exiting, get the value of the return-value-variable (a special
   * variable, named __rval) and put it into the retVal field of the
   * stackFrame, dereferencing it as needed. */

  id = BSKFindIdentifier( env->db->idTable, RULE_RETURN_VAL );
  var = BSKFindVariable( env->stackFrame->variables, id );
  if( var != 0 ) {
    BSKDereferenceValue( env->stackFrame->retVal, &(var->value) );
  }

  return RTE_SUCCESS;
}


static BSKStackFrame* s_newStackFrame( BSKExecutionEnvironment* env ) {
  BSKStackFrame* frame;

  frame = (BSKStackFrame*)BSKMalloc( sizeof( BSKStackFrame ) );
  BSKMemSet( frame, 0, sizeof( *frame ) );

  /* increment the level of the stack frame */
    
  if( env->stackFrame ) {
    frame->level = env->stackFrame->level + 1;
  } else {
    frame->level = 1;
  }

  /* put the new frame at the top of the stack */

  frame->next = env->stackFrame;
  env->stackFrame = frame;

  return frame;
}


static BSKStackFrame* s_popStackFrame( BSKExecutionEnvironment* env ) {
  BSKStackFrame* frame;
  BSKVariable*   v;
  BSKVariable*   n;
  BSKThing*      t;
  BSKThing*      tn;
  BSKCategory*   c;
  BSKCategory*   cn;
  BSKArray*      a;
  BSKArray*      an;

  frame = env->stackFrame;
  if( frame == 0 ) {
    return 0;
  }

  /* update the stack frame stack */

  env->stackFrame = frame->next;

  /* destroy the variables instantiated by this frame */

  for( v = frame->variables; v != 0; v = n ) {
    n = v->next;
    BSKInvalidateValue( &(v->value) );
    BSKFree( v );
  }

  /* convert all LOCAL objects referenced by the return value to
   * TRANSIENT, so they will persist to the caller. */

  s_convertObjectFlag( frame->retVal, OF_LOCAL, OF_TRANSIENT );

  /* destroy all things that are marked LOCAL */

  for( t = frame->things; t != 0; t = tn ) {
    tn = t->next;
    if( t->flags == OF_LOCAL ) {
      BSKDestroyThing( t );
    } else {
      t->next = 0;
    }
  }

  /* destroy all categories that are marked LOCAL */

  for( c = frame->categories; c != 0; c = cn ) {
    cn = c->next;
    if( c->flags == OF_LOCAL ) {
      BSKDestroyCategory( c );
    } else {
      c->next = 0;
    }
  }

  /* destroy all arrays that are marked LOCAL */

  for( a = frame->arrays; a != 0; a = an ) {
    an = a->next;
    if( a->flags == OF_LOCAL ) {
      BSKDestroyArray( a );
    } else {
      a->next = 0;
    }
  }

  /* clean up the stack -- it could be that we exited rather abruptly
   * from the routine and left some garbage there.  This could happen
   * if an "exit rule" is used within a "for x in y do" type statement */

  if( frame->stack != 0 ) {
    while( frame->stack != 0 ) {
      s_popStackItem( frame );
    }
  }

  /* deallocate the frame object itself */
     
  BSKFree( frame );
  
  /* return the "new" topmost stack frame */
   
  return env->stackFrame;
}


static BSKExecutionStackItem* s_newStackItem( BSKStackFrame* frame ) {
  BSKExecutionStackItem* i;

  i = (BSKExecutionStackItem*)BSKMalloc( sizeof( BSKExecutionStackItem ) );
  BSKMemSet( i, 0, sizeof( *i ) );
  i->next = frame->stack;
  frame->stack = i;

  return i;
}


static void s_popStackItem( BSKStackFrame* frame ) {
  BSKExecutionStackItem* i;

  i = frame->stack;
  if( i == 0 ) {
    return;
  }

  /* remove the item from the top of the stack */

  frame->stack = i->next;

  /* ALWAYS invalidate values when you are done with them */

  BSKInvalidateValue( &(i->value) );
  BSKFree( i );
}


static BSKVariable* s_findVariable( BSKStackFrame* frame, BSKUI32 id ) {
  BSKVariable* var;

  /* first, look to see if the variable exists.  If it does, return it */

  var = BSKFindVariable( frame->variables, id );
  if( var != 0 ) {
    return var;
  }

  /* if it doesn't, create it, initialize it to NULL, and add it to the
   * list of locally instantiated variables. */

  var = (BSKVariable*)BSKMalloc( sizeof( BSKVariable ) );
  var->id = id;
  BSKInitializeValue( &(var->value) );
  var->next = frame->variables;
  frame->variables = var;

  return var;
}


static BSKI32 s_addOperands( BSKExecutionEnvironment* env ) {
  BSKExecutionStackItem* top;
  BSKExecutionStackItem* next;
  BSKUI32 t1;
  BSKUI32 t2;
  BSKValue op1;
  BSKValue op2;
  BSKStackFrame* frame;
  BSKBOOL isValid;

  /* check the operand count */

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "ADD" );
    return RTE_STACK_UNDERFLOW;
  }
  
  top = frame->stack;
  next = top->next;

  /* dereference the topmost two values */

  BSKDereferenceValue( &op1, &(top->value) );
  BSKDereferenceValue( &op2, &(next->value) );

  /* pop the old values off the stack */

  s_popStackItem( frame );
  s_popStackItem( frame );
  
  /* you can only add numbers, catenate strings, or catenate
   * strings and numbers. */
     
  isValid = ( BSKValueIsType( &op1, ( VT_NUMBER | VT_STRING ) ) &&
              BSKValueIsType( &op2, ( VT_NUMBER | VT_STRING ) ) );
  if( !isValid ) {
    BSKInvalidateValue( &op1 );
    BSKInvalidateValue( &op2 );
    s_error( env, RTE_INVALID_OPERANDS, "ADD" );
    return RTE_INVALID_OPERANDS;
  }

  t1 = op1.type;
  t2 = op2.type;

  /* if either value is NULL or STRING, do string catenation */

  if( t1 == VT_NULL || t1 == VT_STRING || t2 == VT_STRING || t2 == VT_NULL ) {
    BSKCHAR* c1;
    BSKCHAR* c2;
    BSKCHAR* c3;
    BSKCHAR  b1[128];
    BSKFLOAT g;
    BSKUI32  i;

    /* string catenation */
    c1 = c2 = 0;

    if( ( t1 & VT_NUMBER ) != 0 ) {
      g = BSKEvaluateNumber( &op1 );
      BSKsprintf( b1, "%.9g", g );
      c1 = b1;
    } else if( ( t1 & VT_STRING ) != 0 ) {
      c1 = (BSKCHAR*)op1.datum;
    } else {
      c1 = "";
    }

    if( ( t2 & VT_NUMBER ) != 0 ) {
      g = BSKEvaluateNumber( &op2 );
      BSKsprintf( b1, "%.9g", g );
      c2 = b1;
    } else if( ( t2 & VT_STRING ) != 0 ) {
      c2 = (BSKCHAR*)op2.datum;
    } else {
      c2 = "";
    }

    /* allocate enough space for the new string */

    i = BSKStrLen( c1 ) + BSKStrLen( c2 ) + 1;
    c3 = (BSKCHAR*)BSKMalloc( i );
    BSKStrCpy( c3, c2 );
    BSKStrCat( c3, c1 );

    top = s_newStackItem( frame );
    BSKSetValueString( &(top->value), c3 );
    BSKFree( c3 );
  } else {
    /* otherwise, if they are both numbers, add them */

    BSKFLOAT v1;
    BSKFLOAT v2;
    BSKUI32  unit1;
    BSKUI32  unit2;

    /* get the value of the numbers */

    v1 = BSKEvaluateNumber( &op1 );
    v2 = BSKEvaluateNumber( &op2 );

    /* make sure the units are the same -- don't try to add feet and
     * seconds, for instance. */

    unit1 = BSKValueUnits( &op1 );
    unit2 = BSKValueUnits( &op2 );
    if( unit1 != unit2 ) {
      if( BSKConvertUnits( env->db->unitDef, v1, unit1, unit2, &v1 ) != 0 ) {
        BSKInvalidateValue( &op1 );
        BSKInvalidateValue( &op2 );
        s_error( env, RTE_WRONG_UNITS, "ADD" );
        return RTE_WRONG_UNITS;
      }
    }

    v1 += v2;

    top = s_newStackItem( frame );
    BSKSetValueNumberU( &(top->value), v1, unit2 );
  }

  /* ALWAYS invalidate unused values */

  BSKInvalidateValue( &op1 );
  BSKInvalidateValue( &op2 );

  return 0;
}


static BSKI32 s_doOperands( BSKExecutionEnvironment* env, BSKUI32 op ) {
  BSKStackFrame* frame;
  BSKExecutionStackItem* top;
  BSKExecutionStackItem* next;
  BSKFLOAT g1;
  BSKFLOAT g2;
  BSKUI32  unit1;
  BSKUI32  unit2;
  BSKValue op1;
  BSKValue op2;
  BSKCHAR* buffer;

  /* for error messages, get the name of the operation being performed */

  switch( op ) {
    case OP_SUB: buffer = "SUB"; break;
    case OP_MUL: buffer = "MUL"; break;
    case OP_DIV: buffer = "DIV"; break;
    case OP_MOD: buffer = "MOD"; break;
    case OP_POW: buffer = "POW"; break;
    default: buffer = "unknown instruction"; break;
  }

  /* make sure we've got enough parameters */

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, buffer );
    return RTE_STACK_UNDERFLOW;
  }
  
  /* get the values to operate on, and make sure they are numbers */

  top = frame->stack;
  next = top->next;

  BSKDereferenceValue( &op1, &(top->value) );
  BSKDereferenceValue( &op2, &(next->value ) );
  if( !BSKValueIsType( &op1, VT_NUMBER ) || !BSKValueIsType( &op2, VT_NUMBER ) ) {
    BSKInvalidateValue( &op1 );
    BSKInvalidateValue( &op2 );
    s_error( env, RTE_INVALID_OPERANDS, buffer );
    return RTE_INVALID_OPERANDS;
  }

  /* get the numbers from the values */

  g1 = BSKEvaluateNumber( &op2 );
  g2 = BSKEvaluateNumber( &op1 );

  unit1 = unit2 = 0;
  
  /* convert the two numbers to the same units for comparison, if possible 
   * MUL, DIV, MOD, and POW only require that if units are used, one of the
   * numbers may be unitless. (i.e., 3 seconds * 2 = 6 seconds) */
  
  unit1 = BSKValueUnits( &op2 );
  unit2 = BSKValueUnits( &op1 );

  if( ( op == OP_MUL ) || ( op == OP_DIV ) || ( op == OP_MOD ) || ( op == OP_POW ) ) {
    if( ( unit1 == 0 ) || ( unit2 == 0 ) ) {
      if( unit1 != 0 ) {
        unit2 = unit1;
      } else {
        unit1 = unit2;
      }
    }
  }

  /* if the units differ and cannot be converted to a common unit, then
   * it is an error. */

  if( unit1 != unit2 ) {
    if( BSKConvertUnits( env->db->unitDef, g2, unit2, unit1, &g2 ) != 0 ) {
      BSKInvalidateValue( &op1 );
      BSKInvalidateValue( &op2 );
      s_error( env, RTE_WRONG_UNITS, buffer );
      return RTE_WRONG_UNITS;
    }
  }

  /* do the operation */

  switch( op ) {
    case OP_SUB: g1 -= g2; break;
    case OP_MUL: g1 *= g2; break;
    case OP_DIV: 
      if( BSKFAbs(g2) < VC_EPSILON ) {
        s_error( env, RTE_DIVIDE_BY_ZERO, buffer );
        return RTE_DIVIDE_BY_ZERO;
      }
      g1 /= g2; 
      break;
    case OP_MOD:
      if( BSKFAbs(g2) < VC_EPSILON ) {
        s_error( env, RTE_DIVIDE_BY_ZERO, buffer );
        return RTE_DIVIDE_BY_ZERO;
      }
      g1 = (BSKFLOAT)BSKFMod( g1, g2 );
      if( BSKFAbs(g1) < VC_EPSILON ) {
        g1 = 0;
      }
      break;
    case OP_POW:
      g1 = (BSKFLOAT)BSKPow( g1, g2 );
      break;
    default:
      s_error( env, RTE_UNKNOWN_INSTRUCTION, buffer );
      return RTE_UNKNOWN_INSTRUCTION;
  }

  s_popStackItem( frame );
  s_popStackItem( frame );

  top = s_newStackItem( frame );
  BSKSetValueNumberU( &(top->value), g1, unit1 );

  /* invalidate the values */

  BSKInvalidateValue( &op1 );
  BSKInvalidateValue( &op2 );

  return 0;
}


static BSKI32 s_doBooleanComparison( BSKExecutionEnvironment* env, BSKUI32 op ) {
  BSKValue first;
  BSKValue second;
  BSKExecutionStackItem* top;
  BSKUI32 i;
  BSKI32 rc;
  BSKBOOL result;
  BSKStackFrame* frame;
  BSKCHAR* buffer;

  /* for error messages, get the name of the operation being performed */

  switch( op ) {
    case OP_EQ: buffer = "EQ"; break;
    case OP_NE: buffer = "NE"; break;
    case OP_LT: buffer = "LT"; break;
    case OP_GT: buffer = "GT"; break;
    case OP_LE: buffer = "LE"; break;
    case OP_GE: buffer = "GE"; break;
    default: buffer = "unknown instruction"; break;
  }

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, buffer );
    return RTE_STACK_UNDERFLOW;
  }

  BSKDereferenceValue( &second, &(frame->stack->value) );
  BSKDereferenceValue( &first, &(frame->stack->next->value) );

  s_popStackItem( frame );
  s_popStackItem( frame );

  top = s_newStackItem( frame );

  /* if one of the values is NULL, only EQ and NE work, everything else
   * (except typeof) is FALSE. */

  if( op != OP_TYPEOF && ( first.type == VT_NULL || second.type == VT_NULL ) ) {
    switch( op ) {                                  
      case OP_EQ: 
        result = ( first.type == second.type );
        break;
      case OP_NE:
        result = ( first.type != second.type );
        break;
      default:
        result = BSKFALSE;
    }
    BSKSetValueNumber( &(top->value), result );
    BSKInvalidateValue( &first );
    BSKInvalidateValue( &second );
    return 0;
  } else if( op != OP_TYPEOF && !s_compareable( &first, &second ) ) {
    BSKInvalidateValue( &first );
    BSKInvalidateValue( &second );
    BSKSetValueNumber( &(top->value), BSKFALSE );
    return 0;
  } else if( op == OP_TYPEOF && second.type != VT_BYTE ) {
    BSKInvalidateValue( &first );
    BSKInvalidateValue( &second );
    BSKSetValueNumber( &(top->value), BSKFALSE );
    return 0;
  }

  /* default to FALSE */

  result = BSKFALSE;
  rc = RTE_SUCCESS;

  if( op == OP_TYPEOF ) {
    i = BSKEvaluateNumber( &second );
    switch( i ) {
      case TID_CATEGORY: result = BSKValueIsType( &first, VT_CATEGORY ); break;
      case TID_THING: result = BSKValueIsType( &first, VT_THING ); break;
      case TID_RULE: result = BSKValueIsType( &first, VT_RULE ); break;
      case TID_NUMBER: result = BSKValueIsType( &first, VT_NUMBER ); break;
      case TID_STRING: result = BSKValueIsType( &first, VT_STRING ); break;
      case TID_BOOLEAN: result = BSKValueIsType( &first, VT_BOOLEAN ); break;
      case TID_ARRAY: result = BSKValueIsType( &first, VT_ARRAY ); break;
    }
  } else if( BSKValueIsType( &first, VT_NUMBER ) ) {
    BSKFLOAT g1;
    BSKFLOAT g2;
    BSKUI32  unit1;
    BSKUI32  unit2;

    g1 = BSKEvaluateNumber( &first );
    g2 = BSKEvaluateNumber( &second );

    /* convert the two numbers to the same units for comparison */
    unit1 = BSKValueUnits( &first );
    unit2 = BSKValueUnits( &second );

    if( unit1 != unit2 ) {
      if( BSKConvertUnits( env->db->unitDef, g2, unit2, unit1, &g2 ) != 0 ) {
        BSKSetValueNumber( &(top->value), BSKFALSE );
        BSKInvalidateValue( &first );
        BSKInvalidateValue( &second );
        return 0;
      }
    }

    switch( op ) {
      case OP_EQ: result = ( BSKFAbs(g1-g2) <= VC_EPSILON ); break;
      case OP_NE: result = ( BSKFAbs(g1-g2) > VC_EPSILON ); break;
      case OP_LT: result = ( g1 - g2 < 0 ); break;
      case OP_GT: result = ( g1 - g2 > 0 ); break;
      case OP_GE: result = ( g1 - g2 >= 0 ); break;
      case OP_LE: result = ( g1 - g2 <= 0 ); break;
    }
  } else if( BSKValueIsType( &first, VT_STRING ) ) {
    BSKI32 comp;

    comp = BSKStrCmp( (BSKCHAR*)first.datum, (BSKCHAR*)second.datum );
    switch( op ) {
      case OP_EQ: result = ( comp == 0 ); break;
      case OP_NE: result = ( comp != 0 ); break;
      case OP_LT: result = ( comp < 0 ); break;
      case OP_GT: result = ( comp > 0 ); break;
      case OP_GE: result = ( comp >= 0 ); break;
      case OP_LE: result = ( comp <= 0 ); break;
    }
  } else {
    if( ( op != OP_EQ ) && ( op != OP_NE ) ) {
      rc = RTE_INVALID_OPERANDS;
    } else {
      result = ( first.datum == second.datum );
      if( op == OP_NE ) {
        result = !result;
      }
    }
  }

  /* always invalidate values */

  BSKInvalidateValue( &first );
  BSKInvalidateValue( &second );
  BSKSetValueNumber( &(top->value), result );

  return rc;
}


static BSKBOOL s_ensureOpCount( BSKStackFrame* frame, BSKUI32 count ) {
  BSKExecutionStackItem* i;

  /* make sure we have at least the requested number of items on the
   * stack. */

  i = frame->stack;
  while( count > 0 ) {
    if( i == 0 ) {
      return BSKFALSE;
    }
    i = i->next;
    count--;
  }

  return BSKTRUE;
}


static BSKBOOL s_compareable( BSKValue* v1, BSKValue* v2 ) {
  if( BSKValueIsType( v1, VT_NUMBER ) && BSKValueIsType( v2, VT_NUMBER ) ) {
    return BSKTRUE;
  }

  if( v1->type == v2->type ) {
    return BSKTRUE;
  }

  return BSKFALSE;
}


static BSKUI32 s_ultimateTypeOf( BSKExecutionStackItem* i ) {
  BSKVariable* var = 0;
  BSKValue* val = 0;

  switch( i->value.type ) {
    case VT_VARIABLE:
    case VT_VALUE:
    case VT_ATTRIBUTE:
      if( i->value.type == VT_VARIABLE ) {
        var = (BSKVariable*)i->value.datum;
        val = &(var->value);
      } else if( i->value.type == VT_VALUE ) {
        val = (BSKValue*)i->value.datum;
      } else if( i->value.type == VT_ATTRIBUTE ) {
        val = &(((BSKAttribute*)i->value.datum)->value);
      }
      return val->type;
  }

  return i->value.type;
}


static void s_evaluateValue( BSKValue* value, BSKNOTYPE ptr ) {
  if( BSKValueIsType( value, VT_NULL ) ) {
    *((BSKCHAR**)ptr) = 0;
  } else if( BSKValueIsType( value, VT_THING ) ) {
    *((BSKThing**)ptr) = (BSKThing*)value->datum;
  } else if( BSKValueIsType( value, VT_CATEGORY ) ) {
    *((BSKCategory**)ptr) = (BSKCategory*)value->datum;
  } else if( BSKValueIsType( value, VT_CATEGORY_ENTRY ) ) {
    *((BSKCategoryEntry**)ptr) = (BSKCategoryEntry*)value->datum;
  } else if( BSKValueIsType( value, VT_RULE ) ) {
    *((BSKRule**)ptr) = (BSKRule*)value->datum;
  } else if( BSKValueIsType( value, VT_STRING ) ) {
    *((BSKCHAR**)ptr) = (BSKCHAR*)value->datum;
  } else if( BSKValueIsType( value, VT_NUMBER ) ) {
    *(BSKFLOAT*)ptr = BSKEvaluateNumber( value );
  } else if( BSKValueIsType( value, VT_VALUE ) ) {
    s_evaluateValue( (BSKValue*)(value->datum), ptr );
  } else if( BSKValueIsType( value, VT_VARIABLE ) ) {
    s_evaluateValue( &(((BSKVariable*)(value->datum))->value), ptr );
  } else if( BSKValueIsType( value, VT_ATTRIBUTE ) ) {
    s_evaluateValue( &(((BSKAttribute*)(value->datum))->value), ptr );
  }
}


static BSKI32 s_callSub( BSKExecutionEnvironment *env,
                         BSKUI32 parmCount )
{
  BSKValue retVal;
  BSKValue** parmList;
  BSKI32 i;
  BSKI32 rc;
  BSKRule* rule;
  BSKUI32  id;
  BSKUI32  type;
  BSKFLOAT g;

  if( !s_ensureOpCount( env->stackFrame, parmCount+1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "too few parameters to CALL [potential BUG]" );
    return RTE_STACK_UNDERFLOW;
  }

  rule = 0;
  id = 0;

  /* dereference the requested number of items on the stack into the
   * parameters array. */

  if( parmCount > 0 ) {
    parmList = (BSKValue**)BSKMalloc( parmCount * sizeof( BSKValue* ) );
    for( i = parmCount-1; i >= 0; i-- ) {
      parmList[i] = (BSKValue*)BSKMalloc( sizeof( BSKValue ) );
      BSKDereferenceValue( parmList[i], &(env->stackFrame->stack->value) );
      s_popStackItem( env->stackFrame );
    }
  } else {
    parmList = 0;
  }

  /* initialize the return value */

  BSKInitializeValue( &retVal );

  /* the function to call must either be a RULE or a BUILTIN (numeric) */

  type = s_ultimateTypeOf( env->stackFrame->stack );
  if( (type & ( VT_RULE | VT_INTEGER ) ) == 0 ) {
    for( i = 0; i < parmCount; i++ ) {
      BSKInvalidateValue( parmList[i] );
      BSKFree( parmList[i] );
    }
    BSKFree( parmList );
    s_error( env, RTE_CALL_OF_NONFUNCTION, 0 );
    return RTE_CALL_OF_NONFUNCTION;
  }

  /* get the rule or built-in to execute */

  switch( type ) {
    case VT_RULE:
      s_evaluateValue( &(env->stackFrame->stack->value), &rule );
      break;
    default:
      s_evaluateValue( &(env->stackFrame->stack->value), &g );
      id = g;
      break;
  }

  /* pop the rule/builtin to call off of the stack */

  s_popStackItem( env->stackFrame );

  if( rule != 0 ) {
    BSKUI32 oldLine;

    /* execute a rule -- create the new stack frame and populate it with
     * the parameters, return value, and currently executing rule */

    s_newStackFrame( env );

    env->stackFrame->parameterCount = parmCount;
    env->stackFrame->parameters = parmList;
    env->stackFrame->retVal = &retVal;
    env->stackFrame->ruleId = rule->id;

    /* for each parameter passed to the rule, associate it with a named
     * parameter of the rule.  If more parameters were passed to the rule
     * than the rule has named parameters, only name as many parameters
     * as the rule has. */

    rc = ( parmCount < rule->parameterCount ? parmCount : rule->parameterCount );
    for( i = 0; i < rc; i++ ) {
      BSKVariable* v;
      v = s_findVariable( env->stackFrame, rule->parameters[i] );
      BSKCopyValue( &(v->value), parmList[i] );
    }

    /* save the currently executing line number */

    oldLine = env->line;

    /* execute the requested rule */

    rc = s_execCode( env, rule );

    /* convert all LOCAL values in the parameters to TRANSIENT, so they
     * won't be destroyed when the stack frame dies */

    if( parmCount > 0 ) {
      for( i = 0; i < parmCount; i++ ) {
        s_convertObjectFlag( parmList[ i ], OF_LOCAL, OF_TRANSIENT );
      }
    }

    /* kill the stack frame */

    s_popStackFrame( env );

    /* restore the currently executing line */

    env->line = oldLine;

    /* collect all transient values from the parameters into the current
     * stack frame, and invalidate and free the parameters. */

    if( parmCount > 0 ) {
      for( i = 0; i < parmCount; i++ ) {
        s_collectTransientObjects( env->stackFrame, parmList[ i ] );
        BSKInvalidateValue( parmList[ i ] );
        BSKFree( parmList[ i ] );
      }
      BSKFree( parmList );
    }

    /* collect all transient values from the return value into the current
     * stack frame. */

    s_collectTransientObjects( env->stackFrame, &retVal );

  } else {
    /* execute a built-in by passing the id of the requested built-in to
     * s_execBuiltin */

    rc = s_execBuiltin( env, id, parmCount, parmList, &retVal );

    /* invalidate and free the parameter list */

    if( parmCount > 0 ) {
      for( i = 0; i < parmCount; i++ ) {
        BSKInvalidateValue( parmList[i] );
        BSKFree( parmList[i] );
      }
      BSKFree( parmList );
    }
  }

  /* push the return code onto the stack */

  s_newStackItem( env->stackFrame );
  BSKCopyValue( &(env->stackFrame->stack->value), &retVal );
  BSKInvalidateValue( &retVal );

  return rc;
}


static BSKI32 s_execBuiltin( BSKExecutionEnvironment* env,
                             BSKUI32 funcId,
                             BSKUI32 parmCount,
                             BSKValue** parameters,
                             BSKValue* retVal )
{
  BSKCHAR buffer[ 128 ];
  BSKFLOAT val;
  BSKCategoryMember* catMember;
  BSKCHAR* ptr;
  BSKCHAR* ptr2;

  /* get the string identifier of the built-in to execute and test to
   * see which built-in it is. */

  BSKGetIdentifier( env->db->idTable, funcId, buffer, sizeof( buffer ) );
  BSKInitializeValue( retVal );

  /* Int(x) -- rounds the  number to the nearest whole number */
  if( BSKStrCaseCmp( buffer, "int" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Int" );
    val = (BSKFLOAT)BSKFloor( BSKEvaluateNumber( parameters[0] ) + 0.5 );
    BSKSetValueNumberU( retVal, val, BSKValueUnits( parameters[0] ) );

  /* Floor(x) -- truncates the number to the nearest whole number y
   * such that x >= y */
  } else if( BSKStrCaseCmp( buffer, "floor" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Floor" );
    val = (BSKFLOAT)BSKFloor( BSKEvaluateNumber( parameters[0] ) );
    BSKSetValueNumberU( retVal, val, BSKValueUnits( parameters[0] ) );

  /* Sqrt(x) -- returns the square root of x */
  } else if( BSKStrCaseCmp( buffer, "sqrt" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Sqrt" );
    val = BSKEvaluateNumber( parameters[0] );
    if( val < 0 ) {
      return RTE_DOMAIN_ERROR;
    }
    BSKSetValueNumberU( retVal, (BSKFLOAT)BSKSqrt(val), BSKValueUnits( parameters[0] ) );

  /* ln(x) -- returns the natural log of x */
  } else if( BSKStrCaseCmp( buffer, "ln" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Ln" );
    val = BSKEvaluateNumber( parameters[0] );
    if( val < 0 ) {
      return RTE_DOMAIN_ERROR;
    }
    BSKSetValueNumberU( retVal, (BSKFLOAT)BSKLog(val), BSKValueUnits( parameters[0] ) );

  /* Any(x) -- randomly returns any member of the given category, taking
   * into account individual member weights */
  } else if( BSKStrCaseCmp( buffer, "any" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Any" );
    catMember = BSKGetAnyCategoryMember( (BSKCategory*)parameters[0]->datum );
    if( catMember == 0 ) {
      retVal->type = VT_NULL;
    } else {
      switch( catMember->reserved ) {
        case OT_THING:
          retVal->type = VT_THING;
          retVal->datum = (BSKThing*)catMember;
          break;
        case OT_CATEGORY:
          retVal->type = VT_CATEGORY;
          retVal->datum = (BSKCategory*)catMember;
          break;
        case OT_RULE:
          retVal->type = VT_RULE;
          retVal->datum = (BSKRule*)catMember;
          break;
      }
    }

  /* Exists(x,y) -- returns true if category x contains member y */
  } else if( BSKStrCaseCmp( buffer, "exists" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Exists" );
    ENFORCETYPE( (parameters[1]->type), ( VT_THING | VT_CATEGORY | VT_RULE ), "2", "Exists" );
    val = BSKCategoryContains( (BSKCategory*)parameters[0]->datum,
                               (BSKCategoryMember*)parameters[1]->datum );
    BSKSetValueNumber( retVal, val );

  /* NewThing() -- creates a new thing and adds it to the list of locally
   * instantiated things of the current stack frame */
  } else if( BSKStrCaseCmp( buffer, "newthing" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 0 );
    retVal->type = VT_THING;
    retVal->datum = s_newThing( env, 0 );

  /* NewCategory() -- creates a new category and adds it to the list of 
   * locally instantiated categories of the current stack frame */
  } else if( BSKStrCaseCmp( buffer, "newcategory" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 0 );
    retVal->type = VT_CATEGORY;
    retVal->datum = s_newCategory( env, 0 );

  /* Duplicate(x) -- creates a "shallow" copy of x, which is a category or
   * a thing. */
  } else if( BSKStrCaseCmp( buffer, "duplicate" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), ( VT_THING | VT_CATEGORY ), "1", "Duplicate" );
    retVal->type = parameters[0]->type;
    switch( retVal->type ) {
      case VT_THING:
        retVal->datum = s_newThing( env, (BSKThing*)parameters[0]->datum );
        break;
      case VT_CATEGORY:
        retVal->datum = s_newCategory( env, (BSKCategory*)parameters[0]->datum );
        break;
    }

  /* Add(x,y{,z}) -- Adds object y to category x, and if z is specified,
   * uses its value as the weight of the new member. The object added is
   * returned. */
  } else if( BSKStrCaseCmp( buffer, "add" ) == 0 ) {
    CHECKPARMCNTIS2( parmCount, 2, 3 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Add" );
    if( parameters[1]->type != VT_NULL ) {
      ENFORCETYPE( (parameters[1]->type), ( VT_THING | VT_CATEGORY | VT_RULE ), "2", "Add" );
    }
    if( parmCount > 2 ) {
      ENFORCETYPE( (parameters[2]->type), VT_NUMBER, "3", "Add" );
      val = BSKEvaluateNumber( parameters[2] );
    } else {
      val = 1;
    }
    retVal->type = parameters[1]->type;
    retVal->datum = parameters[1]->datum;
    BSKAddToCategory( (BSKCategory*)parameters[0]->datum,
                      (BSKUI16)val,
                      (BSKCategoryMember*)parameters[1]->datum );

  /* Remove(x,y) -- Removes object y from category x. */
  } else if( BSKStrCaseCmp( buffer, "remove" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Remove" );
    if( parameters[1]->type != VT_NULL ) {
      ENFORCETYPE( (parameters[1]->type), ( VT_THING | VT_CATEGORY | VT_RULE ), "2", "Remove" );
    }
    BSKRemoveFromCategory( (BSKCategory*)parameters[0]->datum,
                           (BSKCategoryMember*)parameters[1]->datum ); 
    BSKSetValueNumber( retVal, BSKTRUE );

  /* RemoveAll(x) -- Removes all objects from category x. */
  } else if( BSKStrCaseCmp( buffer, "removeall" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Removeall" );
    BSKRemoveAllFromCategory( (BSKCategory*)parameters[0]->datum ); 
    BSKSetValueNumber( retVal, BSKTRUE );

  /* Count(x) -- Returns the number of items in category x. */
  } else if( BSKStrCaseCmp( buffer, "count" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Count" );
    val = BSKCategoryMemberCount( (BSKCategory*)parameters[0]->datum );
    BSKSetValueNumber( retVal, val );

  /* Get(x) -- Returns the number of items in category x. */
  } else if( BSKStrCaseCmp( buffer, "get" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Get" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "Get" );
    val = BSKEvaluateNumber( parameters[1] );
    catMember = BSKGetMemberByIndex( (BSKCategory*)parameters[0]->datum, 
                                     (BSKUI32)val );
    if( catMember == 0 ) {
      retVal->type = VT_NULL;
    } else {
      switch( catMember->reserved ) {
        case OT_THING:
          retVal->type = VT_THING;
          retVal->datum = (BSKThing*)catMember;
          break;
        case OT_CATEGORY:
          retVal->type = VT_CATEGORY;
          retVal->datum = (BSKCategory*)catMember;
          break;
        case OT_RULE:
          retVal->type = VT_RULE;
          retVal->datum = (BSKRule*)catMember;
          break;
      }
    }

  /* Empty(x) -- Returns true if category x is empty. */
  } else if( BSKStrCaseCmp( buffer, "empty" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Empty" );
    val = (BSKFLOAT)( ((BSKCategory*)parameters[0]->datum)->totalWeight < 1 );
    BSKSetValueNumber( retVal, val );
                       
  /* Instr(x,y{,z}) -- Returns the first index (after z) at which string
   * y is found in string x.  Returns -1 if the string is not found in x */
  } else if( BSKStrCaseCmp( buffer, "instr" ) == 0 ) {
    CHECKPARMCNTIS2( parmCount, 2, 3 );
    ENFORCETYPE( (parameters[0]->type), VT_STRING, "1", "Instr" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "Instr" );
    if( parmCount > 2 ) {
      ENFORCETYPE( (parameters[2]->type), VT_NUMBER, "3", "Instr" );
      val = BSKEvaluateNumber( parameters[2] );
    } else {
      val = 0;
    }

    ptr = BSKStrStr( &((BSKCHAR*)parameters[0]->datum)[(BSKUI32)val], 
                      (BSKCHAR*)parameters[1]->datum );

    if( ptr == 0 ) {
      val = -1;
    } else {
      val = (BSKFLOAT)( ptr - (BSKCHAR*)parameters[0]->datum );
    }
    BSKSetValueNumber( retVal, val );

  /* Replace(x,y,z{,w}) -- Replaces the first instance of y in x (after w)
   * with z and returns the new string */
  } else if( BSKStrCaseCmp( buffer, "replace" ) == 0 ) {
    BSKUI32 len;
    BSKUI32 primaryLen;
        
    CHECKPARMCNTIS2( parmCount, 3, 4 );
    ENFORCETYPE( (parameters[0]->type), VT_STRING, "1", "Replace" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "Replace" );
    ENFORCETYPE( (parameters[2]->type), VT_STRING, "3", "Replace" );
    if( parmCount > 3 ) {
      ENFORCETYPE( (parameters[3]->type), VT_NUMBER, "4", "Replace" );
      val = BSKEvaluateNumber( parameters[3] );
    } else {
      val = 0;
    }
    
    primaryLen = BSKStrLen( (BSKCHAR*)parameters[0]->datum );
    len = primaryLen -
          BSKStrLen( (BSKCHAR*)parameters[1]->datum ) +
          BSKStrLen( (BSKCHAR*)parameters[2]->datum ) + 1;
    if( len <= primaryLen ) {
      len = primaryLen + 1;
    }

    ptr2 = (BSKCHAR*)BSKMalloc( len );
    BSKStrCpy( ptr2, (BSKCHAR*)parameters[0]->datum );
    ptr = BSKStringReplace( ptr2,
                            (BSKCHAR*)parameters[1]->datum,
                            (BSKCHAR*)parameters[2]->datum,
                            val );

    BSKSetValueString( retVal, ptr2 );
    BSKFree( ptr2 );

  /* Mid(x,y{,z}) -- Returns z bytes (or to the end of the string) of
   * x starting at y */
  } else if( BSKStrCaseCmp( buffer, "mid" ) == 0 ) {
    BSKUI32 from;
    BSKUI32 length;
        
    CHECKPARMCNTIS2( parmCount, 2, 3 );
    ENFORCETYPE( (parameters[0]->type), VT_STRING, "1", "Mid" );
    ptr = (BSKCHAR*)parameters[0]->datum;
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "Mid" );
    from = BSKEvaluateNumber( parameters[1] );
    if( parmCount > 2 ) {
      ENFORCETYPE( (parameters[2]->type), VT_NUMBER, "3", "Mid" );
      length = BSKEvaluateNumber( parameters[2] );
    } else {
      length = BSKStrLen( ptr ) - from;
    }

    if( length <= 0 ) {
      BSKSetValueString( retVal, "" );
    } else {
      ptr2 = (BSKCHAR*)BSKMalloc( length + 1 );
      BSKMemCpy( ptr2, &(ptr[from]), length );
      ptr2[length] = 0;
      BSKSetValueString( retVal, ptr2 );
      BSKFree( ptr2 );
    }

  /* Dice(x,y) -- creates a new dice number, x of y-sided dice. */
  } else if( BSKStrCaseCmp( buffer, "dice" ) == 0 ) {
    BSKI16 count;
    BSKUI16 type;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Dice" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "Dice" );

    count = (BSKI16)BSKEvaluateNumber( parameters[0] );
    type = (BSKUI16)BSKEvaluateNumber( parameters[1] );

    BSKSetValueDice( retVal, count, type, 0, 0 );

  /* Random(x) -- If x is a dice value, the dice value is evaluated and
   * returned.  Otherwise, a random integer from 0 to x-1 is returned. */
  } else if( BSKStrCaseCmp( buffer, "random" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Random" );

    if( parameters[0]->type == VT_DICE ) {
      val = BSKEvaluateNumber( parameters[0] );
      BSKSetValueNumberU( retVal, val, BSKValueUnits( parameters[0] ) );
    } else {
      val = BSKFAbs( BSKEvaluateNumber( parameters[0] ) );
      if( val < VC_EPSILON ) {
        BSKSetValueNumberU( retVal, 0, BSKValueUnits( parameters[0] ) );
      } else {
        if( val < 1 ) {
          val = 1;
        }
        BSKSetValueNumberU( retVal, ( BSKRand() % (BSKUI32)val ), BSKValueUnits( parameters[0] ) );
      }
    }

  /* Union(x,y) -- returns the union of the two categories */
  } else if( BSKStrCaseCmp( buffer, "union" ) == 0 ) {
    BSKCategory* c;

    CHECKPARMCNTIS( parmCount, 2 );
    if( parameters[0]->type != VT_NULL ) {
      ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Union" );
    }
    if( parameters[1]->type != VT_NULL ) {
      ENFORCETYPE( (parameters[1]->type), VT_CATEGORY, "2", "Union" );
    }

    c = BSKCategoryUnion( (BSKCategory*)parameters[0]->datum,
                          (BSKCategory*)parameters[1]->datum );
    c->ownerLevel = env->stackFrame->level;
    s_addCategoryToFrame( env->stackFrame, c );

    retVal->type = VT_CATEGORY;
    retVal->datum = c;

  /* Intersection(x,y) -- returns the intersection of the two categories */
  } else if( BSKStrCaseCmp( buffer, "intersection" ) == 0 ) {
    BSKCategory* c;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Intersection" );
    ENFORCETYPE( (parameters[1]->type), VT_CATEGORY, "2", "Intersection" );

    c = BSKCategoryIntersection( (BSKCategory*)parameters[0]->datum,
                                 (BSKCategory*)parameters[1]->datum );
    c->ownerLevel = env->stackFrame->level;
    s_addCategoryToFrame( env->stackFrame, c );

    retVal->type = VT_CATEGORY;
    retVal->datum = c;

  /* Subtract(x,y) -- returns the difference of the two categories */
  } else if( BSKStrCaseCmp( buffer, "subtract" ) == 0 ) {
    BSKCategory* c;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "Subtract" );
    ENFORCETYPE( (parameters[1]->type), VT_CATEGORY, "2", "Subtract" );

    c = BSKCategorySubtract( (BSKCategory*)parameters[0]->datum,
                             (BSKCategory*)parameters[1]->datum );
    c->ownerLevel = env->stackFrame->level;
    s_addCategoryToFrame( env->stackFrame, c );

    retVal->type = VT_CATEGORY;
    retVal->datum = c;

  /* WeightOf(x,y) -- returns the weight of the member at index y of 
   * category x. */
  } else if( BSKStrCaseCmp( buffer, "weightof" ) == 0 ) {
    BSKCategoryEntry* entry;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "WeightOf" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "WeightOf" );

    val = BSKEvaluateNumber( parameters[1] );
    entry = BSKGetEntryByIndex( (BSKCategory*)parameters[0]->datum, (BSKUI32)val );

    if( entry == 0 ) {
      BSKSetValueNumber( retVal, 0 );
    } else {
      BSKSetValueNumber( retVal, entry->weight );
    }

  /* IndexOf(x,y) -- returns the index of y in x. */
  } else if( BSKStrCaseCmp( buffer, "indexof" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "IndexOf" );
    if( parameters[1]->type != VT_NULL ) {
      ENFORCETYPE( (parameters[1]->type), ( VT_THING | VT_RULE | VT_CATEGORY ), "2", "IndexOf" );
    }
    val = BSKGetMemberIndex( (BSKCategory*)parameters[0]->datum,
                             (BSKCategoryMember*)parameters[1]->datum );
    BSKSetValueNumber( retVal, val );

  /* Has(x,y) -- returns true if the given thing has an attribute named
   * y. */
  } else if( BSKStrCaseCmp( buffer, "has" ) == 0 ) {
    BSKUI32 id;
    BSKSymbolTableEntry* sym;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_THING, "1", "Has" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "Has" );

    id = BSKFindIdentifier( env->db->idTable, (BSKCHAR*)parameters[1]->datum );
    sym = BSKGetSymbol( env->db->symbols, id );
    if( sym == 0 ) {
      return RTE_INVALID_OPERANDS;
    } else if( sym->type != ST_ATTRIBUTE ) {
      return RTE_INVALID_OPERANDS;
    }

    id = ( BSKGetAttributeOf( (BSKThing*)parameters[0]->datum, id ) != 0 );
    BSKSetValueNumber( retVal, id );

  /* Parameter(x) -- returns true if the given thing has an attribute named
   * y. */
  } else if( BSKStrCaseCmp( buffer, "parameter" ) == 0 ) {
    BSKI32 pCnt;
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "Parameter" );
    pCnt = (BSKI32)BSKEvaluateNumber( parameters[0] );

    if( ( pCnt < 0 ) || ( pCnt >= env->stackFrame->parameterCount ) ) {
      BSKInitializeValue( retVal );
      return RTE_SUCCESS;
    }

    BSKCopyValue( retVal, env->stackFrame->parameters[pCnt] );

  /* SearchCategory(x,y{,z}) -- returns the first thing of category x that
   * has an attribute named y (with value z). Returns null if nothing is
   * found. */
  } else if( BSKStrCaseCmp( buffer, "searchcategory" ) == 0 ) {
    BSKValue* value = 0;
    BSKUI32 id;
    BSKSymbolTableEntry* sym;
    
    CHECKPARMCNTIS2( parmCount, 2, 3 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "SearchCategory" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "SearchCategory" );
    if( parmCount > 2 ) {
      value = parameters[2];
    }

    id = BSKFindIdentifier( env->db->idTable, (BSKCHAR*)parameters[1]->datum );
    sym = BSKGetSymbol( env->db->symbols, id );
    if( sym == 0 ) {
      s_error( env, RTE_INVALID_OPERANDS, "SearchCategory [not a symbol]" );
      return RTE_INVALID_OPERANDS;
    } else if( sym->type != ST_ATTRIBUTE ) {
      s_error( env, RTE_INVALID_OPERANDS, "SearchCategory [not an attribute]" );
      return RTE_INVALID_OPERANDS;
    }

    retVal->type = VT_THING;
    retVal->datum = BSKFindThingInCategory( (BSKCategory*)parameters[0]->datum,
                                            id, value );

    if( retVal->datum == 0 ) {
      retVal->type = VT_NULL;
    }

  /* AttributeNameOf(x,y) -- returns the name of the attribute at index y
   * in thing x. */
  } else if( BSKStrCaseCmp( buffer, "attributenameof" ) == 0 ) {
    BSKUI32 idx;
    BSKAttribute* attribute;
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_THING, "1", "AttributeNameOf" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "AttributeNameOf" );
    idx = (BSKUI32)BSKEvaluateNumber( parameters[1] );

    attribute = BSKGetAttributeAt( (BSKThing*)parameters[0]->datum, idx );
    if( attribute == 0 ) {
      retVal->type = VT_NULL;
    } else {
      if( attribute->id > 0 ) {
        BSKUI32 len;
        len = BSKGetIdentifierLength( env->db->idTable, attribute->id ) + 1;
        ptr = BSKMalloc( len );
        BSKGetIdentifier( env->db->idTable, attribute->id, ptr, len );
        BSKSetValueString( retVal, ptr );
        BSKFree( ptr );
      } else {
        BSKSetValueString( retVal, "" );
      }
    }

  /* AttributeValueOf(x,y) -- returns the value of the attribute at index y
   * in thing x. */
  } else if( BSKStrCaseCmp( buffer, "attributevalueof" ) == 0 ) {
    BSKUI32 idx;
    BSKAttribute* attribute;
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_THING, "1", "AttributeValueOf" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "AttributeValueOf" );
    idx = (BSKUI32)BSKEvaluateNumber( parameters[1] );

    attribute = BSKGetAttributeAt( (BSKThing*)parameters[0]->datum, idx );
    if( attribute == 0 ) {
      retVal->type = VT_NULL;
    } else {
      BSKCopyValue( retVal, &(attribute->value) );
    }

  /* AttributeOf(x,y) -- returns the actual ATTRIBUTE object of the first
   * attribute named y of thing x. */
  } else if( BSKStrCaseCmp( buffer, "attributeof" ) == 0 ) {
    BSKUI32 id;
    BSKAttribute* attribute;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_THING, "1", "AttributeOf" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "AttributeOf" );

    id = BSKFindIdentifier( env->db->idTable, (BSKCHAR*)parameters[1]->datum );
    if( id < 1 ) {
      s_error( env, RTE_INVALID_OPERANDS, "parameter 2 must be a valid attribute" );
      return RTE_INVALID_OPERANDS;
    }

    attribute = BSKGetAttributeOf( (BSKThing*)parameters[0]->datum, id );
    if( attribute == 0 ) {
      attribute = s_initializeNewAttribute( env,
                                            (BSKThing*)parameters[0]->datum,
                                            id );
    }

    retVal->type = VT_ATTRIBUTE;
    retVal->datum = attribute;

  /* Print(x) -- prints the string or number value of the parameter. */
  } else if( BSKStrCaseCmp( buffer, "print" ) == 0 ) {
    BSKCHAR* c;
    BSKCHAR  buf[128];
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), ( VT_NUMBER | VT_STRING | VT_NULL ), "1", "Print" );

    if( BSKValueIsType( parameters[0], VT_NUMBER ) ) {
      BSKFLOAT f;
      f = BSKEvaluateNumber( parameters[0] );
      BSKsprintf( buf, "%.9g", f );
      c = buf;
    } else if( BSKValueIsType( parameters[0], VT_STRING ) ) {
      c = (BSKCHAR*)parameters[0]->datum;
    } else {
      c = "[null]";
    }

    env->console( c, env, env->userData );
    retVal->type = VT_NULL;

  /* NewArray({x}) -- creates a new array object (initialized to x elements).
   * If x is not specified, the array is initializes with 0 elements. */
  } else if( BSKStrCaseCmp( buffer, "newarray" ) == 0 ) {
    CHECKPARMCNTIS2( parmCount, 0, 1 );
    if( parmCount > 0 ) {
      ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "NewArray" );
      val = BSKEvaluateNumber( parameters[0] );
    } else {
      val = 0;
    }
    retVal->type = VT_ARRAY;
    retVal->datum = s_newArray( env, (BSKUI16)val );

  /* Sort(x,y) -- Sorts array x in place, using rule y as the comparison
   * operator for each element.  The rule must take two parameters and
   * return 1, 0, or -1. */
  } else if( BSKStrCaseCmp( buffer, "sort" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_ARRAY, "1", "Sort" );
    ENFORCETYPE( (parameters[1]->type), VT_RULE, "2", "Sort" );

    s_doSort( env,
              (BSKArray*)parameters[0]->datum,
              (BSKRule*)parameters[1]->datum );

    BSKCopyValue( retVal, parameters[0] );

  /* Length(x) -- if x is a string, returns the string length.  If x is
   * an array, returns its number of elements. */
  } else if( BSKStrCaseCmp( buffer, "length" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), ( VT_ARRAY | VT_STRING ), "1", "Length" );

    if( BSKValueIsType( parameters[0], VT_ARRAY ) ) {
      BSKSetValueNumber( retVal, ((BSKArray*)parameters[0]->datum)->length );
    } else {
      BSKSetValueNumber( retVal, BSKStrLen((BSKCHAR*)parameters[0]->datum) );
    }

  /* MagnitudeOf(x) -- returns the number x, stripped of units. */
  } else if( BSKStrCaseCmp( buffer, "magnitudeof" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "MagnitudeOf" );

    BSKSetValueNumber( retVal, BSKEvaluateNumber( parameters[0] ) );

  /* UnitsOf(x) -- returns a string naming the units of number x. */
  } else if( BSKStrCaseCmp( buffer, "unitsof" ) == 0 ) {
    BSKUI32 id;

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "UnitsOf" );

    id = BSKValueUnits( parameters[0] );
    if( id == 0 ) {
      BSKSetValueString( retVal, "" );
    } else {
      BSKGetIdentifier( env->db->idTable, id, buffer, sizeof( buffer ) );
      BSKSetValueString( retVal, buffer );
    }

  /* SetUnits(x,y) -- creates a new number of magnitude x and units y. */
  } else if( BSKStrCaseCmp( buffer, "setunits" ) == 0 ) {
    BSKUI32 id;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "SetUnits" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "SetUnits" );

    val = BSKEvaluateNumber( parameters[0] );
    id = BSKFindIdentifier( env->db->idTable, (BSKCHAR*)parameters[1]->datum );

    BSKSetValueNumberU( retVal, val, id );

  /* Eval(x) -- if x is not a number, this function simply returns x.  If
   * x is a number, it is evaluated, meaning that dice values are computed.
   * All other number values are copied over exactly. */
  } else if( BSKStrCaseCmp( buffer, "eval" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );

    if( BSKValueIsType( parameters[0], VT_NUMBER ) ) {
      BSKSetValueNumberU( retVal, 
                          BSKEvaluateNumber( parameters[0] ),
                          BSKValueUnits( parameters[0] ) );
    } else {
      BSKDereferenceValue( retVal, parameters[0] );
    }

  /* ConvertUnits(x,y) -- converts number x from its current units to the
   * units named by y, and returns the new number.  If the conversion can't
   * take place, a run-time error RTE_WRONG_UNITS is triggered. */
  } else if( BSKStrCaseCmp( buffer, "convertunits" ) == 0 ) {
    BSKUI32 from;
    BSKUI32 to;

    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "ConvertUnits" );
    ENFORCETYPE( (parameters[1]->type), VT_STRING, "2", "ConvertUnits" );

    val = BSKEvaluateNumber( parameters[0] );
    from = BSKValueUnits( parameters[0] );
    to = BSKFindIdentifier( env->db->idTable, (BSKCHAR*)parameters[1]->datum );

    if( BSKConvertUnits( env->db->unitDef, val, from, to, &val ) != 0 ) {
      s_error( env, RTE_WRONG_UNITS, "cannot convert specified units" );
      return RTE_WRONG_UNITS;
    }

    BSKSetValueNumberU( retVal, val, to );

  /* AddressOf(x) -- for debugging purposes, prints the memory address of
   * the requested value. */
  } else if( BSKStrCaseCmp( buffer, "addressof" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    BSKsprintf( buffer, "%p", parameters[0]->datum );
    BSKSetValueString( retVal, buffer );

  /* TotalWeightOf(x) -- returns the total weight of the given category. */
  } else if( BSKStrCaseCmp( buffer, "totalweightof" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "TotalWeightOf" );
    BSKSetValueNumber( retVal, 
                       ((BSKCategory*)parameters[0]->datum)->totalWeight );

  /* GetByWeight(x,y) -- returns the member of category x at weight value
   * y. */
  } else if( BSKStrCaseCmp( buffer, "getbyweight" ) == 0 ) {
    CHECKPARMCNTIS( parmCount, 2 );
    ENFORCETYPE( (parameters[0]->type), VT_CATEGORY, "1", "GetByWeight" );
    ENFORCETYPE( (parameters[1]->type), VT_NUMBER, "2", "GetByWeight" );
    
    val = BSKEvaluateNumber( parameters[1] );
    catMember = BSKGetCategoryMemberByWeight( (BSKCategory*)parameters[0]->datum,
                                              val );

    retVal->type = VT_NULL;
    if( catMember != 0 ) {
      switch( catMember->reserved ) {
        case OT_THING:
          retVal->type = VT_THING;
          retVal->datum = (BSKThing*)catMember;
          break;
        case OT_CATEGORY:
          retVal->type = VT_CATEGORY;
          retVal->datum = (BSKCategory*)catMember;
          break;
        case OT_RULE:
          retVal->type = VT_RULE;
          retVal->datum = (BSKRule*)catMember;
          break;
      }
    }

  } else if( BSKStrCaseCmp( buffer, "dicecount" ) == 0 ) {
    BSKI16 count;
    BSKUI16 type;
    BSKI16 modifier;
    BSKCHAR op[2];

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "DiceCount" );

    op[1] = 0;
    BSKGetDiceParts( parameters[0], &count, &type, op, &modifier );
    BSKSetValueNumber( retVal, count );

  } else if( BSKStrCaseCmp( buffer, "dicetype" ) == 0 ) {
    BSKI16 count;
    BSKUI16 type;
    BSKI16 modifier;
    BSKCHAR op[2];

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "DiceType" );

    op[1] = 0;
    BSKGetDiceParts( parameters[0], &count, &type, op, &modifier );
    BSKSetValueNumber( retVal, type );

  } else if( BSKStrCaseCmp( buffer, "dicemodtype" ) == 0 ) {
    BSKI16 count;
    BSKUI16 type;
    BSKI16 modifier;
    BSKCHAR op[2];

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "DiceModType" );

    op[1] = 0;
    BSKGetDiceParts( parameters[0], &count, &type, op, &modifier );
    BSKSetValueString( retVal, op );

  } else if( BSKStrCaseCmp( buffer, "dicemodifier" ) == 0 ) {
    BSKI16 count;
    BSKUI16 type;
    BSKI16 modifier;
    BSKCHAR op[2];

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_NUMBER, "1", "DiceModifier" );

    op[1] = 0;
    BSKGetDiceParts( parameters[0], &count, &type, op, &modifier );
    BSKSetValueNumber( retVal, modifier );

  } else if( BSKStrCaseCmp( buffer, "uppercase" ) == 0 ) {
    BSKI16 i;

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_STRING, "1", "UpperCase" );

    ptr = BSKStrDup( (BSKCHAR*)parameters[0]->datum );
    for( i = 0; ptr[i] != 0; i++ ) {
      ptr[i] = BSKToUpper( ptr[i] );
    }
    BSKSetValueString( retVal, ptr );
    BSKFree( ptr );
     
  } else if( BSKStrCaseCmp( buffer, "lowercase" ) == 0 ) {
    BSKI16 i;

    CHECKPARMCNTIS( parmCount, 1 );
    ENFORCETYPE( (parameters[0]->type), VT_STRING, "1", "LowerCase" );

    ptr = BSKStrDup( (BSKCHAR*)parameters[0]->datum );
    for( i = 0; ptr[i] != 0; i++ ) {
      ptr[i] = BSKToLower( ptr[i] );
    }
    BSKSetValueString( retVal, ptr );
    BSKFree( ptr );
     
  } else {
    /* return an error if the built-in was not known. */
    s_error( env, RTE_CALL_OF_NONFUNCTION, buffer );
    return RTE_CALL_OF_NONFUNCTION;
  }

  return RTE_SUCCESS;
}


static BSKI32 s_doNegative( BSKExecutionEnvironment* env ) {
  BSKFLOAT val;
  BSKStackFrame* frame;

  frame = env->stackFrame;

  if( !s_ensureOpCount( frame, 1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "NEG" );
    return RTE_STACK_UNDERFLOW;
  }

  ENFORCETYPE( (frame->stack->value.type), VT_NUMBER, "1", "NEG" );
  val = -BSKEvaluateNumber( &(frame->stack->value) );

  s_popStackItem( frame );
  s_newStackItem( frame );

  BSKSetValueNumber( &(frame->stack->value), val );

  return RTE_SUCCESS;
}


static BSKI32 s_doDereference( BSKExecutionEnvironment* env,
                               BSKStackFrame* frame ) 
{
  BSKValue parent;
  BSKUI32  id;
  BSKSymbolTableEntry* sym;
  BSKAttribute* attribute;

  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "DREF" );
    return RTE_STACK_UNDERFLOW;
  }

  ENFORCETYPE( (frame->stack->value.type), VT_INTEGER, "1", "DREF" );

  /* get the parent object and make sure it is a THING */
  BSKDereferenceValue( &parent, &(frame->stack->next->value) );
  if( parent.type != VT_THING ) {
    BSKInvalidateValue( &parent );
    s_error( env, RTE_DOMAIN_ERROR, "only THINGs can be dereferenced ['.' operator]" );
    return RTE_DOMAIN_ERROR;
  }

  /* get the id of the attribute being referenced */
  id = (BSKUI32)BSKEvaluateNumber( &(frame->stack->value) );

  s_popStackItem( frame );
  s_popStackItem( frame );

  /* make sure the id references a valid ATTRIBUTE symbol */
  sym = BSKGetSymbol( env->db->symbols, id );
  if( sym == 0 ) {
    s_error( env, RTE_INVALID_OPERANDS, "specified member is not a symbol" );
    return RTE_INVALID_OPERANDS;
  } else if( sym->type != ST_ATTRIBUTE ) {
    s_error( env, RTE_INVALID_OPERANDS, "specified member is not an ATTRIBUTE" );
    return RTE_INVALID_OPERANDS;
  }

  /* get the ATTRIBUTE from the THING, or create it if it does not yet
   * exist */
  attribute = BSKGetAttributeOf( (BSKThing*)parent.datum, id );
  if( attribute == 0 ) {
    attribute = s_initializeNewAttribute( env, 
                                          (BSKThing*)parent.datum, 
                                          id );
  }

  /* push the new ATTRIBUTE onto the stack */
  s_newStackItem( frame );
  frame->stack->value.type = VT_ATTRIBUTE;
  frame->stack->value.datum = attribute;

  return RTE_SUCCESS;
}


static BSKI32 s_doLogicalNegation( BSKExecutionEnvironment* env ) {
  BSKValue value;
  BSKI32 i;
  BSKStackFrame* frame;

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "NOT" );
    return RTE_STACK_UNDERFLOW;
  }

  BSKDereferenceValue( &value, &(frame->stack->value) );
  if( !BSKValueIsType( &value, VT_INTEGER ) ) {
    BSKInvalidateValue( &value );
    s_error( env, RTE_INVALID_OPERANDS, "NOT" );
    return RTE_INVALID_OPERANDS;
  }

  i = (BSKI32)BSKEvaluateNumber( &value );
  i = !i;

  s_popStackItem( frame );
  s_newStackItem( frame );

  BSKSetValueNumber( &(frame->stack->value), i );
  BSKInvalidateValue( &value );

  return RTE_SUCCESS;
}


static BSKI32 s_testTopValue( BSKExecutionEnvironment* env, 
                              BSKBOOL* result ) 
{
  BSKValue value;
  BSKStackFrame* frame;

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "true/false test" );
    return RTE_STACK_UNDERFLOW;
  }

  BSKDereferenceValue( &value, &(frame->stack->value) );
  if( !BSKValueIsType( &value, VT_INTEGER ) ) {
    BSKInvalidateValue( &value );
    s_error( env, RTE_INVALID_OPERANDS, "true/false test" );
    return RTE_INVALID_OPERANDS;
  }

  *result = (BSKBOOL)BSKEvaluateNumber( &value );

  s_popStackItem( frame );
  BSKInvalidateValue( &value );

  return RTE_SUCCESS;
}


static BSKI32 s_doPut( BSKExecutionEnvironment* env ) {
  BSKValue dest;
  BSKValue value;
  BSKI32 rc;
  BSKUI32 id;

  if( !s_ensureOpCount( env->stackFrame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "PUT" );
    return RTE_STACK_UNDERFLOW;
  }

  /* dereference the value to put, and simply get the destination. */

  BSKDereferenceValue( &value, &(env->stackFrame->stack->value) );
  BSKCopyValue( &dest, &(env->stackFrame->stack->next->value) );

  s_popStackItem( env->stackFrame );
  s_popStackItem( env->stackFrame );

  /* if the destination is a rule with the same name as the current function,
   * then we are actually going to put the value into the RULE_RETURN_VAL
   * variable. */

  if( ( dest.type == VT_RULE ) && ( ((BSKRule*)dest.datum)->id == env->stackFrame->ruleId ) ) {
    BSKInvalidateValue( &dest );
    id = BSKFindIdentifier( env->db->idTable, RULE_RETURN_VAL );
    dest.type = VT_VARIABLE;
    dest.datum = s_findVariable( env->stackFrame, id );
  }

  /* only LVALUE (or DEREFERENCABLE) values may receive values */

  if( !BSKValueIsType( &dest, VT_DEREFERENCABLE ) ) {
    BSKInvalidateValue( &dest );
    BSKInvalidateValue( &value );
    s_error( env, RTE_INVALID_OPERANDS, "can only assign to an LVALUE" );
    return RTE_INVALID_OPERANDS;
  }

  rc = RTE_SUCCESS;
  switch( dest.type ) {
    case VT_VARIABLE:
      BSKInvalidateValue( &(((BSKVariable*)dest.datum)->value) );
      BSKCopyValue( &(((BSKVariable*)dest.datum)->value), &value );
      break;
    case VT_VALUE:
      BSKInvalidateValue( (BSKValue*)dest.datum );
      BSKCopyValue( (BSKValue*)dest.datum, &value );
      break;
    case VT_ATTRIBUTE:
      {
        BSKAttribute* attr;
        BSKAttributeDef* attrDef;

        attr = (BSKAttribute*)dest.datum;
        attrDef = BSKFindAttributeDef( env->db->attrDef, attr->id );
        if( attrDef == 0 ) {
          s_error( env, RTE_BUG, "ATTRIBUTE not defined" );
          return RTE_BUG;
        }

        /* make sure the value being assigned has the same type as the
         * attribute being assigned to, otherwise trigger an 
         * RTE_DOMAIN_ERROR */

        BSKInvalidateValue( &(attr->value) );
        switch( attrDef->type ) {
          case AT_NONE: 
            rc = RTE_BUG;
            s_error( env, RTE_BUG, "ATTRIBUTE type not defined" );
            break;
          case AT_THING:
            if( value.type != VT_THING ) {
              s_error( env, RTE_DOMAIN_ERROR, "ATTRIBUTE and value do not match" );
              rc = RTE_DOMAIN_ERROR;
            } else {
              attr->value.type = VT_THING;
              attr->value.datum = value.datum;
            }
            break;
          case AT_CATEGORY:
            if( value.type != VT_CATEGORY ) {
              s_error( env, RTE_DOMAIN_ERROR, "ATTRIBUTE and value do not match" );
              rc = RTE_DOMAIN_ERROR;
            } else {
              attr->value.type = VT_CATEGORY;
              attr->value.datum = value.datum;
            }
            break;
          case AT_RULE:
            if( value.type != VT_RULE ) {
              s_error( env, RTE_DOMAIN_ERROR, "ATTRIBUTE and value do not match" );
              rc = RTE_DOMAIN_ERROR;
            } else {
              attr->value.type = VT_RULE;
              attr->value.datum = value.datum;
            }
            break;
          case AT_NUMBER:
          case AT_BOOLEAN:
            if( !BSKValueIsType( &value, VT_NUMBER ) ) {
              s_error( env, RTE_DOMAIN_ERROR, "ATTRIBUTE and value do not match" );
              rc = RTE_DOMAIN_ERROR;
            } else {
              BSKCopyValue( &(attr->value), &value );
            }
            break;
          case AT_STRING:
            if( !BSKValueIsType( &value, VT_STRING ) ) {
              s_error( env, RTE_DOMAIN_ERROR, "ATTRIBUTE and value do not match" );
              rc = RTE_DOMAIN_ERROR;
            } else {
              BSKCopyValue( &(attr->value), &value );
            }
            break;
        }
      }
      break;
    case VT_CATEGORY_ENTRY:
      if( !BSKValueIsType( &value, ( VT_NULL | VT_THING | VT_CATEGORY | VT_RULE ) ) ) {
        s_error( env, RTE_DOMAIN_ERROR, "A category may only contain THINGs, CATEGORYs, and RULEs" );
        rc = RTE_DOMAIN_ERROR;
      } else {
        ((BSKCategoryEntry*)dest.datum)->member = value.datum;
      }
      break;
  }

  BSKInvalidateValue( &dest );
  BSKInvalidateValue( &value );

  return rc;
}


static BSKI32 s_doBooleanConnector( BSKExecutionEnvironment* env, BSKUI32 op ) {
  BSKI32 v1;
  BSKI32 v2;
  BSKStackFrame* frame;
  BSKValue op1;
  BSKValue op2;

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "AND/OR" );
    return RTE_STACK_UNDERFLOW;
  }

  BSKDereferenceValue( &op1, &(frame->stack->next->value) );
  BSKDereferenceValue( &op2, &(frame->stack->value) );

  if( !BSKValueIsType( &op1, VT_INTEGER ) ) {
    BSKInvalidateValue( &op1 );
    BSKInvalidateValue( &op2 );
    s_error( env, RTE_INVALID_OPERANDS, "wrong type for parameter #1" );
    return RTE_INVALID_OPERANDS;
  }
    
  if( !BSKValueIsType( &op2, VT_INTEGER ) ) {
    BSKInvalidateValue( &op1 );
    BSKInvalidateValue( &op2 );
    s_error( env, RTE_INVALID_OPERANDS, "wrong type for parameter #2" );
    return RTE_INVALID_OPERANDS;
  }
    
  v1 = (BSKI32)BSKEvaluateNumber( &op1 );
  v2 = (BSKI32)BSKEvaluateNumber( &op2 );

  BSKInvalidateValue( &op1 );
  BSKInvalidateValue( &op2 );

  s_popStackItem( frame );
  s_popStackItem( frame );

  s_newStackItem( frame );
  switch( op ) {
    case OP_AND: v1 = ( v1 && v2 ); break;
    case OP_OR:  v1 = ( v1 || v2 ); break;
  }

  BSKSetValueNumber( &(frame->stack->value), v1 );

  return 0;
}


static BSKI32 s_doFirst( BSKExecutionEnvironment* env ) {
  BSKCategory* category;
  BSKValue val;

  if( !s_ensureOpCount( env->stackFrame, 1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "FIRST" );
    return RTE_STACK_UNDERFLOW;
  }

  /* get the category object */

  BSKDereferenceValue( &val, &(env->stackFrame->stack->value) );
  if( !BSKValueIsType( &val, VT_CATEGORY ) ) {
    BSKInvalidateValue( &val );
    s_error( env, RTE_INVALID_OPERANDS, "expected category" );
    return RTE_INVALID_OPERANDS;
  }

  category = (BSKCategory*)val.datum;
  s_popStackItem( env->stackFrame );
  s_newStackItem( env->stackFrame );

  /* if it has no members, put a NULL on the stack, otherwise put the
   * category's first member on the stack. */

  if( category->members == 0 ) {
    env->stackFrame->stack->value.type = VT_NULL;
  } else {
    env->stackFrame->stack->value.type = VT_CATEGORY_ENTRY;
    env->stackFrame->stack->value.datum = category->members;
  }

  BSKInvalidateValue( &val );

  return 0;
}


static BSKI32 s_doNext( BSKExecutionEnvironment* env ) {
  BSKCategoryEntry* entry;

  if( !s_ensureOpCount( env->stackFrame, 1 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "NEXT" );
    return RTE_STACK_UNDERFLOW;
  }

  if( !BSKValueIsType( (&env->stackFrame->stack->value), VT_CATEGORY_ENTRY ) ) {
    s_error( env, RTE_INVALID_OPERANDS, "expected category item" );
  }

  entry = (BSKCategoryEntry*)env->stackFrame->stack->value.datum;
  s_popStackItem( env->stackFrame );
  s_newStackItem( env->stackFrame );

  /* get the category entry's next member, or NULL If the category entry
   * is null */

  env->stackFrame->stack->value.type = VT_CATEGORY_ENTRY;
  env->stackFrame->stack->value.datum = ( entry == 0 ? entry : entry->next );

  /* if the result is NULL, change the value's type to NULL */

  if( env->stackFrame->stack->value.datum == 0 ) {
    env->stackFrame->stack->value.type = VT_NULL;
  }

  return 0;
}


static BSKThing* s_addThingToFrame( BSKStackFrame* frame, BSKThing* thing ) {
  BSKThing* i;

  /* set the flag to local, since this object is now (or could be) the
   * property of this stack frame. */

  thing->flags = OF_LOCAL;

  /* don't add the thing if it already exists in the frame's thing list */

  for( i = frame->things; i != 0; i = i->next ) {
    if( i == thing ) {
      return thing;
    }
  }

  /* don't add the thing if it is owned by a lower-level stack frame */

  if( thing->ownerLevel >= frame->level ) {
    thing->ownerLevel = frame->level;
    thing->next = frame->things;
    frame->things = thing;
  }

  return thing;
}


static BSKCategory* s_addCategoryToFrame( BSKStackFrame* frame, BSKCategory* category ) {
  BSKCategory* i;

  /* set the flag to local, since this object is now (or could be) the
   * property of this stack frame. */

  category->flags = OF_LOCAL;

  /* don't add the category if it already exists in the frame's category
   * list */

  for( i = frame->categories; i != 0; i = i->next ) {
    if( i == category ) {
      return category;
    }
  }

  /* don't add the category if it is owned by a lower-level stack frame */

  if( category->ownerLevel >= frame->level ) {
    category->ownerLevel = frame->level;
    category->next = frame->categories;
    frame->categories = category;
  }

  return category;
}


static BSKArray* s_addArrayToFrame( BSKStackFrame* frame, BSKArray* array ) {
  BSKArray* i;

  /* set the flag to local, since this object is now (or could be) the
   * property of this stack frame. */

  array->flags = OF_LOCAL;

  /* don't add the array if it already exists in the frame's array list */

  for( i = frame->arrays; i != 0; i = i->next ) {
    if( i == array ) {
      return array;
    }
  }

  /* don't add the array if it is owned by a lower-level stack frame */

  if( array->ownerLevel >= frame->level ) {
    array->ownerLevel = frame->level;
    array->next = frame->arrays;
    frame->arrays = array;
  }

  return array;
}


static void s_convertObjectFlag( BSKValue* value, BSKUI8 from, BSKUI8 to ) {
  if( value->type == VT_THING ) {
    s_convertThingFlag( (BSKThing*)value->datum, from, to );
  } else if( value->type == VT_CATEGORY ) {
    s_convertCategoryFlag( (BSKCategory*)value->datum, from, to );
  } else if( value->type == VT_ARRAY ) {
    BSKArray* array;
    BSKUI16   i;

    /* if the array flag is 'from', set it to 'to', otherwise, if the
     * flag is already 'to', then return (to avoid recursive revisiting of
     * this object). */

    array = (BSKArray*)value->datum;
    if( array->flags == from ) {
      array->flags = to;
    } else if( array->flags == to ) {
      return;
    }

    /* convert the flags of each element of the array */

    for( i = 0; i < array->length; i++ ) {
      s_convertObjectFlag( BSKGetElement( array, i ), from, to );
    }
  }
}


static void s_convertThingFlag( BSKThing* thing, BSKUI8 from, BSKUI8 to ) {
  BSKAttribute* attr;

  /* if the thing flag is 'from', set it to 'to', otherwise, if the
   * flag is already 'to', then return (to avoid recursive revisiting of
   * this object). */

  if( thing->flags == from ) {
    thing->flags = to;
  } else if( thing->flags == to ) {
    return;
  }

  /* convert the flags of each object in the thing */

  for( attr = thing->list; attr != 0; attr = attr->next ) {
    s_convertObjectFlag( &(attr->value), from, to );
  }
}


static void s_convertCategoryFlag( BSKCategory* category, BSKUI8 from, BSKUI8 to ) {
  BSKCategoryEntry* entry;

  /* if the category flag is 'from', set it to 'to', otherwise, if the
   * flag is already 'to', then return (to avoid recursive revisiting of
   * this object). */

  if( category->flags == from ) {
    category->flags = to;
  } else if( category->flags == to ) {
    return;
  }

  /* convert the flags for each non-NULL memeber of the category */

  for( entry = category->members; entry != 0; entry = entry->next ) {
    if( entry->member != 0 ) {
      if( entry->member->reserved == OT_THING ) {
        s_convertThingFlag( (BSKThing*)entry->member, from, to );
      } else if( entry->member->reserved == OT_CATEGORY ) {
        s_convertCategoryFlag( (BSKCategory*)entry->member, from, to );
      }
    }
  }
}


static void s_collectTransientObjects( BSKStackFrame* frame, BSKValue* value ) {
  if( value->type == VT_THING ) {
    s_collectTransientThing( frame, (BSKThing*)value->datum );
  } else if( value->type == VT_CATEGORY ) {
    s_collectTransientCategory( frame, (BSKCategory*)value->datum );
  } else if( value->type == VT_ARRAY ) {
    BSKArray* array;
    BSKUI16   i;
    BSKUI8    temp;

    array = (BSKArray*)value->datum;

    /* if the array has already been visited, don't visit it! */

    if( array->flags == OF_VISITED ) {
      return;
    }

    /* if the array is TRANSIENT, add it to the frame. */

    if( array->flags == OF_TRANSIENT ) {
      s_addArrayToFrame( frame, array );
    }

    temp = array->flags;
    array->flags = OF_VISITED;

    /* check each element of the array to see if it is TRANSIENT. */

    for( i = 0; i < array->length; i++ ) {
      s_collectTransientObjects( frame, BSKGetElement( array, i ) );
    }

    array->flags = temp;
  }
}


static void s_collectTransientThing( BSKStackFrame* frame, BSKThing* thing ) {
  BSKAttribute* attr;
  BSKUI8 temp;

  /* if the thing has already been visited, don't visit it again! */

  if( thing->flags == OF_VISITED ) {
    return;
  }

  /* if the thing is TRANSIENT, add it to the current frame */

  if( thing->flags == OF_TRANSIENT ) {
    s_addThingToFrame( frame, thing );
  }

  temp = thing->flags;
  thing->flags = OF_VISITED;

  for( attr = thing->list; attr != 0; attr = attr->next ) {
    s_collectTransientObjects( frame, &(attr->value) );
  }

  thing->flags = temp;
}


static void s_collectTransientCategory( BSKStackFrame* frame, BSKCategory* category ) {
  BSKCategoryEntry* entry;
  BSKUI8            temp;

  /* if the category has already been visited, don't visit it again! */
  if( category->flags == OF_VISITED ) {
    return;
  }

  /* if the category is TRANSIENT, add it to the current frame */

  if( category->flags == OF_TRANSIENT ) {
    s_addCategoryToFrame( frame, category );
  }

  /* mark the category visited */

  temp = category->flags;
  category->flags = OF_VISITED;

  for( entry = category->members; entry != 0; entry = entry->next ) {
    if( entry->member != 0 ) {
      if( entry->member->reserved == OT_THING ) {
        s_collectTransientThing( frame, (BSKThing*)entry->member );
      } else if( entry->member->reserved == OT_CATEGORY ) {
        s_collectTransientCategory( frame, (BSKCategory*)entry->member );
      }
    }
  }

  /* restore the category's flag to what it was before we marked it visited */

  category->flags = temp;
}


static void s_cleanupValue( BSKValue* value, BSKUI8 type ) {
  if( value->type == VT_THING ) {
    s_cleanupThing( (BSKThing*)value->datum, type );
  } else if( value->type == VT_CATEGORY ) {
    s_cleanupCategory( (BSKCategory*)value->datum, type );
  } else if( value->type == VT_ARRAY ) {
    BSKArray* array;
    BSKUI16   i;

    /* cleanup each element of the array */

    array = (BSKArray*)value->datum;
    for( i = 0; i < array->length; i++ ) {
      s_cleanupValue( BSKGetElement( array, i ), type );
    }

    /* destroy the array if it is of the requested type */

    if( array->flags == type ) {
      BSKDestroyArray( array );
    }
  }

  BSKInvalidateValue( value );
}


static void s_cleanupThing( BSKThing* thing, BSKUI8 type ) {
  BSKAttribute* attr;

  /* cleanup the attributes of the thing */

  for( attr = thing->list; attr != 0; attr = attr->next ) {
    s_cleanupValue( &(attr->value), type );
  }

  /* destroy the thing if it is of the requested type */

  if( thing->flags == type ) {
    BSKDestroyThing( thing );
  }
}


static void s_cleanupCategory( BSKCategory* category, BSKUI8 type ) {
  BSKCategoryEntry* entry;

  /* cleanup the members of the category */

  for( entry = category->members; entry != 0; entry = entry->next ) {
    if( entry->member != 0 ) {
      if( entry->member->reserved == OT_THING ) {
        s_cleanupThing( (BSKThing*)entry->member, type );
      } else if( entry->member->reserved == OT_CATEGORY ) {
        s_cleanupCategory( (BSKCategory*)entry->member, type );
      }
    }
  }

  /* destroy the category if it is of the requested type */

  if( category->flags == type ) {
    BSKDestroyCategory( category );
  }
}


static BSKI32 s_doGetElement( BSKExecutionEnvironment* env ) {
  BSKUI16   index;
  BSKArray* array;
  BSKValue  idxVal;
  BSKValue  arrVal;
  BSKStackFrame* frame;

  frame = env->stackFrame;
  if( !s_ensureOpCount( frame, 2 ) ) {
    s_error( env, RTE_STACK_UNDERFLOW, "get array element" );
    return RTE_STACK_UNDERFLOW;
  }

  BSKDereferenceValue( &idxVal, &(frame->stack->value) );
  BSKDereferenceValue( &arrVal, &(frame->stack->next->value) );

  s_popStackItem( frame );
  s_popStackItem( frame );

  /* make sure the operands are of the correct type */

  if( ( !BSKValueIsType( (&idxVal), VT_NUMBER ) ) || ( !BSKValueIsType( (&arrVal), VT_ARRAY ) ) ) {
    BSKInvalidateValue( &idxVal );
    BSKInvalidateValue( &arrVal );
    s_error( env, RTE_INVALID_OPERANDS, "not an array, or index is not a number" );
    return RTE_INVALID_OPERANDS;
  }

  /* get the index and the array, and get the value of the element at that
   * index. */

  index = (BSKUI16)BSKEvaluateNumber( &idxVal );
  array = (BSKArray*)arrVal.datum;

  s_newStackItem( frame );
  frame->stack->value.type = VT_VALUE;
  frame->stack->value.datum = BSKGetElement( array, index );

  BSKInvalidateValue( &idxVal );
  BSKInvalidateValue( &arrVal );

  return 0;
}


static void s_doSort( BSKExecutionEnvironment* env, 
                      BSKArray* array, 
                      BSKRule* rule ) 
{
  BSKI32 i;
  BSKI32 j;
  BSKValue retVal;
  BSKValue* parms[2];
  BSKI32 rc;
  BSKI32 size;
  BSKExecOpts opts;

  /* for now, this implements a simple bubble-sort algorithm.  It wouldn't
   * be hard to use a different algorithm -- this is the place it would
   * go.  Perhaps it would be desirable to have multiple algorithms that
   * the user could choose from -- shouldn't be hard to do, either way. */

  BSKMemSet( &opts, 0, sizeof( opts ) );
  opts.db = env->db;
  opts.ruleId = rule->id;
  opts.parameterCount = 2;
  opts.parameters = parms;
  opts.rval = &retVal;
  opts.errorHandler = env->errorHandler;
  opts.console = env->console;
  opts.userData = env->userData;
  opts.halt = env->opts->halt;

  size = (BSKI32)array->length;
  for( i = 0; i < size-1; i++ ) {
    for( j = i+1; j < size; j++ ) {
      
      /* if execution has been halted, break out of the sort */
      if( *(env->opts->halt) ) {
        return;
      }

      parms[0] = &array->elements[ i ];
      parms[1] = &array->elements[ j ];
      rc = BSKExec( &opts );
      if( rc != 0 ) {
        /* if BSKExec returns an error, it will have already called the
         * error handling routine, so we don't need to do anything here
         * except return. */
        return;
      }
      if( !BSKValueIsType( &retVal, VT_NUMBER ) ) {
        BSKInvalidateValue( &retVal );
        s_error( env, RTE_DOMAIN_ERROR, "sort routine must return a number" );
        return;
      }
      rc = (BSKI32)BSKEvaluateNumber( &retVal );
      BSKInvalidateValue( &retVal );

      /* here's the time consuming part -- moving memory around */
      if( rc > 0 ) {
        BSKMemCpy( &retVal, &array->elements[i], sizeof( BSKValue ) );
        BSKMemCpy( &array->elements[i], &array->elements[j], sizeof( BSKValue ) );
        BSKMemCpy( &array->elements[j], &retVal, sizeof( BSKValue ) );
      }
    }
  }
}


static void s_error( BSKExecutionEnvironment* env, BSKI32 code, BSKCHAR* pfx ) {
  if( env->errorHandler != 0 ) {
    env->errorHandler( code, pfx, env, env->userData );
  }
}


static BSKThing* s_newThing( BSKExecutionEnvironment* env, BSKThing* model ) {
  BSKThing* thing;

  /* if a model is provided, duplicate it, otherwise create a brand new
   * thing.  Either way, add the new thing to the current frame. */

  if( model == 0 ) {
    thing = BSKNewThing( 0 );
  } else {
    thing = BSKDuplicateThing( model );
  }

  thing->ownerLevel = env->stackFrame->level;
  s_addThingToFrame( env->stackFrame, thing );

  return thing;
}


static BSKCategory* s_newCategory( BSKExecutionEnvironment* env, BSKCategory* model ) {
  BSKCategory* category;

  /* if a model is provided, duplicate it, otherwise create a brand new
   * category.  Either way, add the new category to the current frame. */

  if( model == 0 ) {
    category = BSKNewCategory( 0 );
  } else {
    category = BSKDuplicateCategory( model );
  }

  category->ownerLevel = env->stackFrame->level;
  s_addCategoryToFrame( env->stackFrame, category );

  return category;
}


static BSKArray* s_newArray( BSKExecutionEnvironment* env, BSKUI16 init ) {
  BSKArray* array;

  array = BSKNewArray( init );
  array->ownerLevel = env->stackFrame->level;
  s_addArrayToFrame( env->stackFrame, array );

  return array;
}


static BSKAttribute* s_initializeNewAttribute( BSKExecutionEnvironment* env,
                                               BSKThing* thing,
                                               BSKUI32   attribute )
{
  BSKValue         newValue;
  BSKAttributeDef* def;

  def = BSKFindAttributeDef( env->db->attrDef, attribute );
  if( def == 0 ) {
    return 0;
  }

  BSKInitializeValue( &newValue );

  switch( def->type ) {
    case AT_NUMBER:
      BSKSetValueNumber( &newValue, 0 );
      break;
    case AT_STRING:
      BSKSetValueString( &newValue, "" );
      break;
    case AT_BOOLEAN:
      BSKSetValueNumber( &newValue, BSKFALSE );
      break;
  }

  BSKAddAttributeTo( thing, attribute, &newValue );
  BSKInvalidateValue( &newValue );

  return BSKGetAttributeOf( thing, attribute );
}

