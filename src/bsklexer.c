/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- */

#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "bskenv.h"
#include "bsktypes.h"
#include "bsktokens.h"
#include "bskstream.h"
#include "bskidtbl.h"
#include "bsklexer.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

#define MAX_NUMERIC_CONSTANT_LEN      ( 64 )
#define MAX_IDENTIFIER_LEN            ( 64 )
#define MAX_STRING_LEN                ( 4095 )

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * s_nextChar
   *
   * Returns the next character on the stream, or EOF if the end of the
   * stream has been reached.  If there are any characters on the back
   * character buffer, the most recently cached one is returned and taken
   * out of the buffer.  The column and row are also updated when a '\n'
   * character or a "\r\n" sequence is encountered.
   * -------------------------------------------------------------------- */
static BSKI16 s_nextChar( BSKLexer* lexer, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_eatWhite
   *
   * Skips over comments and white-space (spaces, tabs, and newlines).
   * Stops at EOF or at the first non-whitespace character.  Returns 0.
   * -------------------------------------------------------------------- */
static BSKI16 s_eatWhite( BSKLexer* lexer, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_eatToEOL
   *
   * Eats all characters to EOL.  Returns 0.
   * -------------------------------------------------------------------- */
static BSKI16 s_eatToEOL( BSKLexer* lexer, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_eatToEndOfComment
   *
   * Eats all characters until the end of a multiline comment is reached
   * (/ *...* /).  Note that comments may be nested.  Returns 0.
   * -------------------------------------------------------------------- */
static BSKI16 s_eatToEndOfComment( BSKLexer* lexer, BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_parseNumber
   *
   * Parses a number (integer, dice, or floating point) from the stream
   * sets the appropriate token type and value in the token object.
   * Returns -1 if an error is detected, or 0 if successful.
   * -------------------------------------------------------------------- */
static BSKI16 s_parseNumber( BSKLexer* lexer, 
                             BSKStream* stream, 
                             BSKToken* token );

  /* -------------------------------------------------------------------- *
   * s_parseString
   *
   * Parses a string ("...") from the input and sets the appropriate token
   * type and value in the given token object.  The following special
   * characters are supported:
   *   \n -- newline
   *   \r -- carriage return
   *   \t -- tab
   *   \\ -- backslash
   * Returns -1 if an error is detected, or 0 if successful.
   * -------------------------------------------------------------------- */
static BSKI16 s_parseString( BSKLexer* lexer, 
                             BSKStream* stream, 
                             BSKToken* token );

  /* -------------------------------------------------------------------- *
   * s_parseIdent
   *
   * Parses an identifier (beginning with '_' or a letter, followed by
   * zero or more '_', letters, or numbers).  Checks to see if the
   * identifier is a reserved word, and if it is, sets the token's type
   * field to that identifier.  Otherwise, sets the token's type field to
   * TT_IDENTIFIER, set's the data.identifier field to the identifier's
   * unique numeric ID, and returns 0.  Returns -1 on an error.
   * -------------------------------------------------------------------- */
static BSKI16 s_parseIdent( BSKLexer* lexer, 
                            BSKStream* stream, 
                            BSKToken* token );

  /* -------------------------------------------------------------------- *
   * s_pushChar
   *
   * Pushes the given character 'c' onto the back character buffer of the
   * given lexer.  The next call to s_nextChar will return the character
   * most recently placed on the back character buffer.
   * -------------------------------------------------------------------- */
static BSKI16 s_pushChar( BSKLexer* lexer, BSKI16 c );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

void BSKInitLexer( BSKLexer* lexer ) {
  BSKMemSet( lexer, 0, sizeof( *lexer ) );
  lexer->row = 1;
}


BSKTokenId BSKNextToken( BSKLexer* lexer,
                         BSKStream* stream,
                         BSKToken* token )
{
  BSKI16 c;

  /* if there are tokens on the back buffer, return the topmost one instead
   * of reading a new token from the stream. */

  if( lexer->tokBufferPos > 0 ) {
    lexer->tokBufferPos--;
    BSKMemCpy( token, 
               &(lexer->tokenBuffer[lexer->tokBufferPos]),
               sizeof( BSKToken ) );
    return token->type;
  }

  /* initialize the token */

  BSKMemSet( token, 0, sizeof( BSKToken ) );
  token->type = TT_EMPTY;

  /* set the row and column here in case there is an error */

  token->row = lexer->row;
  token->col = lexer->col;

  /* eat whitespace */

  if( s_eatWhite( lexer, stream ) != 0 ) {
    return TT_EMPTY;
  }

  /* set the row and column to the current row and column */

  token->row = lexer->row;
  token->col = lexer->col;

  /* get the next character and determine what needs to be done */

  c = s_nextChar( lexer, stream );
  switch( c ) {
    case -1: token->type = TT_EOF; break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      s_pushChar( lexer, c );
      s_parseNumber( lexer, stream, token );
      break;

    case '"':
      s_pushChar( lexer, c );
      s_parseString( lexer, stream, token );
      break;

    case '=': token->type = TT_PUNCT_EQ; break;
    case '(': token->type = TT_PUNCT_LPAREN; break;
    case ')': token->type = TT_PUNCT_RPAREN; break;
    case '[': token->type = TT_PUNCT_LBRACKET; break;
    case ']': token->type = TT_PUNCT_RBRACKET; break;
    case '{': token->type = TT_PUNCT_LBRACE; break;
    case '}': token->type = TT_PUNCT_RBRACE; break;
    case '*': token->type = TT_PUNCT_STAR; break;
    case '-': token->type = TT_PUNCT_DASH; break;
    case '+': token->type = TT_PUNCT_PLUS; break;
    case ';': token->type = TT_PUNCT_SEMICOLON; break;
    case '.': token->type = TT_PUNCT_DOT; break;
    case ',': token->type = TT_PUNCT_COMMA; break;
    case '%': token->type = TT_PUNCT_PERCENT; break;
    case '^': token->type = TT_PUNCT_CARET; break;
    case '#': token->type = TT_PUNCT_OCTOTHORP; break;
    case '$': token->type = TT_PUNCT_DOLLAR; break;
    case '/': token->type = TT_PUNCT_SLASH; break;
    case ':': token->type = TT_PUNCT_COLON; break;

    default:
      if( BSKIsAlpha( c ) || ( c == '_' ) ) {
        s_pushChar( lexer, c );
        s_parseIdent( lexer, stream, token );
      }
  }

  return token->type;
}


void BSKPutToken( BSKLexer* lexer,
                  BSKToken* token )
{
  if( lexer->tokBufferPos < MAX_PUTBACK_BUFFER_LENGTH ) {
    BSKMemCpy( &(lexer->tokenBuffer[ lexer->tokBufferPos ]), token, sizeof( BSKToken ) );
    lexer->tokBufferPos++;
  }
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKI16 s_nextChar( BSKLexer* lexer, BSKStream* stream ) {
  BSKI16 c;

  /* if there are any characters on the back buffer, grab the topmost and
   * return it instead of reading a new character from the stream */

  if( lexer->charBufferPos > 0 ) {
    lexer->charBufferPos--;
    return lexer->charBuffer[ lexer->charBufferPos ];
  }

  /* increment the column */

  lexer->col++;
  c = stream->getch( stream );

  /* if a newline is detected, increment the row and reset the column */

  if( c == '\r' ) {
    c = stream->getch( stream );
    if( c != '\n' ) {
      s_pushChar( lexer, c );
    } else {
      c = '\r';
    }
    lexer->row++;
    lexer->col = 0;
  } else if( c == '\n' ) {
    lexer->row++;
    lexer->col = 0;
  }

  return c;
}


static BSKI16 s_eatWhite( BSKLexer* lexer, BSKStream* stream ) {
  BSKI16 c;

  /* eat all spaces and comments */

  do {
    c = s_nextChar( lexer, stream );

    if( !isspace( c ) ) {
      if( c == '/' ) {
        c = s_nextChar( lexer, stream );
        if( c == '/' ) {
          s_eatToEOL( lexer, stream );
        } else if( c == '*' ) {
          s_eatToEndOfComment( lexer, stream );
        } else {
          s_pushChar( lexer, c );
          c = '/';
          break;
        }
      } else {
        break;
      }
    }
  } while( BSKTRUE );

  /* put the last character read on the back buffer, so that it is not
   * lost -- it must be a non-space character if it stopped the loop
   * above. */

  s_pushChar( lexer, c );
  return 0;
}


static BSKI16 s_eatToEOL( BSKLexer* lexer, BSKStream* stream ) {
  BSKI16 c;

  /* eat to the end of the line */

  do {
    c = s_nextChar( lexer, stream );
    if( c == -1 ) {
      return -1;
    } else if( c == '\r' ) {
      c = s_nextChar( lexer, stream );
      if( c != '\n' ) {
        s_pushChar( lexer, c );
        c = '\r';
      }
      break;
    } else if( c == '\n' ) {
      break;
    }
  } while( 1 );

  return 0;
}


static BSKI16 s_parseNumber( BSKLexer* lexer, BSKStream* stream, BSKToken* token ) {
  BSKBOOL  seenDot;
  BSKBOOL  seenD;
  BSKBOOL  seenSlash;
  BSKCHAR  buffer[ MAX_NUMERIC_CONSTANT_LEN + 1 ];
  BSKCHAR* p;
  BSKI16   c;
  double   numerator;

  /* initialize our flags */

  numerator = 0;
  seenSlash = seenD = seenDot = BSKFALSE;

  p = buffer;
  do {
    c = s_nextChar( lexer, stream );
    if( BSKIsDigit( c ) ) {
      *p = c;
      p++;
    } else if( c == '.' ) {
      
      /* if we've already seen a 'd' or a decimal point, then it as an error
       * to see a decimal point here.  Otherwise, set the seenDot flag to
       * true and eat the decimal point. */

      if( seenD || seenDot ) {
        break;
      } else {
        seenDot = BSKTRUE;
        *p = '.';
        p++;
      }
    } else if( ( c == 'd' ) || ( c == 'D' ) ) {

      /* if we've seen a decimal point, a 'd', or a slash '/' already, then
       * it is an error to see a 'd' here.  Otherwise, eat the 'd' and set
       * the seenD flag. */

      if( seenD || seenDot || seenSlash ) {
        break;
      } else {
        seenD = BSKTRUE;
        *p = 'd';
        p++;
      }
    } else if( c == '/' ) {
    
      /* if we've seen a 'd' or a slash '/' already, then it is an error to
       * see a slash '/' here.  Otherwise, set the numerator to be the
       * number read so far, reset the buffer, and set the seenSlash flag
       * to true. */

      if( seenD || seenSlash ) {
        break;
      }
      *p = 0;
      numerator = BSKAtoF( buffer );
      seenSlash = BSKTRUE;
      p = buffer;
      seenDot = BSKFALSE;
    } else {
      break;
    }

    /* if the number is longer than the maximum length, just quit */

    if( p - buffer >= MAX_NUMERIC_CONSTANT_LEN ) {
      break;
    }
  } while( BSKTRUE );

  /* put the last character read back on the stream, to be read later */

  s_pushChar( lexer, c );
  *p = 0;

  /* if we've seen a 'd', then we just read a dice value */

  if( seenD ) {
    token->type = TT_DICE;
    p = BSKStrChr( buffer, 'd' );
    *p = 0;
    p++;
    if( *p == 0 ) {
      token->type = TT_EMPTY;
      return -1;
    }
    token->data.dice.dCount = BSKAtoI( buffer );
    token->data.dice.dType  = BSKAtoI( p );
  } else {

    /* otherwise it's a number.  If we've seen a slash, then we need to
     * divide the numerator value by the number in the buffer.  If no
     * number is in the buffer, then it wasn't a "fraction" value we read,
     * it was a division operation, and we need to put the slash back on
     * the stream. */

    token->type = TT_NUMBER;
    if( seenSlash ) {
      double tmp;

      if( buffer[0] == 0 ) {
        BSKStrCpy( buffer, "1" );
        s_pushChar( lexer, '/' );
      }

      tmp = BSKAtoF( buffer );
      if( BSKFAbs( tmp ) < 0.0001 ) {
        token->data.dblValue = 0;
      } else {
        token->data.dblValue = numerator / tmp;
      }
    } else {
      token->data.dblValue = BSKAtoF( buffer );
    }
  }

  return 0;
}


static BSKI16 s_parseString( BSKLexer* lexer, BSKStream* stream, BSKToken* token ) {
  static BSKCHAR buffer[ MAX_STRING_LEN + 1 ];
  BSKCHAR* p;
  BSKI16   c;

  if( s_nextChar( lexer, stream ) != '"' ) {
    return -1;
  }

  /* read all characters up to the next unescaped '"' character */

  p = buffer;
  do {
    c = s_nextChar( lexer, stream );
    if( c == '"' ) {
      break;
    } else if( c == '\\' ) {
      c = s_nextChar( lexer, stream );
      switch( c ) {
        case 'n': *p = '\n'; break;
        case 'r': *p = '\r'; break;
        case 't': *p = '\t'; break;
        default:
          *p = c;
      }
    } else if( c == '\n' || c == '\r' ) {
      return -1;
    } else {
      *p = c;
    }
    p++;
  } while( BSKTRUE );

  *p = 0;

  token->type = TT_STRING;
  token->data.strValue = buffer;

  return 0;
}


static BSKI16 s_parseIdent( BSKLexer* lexer, BSKStream* stream, BSKToken* token ) {
  static BSKCHAR buffer[ MAX_IDENTIFIER_LEN + 1 ];
  BSKCHAR* p;
  BSKI16   c;

  p = buffer;
  do {
    c = s_nextChar( lexer, stream );
    if( BSKIsAlNum( c ) || c == '_' ) {
      *p = c;
    } else {
      break;
    }
    p++;
  } while( BSKTRUE );

  s_pushChar( lexer, c );
  *p = 0;

  /* is the identifier a keyword? */

  token->type = BSKFindKeyword( buffer );

  /* if not, add it to (or find it in) the identifier table */

  if( token->type == TT_IDENTIFIER ) {
    if( lexer->idTable != NULL ) {
      token->data.identifier = BSKAddIdentifier( lexer->idTable,
                                                 buffer );
    } else {
      token->data.strValue = buffer;
    }
  }

  return 0;
}


static BSKI16 s_pushChar( BSKLexer* lexer, BSKI16 c ) {
  if( lexer->charBufferPos < MAX_PUTBACK_BUFFER_LENGTH ) {
    lexer->charBuffer[ lexer->charBufferPos++ ] = c;
  }

  return c;
}


static BSKI16 s_eatToEndOfComment( BSKLexer* lexer, BSKStream* stream ) {
  BSKUI32 nestingDepth;
  BSKI16  c;

  nestingDepth = 1;
  do {
    c = s_nextChar( lexer, stream );
    if( c == '*' ) {
      c = s_nextChar( lexer, stream );
      if( c == '/' ) {
        nestingDepth--;
        if( nestingDepth < 1 ) {
          break;
        }
      } else {
        s_pushChar( lexer, c );
      }
    } else if( c == '/' ) {
      c = s_nextChar( lexer, stream );
      if( c == '*' ) {
        nestingDepth++;
      }
    } else if( c < 0 ) {
      return -1;
    }
  } while( BSKTRUE );

  return 0;
}
