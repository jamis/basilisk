/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bsktokens.h
 *
 * This file defines all the different tokens that the Basilisk parser
 * (bskparse.h) accepts.  This file also defines the BSKToken object (used
 * by the lexer and parser) and routines for querying tokens.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKTOKENS_H__
#define __BSKTOKENS_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * error
   * -------------------------------------------------------------------- */
#define TT_EMPTY                     (   0 )

  /* -------------------------------------------------------------------- *
   * end-of-file
   * -------------------------------------------------------------------- */
#define TT_EOF                       (   1 )

  /* -------------------------------------------------------------------- *
   * token "classes"
   * -------------------------------------------------------------------- */
#define TT_STRING                    (   2 )
#define TT_NUMBER                    (   3 )
#define TT_IDENTIFIER                (   4 )
#define TT_DICE                      (   5 )

  /* -------------------------------------------------------------------- *
   * keywords
   * -------------------------------------------------------------------- */
#define TT_UNIT                      (  11 )
#define TT_ATTRIBUTE                 (  12 )
#define TT_CATEGORY                  (  13 )
#define TT_RULE                      (  14 )
#define TT_EXIT                      (  15 )
#define TT_NUMBER_WORD               (  16 )
#define TT_STRING_WORD               (  17 )
#define TT_BOOLEAN_WORD              (  18 )
#define TT_THING                     (  19 )
#define TT_END                       (  20 )
#define TT_TO                        (  21 )
#define TT_IF                        (  22 )
#define TT_WHILE                     (  23 )
#define TT_DO                        (  24 )
#define TT_FOR                       (  25 )
#define TT_ELSE                      (  26 )
#define TT_THEN                      (  27 )
#define TT_LOOP                      (  28 )
#define TT_YES                       (  29 )
#define TT_NO                        (  30 )
#define TT_TRUE                      (  31 )
#define TT_FALSE                     (  32 )
#define TT_ON                        (  33 )
#define TT_OFF                       (  34 )
#define TT_EQ                        (  35 )
#define TT_NE                        (  36 )
#define TT_LT                        (  37 )
#define TT_GT                        (  38 )
#define TT_GE                        (  39 )
#define TT_LE                        (  40 )
#define TT_NOT                       (  41 )
#define TT_INCLUDE                   (  42 )
#define TT_TEMPLATE                  (  43 )
#define TT_AND                       (  44 )
#define TT_OR                        (  45 )
#define TT_NULL                      (  46 )
#define TT_IN                        (  47 )
#define TT_TYPEOF                    (  48 )
#define TT_FORWARD                   (  49 )
#define TT_CASE                      (  50 )
#define TT_IS                        (  51 )
#define TT_DEFAULT                   (  52 )
#define TT_ELSEIF                    (  53 )
#define TT_ARRAY                     (  54 )

  /* -------------------------------------------------------------------- *
   * punctuation
   * -------------------------------------------------------------------- */
#define TT_PUNCT_EQ                  ( 101 )
#define TT_PUNCT_LPAREN              ( 102 )
#define TT_PUNCT_RPAREN              ( 103 )
#define TT_PUNCT_LBRACKET            ( 104 )
#define TT_PUNCT_RBRACKET            ( 105 )
#define TT_PUNCT_LBRACE              ( 106 )
#define TT_PUNCT_RBRACE              ( 107 )
#define TT_PUNCT_STAR                ( 108 )
#define TT_PUNCT_DASH                ( 109 )
#define TT_PUNCT_PLUS                ( 110 )
#define TT_PUNCT_SEMICOLON           ( 111 )
#define TT_PUNCT_DOT                 ( 112 )
#define TT_PUNCT_COMMA               ( 113 )
#define TT_PUNCT_PERCENT             ( 114 )
#define TT_PUNCT_CARET               ( 115 )
#define TT_PUNCT_OCTOTHORP           ( 116 )
#define TT_PUNCT_DOLLAR              ( 117 )
#define TT_PUNCT_SLASH               ( 118 )

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bsktoken__ BSKToken;
typedef BSKI16              BSKTokenId;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bsktoken__ {
  BSKTokenId type;       /* one of the TT_xxx constants (above) */
  BSKUI32    row;        /* row in the current file where this token starts */
  BSKUI32    col;        /* column in the current file where this token starts */
  union {
    BSKUI32  identifier; /* the identifier (for TT_IDENTIFIER tokens) */
    double   dblValue;   /* floating point value (for TT_NUMBER tokens) */
    BSKCHAR* strValue;   /* string value (for TT_STRING tokens) */
    struct {
      BSKUI16 dCount;    /* dice count (for TT_DICE tokens) */
      BSKUI16 dType;     /* dice type (for TT_DICE tokens) */
    } dice;
  } data;
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
   * BSKFindKeyword
   *
   * Searches the array of keywords to see if the given string is one of
   * them.  The search is done case-insensitively, and returns the token
   * type (TT_xxx) of the keyword if the string is a keyword, or
   * TT_IDENTIFIER if it is not.
   * -------------------------------------------------------------------- */
BSKTokenId BSKFindKeyword( const BSKCHAR* ident );

  /* -------------------------------------------------------------------- *
   * BSKGetTokenDescription
   *
   * Retrieves a description of the given token.  For token "classes", this
   * is just a description of the class, not the actual data.  For
   * the keyword and punctation tokens, this is the actual keyword or
   * punctuation.  No more than 'length' bytes are copied into the buffer.
   * -------------------------------------------------------------------- */
void BSKGetTokenDescription( BSKTokenId type, 
                             BSKCHAR* buffer,
                             BSKUI32 length );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKTOKENS_H__ */
