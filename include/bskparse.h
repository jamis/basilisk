/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskparse.h
 *
 * The parser reads in tokens from the lexer (bsklexer.h) and interprets
 * them into data and logic.  Things (bskthing.h) and categories
 * (bskctgry.h) are read and put into the database, and rules (bskrule.h)
 * are parsed and compiled to a pseudo-bytecode for more effecient
 * execution (bskexec.h).  
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKPARSE_H__
#define __BSKPARSE_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskdb.h"
#include "bskstream.h"
#include "bsktokens.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

extern BSKCHAR* builtInFunctions[];

  /* -------------------------------------------------------------------- *
   * Parser Errors
   * -------------------------------------------------------------------- */
#define PE_NOERROR                      (     0 )  /* success */
#define PE_UNEXPECTED_TOKEN             ( -1001 )  /* unexpected token encountered */
#define PE_REDEFINED_IDENTIFIER         ( -1002 )  /* identifier has already been declared */
#define PE_UNDECLARED_IDENTIFIER        ( -1003 )  /* identifier has not been declared */
#define PE_WRONG_TYPE                   ( -1004 )  /* identifier is of the wrong type */
#define PE_BUG_DETECTED                 ( -1005 )  /* a bug has been found */
#define PE_TOO_MANY_ATTRIBUTES          ( -1006 )  /* too many values given in template definition */
#define PE_EXIT_LOOP_NOT_IN_LOOP        ( -1007 )  /* "exit loop" not in a loop context */
#define PE_CANNOT_OPEN_FILE             ( -1008 )  /* can't open file on "include" */
#define PE_FORWARD_NOT_DEFINED          ( -1009 )  /* a forwarded identifier was never actually declared */

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef BSKI32 (*BSKParserErrorHandler)( BSKI32,        /* code */
                                         BSKDatabase*,  /* database */
                                         BSKCHAR*,      /* message */
                                         BSKToken*,     /* current token */
                                         BSKUI32,       /* explanatory data */
                                         BSKNOTYPE );   /* user data */

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

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
   * BSKParse
   *
   * Parses the given stream, putting all parsed data and logic into the
   * given database object.  Errors are handled by calling the given
   * error handler, and passing the given userData to it.  BSKParse will
   * return -1 if an error has been detected, or 0 if the parse was
   * successful.  The searchPaths parameter is a list of paths to search
   * for files that are 'included'.  Each path must be terminated by a
   * a null byte ('\0') and the list itself must be terminated by two
   * such bytes.
   * -------------------------------------------------------------------- */
BSKI32 BSKParse( BSKStream* stream,
                 BSKDatabase* db,
                 BSKCHAR* searchPaths,
                 BSKParserErrorHandler err,
                 BSKNOTYPE userData );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKPARSE_H__ */
