/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bsklexer.h
 *
 * The lexer (or scanner) object defines the routines that scan the input
 * text, breaking it into tokens that the parser (bskparse.h) uses to
 * interpret the input.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKLEXER_H__
#define __BSKLEXER_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bsktokens.h"
#include "bskstream.h"
#include "bskidtbl.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

#define MAX_PUTBACK_BUFFER_LENGTH    ( 8 )

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bsklexer__ BSKLexer;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bsklexer__ {
  BSKI16   charBuffer[ MAX_PUTBACK_BUFFER_LENGTH ];    /* back character buffer */
  BSKI16   charBufferPos;                              /* current position in the back character buffer */

  BSKToken tokenBuffer[ MAX_PUTBACK_BUFFER_LENGTH ];   /* back token buffer */
  BSKI16   tokBufferPos;                               /* current position in the back token buffer */

  BSKUI32  row;                                        /* current row in source file */
  BSKUI32  col;                                        /* current column in source file */

  BSKIdentifierTable* idTable;                         /* identifier table into which to put encountered identifiers */
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
   * BSKInitLexer
   *
   * Initializes the lexer.  The identifier table is set to NULL (0).
   * -------------------------------------------------------------------- */
void BSKInitLexer( BSKLexer* lexer );

  /* -------------------------------------------------------------------- *
   * BSKNextToken
   *
   * Retrieves the next token in the given stream and stores it in the
   * given token object.  The token type is returned.  A token type of
   * TT_EMTPY is returned if an error is detected.  If any token exists
   * in the back token buffer, the most recently stored one is returned
   * and removed from the buffer.
   * -------------------------------------------------------------------- */
BSKTokenId BSKNextToken( BSKLexer* lexer,
                         BSKStream* stream,
                         BSKToken* token );

  /* -------------------------------------------------------------------- *
   * BSKPutToken
   *
   * Stores the given token in the given lexer's back token buffer.  The
   * next call to BSKNextToken for the given lexer will return this
   * token.
   * -------------------------------------------------------------------- */
void BSKPutToken( BSKLexer* lexer,
                  BSKToken* token );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKLEXER_H__ */
