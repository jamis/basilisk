/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktokens.h"
#include "bskutil.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

typedef struct __bsklexemes__  BSKLexemes;

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

struct __bsklexemes__ {
  BSKCHAR*   lexeme;     /* keyword */
  BSKTokenId id;         /* token type */
};

/* ---------------------------------------------------------------------- *
 * Constants
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * This is the list of keywords and their corresponding token type for
   * all defined keywords in the Basilisk parser.  This array MUST be
   * terminated by a BSKLexemes object with both its fields set to 0.
   * -------------------------------------------------------------------- */
static BSKLexemes s_keywords[] = {
  { "unit",              TT_UNIT },
  { "attribute",         TT_ATTRIBUTE },
  { "category",          TT_CATEGORY },
  { "rule",              TT_RULE },
  { "exit",              TT_EXIT },
  { "number",            TT_NUMBER_WORD },
  { "string",            TT_STRING_WORD },
  { "boolean",           TT_BOOLEAN_WORD },
  { "thing",             TT_THING },
  { "end",               TT_END },
  { "to",                TT_TO },
  { "if",                TT_IF },
  { "while",             TT_WHILE },
  { "do",                TT_DO },
  { "for",               TT_FOR },
  { "else",              TT_ELSE },
  { "then",              TT_THEN },
  { "loop",              TT_LOOP },
  { "yes",               TT_YES },
  { "no",                TT_NO },
  { "true",              TT_TRUE },
  { "false",             TT_FALSE },
  { "on",                TT_ON },
  { "off",               TT_OFF },
  { "eq",                TT_EQ },
  { "ne",                TT_NE },
  { "lt",                TT_LT },
  { "gt",                TT_GT },
  { "ge",                TT_GE },
  { "le",                TT_LE },
  { "not",               TT_NOT },
  { "include",           TT_INCLUDE },
  { "template",          TT_TEMPLATE },
  { "and",               TT_AND },
  { "or",                TT_OR },
  { "null",              TT_NULL },
  { "in",                TT_IN },
  { "typeof",            TT_TYPEOF },
  { "forward",           TT_FORWARD },
  { "case",              TT_CASE },
  { "is",                TT_IS },
  { "default",           TT_DEFAULT },
  { "elseif",            TT_ELSEIF },
  { "array",             TT_ARRAY },
  { 0,                   0 }
};

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKTokenId BSKFindKeyword( const BSKCHAR* ident ) {
  BSKI16 i;

  for( i = 0; s_keywords[ i ].lexeme != 0; i++ ) {
    if( BSKStrCaseCmp( ident, s_keywords[ i ].lexeme ) == 0 ) {
      return s_keywords[ i ].id;
    }
  }

  return TT_IDENTIFIER;
}


void BSKGetTokenDescription( BSKTokenId type, 
                             BSKCHAR* buffer,
                             BSKUI32 length )
{
  BSKI16 i;

  switch( type ) {
    case TT_EOF:
      BSKStrNCpy( buffer, "<EOF>", length );
      break;
    case TT_EMPTY:
      BSKStrNCpy( buffer, "<err>", length );
      break;
    case TT_STRING:
      BSKStrNCpy( buffer, "<string>", length );
      break;
    case TT_NUMBER:
      BSKStrNCpy( buffer, "<number>", length );
      break;
    case TT_IDENTIFIER:
      BSKStrNCpy( buffer, "<identifier>", length );
      break;
    case TT_DICE:
      BSKStrNCpy( buffer, "<dice>", length );
      break;
    
    case TT_PUNCT_EQ:
      BSKStrNCpy( buffer, "'='", length );
      break;
    case TT_PUNCT_LPAREN:
      BSKStrNCpy( buffer, "'('", length );
      break;
    case TT_PUNCT_RPAREN:
      BSKStrNCpy( buffer, "')'", length );
      break;
    case TT_PUNCT_LBRACKET:
      BSKStrNCpy( buffer, "'['", length );
      break;
    case TT_PUNCT_RBRACKET:
      BSKStrNCpy( buffer, "']'", length );
      break;
    case TT_PUNCT_LBRACE:
      BSKStrNCpy( buffer, "'{'", length );
      break;
    case TT_PUNCT_RBRACE:
      BSKStrNCpy( buffer, "'}'", length );
      break;
    case TT_PUNCT_STAR:
      BSKStrNCpy( buffer, "'*'", length );
      break;
    case TT_PUNCT_DASH:
      BSKStrNCpy( buffer, "'-'", length );
      break;
    case TT_PUNCT_PLUS:
      BSKStrNCpy( buffer, "'+'", length );
      break;
    case TT_PUNCT_SEMICOLON:
      BSKStrNCpy( buffer, "';'", length );
      break;
    case TT_PUNCT_DOT:
      BSKStrNCpy( buffer, "'.'", length );
      break;
    case TT_PUNCT_COMMA:
      BSKStrNCpy( buffer, "','", length );
      break;
    case TT_PUNCT_PERCENT:
      BSKStrNCpy( buffer, "'%'", length );
      break;
    case TT_PUNCT_CARET:
      BSKStrNCpy( buffer, "'^'", length );
      break;
    case TT_PUNCT_OCTOTHORP:
      BSKStrNCpy( buffer, "'#'", length );
      break;
    case TT_PUNCT_DOLLAR:
      BSKStrNCpy( buffer, "'$'", length );
      break;
    case TT_PUNCT_SLASH:
      BSKStrNCpy( buffer, "'/'", length );
      break;

    default:
      BSKStrNCpy( buffer, "<unknown>", length );
      for( i = 0; s_keywords[ i ].lexeme != 0; i++ ) {
        if( s_keywords[ i ].id == type ) {
          BSKStrNCpy( buffer, s_keywords[ i ].lexeme, length );
        }
      }
  }

  buffer[ length-1 ] = 0;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

