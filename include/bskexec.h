/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskexec.h
 *
 * This file defines the Basilisk Virtual Machine (VM).  The VM is used
 * to execute the rules (bskrule.h) stored in the database (bskdb.h).
 * Rules may accept zero or more parameters, and can return a value, so
 * the VM also works closely with the values API (bskvalue.h).
 *
 * Run-time errors are handled via an error handler that is passed into the
 * VM when it is called.  The calling routine(s) must define this error
 * handler, or pass a null (0) in place of that parameter.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef ___BSKEXEC_H__
#define ___BSKEXEC_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskthing.h"
#include "bskctgry.h"
#include "bskrule.h"
#include "bskdb.h"
#include "bskvalue.h"
#include "bskvar.h"
#include "bskarray.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

#define RTE_SUCCESS                   (     0 )
#define RTE_INVALID_RULE              ( -5001 ) /* the requested rule does not exist or is not a rule */
#define RTE_BUG                       ( -5002 ) /* bug detected -- notify the programmer! */
#define RTE_STACK_UNDERFLOW           ( -5003 ) /* not enough items on stack to satisfy the request */
#define RTE_INVALID_OPERANDS          ( -5004 ) /* invalid operands to operator (adding category objects, for instance) */
#define RTE_DIVIDE_BY_ZERO            ( -5005 ) /* division by zero */
#define RTE_UNKNOWN_INSTRUCTION       ( -5006 ) /* unknown instruction encountered */
#define RTE_WRONG_PARAM_COUNT         ( -5007 ) /* wrong number of parameters passed to rule or subroutine */
#define RTE_DOMAIN_ERROR              ( -5008 ) /* operation requested was incompatible with data provided -- adding a string to a category, for instance */
#define RTE_WRONG_UNITS               ( -5009 ) /* wrong units for numbers in operation -- attempt to add hours to dollars, for instance */
#define RTE_CALL_OF_NONFUNCTION       ( -5010 ) /* attempt to call on something other than a rule or subroutine */
#define RTE_HALTED                    ( -5011 ) /* execution was terminated at user's request */

/* ---------------------------------------------------------------------- *
 * Macro defines
 * ---------------------------------------------------------------------- */

  /* BSKDatabase* BSKExecEnvironmentGetDB( BSKExecutionEnvironment* env ) */
#define BSKExecEnvironmentGetDB(x)               ((x)->db)

  /* BSKStackFrame* BSKExecEnvironmentGetCurrentFrame( BSKExecutionEnvironment* env ) */
#define BSKExecEnvironmentGetCurrentFrame(x)     ((x)->stackFrame)

  /* BSKNOTYPE BSKExecEnvironmentGetUserData( BSKExecutionEnvironment* env ) */
#define BSKExecEnvironmentGetUserData(x)         ((x)->userData)

  /* BSKUI32 BSKExecEnvironmentGetCurrentLine( BSKExecutionEnvironment* env ) */
#define BSKExecEnvironmentGetCurrentLine(x)      ((x)->line)

  /* BSKUI32 BSKStackFrameGetRuleID( BSKStackFrame* frame ) */
#define BSKStackFrameGetRuleID(x)                ((x)->ruleId)

  /* BSKUI8 BSKStackFrameGetLevel( BSKStackFrame* frame ) */
#define BSKStackFrameGetLevel(x)                 ((x)->level)

  /* BSKUI32 BSKStackFrameGetParameterCount( BSKStackFrame* frame ) */
#define BSKStackFrameGetParameterCount(x)        ((x)->parameterCount)

  /* BSKValue** BSKStackFrameGetParameters( BSKStackFrame* frame ) */
#define BSKStackFrameGetParameteris(x)           ((x)->parameters)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskexecutionstackitem__   BSKExecutionStackItem;
typedef struct __bskexecutionenvironment__ BSKExecutionEnvironment;
typedef struct __bskstackframe__           BSKStackFrame;
typedef struct __bskexecopts__             BSKExecOpts;

typedef BSKI32 (*BSKRuntimeErrorHandler)( BSKI32,                   /* code */
                                          BSKCHAR*,                 /* message */
                                          BSKExecutionEnvironment*, /* env  */
                                          BSKNOTYPE );              /* user data */

typedef BSKI32 (*BSKConsoleHook)( BSKCHAR*,                  /* message */
                                  BSKExecutionEnvironment*,  /* env */
                                  BSKNOTYPE );               /* user data */

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskexecutionstackitem__ {
  BSKValue value;                  /* the value on the stack */
  BSKExecutionStackItem* next;
};


struct __bskexecutionenvironment__ {
  BSKDatabase* db;                     /* the database object */
  BSKStackFrame* stackFrame;           /* the stack of stackFrame objects */
  BSKRuntimeErrorHandler errorHandler; /* the error handler to use */
  BSKConsoleHook console;              /* the console call-back function */
  BSKNOTYPE userData;                  /* application specific data */
  BSKUI32 line;                        /* the current line of the rule being executed */
  BSKExecOpts* opts;                   /* the options that defined the current environment */
};


struct __bskstackframe__ {
  BSKUI32 ruleId;                      /* the rule represented by this stack frame */
  BSKUI8  level;                       /* how deep in recursion we are (ie, the number of stackFrames) */
  BSKUI32 parameterCount;              /* # of parameters in 'parameters' array */
  BSKValue** parameters;               /* array of parameters passed to this rule */
  BSKValue* retVal;                    /* the value to hold the return value from this rule */
  BSKVariable* variables;              /* list of all locally instantiated variables */
  BSKExecutionStackItem* stack;        /* value stack used by most operations */
  BSKThing* things;                    /* list of all locally instantiated things */
  BSKCategory* categories;             /* list of all locally instantiated categories */
  BSKArray* arrays;                    /* list of all locally instantiated arrays */
  BSKStackFrame* next;                 /* next stack frame on the stack */
};

struct __bskexecopts__ {
  BSKDatabase*           db;
  BSKUI32                ruleId;
  BSKUI32                parameterCount;
  BSKValue**             parameters;
  BSKValue*              rval;
  BSKRuntimeErrorHandler errorHandler;
  BSKConsoleHook         console;
  BSKNOTYPE              userData;
  BSKBOOL*               halt;        /* Set to true to halt execution of the VM */
};

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
   * BSKExec
   *
   * Executes the requested rule with the given database and parameters,
   * and puts the return value in 'rval'.  Run-time errors are handled by
   * 'errorHandler' (unless it is 0).  The 'userData' variable is passed
   * to all calls to the 'errorHandler.'  The 'console' callback function
   * is used by the "print" built-in function to display (or not) data.
   *
   * Returns RTE_SUCCESS when successful, or one of the other RTE_xxx
   * values (defined above) when there was an error.
   * -------------------------------------------------------------------- */
BSKI32 BSKExec( BSKExecOpts* opts );

  /* -------------------------------------------------------------------- *
   * BSKCleanupReturnValue
   *
   * "Cleans up" the given value.  The value is invalidated at the very
   * least (see bskvalue.h, BSKInvalidateValue), but if the value is a
   * thing or category, its members and/or attributes are recursively
   * descended and cleaned up, and all objects not owned by the database
   * are destroyed (ownerLevel != 0).  You MUST call this routine on the
   * value returned by BSKExec when you are done with it, or you may end
   * up with memory leaks.
   * -------------------------------------------------------------------- */
void BSKCleanupReturnValue( BSKValue* value );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* ___BSKEXEC_H__ */
