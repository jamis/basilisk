/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskrule.h
 *
 * A rule object represents a single function in a Basilisk data file.
 * It contains the bytecode that the VM (bskexec.h) executes.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKRULE_H__
#define __BSKRULE_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bsksymtb.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * These are the bytecode operators.  They are described more fully
   * in bskexec.c, in the s_execCode function.
   * -------------------------------------------------------------------- */
#define OP_NOOP              (  0 )
#define OP_LSTR              (  1 )
#define OP_LID               (  2 )
#define OP_LBYTE             (  3 )
#define OP_LWORD             (  4 )
#define OP_LDWORD            (  5 )
#define OP_LDICE             (  6 )
#define OP_LFLOAT            (  7 )
#define OP_NULL              (  8 )
#define OP_CALL              (  9 )
#define OP_ADD               ( 10 )
#define OP_SUB               ( 11 )
#define OP_MUL               ( 12 )
#define OP_DIV               ( 13 )
#define OP_POW               ( 14 )
#define OP_NEG               ( 15 )
#define OP_DREF              ( 16 )
#define OP_LT                ( 17 )
#define OP_GT                ( 18 )
#define OP_LE                ( 19 )
#define OP_GE                ( 20 )
#define OP_EQ                ( 21 )
#define OP_NE                ( 22 )
#define OP_TYPEOF            ( 23 )
#define OP_NOT               ( 24 )
#define OP_JUMP              ( 25 )
#define OP_JMPT              ( 26 )
#define OP_JMPF              ( 27 )
#define OP_END               ( 28 )
#define OP_GET               ( 29 )
#define OP_PUT               ( 30 )
#define OP_AND               ( 31 )
#define OP_OR                ( 32 )
#define OP_MOD               ( 33 )
#define OP_FIRST             ( 34 )
#define OP_NEXT              ( 35 )
#define OP_DUP               ( 36 )
#define OP_POP               ( 37 )
#define OP_ELEM              ( 38 )
#define OP_SWAP              ( 39 )
#define OP_LINE              ( 40 )

  /* -------------------------------------------------------------------- *
   * This is the name of the variable that all rules store their return
   * code in.
   * -------------------------------------------------------------------- */
#define RULE_RETURN_VAL      "__rval"

/* ---------------------------------------------------------------------- *
 * Macro definitions
 * ---------------------------------------------------------------------- */

  /* BSKUI32 BSKRuleGetID( BSKRule* rule ) */
#define BSKRuleGetID(x)                    ((x)->id)

  /* BSKUI32 BSKRuleGetParameterCount( BSKRule* rule ) */
#define BSKRuleGetParameterCount(x)        ((x)->parameterCount)

  /* BSKUI32* BSKRuleGetParameterIDs( BSKRule* rule ) */
#define BSKRuleGetParameterIDs(x)          ((x)->parameters)

  /* BSKUI32 BSKRuleGetCodeLength( BSKRule* rule ) */
#define BSKRuleGetCodeLength(x)            ((x)->codeLength)

  /* BSKUI8* BSKRuleGetCode( BSKRule* rule ) */
#define BSKRuleGetCode(x)                  ((x)->code)

  /* BSKCHAR* BSKRuleGetSourceFile( BSKRule* rule ) */
#define BSKRuleGetSourceFile(x)            ((x)->file)

  /* BSKUI32 BSKRuleGetStartingLine( BSKRule* rule ) */
#define BSKRuleGetStartingLine(x)          ((x)->startLine)

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskrule__ BSKRule;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskrule__ {
  BSKUI8  reserved;         /* reserved -- API internal use only */
  BSKUI32 id;               /* identifier of this rule */

  BSKUI32  parameterCount;  /* # of named parameters to this rule */
  BSKUI32* parameters;      /* array of ID's of named parameters */

  BSKUI32 codeLength;       /* # of bytes in the 'code' buffer */
  BSKUI8* code;             /* the bytecode for this rule */

  BSKSymbolTable* symbols;  /* this rule's local symbol table */

  BSKCHAR* file;            /* the file that defines this rule */
  BSKUI32  startLine;       /* the line at which the rule's definition starts */

  BSKRule* next;            /* next rule in the list */
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
   * BSKNewRule
   *
   * Creates and returns a new, empty rule, with the given identifier.
   * -------------------------------------------------------------------- */
BSKRule* BSKNewRule( BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKDestroyRule
   *
   * Destroys and deallocates all memory in use by the rule, including its
   * array of named parameters, its bytecode buffer, its local symbol
   * table, and the name of the file that contained the rule.
   * -------------------------------------------------------------------- */
void BSKDestroyRule( BSKRule* rule );

  /* -------------------------------------------------------------------- *
   * BSKFindRule
   *
   * Looks for the given rule in the given list of rules.  Returns the
   * rule object if it is found, or 0 if it is not found.
   * -------------------------------------------------------------------- */
BSKRule* BSKFindRule( BSKRule* list, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * BSKSerializeRuleListOut
   *
   * Writes the given list of rules to the given stream.  Returns 0 on
   * success.
   * -------------------------------------------------------------------- */
BSKI32 BSKSerializeRuleListOut( BSKRule* list, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * BSKSerializeRuleListIn
   *
   * Reads a list of rules from the given stream and puts them each into
   * the given database object.  Returns 0 on success.
   * -------------------------------------------------------------------- */
void BSKSerializeRuleListIn( struct __bskdatabase__* db, BSKStream* stream );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKRULE_H__ */
