/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * A note on the implementation:  you will notice a BSKBitSet object named
 * "follow" being passed to the parser functions.  This set represents the
 * set of tokens that may follow the given grammar rule, and is used in
 * error handling when an unexpected token is encountered.  When an
 * unexpected token is encountered, all tokens up to EOF or any token in
 * the follow set are skipped, in an effort to get the parser "back on
 * track."
 *
 * See the BNF for the Basilisk script grammar for more information on
 * the follow sets.
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"
#include "bskatdef.h"
#include "bskutdef.h"
#include "bskdb.h"
#include "bskparse.h"
#include "bsktokens.h"
#include "bsklexer.h"
#include "bsksymtb.h"
#include "bskbtset.h"
#include "bskrule.h"
#include "bskutil.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

typedef struct __bskparser__         BSKParser;
typedef struct __bskidentlist__      BSKIdentList;
typedef struct __bskwtidentlist__    BSKWeightedIdentList;
typedef struct __bsksubnumber__      BSKSubNumber;

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

struct __bskparser__ {
  BSKStream* stream;             /* stream being parsed */
  BSKLexer   lexer;              /* lexer doing the scanning */

  BSKSymbolTable* symbols;       /* where global symbols are defined */

  BSKDatabase* db;               /* the database recieving all the entities */

  BSKBOOL error;                 /* the error flag */
  BSKParserErrorHandler handler; /* the error handler */
  BSKNOTYPE userData;            /* application-defined data */

  BSKToken currentToken;         /* current token */

  BSKUI32 exitFixups[ 256 ];     /* backfix locations for forward jumps (in rules) */
  BSKUI32 eFixups;               /* number of exit fixups */

  BSKBOOL  inLoop;               /* are we within a loop context? */
  BSKUI32  line;                 /* line currently being parsed */
  BSKRule* currentRule;          /* rule currently being parsed */

  BSKCHAR* currentFile;          /* file currently being parsed */
  BSKCHAR* searchPaths;          /* the search paths for included files */

  BSKCHAR** includedFiles;       /* the list of files that have been included */
  BSKUI16   includedCount;       /* number of files that have been included */
};


struct __bskidentlist__ {
  BSKUI32 id;
  BSKIdentList* next;
};


struct __bskwtidentlist__ {
  BSKUI32 id;
	BSKUI16 weight;
  BSKWeightedIdentList* next;
};


struct __bsksubnumber__ {
  BSKUI16 type;
  BSKCHAR op;
  union {
    BSKFLOAT floatVal;
    struct {
      BSKI16  count;
      BSKUI16 type;
      BSKI16  modifier;
    } dice;
  } data;
};

/* ---------------------------------------------------------------------- *
 * Private Constants
 * ---------------------------------------------------------------------- */

#if defined( __unix__ )
#define FILE_SEPARATOR     "/"
#else
#define FILE_SEPARATOR     "\\"
#endif

  /* -------------------------------------------------------------------- *
   * This is the list of built-in fuctions that the VM recognizes.  If you
   * add a built-in function, be sure to add it to this list, as well as
   * to the VM, so that the parser can flag the symbol as a built-in rather
   * than a user-defined rule.
   * -------------------------------------------------------------------- */
BSKCHAR* builtInFunctions[] = {
  "Int",
  "Floor",
  "Any",
  "Exists",
  "Duplicate",
  "NewThing",
  "NewCategory",
  "Random",
  "Sqrt",
  "Add",
  "Remove",
  "Empty",
  "Removeall",
  "Count",
  "Get",
  "WeightOf",
  "IndexOf",
  "Union",
  "Intersection",
  "Subtract",
  "Instr",
  "Replace",
  "Mid",
  "Ln",
  "SearchCategory",
  "Has",
  "Parameter",
  "AttributeOf",
  "AttributeNameOf",
  "AttributeValueOf",
  "Dice",
  "Print",
  "NewArray",
  "Sort",
  "Length",
  "MagnitudeOf",
  "UnitsOf",
  "SetUnits",
  "Eval",
  "ConvertUnits",
  "AddressOf",
  "TotalWeightOf",
  "GetByWeight",
  "DiceCount",
  "DiceType",
  "DiceModType",
  "DiceModifier",
  "UpperCase",
  "LowerCase",
  0
};

  /* -------------------------------------------------------------------- *
   * Grammar symbol "heads" -- didn't wind up using them as extensively as
   * I should have -- a nice FIXME would be to use them more and improve
   * the error handling by so doing.
   * -------------------------------------------------------------------- */
#define hCONFIGFILE         hConfigEntry,TT_EOF
#define hCONFIGENTRY        hATTRIBUTE,hUNIT,hTHING,hCATEGORY,hTEMPLATE,hRULE,hMETA
#define hATTRIBUTE          TT_ATTRIBUTE
#define hUNIT               TT_UNIT
#define hTHING              TT_IDENTIFIER
#define hCATEGORY           TT_CATEGORY
#define hTEMPLATE           TT_TEMPLATE
#define hRULE               TT_RULE
#define hMETA               TT_PUNCT_OCTOTHORP
#define hTYPE               TT_STRING_WORD,TT_NUMBER_WORD,TT_BOOLEAN_WORD,TT_THING,TT_CATEGORY,TT_RULE
#define hBOOLEAN            TT_YES,TT_NO,TT_ON,TT_OFF,TT_TRUE,TT_FALSE
#define hNUMBER             TT_PUNCT_STAR,TT_PUNCT_PLUS,TT_PUNCT_DASH,TT_NUMBER,TT_DICE
#define hLOGEXPR            TT_IDENTIFIER,TT_NUMBER,TT_STRING,hBOOLEAN,TT_PUNCT_LPAREN,TT_NOT,TT_NULL,TT_CATEGORY,TT_RULE,TT_THING,TT_NUMBER_WORD,TT_STRING_WORD,TT_BOOLEAN_WORD
#define hSTATEMENT          TT_IDENTIFIER,TT_WHILE,TT_DO,TT_IF,TT_EXIT,TT_FOR

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * s_parseSubStream
   *
   * Clones the given parser and parses the given stream by calling
   * s_parseConfigFile.  Returns what s_parseConfigFile returns.
   * -------------------------------------------------------------------- */
static BSKI32 s_parseSubStream( BSKParser* parser, 
                                BSKStream* stream,
                                BSKCHAR* streamName );

  /* -------------------------------------------------------------------- *
   * s_parseConfigFile
   *
   * Top level of the parse tree -- parses a configuration file (ie, a
   * Basilisk data file).  This is accomplished by calling 
   * s_parseConfigEntry until EOF is reached.  Returns -1 on error, or 0
   * on success.
   *
   * config-file := config-entry*
   * -------------------------------------------------------------------- */
static BSKI32 s_parseConfigFile( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseConfigEntry
   *
   * Parses a single configuration file entry -- a template, a thing, a
   * category, an attribute definition, a unit definition, a forward
   * declaration, rule, or parser directive.  This is done by calling the 
   * appropriate parser routine.
   *
   * config-entry := unit-def
   *               | attribute-def
   *               | thing-def
   *               | category-def
   *               | rule-def
   *               | template-def
   *               | meta-def
   *               | forward-def
   * -------------------------------------------------------------------- */
static BSKI32 s_parseConfigEntry( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseUnitDef
   *
   * Parses a single unit definition.
   *
   * unit-def := "unit" ident { "=" num ident } ";"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseUnitDef( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseAttributeDef
   *
   * Parses a single attribute definition.
   *
   * attribute-def := "attribute" ident type
   * -------------------------------------------------------------------- */
static BSKI32 s_parseAttributeDef( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseThingDef
   *
   * Parses a single Thing definition.
   *
   * thing-def := ident { "(" ident-list ")" } { property-list } "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseThingDef( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseCategoryDef
   *
   * Parses a single Category definition.
   *
   * category-def := "category" ident category-body "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseCategoryDef( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseTemplateDef
   *
   * Parses a single template definition.
   *
   * template-def := "template" { "(" ident-list ")" } "{" ident-list "}" 
   *                 template-entry-list
   *                 "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseTemplateDef( BSKParser* parser, BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleDef
   *
   * Parses a single rule definition.
   *
   * rule-def := "rule" ident "(" { ident-list } ")" 
   *             { rule-statement-list } 
   *             "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleDef( BSKParser* parser, BSKBitSet* follow );

 /* -------------------------------------------------------------------- *
   * s_parseIdentList
   *
   * Parses a space-delimited list of identifiers.  Note that the created
   * identifier list will list the identifiers in reverse order from the
   * way they were parsed.  See r_reverseIdentList for information on how
   * to reverse an identifier list.
   *
   * ident-list := ident
   *             | ident ident-list
   * -------------------------------------------------------------------- */
static BSKIdentList* s_parseIdentList( BSKParser* parser, 
                                       BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseWeightedIdentList
   *
   * Parses a space-delimited list of identifiers.  Note that the created
   * identifier list will list the identifiers in reverse order from the
   * way they were parsed.  Each identifier may be preceded by an integer
	 * which will be treated as its weight.
   *
   * weighted-ident-list := weighted-ident-list-entry
   *                      | weighted-ident-list-entry ident-list
   * -------------------------------------------------------------------- */
static BSKWeightedIdentList* s_parseWeightedIdentList( BSKParser* parser, 
                                                       BSKBitSet* follow );

   /* -------------------------------------------------------------------- *
   * s_parsePropertyList
   *
   * Parses a list of properties.
   *
   * property-list := property
   *                | property property-list
   * property := "." ident property-value
   * -------------------------------------------------------------------- */
static BSKI32 s_parsePropertyList( BSKParser* parser, 
                                   BSKThing* thing, 
                                   BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseValue
   *
   * Parses a value (string, number, dice, etc.).
   *
   * property-value := number
   *                 | string
   *                 | boolean
   *                 | ident
   *                 | "{" property-list "}"
   *                 | "(" category-body ")"
   *                 | "null"
   *
   * value := property-value
   *        | "(" logexpr ")"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseValue( BSKParser* parser,
                            BSKValue* value,
                            BSKUI32 expectedType,
                            BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseNumber
   *
   * Parses a number or dice value from the stream.
   *
   * number := {*}number2 { {+|-}? number } { ident }
   * number2 := {+|-}num
   *          | {+|-}dice
   *          | {+|-}num "/" num
   * num := digit+ { "." digit* }
   * dice := num "d" num
   * -------------------------------------------------------------------- */
static BSKI32 s_parseNumber( BSKParser* parser,
                             BSKValue* value,
                             BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseSubNumber
   *
   * Parses a number or dice value from the stream.
   *
   * number := {*}number2 { {+|-}? number } { ident }
   * number2 := {+|-}num
   *          | {+|-}dice
   *          | {+|-}num "/" num
   * num := digit+ { "." digit* }
   * dice := num "d" num
   * -------------------------------------------------------------------- */
static BSKI32 s_parseSubNumber( BSKParser* parser,
                                BSKSubNumber* subn,
                                BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseCategoryBody
   *
   * Parses a series of members of a category and puts them into the given
   * category.
   *
   * category-body := category-entry
   *                | category-entry category-body
   * -------------------------------------------------------------------- */
static BSKI32 s_parseCategoryBody( BSKParser* parser,
                                   BSKCategory* category,
                                   BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseCategoryEntry
   *
   * Parses a single member of a category and puts it into the category.
   *
   * category-entry := { "[" num "]" } category-entry2
   * category-entry2 := ident
   *                  | "{" property-list "}"
   *                  | "(" category-body ")"
   *                  | "null"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseCategoryEntry( BSKParser* parser,
                                    BSKCategory* category,
                                    BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseTemplateEntry
   *
   * Parses a single member of a template.  An value entry of '$' indicates
   * that the corresponding value does not exist in the templated object.
   *
   *  template-entry := ident { "(" ident-list ")" } 
   *                    "{" template-value-list "}"
   *  template-value-list := template-value
   *                       | template-value template-value-list
   *  template-value := property-value
   *                  | "$"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseTemplateEntry( BSKParser* parser,
                                    BSKWeightedIdentList* categories,
                                    BSKIdentList* attributes,
                                    BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatementList
   *
   * Parses and compiles a list of statements in a rule.
   *
   * rule-statement-list := rule-statement
   *                      | rule-statement rule-statement-list
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatementList( BSKParser* parser,
                                        BSKRule* rule,
                                        BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement
   *
   * Parses and compiles a single rule statement.
   *
   * rule-statement := ident ident-tail { "=" logexpr } ";"
   *                 | "if" logexpr "then" rule-statement-list if2 "end"
   *                 | "while" logexpr "do" rule-statement-list "end"
   *                 | "do" rule-statement-list "loop" "while" logexpr ";"
   *                 | "for" ident for2 "do" rule-statement-list "end"
   *                 | "exit" exit-block ";"
   *                 | "case" logexpr case-list { "default" rule-statement-list } "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement( BSKParser* parser,
                                    BSKRule* rule,
                                    BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_Identifier
   *
   * Parses and compiles any rule statement that begins with an
   * identifier.  This typically includes function calls and assignment
   * statements.
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_Identifier( BSKParser* parser, 
                                               BSKRule* rule, 
                                               BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_Case
   *
   * Parses and compiles a "case" rule statement.
   *
   * "case" logexpr case-list { "default" rule-statement-list } "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_Case( BSKParser* parser, 
                                         BSKRule* rule, 
                                         BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_CaseList
   *
   * Parses and compiles the entries in a case statement.
   *
   * case-list := case-statement
   *            | case-statement case-list
   * case-statement := "is" { "not" } factor2 "then" rule-statement-list
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_CaseList( BSKParser* parser, 
                                             BSKRule* rule, 
                                             BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_If
   *
   * Parses and compiles an "if" rule statement.
   *
   * "if" logexpr "then" rule-statement-list if2 "end"
   * if2 := ()
   *      | "elseif" logexpr "then" rule-statement-list if2
   *      | "else" rule-statement
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_If( BSKParser* parser,
                                       BSKRule* rule, 
                                       BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_While
   *
   * Parses and compiles a "while" rule statement.
   *
   * "while" logexpr "do" rule-statement-list "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_While( BSKParser* parser, 
                                          BSKRule* rule, 
                                          BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_Do
   *
   * Parses and compiles a "do" rule statement.
   *
   * "do" rule-statement-list "loop" "while" logexpr ";"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_Do( BSKParser* parser, 
                                       BSKRule* rule, 
                                       BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_For
   *
   * Parses and compiles a "for" rule statement.
   *
   * "for" ident for2 "do" rule-statement-list "end"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_For( BSKParser* parser, 
                                        BSKRule* rule, 
                                        BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseRuleStatement_Exit
   *
   * Parses and compiles "exit" rule statements.  Exit statements are used
   * to exit either a rule ("exit rule") or the innermost loop being
   * executed ("exit loop").
   *
   * "exit" exit-block ";"
   * exit-block := "loop"
   *             | "rule"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseRuleStatement_Exit( BSKParser* parser, 
                                         BSKRule* rule, 
                                         BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseIdentHead
   *
   * Parses a sequence of identifier { "." identifier { "." identifier { ... } } }
   * tokens.
   *
   * ident-head := ident
   *             | ident "." ident-head
   * -------------------------------------------------------------------- */
static BSKIdentList* s_parseIdentHead( BSKParser* parser,
                                       BSKRule* rule,
                                       BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseParameterList
   *
   * Parses and compiles a sequence of comma delimited values as
   * parameters to a function or rule.
   *
   * parameter-list := logexpr
   *                 | logexpr "," parameter-list
   * -------------------------------------------------------------------- */
static BSKUI32 s_parseParameterList( BSKParser* parser,
                                     BSKRule* rule,
                                     BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseLogicalExpr
   *
   * Parses and compiles a "logical expression" (see the BNF for a full
   * description of expression syntax).
   *
   * logexpr := logexpr2
   *          | logexpr2 "or" logexpr
   * -------------------------------------------------------------------- */
static BSKI32 s_parseLogicalExpr( BSKParser* parser,
                                  BSKRule* rule,
                                  BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseLogicalExpr2
   *
   * Parses and compiles a "logical expression" (see the BNF for a full
   * description of expression syntax).
   *
   * logexpr2 := logexpr3
   *           | logexpr3 "and" logexpr2
   * -------------------------------------------------------------------- */
static BSKI32 s_parseLogicalExpr2( BSKParser* parser,
                                   BSKRule* rule,
                                   BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseLogicalExpr3
   *
   * Parses and compiles a "logical expression" (see the BNF for a full
   * description of expression syntax).
   *
   * logexpr3 := expression
   *           | expression "eq" expression
   *           | expression "ne" expression
   *           | expression "lt" expression
   *           | expression "gt" expression
   *           | expression "le" expression
   *           | expression "ge" expression
   *           | expression "typeof" type
   * -------------------------------------------------------------------- */
static BSKI32 s_parseLogicalExpr3( BSKParser* parser,
                                   BSKRule* rule,
                                   BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseExpression
   *
   * Parses and compiles an "expression" (see the BNF for a full
   * description of expression syntax).
   *
   * expression := term
   *             | term "+" expression
   *             | term "-" expression
   * -------------------------------------------------------------------- */
static BSKI32 s_parseExpression( BSKParser* parser,
                                 BSKRule* rule,
                                 BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseTerm
   *
   * Parses and compiles a "term" (see the BNF for a full description of 
   * expression syntax).
   *
   * term := factor
   *       | factor "*" term
   *       | factor "/" term
   *       | factor "%" term
   * -------------------------------------------------------------------- */
static BSKI32 s_parseTerm( BSKParser* parser,
                           BSKRule* rule,
                           BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseFactor
   *
   * Parses and compiles a "factor" (see the BNF for a full description of 
   * expression syntax).
   *
   * factor := factor2
   *         | factor2 "^" factor
   * -------------------------------------------------------------------- */
static BSKI32 s_parseFactor( BSKParser* parser,
                             BSKRule* rule,
                             BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseFactor2
   *
   * Parses and compiles a "factor" (see the BNF for a full description of 
   * expression syntax).
   *
   * factor2 := ident ident-tail
   *          | num
   *          | string
   *          | boolean
   *          | "(" logexpr ")"
   *          | "not" factor2
   *          | "null"
   * -------------------------------------------------------------------- */
static BSKI32 s_parseFactor2( BSKParser* parser,
                              BSKRule* rule,
                              BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseFactor2_Identifier
   *
   * Parses and compiles a "factor" that begins with an identifier.  This
   * is typically an ident-head followed by an ident-tail (see the BNF
   * for a full description).
   *
   * ident-tail := ()
   *             | "(" { paremeter-list } ")" ident-tail
   *             | "." ident ident-tail
   *             | "[" expression "]" ident-tail
   * -------------------------------------------------------------------- */
static BSKI32 s_parseFactor2_Identifier( BSKParser* parser,
                                         BSKRule* rule,
                                         BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_parseFactor2_Number
   *
   * Parses and compiles a number value in an expression.  (see the BNF
   * for a full description).
   * -------------------------------------------------------------------- */
static BSKI32 s_parseFactor2_Number( BSKParser* parser,
                                     BSKRule* rule,
                                     BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_metaDef
   *
   * Parses a "meta" definition -- a directive for the compiler (like
   * "include").
   *
   * meta-def := "#" meta-def2
   * meta-def2 := "include" string
   * -------------------------------------------------------------------- */
static BSKI32 s_metaDef( BSKParser* parser,
                         BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_forwardDef
   *
   * Parses a forward declaration of a thing, category, or rule.
   *
   * forward-def := "forward" forward-decl
   * forward-decl := "thing" ident
   *               | "category" ident
   *               | "rule" ident
   * -------------------------------------------------------------------- */
static BSKI32 s_forwardDef( BSKParser* parser,
                            BSKBitSet* follow );

  /* -------------------------------------------------------------------- *
   * s_getToken
   *
   * Retrieves the next token from the lexer and populates the parser's
   * currentToken field.  Returns the type of the retrieved token.
   * -------------------------------------------------------------------- */
static BSKTokenId s_getToken( BSKParser* parser );

  /* -------------------------------------------------------------------- *
   * s_error
   *
   * If the parser's error handling function has been set, it is called.
   * The parser's error flag is set.
   * -------------------------------------------------------------------- */
static void s_error( BSKParser* parser, 
                     BSKI32 code, 
                     BSKUI32 data,
                     BSKBitSet* valid );

  /* -------------------------------------------------------------------- *
   * s_eatToken
   *
   * If the currentToken is not of type 'type', a PE_UNEXPECTED_TOKEN
   * error is raised and all tokens up until any in the 'valid' set are
   * thrown away (see s_throwAwayUntil).
   * -------------------------------------------------------------------- */
static BSKI32 s_eatToken( BSKParser* parser, 
                          BSKTokenId type, 
                          BSKBitSet* valid );

  /* -------------------------------------------------------------------- *
   * s_throwAwayUntil
   *
   * Throws away tokens until a token is found that it is in the given
   * set.
   * -------------------------------------------------------------------- */
static void s_throwAwayUntil( BSKParser* parser, BSKBitSet* valid );

  /* -------------------------------------------------------------------- *
   * s_reverseIdentList
   *
   * Reverses the order of identifiers in the given list.
   * -------------------------------------------------------------------- */
static BSKIdentList* s_reverseIdentList( BSKIdentList* list );

  /* -------------------------------------------------------------------- *
   * s_destroyIdentList
   *
   * Destroys and deallocates the given identifier list.
   * -------------------------------------------------------------------- */
static void s_destroyIdentList( BSKIdentList* list );

  /* -------------------------------------------------------------------- *
   * s_destroyWeightedIdentList
   *
   * Destroys and deallocates the given weighted identifier list.
   * -------------------------------------------------------------------- */
static void s_destroyWeightedIdentList( BSKWeightedIdentList* list );

  /* -------------------------------------------------------------------- *
   * s_addToIdentList
   *
   * Adds the given identifier ID to the given identifier list, and returns
   * the new list (with 'id' at the head of the list).
   * -------------------------------------------------------------------- */
static BSKIdentList* s_addToIdentList( BSKIdentList* list, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * s_getIdentListLength
   *
   * Returns the number of elements in the given list, or 0 if the list is
   * NULL.
   * -------------------------------------------------------------------- */
static BSKUI32 s_getIdentListLength( BSKIdentList* list );

  /* -------------------------------------------------------------------- *
   * s_emitOp
   *
   * Emits the given operand at the current position in the code of the
   * given rule, and increments the current position by 1.
   * -------------------------------------------------------------------- */
static void s_emitOp( BSKRule* rule, BSKUI32 op );

  /* -------------------------------------------------------------------- *
   * s_emitID
   *
   * Emits the given identifier at the current position in the code of the
   * given rule, and increments the current position by the size of an
   * identifier.
   * -------------------------------------------------------------------- */
static void s_emitID( BSKRule* rule, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * s_emitWord
   *
   * Emits the given word value at the current position in the code of the
   * given rule, and increments the current position by 2.
   * -------------------------------------------------------------------- */
static void s_emitWord( BSKRule* rule, BSKUI16 word );

  /* -------------------------------------------------------------------- *
   * s_emitDWord
   *
   * Emits the given double word value at the current position in the code 
   * of the given rule, and increments the current position by 4.
   * -------------------------------------------------------------------- */
static void s_emitDWord( BSKRule* rule, BSKUI32 dword );

  /* -------------------------------------------------------------------- *
   * s_emitDWord
   *
   * Emits the given byte value at the current position in the code of the 
   * given rule, and increments the current position by 1.
   * -------------------------------------------------------------------- */
static void s_emitByte( BSKRule* rule, BSKUI8 byte );

  /* -------------------------------------------------------------------- *
   * s_emitBytes
   *
   * Emits the given bytes at the current position in the code of the given
   * rule, and increments the current position by 'len'.
   * -------------------------------------------------------------------- */
static void s_emitBytes( BSKRule* rule, BSKNOTYPE buffer, BSKUI32 len );

  /* -------------------------------------------------------------------- *
   * s_forwardJump
   *
   * Sets the next 4 bytes to 0, increments the position by 4, and returns
   * the original position.  This is for use in marking places that need
   * to reference the address of a later position.  See also s_backFix.
   * -------------------------------------------------------------------- */
static BSKUI32 s_forwardJump( BSKRule* rule );

  /* -------------------------------------------------------------------- *
   * s_backFix
   *
   * Writes the current position to the address in the rule's code indicated
   * by 'at.'  This is for use in patching up blanks left by s_forwardJump,
   * after the target address has been reached.
   * -------------------------------------------------------------------- */
static void s_backFix( BSKRule* rule, BSKUI32 at );

  /* -------------------------------------------------------------------- *
   * s_emitString
   *
   * Writes the given string to the current position in the given rule's
   * code, and increments the position by BSKStrLen(string)+1.
   * -------------------------------------------------------------------- */
static void s_emitString( BSKRule* rule, BSKCHAR* string );

  /* -------------------------------------------------------------------- *
   * s_emitIdentHead
   *
   * Emits the identifier list as a group of dereferences.  If the list
   * contains only one identifier, it is emitted as an LID operation.
   * If the list contains two or more identifiers, it is emitted as two
   * LID operations, followed by a DREF, followed (depending on how many
   * identifiers there are) by another LID and DREF, etc.
   * -------------------------------------------------------------------- */
static void s_emitIdentHead( BSKRule* rule, BSKIdentList* list );

  /* -------------------------------------------------------------------- *
   * s_isAttributeID
   *
   * Returns BSKTRUE if the given id identifies an attribute definition,
   * BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
static BSKBOOL s_isAttributeID( BSKDatabase* db, BSKUI32 id );

  /* -------------------------------------------------------------------- *
   * s_fileIsIncluded
   *
   * Returns BSKTRUE if the given file (identified by name) has already
   * been included.
   * -------------------------------------------------------------------- */
static BSKBOOL s_fileIsIncluded( BSKParser* parser, BSKCHAR* name );

  /* -------------------------------------------------------------------- *
   * s_addFileToIncludedList
   *
   * Adds the given file name to the parser's list of included files.
   * -------------------------------------------------------------------- */
static void s_addFileToIncludedList( BSKParser* parser, BSKCHAR* name );

  /* -------------------------------------------------------------------- *
   * s_findStreamInPaths
   *
   * Searches the search paths given to the parser for a stream (file) of
   * the given name.  Returns the name (with path) of the file that was
   * found, or NULL if no such file was found.
   * -------------------------------------------------------------------- */
static BSKCHAR* s_findStreamInPaths( BSKParser* parser,
                                     BSKCHAR* name,
                                     BSKCHAR* dest,
                                     BSKUI32 destLen );

  /* -------------------------------------------------------------------- *
   * s_fileExists
   *
   * Returns BSKTRUE if the specified file exists and is readable, and
   * BSKFALSE otherwise.
   * -------------------------------------------------------------------- */
static BSKBOOL s_fileExists( BSKCHAR* file );

  /* -------------------------------------------------------------------- *
   * s_addMemberToCategories
   *
   * Adds the given category member to all categories in the given
	 * weighted list.
   * -------------------------------------------------------------------- */
static void s_addMemberToCategories( BSKParser* parser,
		                                 BSKCategoryMember* member,
																		 BSKWeightedIdentList* list );

/* ---------------------------------------------------------------------- *
 * Private Macros
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKI32 BSKParse( BSKStream* stream,
                 BSKDatabase* db,
                 BSKCHAR* searchPaths,
                 BSKParserErrorHandler err,
                 BSKNOTYPE userData )
{
  BSKParser parser;
  BSKI32    rc;
  BSKBitSet follow;
  BSKSymbolTableEntry* sym;

  /* initialize the parser */

  BSKMemSet( &parser, 0, sizeof( parser ) );
  parser.stream = stream;
  parser.db = db;
  parser.searchPaths = searchPaths;
  parser.handler = err;
  parser.userData = userData;

  /* initialize the lexer.  Have the lexer use the identifier table
   * in the given database. */

  BSKInitLexer( &(parser.lexer) );
  parser.lexer.idTable = db->idTable;

  /* have the parser use the symbol table in the given database */

  parser.symbols = db->symbols;

  /* install the built-in functions */

  for( rc = 0; builtInFunctions[ rc ] != 0; rc++ ) {
    BSKI32 id;

    id = BSKAddIdentifier( db->idTable, builtInFunctions[ rc ] );
    if( id < 0 ) {
      /* FIXME: error handling/reporting */
      return -1;
    }

    BSKAddSymbol( parser.symbols, ST_BUILTIN, SF_NONE, id );
  }

  /* set the follow set to EOF (this is the only valid token that can
   * follow a configuration file.  This means a file cannot have any
   * garbage at the end of it. */

  BSKClearBitSet( &follow );
  BSKSetBit( &follow, TT_EOF );

  /* get the first token (to prime the parser) and begin parsing the
   * configuration file */

  s_getToken( &parser );
  rc = s_parseConfigFile( &parser, &follow );

  /* make sure there are no undefined symbols that were declared 
   * 'forward' */
  for( sym = parser.symbols->list; sym != 0; sym = sym->next ) {
    if( ( sym->flags & SF_FORWARD_DECL ) != 0 ) {
      s_error( &parser, PE_FORWARD_NOT_DEFINED, sym->id, 0 );
    }
  }

  /* deallocate the buffer containing the name of the current file */

  if( parser.currentFile != 0 ) {
    BSKFree( parser.currentFile );
  }

  /* deallocate all of the included file names */
  
  if( parser.includedCount > 0 ) {
    for( rc = 0; rc < parser.includedCount; rc++ ) {
      BSKFree( parser.includedFiles[ rc ] );
    }
    BSKFree( parser.includedFiles );
  }

  /* if there was an error, return -1 */

  if( parser.error ) {
    return -1;
  }

  return 0;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKI32 s_parseSubStream( BSKParser* parser, BSKStream* stream, BSKCHAR* streamName ) {
  BSKParser subParser;
  BSKBitSet follow;
  BSKUI32   rc;

  /* initialize the cloned parser object, and set it equal to the given
   * parser.  Set it's currentFile buffer to the given streamName buffer */

  BSKMemSet( &subParser, 0, sizeof( subParser ) );
  subParser.stream = stream;
  subParser.db = parser->db;
  subParser.handler = parser->handler;
  subParser.userData = parser->userData;
  subParser.searchPaths = parser->searchPaths;
  subParser.currentFile = BSKStrDup( streamName );
  subParser.error = parser->error;
  subParser.includedFiles = parser->includedFiles;
  subParser.includedCount = parser->includedCount;

  /* initialize the cloned parser's lexer */

  BSKInitLexer( &(subParser.lexer) );
  subParser.lexer.idTable = parser->db->idTable;

  subParser.symbols = parser->db->symbols;
  BSKClearBitSet( &follow );
  BSKSetBit( &follow, TT_EOF );
  
  s_getToken( &subParser );

  rc = s_parseConfigFile( &subParser, &follow );
  BSKFree( subParser.currentFile );

  /* propogate the error flag */
  
  parser->error = subParser.error;

  /* make sure we remember which files have been included */

  parser->includedFiles = subParser.includedFiles;
  parser->includedCount = subParser.includedCount;

  return rc;
}


static BSKI32 s_parseConfigFile( BSKParser* parser, BSKBitSet* follow ) {  
  BSKBitSet subf;

  /* set the list of valid tokens we can encounter */

  BSKClearBitSet( &subf );
  BSKSetBits( &subf, TT_RULE, TT_CATEGORY, TT_IDENTIFIER, TT_TEMPLATE, TT_PUNCT_OCTOTHORP, TT_UNIT, TT_ATTRIBUTE, TT_FORWARD, 0 );
  BSKOrBitSets( &subf, &subf, follow );

  /* until we hit EOF, continue parsing */

  while( parser->currentToken.type != TT_EOF ) {
    
    /* if the current token is not one of the expected ones, throw and
     * error condition. */

    if( !BSKTestBit( &subf, parser->currentToken.type ) ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &subf );
    }
    
    s_parseConfigEntry( parser, &subf );
  }

  /* if we did not quit on EOF, then we have another error */

  if( s_eatToken( parser, TT_EOF, follow ) != 0 ) {
    return -1;
  }

  return 0;
}


static BSKI32 s_parseConfigEntry( BSKParser* parser, BSKBitSet* follow ) {
  switch( parser->currentToken.type ) {
    case TT_UNIT:
      return s_parseUnitDef( parser, follow );
    case TT_ATTRIBUTE:
      return s_parseAttributeDef( parser, follow );
    case TT_IDENTIFIER:
      return s_parseThingDef( parser, follow );
    case TT_TEMPLATE:
      return s_parseTemplateDef( parser, follow );
    case TT_CATEGORY:
      return s_parseCategoryDef( parser, follow );
    case TT_RULE:
      return s_parseRuleDef( parser, follow );
    case TT_PUNCT_OCTOTHORP:
      return s_metaDef( parser, follow );
    case TT_FORWARD:
      return s_forwardDef( parser, follow );
    case TT_EOF:
      return 0;
    default:
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_EOF, follow );
  }

  return 0;
}


static BSKI32 s_parseUnitDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKBitSet keys;
  BSKDBL    num;
  BSKUI32   ident;
  BSKUI32   refIdent;
  BSKSymbolTableEntry* sym;

  num = 1;
  refIdent = 0;
  ident = 0;

  /* set the list of valid tokens that can be encountered in this grammar
   * rule. */

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IDENTIFIER, TT_PUNCT_EQ, TT_NUMBER, TT_PUNCT_SEMICOLON, 0 );

  s_eatToken( parser, TT_UNIT, &keys );

  /* we would normally remove the TT_IDENTIFIER bit from the set of keys,
   * but it appears both later in this grammar rule, as well as in the
   * follow set, so it stays */
  
  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {

    /* make sure the identifier has not already been declared */

    ident = parser->currentToken.data.identifier;
    if( BSKGetSymbol( parser->symbols, ident ) != 0 ) {
      s_error( parser, PE_REDEFINED_IDENTIFIER, ident, 0 );
    } else {
      BSKAddSymbol( parser->symbols, ST_UNIT, SF_NONE, ident );
    }
    s_getToken( parser );
  }

  /* eat the '=' sign, if it exists */
  BSKClearBit( &keys, TT_PUNCT_EQ );

  if( parser->currentToken.type == TT_PUNCT_EQ ) {
    s_eatToken( parser, TT_PUNCT_EQ, &keys );

    /* eat a number (this is the relative value of the unit) */

    BSKClearBit( &keys, TT_NUMBER );
    if( parser->currentToken.type != TT_NUMBER ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_NUMBER, &keys );
    } else {
      num = parser->currentToken.data.dblValue;
      s_getToken( parser );
    }

    /* check to see if the unit is given another unit to which it is
     * relative */

    if( parser->currentToken.type == TT_IDENTIFIER ) {
     
      /* make sure the identifier has already been declared as a unit */

      refIdent = parser->currentToken.data.identifier;
      sym = BSKGetSymbol( parser->symbols, refIdent );
      if( sym == 0 ) {
        s_error( parser, PE_UNDECLARED_IDENTIFIER, refIdent, 0 );
      } else if( sym->type != ST_UNIT ) {
        s_error( parser, PE_WRONG_TYPE, refIdent, 0 );
      }
      s_getToken( parser );
    } else {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
    }
  } else {
    BSKClearBit( &keys, TT_NUMBER );
  }

  BSKClearBit( &keys, TT_PUNCT_SEMICOLON );
  s_eatToken( parser, TT_PUNCT_SEMICOLON, &keys );

  /* if we didn't encounter any problems, add the unit definition to the
   * database. */

  if( ident != 0 ) {
    parser->db->unitDef = BSKAddUnitDef( parser->db->unitDef,
                                         ident,
                                         num,
                                         refIdent );
  }

  return 0;
}


static BSKI32 s_parseAttributeDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKBitSet keys;
  BSKUI32   attrName;
  BSKUI8    attrType;

  attrName = 0;
  attrType = AT_NONE;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IDENTIFIER, hTYPE, 0 );

  s_eatToken( parser, TT_ATTRIBUTE, &keys );

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
  
    /* make sure the attribute has not already been defined */

    attrName = parser->currentToken.data.identifier;
    if( BSKGetSymbol( parser->symbols, attrName ) != 0 ) {
      s_error( parser, PE_REDEFINED_IDENTIFIER, attrName, 0 );
      attrName = 0;
    } else {
      BSKAddSymbol( parser->symbols, ST_ATTRIBUTE, SF_NONE, attrName );
    }
    s_getToken( parser );
  }

  /* get the type of the attribute */

  BSKClearBits( &keys, TT_NUMBER_WORD, TT_STRING_WORD, TT_BOOLEAN_WORD, TT_THING, 0 );
  switch( parser->currentToken.type ) {
    case TT_NUMBER_WORD:
      attrType = AT_NUMBER; s_getToken( parser ); break;
    case TT_STRING_WORD:
      attrType = AT_STRING; s_getToken( parser ); break;
    case TT_BOOLEAN_WORD:
      attrType = AT_BOOLEAN; s_getToken( parser ); break;
    case TT_THING:
      attrType = AT_THING; s_getToken( parser ); break;
    case TT_CATEGORY:
      attrType = AT_CATEGORY; s_getToken( parser ); break;
    case TT_RULE:
      attrType = AT_RULE; s_getToken( parser ); break;
    default:
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_THING, &keys );
  }

  /* if all is okay, add the attribute to the database */

  if( attrType != AT_NONE && attrName != 0 ) {
    parser->db->attrDef = BSKAddAttributeDef( parser->db->attrDef,
                                              attrName,
                                              attrType );
  }

  return 0;
}


static BSKI32 s_parseThingDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKBitSet keys;
  BSKWeightedIdentList* list;
  BSKSymbolTableEntry* sym;
  BSKThing* thing;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_PUNCT_LPAREN, TT_IDENTIFIER, TT_PUNCT_RPAREN, TT_PUNCT_DOT, TT_END, TT_IN, 0 );

  thing = 0;

  /* get the thing's identifier, and make sure that if it has already been
   * defined, that it was defined as a THING.  If it was already defined,
   * get the thing object that it represents, otherwise, create a new thing
   * with that id and add it to the database */

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym != 0 ) {
      if( sym->type != ST_THING ) {
        s_error( parser, PE_REDEFINED_IDENTIFIER, parser->currentToken.data.identifier, 0 );
      } else {
        thing = BSKFindThing( parser->db, parser->currentToken.data.identifier );
        sym->flags = SF_NONE; /* mark it as parsed, if this was previously declared "forward" */
      }
    } else {
      BSKAddSymbol( parser->symbols, ST_THING, SF_NONE, parser->currentToken.data.identifier );
      thing = BSKAddThingToDB( parser->db, BSKNewThing( parser->currentToken.data.identifier ) );
    }

    s_getToken( parser );
  }

  /* if there is the keyword "in" here, then there must be an lparen and a list of
	 * weighted identifiers as well.  Otherwise, if there is just a left paren here,
	 * expect a list of zero or more category names that this thing belongs to. 
	 * Read them all and add the thing to each category */

  BSKClearBit( &keys, TT_PUNCT_LPAREN );
	BSKClearBit( &keys, TT_IN );

	if( parser->currentToken.type == TT_IN ) {
		s_getToken( parser );
		if( parser->currentToken.type != TT_PUNCT_LPAREN ) {
			s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_LPAREN, &keys );
		}
	}

  if( parser->currentToken.type == TT_PUNCT_LPAREN ) {
    s_eatToken( parser, TT_PUNCT_LPAREN, &keys );
    list = s_parseWeightedIdentList( parser, &keys );

    BSKClearBit( &keys, TT_PUNCT_RPAREN );
    s_eatToken( parser, TT_PUNCT_RPAREN, &keys );

    /* add the "thing" to each category identifier in the identifier list */
    if( thing != 0 ) {
			s_addMemberToCategories( parser, (BSKCategoryMember*)thing, list );
    }

    s_destroyWeightedIdentList( list );
  }

  /* read the list of properties that defines the thing */

  BSKClearBit( &keys, TT_PUNCT_DOT );
  s_parsePropertyList( parser, thing, &keys );

  /* eat the "end" keyword */

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  return 0;
}


static BSKI32 s_parseCategoryDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKBitSet keys;
  BSKCategory* category;
  BSKSymbolTableEntry* sym;

  category = 0;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_END, TT_PUNCT_LPAREN, TT_IN, 0 );

  s_eatToken( parser, TT_CATEGORY, &keys );

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
 
    /* make sure that if the identifier has already been declared, that it
     * was declared as a category.  If it has been declared, grab the
     * existing category, otherwise create a new category with the given
     * id and add it to the database */

    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym == 0 ) {
      category = BSKAddCategoryToDB( parser->db, BSKNewCategory( parser->currentToken.data.identifier ) );
      BSKAddSymbol( parser->symbols, ST_CATEGORY, SF_NONE, category->id );
    } else if( sym->type == ST_CATEGORY ) {
      sym->flags = SF_NONE; /* mark it as defined, in case it was previously declared "forward" */
      category = BSKFindCategory( parser->db, parser->currentToken.data.identifier );
      if( category == 0 ) {
        s_error( parser, PE_BUG_DETECTED, (BSKUI32)"category symbol exists, but not as category (s_parseCategoryDef)", 0 );
      }
    } else {
      s_error( parser, PE_REDEFINED_IDENTIFIER, parser->currentToken.type, 0 );
    }
    s_getToken( parser );
  }

	/* if the keyword "in" is found, expect a weighted list of identifiers representing
	 * the categories that this category should be placed in. */

	BSKClearBit( &keys, TT_IN );
	if( parser->currentToken.type == TT_IN ) {
		BSKWeightedIdentList* list;

		s_getToken( parser );
		BSKSetBit( &keys, TT_PUNCT_RPAREN );
		s_eatToken( parser, TT_PUNCT_LPAREN, &keys );
		BSKClearBit( &keys, TT_PUNCT_RPAREN );

    list = s_parseWeightedIdentList( parser, &keys );
		if( category != 0 ) {
			s_addMemberToCategories( parser, (BSKCategoryMember*)category, list );
		}

		s_destroyWeightedIdentList( list );
	}

  /* populate the category */

  s_parseCategoryBody( parser, category, follow );

  /* eat the "end" keyword */

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  return 0;
}


static BSKI32 s_parseTemplateDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKWeightedIdentList* categoryList;
  BSKIdentList* attributeList;
  BSKIdentList* i;
  BSKWeightedIdentList* wi;
  BSKBitSet keys;
  BSKSymbolTableEntry* sym;
  BSKCategory* category;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_PUNCT_LBRACE, TT_END, TT_IDENTIFIER, TT_IN, 0 );

  s_eatToken( parser, TT_TEMPLATE, &keys );

  categoryList = 0;
	attributeList = 0;

  /* if there is a category list, parse it into an identifier list.  All
   * things defined by this template will automatically be added to these
   * categories.  Make sure here, therefore, that all these identifiers
   * are categories, and if they aren't yet defined, create new categories
   * for them.
	 *
	 * Added 18 Feb 2001: if the keyword "in" is present, it must be followed
	 * by an lparen that contains a weighted list of identifiers. */

	BSKClearBit( &keys, TT_IN );
	if( parser->currentToken.type == TT_IN ) {
		s_getToken( parser );
		BSKClearBit( &keys, TT_PUNCT_LPAREN );
		if( parser->currentToken.type != TT_PUNCT_LPAREN ) {
			s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_LPAREN, &keys );
		}
	}

  if( parser->currentToken.type == TT_PUNCT_LPAREN ) {
    s_eatToken( parser, TT_PUNCT_LPAREN, &keys );
    BSKSetBit( &keys, TT_PUNCT_RPAREN );
    categoryList = s_parseWeightedIdentList( parser, &keys );
    BSKClearBit( &keys, TT_PUNCT_RPAREN );
    s_eatToken( parser, TT_PUNCT_RPAREN, &keys );

    for( wi = categoryList; wi != 0; wi = wi->next ) {
      sym = BSKGetSymbol( parser->symbols, wi->id );
      if( sym == 0 ) {
        category = BSKAddCategoryToDB( parser->db, BSKNewCategory( wi->id ) );
        BSKAddSymbol( parser->symbols, ST_CATEGORY, SF_NONE, wi->id );
      } else if( sym->type != ST_CATEGORY ) {
        s_error( parser, PE_WRONG_TYPE, wi->id, 0 );
        wi->id = 0;
      } else {
        /* mark the symbol as declared, in case it was previously declared "forward" */
        sym->flags = SF_NONE;
      }
    }
  }

  BSKClearBit( &keys, TT_PUNCT_LBRACE );
  s_eatToken( parser, TT_PUNCT_LBRACE, &keys );

  /* get the list of attributes that each thing can have in this template */

  BSKSetBit( &keys, TT_PUNCT_RBRACE );
  attributeList = s_parseIdentList( parser, &keys );
  BSKClearBit( &keys, TT_PUNCT_RBRACE );

  /* make sure that each identifier read in the attribute list is actually
   * an attribute. */

  for( i = attributeList; i != 0; i = i->next ) {
    sym = BSKGetSymbol( parser->symbols, i->id );
    if( sym == 0 ) {
      s_error( parser, PE_UNDECLARED_IDENTIFIER, i->id, 0 );
      i->id = 0;
    } else if( sym->type != ST_ATTRIBUTE ) {
      s_error( parser, PE_WRONG_TYPE, i->id, 0 );
      i->id = 0;
    }
  }
  attributeList = s_reverseIdentList( attributeList );

  s_eatToken( parser, TT_PUNCT_RBRACE, &keys );

  /* parse each entry in the template */

  while( parser->currentToken.type == TT_IDENTIFIER ) {
    s_parseTemplateEntry( parser,
                          categoryList,
                          attributeList,
                          &keys );
  }

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  /* clean up */

  s_destroyWeightedIdentList( categoryList );
  s_destroyIdentList( attributeList );

  return 0;
}


static BSKI32 s_parseRuleDef( BSKParser* parser, BSKBitSet* follow ) {
  BSKRule* rule;
  BSKIdentList* parmList;
  BSKIdentList* i;
  BSKSymbolTableEntry* sym;
  BSKBitSet keys;
  BSKUI32 count;
  BSKUI32 id;

  BSKCopyBitSet( &keys, follow );

  s_eatToken( parser, TT_RULE, &keys );

  rule = 0;

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
    /* make sure that the rule has not already been declared, and that the
     * identifier is not yet defined (or, if it is, that the rule it
     * corresponds to was declared with the 'forward' keyword). */

    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym != 0 ) {

      /* this is an error ONLY if the symbol was not declared "forward" and
       * it is not declared as a rule */

      if( ( sym->type == ST_RULE ) && ( sym->flags & SF_FORWARD_DECL ) != 0 ) {
        rule = BSKFindRule( parser->db->rules, sym->id );
        sym->flags = SF_NONE;
      } else {
        s_error( parser, PE_REDEFINED_IDENTIFIER, sym->id, 0 );
      }
    } else {
      rule = BSKAddRuleToDB( parser->db, BSKNewRule( parser->currentToken.data.identifier ) );
      BSKAddSymbol( parser->symbols, ST_RULE, SF_NONE, rule->id );

      /* every rule has it's own symbol table, which links to the master
       * symbol table in the parser and database as it's parent */

      rule->symbols = BSKNewSymbolTable();
      rule->symbols->parent = parser->symbols;
    }

    /* the "currentRule" field tells later routines what rule is being
     * parsed. */

    parser->currentRule = rule;

    if( rule != 0 ) {
      if( parser->currentFile != 0 ) {
        rule->file = BSKStrDup( parser->currentFile );
      }
      rule->startLine = parser->currentToken.row;
    }
    s_getToken( parser );
  }

  BSKSetBits( &keys, TT_PUNCT_RPAREN, TT_END, 0 );
  s_eatToken( parser, TT_PUNCT_LPAREN, &keys );

  /* get the list of parameters.  we need to reverse the list because
   * s_parseIdentList returns them in reverse order. */

  parmList = s_reverseIdentList( s_parseIdentList( parser, &keys ) );
  count = 0;
  for( i = parmList; i != 0; i = i->next ) {

    /* make sure that each parameter is not already defined somewhere, or
     * that it has only been defined as a variable */

    sym = BSKGetSymbol( ( rule ? rule->symbols : parser->symbols ), i->id );
    if( sym != 0 ) {
      if( sym->type != ST_VARIABLE ) {
        s_error( parser, PE_REDEFINED_IDENTIFIER, i->id, 0 );
        i->id = 0;
      } else {
        count++;
      }
    } else {
      count++;
      if( rule != 0 ) {
        BSKAddSymbol( rule->symbols, ST_VARIABLE, SF_NONE, i->id );
      }
    }
  }

  /* create the rule's named parameter list */

  if( ( rule != 0 ) && ( count > 0 ) ) {
    rule->parameterCount = count;
    rule->parameters = (BSKUI32*)BSKMalloc( sizeof( BSKUI32 ) * count );
    count = 0;
    for( i = parmList; i != 0; i = i->next ) {
      if( i->id != 0 ) {
        rule->parameters[ count++ ] = i->id;
      }
    }
  }

  s_destroyIdentList( parmList );

  BSKClearBit( &keys, TT_PUNCT_RPAREN );
  s_eatToken( parser, TT_PUNCT_RPAREN, &keys );

  /* emit the code that initializes the rule when it is run.  Essentially,
   * this code simply sets the rule's return value to null, so if a return
   * value is never specified, it can at least be relied upon to return
   * null. */

  if( rule != 0 ) {
    id = BSKAddIdentifier( parser->db->idTable, RULE_RETURN_VAL );
    BSKAddSymbol( rule->symbols, ST_VARIABLE, SF_NONE, id );

    s_emitOp( rule, OP_LID ); s_emitID( rule, id );
    s_emitOp( rule, OP_NULL );
    s_emitOp( rule, OP_PUT );
  }

  /* parse and compile the body of the rule */

  s_parseRuleStatementList( parser, rule, &keys );
  parser->currentRule = 0;

  /* always terminate the rule's execution with END */

  if( rule != 0 ) {
    s_emitOp( rule, OP_END );
  }

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  return 0;
}


static BSKI32 s_parseRuleStatementList( BSKParser* parser,
                                        BSKRule* rule,
                                        BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet hKeys;

  /* set hKeys to the set of tokens that are valid for starting a
   * rule statement.  Set 'keys' to the new follow set, which
   * is whatever the follow set was that was passed in, plus
   * everything in hKeys. */

  BSKClearBitSet( &hKeys );
  BSKSetBits( &hKeys, TT_IDENTIFIER,
                      TT_IF,
                      TT_WHILE,
                      TT_DO,
                      TT_FOR,
                      TT_EXIT,
                      TT_CASE,
                      0 );
  BSKOrBitSets( &keys, &hKeys, follow );

  /* parse and compile each rule statement */

  while( BSKTestBit( &hKeys, parser->currentToken.type ) ) {
    s_parseRuleStatement( parser, rule, &keys );
  }

  return 0;
}


static BSKIdentList* s_parseIdentList( BSKParser* parser, BSKBitSet* follow ) {
  BSKIdentList* list;

  /* keep adding identifiers to the list for as long as they appear
   * in the input. */

  list = 0;
  while( parser->currentToken.type == TT_IDENTIFIER ) {
    list = s_addToIdentList( list, parser->currentToken.data.identifier );
    s_getToken( parser );
  }

  return list;
}


static BSKWeightedIdentList* s_parseWeightedIdentList( BSKParser* parser, BSKBitSet* follow ) {
  BSKWeightedIdentList* list;

  /* keep adding identifiers to the list for as long as they appear
   * in the input. */

  list = 0;
  while( parser->currentToken.type == TT_NUMBER || parser->currentToken.type == TT_IDENTIFIER ) {
		BSKWeightedIdentList* i;
		BSKUI16 weight;

		if( parser->currentToken.type == TT_NUMBER ) {
			weight = (BSKUI16)parser->currentToken.data.dblValue;
			s_getToken( parser );
			if( parser->currentToken.type != TT_PUNCT_COLON ) {
				s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_COLON, follow );
			} else {
				s_getToken( parser );
			}
		} else {
			weight = 1;
		}

		if( parser->currentToken.type != TT_IDENTIFIER ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, follow );
		} else {
			i = (BSKWeightedIdentList*)BSKMalloc( sizeof( BSKWeightedIdentList ) );
			i->weight = weight;
			i->id = parser->currentToken.data.identifier;
			i->next = list;

			list = i;
			s_getToken( parser );
		}
  }

  return list;
}


static BSKI32 s_parsePropertyList( BSKParser* parser, 
                                   BSKThing* thing, 
                                   BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet keys2;
  BSKUI32   attrName;
  BSKUI32   attrType;
  BSKBOOL   isValid;

  BSKAttributeDef* attr;

  BSKValue value;

  attrName = 0;
  attrType = 0;
  isValid  = BSKFALSE;

  /* set 'keys' to be whatever 'follow' was, and then add all the
   * valid tokens that can appear in the property list rule */

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IDENTIFIER,
                     TT_PUNCT_STAR,
                     TT_PUNCT_PLUS,
                     TT_PUNCT_DASH,
                     TT_NUMBER,
                     TT_STRING,
                     hBOOLEAN,
                     TT_PUNCT_LBRACE,
                     TT_PUNCT_LPAREN,
                     0 );

  /* all properties must begin with a '.' character */

  while( parser->currentToken.type == TT_PUNCT_DOT ) {
    BSKCopyBitSet( &keys2, &keys );

    BSKClearBit( &keys2, TT_PUNCT_DOT );
    s_eatToken( parser, TT_PUNCT_DOT, &keys2 );

    attrType = 0;
    attr = 0;

    /* make sure the next identifier is a declared attribute */

    if( parser->currentToken.type != TT_IDENTIFIER ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys2 );
    } else {
      attrName = parser->currentToken.data.identifier;
      if( !s_isAttributeID( parser->db, attrName ) ) {
        s_error( parser, PE_WRONG_TYPE, attrName, 0 );
      } else {
        attr = BSKFindAttributeDef( parser->db->attrDef, attrName );
        attrType = attr->type;
      }
      s_getToken( parser );
    }

    BSKClearBits( &keys2, TT_PUNCT_STAR, TT_PUNCT_PLUS, TT_PUNCT_DASH,
                          TT_NUMBER, TT_STRING, hBOOLEAN, TT_PUNCT_LBRACE,
                          TT_PUNCT_LPAREN, 0 );

    /* get the value associated with the property */

    s_parseValue( parser, &value, attrType, &keys2 );

    if( ( thing != 0 ) && ( attr != 0 ) ) {
      isValid = BSKFALSE;
      switch( attr->type ) {
        case AT_BOOLEAN:
        case AT_NUMBER:
          isValid = BSKValueIsType( &value, VT_NUMBER ); break;
        case AT_STRING:
          isValid = BSKValueIsType( &value, VT_STRING ); break;
        case AT_THING:
          isValid = BSKValueIsType( &value, VT_THING ); break;
        case AT_CATEGORY:
          isValid = BSKValueIsType( &value, VT_CATEGORY ); break;
        case AT_RULE:
          isValid = BSKValueIsType( &value, VT_RULE ); break;
      }
      if( isValid ) {
        BSKAddAttributeTo( thing, attrName, &value );
      } else {
        s_error( parser, PE_WRONG_TYPE, attrName, 0 );
      }
    }
    BSKInvalidateValue( &value );
  }

  return 0;
}


static BSKI32 s_parseValue( BSKParser* parser,
                            BSKValue* value,
                            BSKUI32 expectedType,
                            BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKSymbolTableEntry* sym;
  BSKRule* rule;
  BSKThing* thing;
  BSKCategory* category;

  BSKClearBitSet( &keys );

  BSKInitializeValue( value );

  /* check for a number */

  BSKSetBits( &keys, hNUMBER, 0 );
  if( BSKTestBit( &keys, parser->currentToken.type ) ) {
    s_parseNumber( parser, value, follow );
  } else if( parser->currentToken.type == TT_STRING ) {

    /* check for a string */

    BSKSetValueString( value, parser->currentToken.data.strValue );
    s_getToken( parser );
  } else if( parser->currentToken.type == TT_IDENTIFIER ) {

    /* if it is an identifier, determine if the identifier is a thing,
     * category or rule.  If the symbol has not yet been defined, then
     * based on what this value is expected to be, create a new object
     * and add it to the database.  Otherwise, if the symbol is not a
     * thing, category, or rule, throw an error. */

    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym == 0 ) {
      switch( expectedType ) {
        case AT_THING:
          value->type = VT_THING;
          value->datum = BSKAddThingToDB( parser->db, BSKNewThing( parser->currentToken.data.identifier ) );
          BSKAddSymbol( parser->symbols, ST_THING, SF_NONE, parser->currentToken.data.identifier );
          break;
        case AT_CATEGORY:
          value->type = VT_CATEGORY;
          value->datum = BSKAddCategoryToDB( parser->db, BSKNewCategory( parser->currentToken.data.identifier ) );
          BSKAddSymbol( parser->symbols, ST_CATEGORY, SF_NONE, parser->currentToken.data.identifier );
          break;
        case AT_RULE:
          value->type = VT_RULE;
          rule = BSKAddRuleToDB( parser->db, BSKNewRule( parser->currentToken.data.identifier ) );
          BSKAddSymbol( parser->symbols, ST_RULE, SF_FORWARD_DECL, rule->id );
          rule->symbols = BSKNewSymbolTable();
          rule->symbols->parent = parser->symbols;
          value->datum = rule;
          break;
      }
    } else {
      switch( sym->type ) {
        case ST_THING:
          value->type = VT_THING;
          value->datum = BSKFindThing( parser->db, parser->currentToken.data.identifier );
          break;
        case ST_CATEGORY:
          value->type = VT_CATEGORY;
          value->datum = BSKFindCategory( parser->db, parser->currentToken.data.identifier );
          break;
        case ST_RULE:
          value->type = VT_RULE;
          value->datum = BSKFindRule( parser->db->rules, parser->currentToken.data.identifier );
          break;
        default:
          s_error( parser, PE_WRONG_TYPE, parser->currentToken.data.identifier, 0 );
          break;
      }
    }
    s_getToken( parser );
  } else if( parser->currentToken.type == TT_PUNCT_LBRACE ) {

    /* an LBRACE '{' anticipates the definition of an anonymous (unnamed)
     * thing -- create a new thing object and parse the property list. */

    BSKCopyBitSet( &keys, follow );
    s_eatToken( parser, TT_PUNCT_LBRACE, &keys );

    thing = BSKAddThingToDB( parser->db, 
                             BSKNewThing( BSKAddIdentifier( parser->db->idTable, 0 ) ) );

    s_parsePropertyList( parser, thing, &keys );
    s_eatToken( parser, TT_PUNCT_RBRACE, &keys );

    value->type = VT_THING;
    value->datum = thing;
  } else if( parser->currentToken.type == TT_PUNCT_LPAREN ) {

    /* an LPAREN '(' anticipates the definition of an anonymous (unnamed)
     * category -- create a new category and parse it's member list */

    BSKCopyBitSet( &keys, follow );
    s_getToken( parser );

    category = BSKAddCategoryToDB( parser->db, 
                                   BSKNewCategory( BSKAddIdentifier( parser->db->idTable, 0 ) ) );

    BSKSetBit( &keys, TT_PUNCT_RPAREN );
    s_parseCategoryBody( parser, category, &keys );
    BSKClearBit( &keys, TT_PUNCT_RPAREN );
    s_eatToken( parser, TT_PUNCT_RPAREN, &keys );

    value->type = VT_CATEGORY;
    value->datum = category;
  } else {

    /* otherwise, anticipate a boolean value */

    BSKClearBitSet( &keys );
    BSKSetBits( &keys, hBOOLEAN, 0 );
    if( BSKTestBit( &keys, parser->currentToken.type ) ) {
      value->type = VT_BOOLEAN;
      switch( parser->currentToken.type ) {
        case TT_YES:
        case TT_ON:
        case TT_TRUE:
          BSKSetValueNumber( value, BSKTRUE );
          break;
        default:
          BSKSetValueNumber( value, BSKFALSE );
      }
    }
    s_getToken( parser );
  }

  return 0;
}


static BSKI32 s_parseNumber( BSKParser* parser,
                             BSKValue*  value,
                             BSKBitSet* follow )
{
  BSKToken     tk1;
  BSKToken     tk2;
  BSKSubNumber subnum;
  BSKSubNumber subnum2;
  BSKUI32      units;

  BSKInitializeValue( value );

  /* get the primary number (or dice) value and put it into subnum */

  s_parseSubNumber( parser, &subnum, follow );

  BSKMemSet( &subnum2, 0, sizeof( subnum2 ) );

  /* if the next character is a '-', '+', or '*', parse another number
   * into subnum2. (subnum must have been a DICE number) */

  BSKMemCpy( &tk1, &(parser->currentToken), sizeof( BSKToken ) );
  if( tk1.type == TT_PUNCT_DASH || tk1.type == TT_PUNCT_PLUS || tk1.type == TT_PUNCT_STAR ) {
    if( subnum.type == VT_DICE ) {
      s_getToken( parser );
      BSKMemCpy( &tk2, &(parser->currentToken), sizeof( BSKToken ) );

      BSKPutToken( &(parser->lexer), &tk2 );
      BSKPutToken( &(parser->lexer), &tk1 );
      s_getToken( parser );

      if( tk2.type == TT_NUMBER ) {
        s_parseSubNumber( parser, &subnum2, follow );
      }
    }
  }

  /* assume that no units are given */

  units = 0;

  /* if an identifier is specified, make sure it identifies a unit (otherwise
   * it's an error). */

  if( parser->currentToken.type == TT_IDENTIFIER ) {
    BSKSymbolTableEntry* sym;

    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym == 0 ) {
      s_error( parser, PE_UNDECLARED_IDENTIFIER, parser->currentToken.data.identifier, 0 );
    } else if( sym->type != ST_UNIT ) {
      BSKPutToken( &(parser->lexer), &(parser->currentToken) );
    } else {
      units = parser->currentToken.data.identifier;      
    }

    s_getToken( parser );
  }

  /* Set the parsed number (and units) in the given value */

  if( subnum.type == VT_DICE ) {
    BSKSetValueDiceU( value, subnum.data.dice.count,
                             subnum.data.dice.type,
                             subnum2.op,
                             (BSKI16)subnum2.data.floatVal,
                             units );
  } else {
    BSKSetValueNumberU( value, subnum.data.floatVal, units );
  }

  return 0;                                                    
}


static BSKI32 s_parseSubNumber( BSKParser* parser,
                                BSKSubNumber* subn,
                                BSKBitSet* follow )
{
  BSKTokenId id;
  BSKI8      sign;

  BSKMemSet( subn, 0, sizeof( *subn ) );

  id = parser->currentToken.type;
  sign = 1;

  if( id == TT_PUNCT_STAR ) {
    subn->op = '*';
    s_getToken( parser );
    id = parser->currentToken.type;
  } else {
    subn->op = '+';
  }

  if( id == TT_PUNCT_DASH ) {
    sign = -1;
    s_getToken( parser );
    id = parser->currentToken.type;
  } else if( id == TT_PUNCT_PLUS ) {
    sign = 1;
    s_getToken( parser );
    id = parser->currentToken.type;
  }
  
  switch( id ) {
    case TT_NUMBER:
      subn->type = VT_FLOAT;
      subn->data.floatVal = sign * (BSKFLOAT)parser->currentToken.data.dblValue;
      s_getToken( parser );
      break;
    case TT_DICE:
      subn->type = VT_DICE;
      subn->data.dice.count = sign * parser->currentToken.data.dice.dCount;
      subn->data.dice.type = parser->currentToken.data.dice.dType;
      s_getToken( parser );
      break;
    default:
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_NUMBER, follow );
  }

  return 0;
}


static BSKI32 s_parseCategoryBody( BSKParser* parser,
                                   BSKCategory* category,
                                   BSKBitSet* follow )
{
  BSKBitSet keys;

  BSKClearBitSet( &keys );
  BSKSetBits( &keys, TT_PUNCT_LBRACE, TT_IDENTIFIER, TT_PUNCT_LBRACKET, TT_NULL, 0 );
  while( BSKTestBit( &keys, parser->currentToken.type ) ) {
    s_parseCategoryEntry( parser, category, follow );
  }

  return 0;
}


static BSKI32 s_parseCategoryEntry( BSKParser* parser,
                                    BSKCategory* category,
                                    BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI16 weight;
  BSKCategoryMember* member;
  BSKThing* thing;
  BSKCategory* subCategory;
  BSKSymbolTableEntry* sym;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IDENTIFIER, TT_PUNCT_LBRACE, TT_PUNCT_LPAREN, TT_NULL, 0 );

  /* assume a weight of 1 for each new member */

  weight = 1;

  /* if an LBRACKET '[' is found, anticipate the weight value for the
   * new member */

  if( parser->currentToken.type == TT_PUNCT_LBRACKET ) {
    s_eatToken( parser, TT_PUNCT_LBRACKET, &keys );

    if( parser->currentToken.type != TT_NUMBER ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_NUMBER, &keys );
    } else {
      weight = parser->currentToken.data.dblValue;
    }

    s_getToken( parser );
    s_eatToken( parser, TT_PUNCT_RBRACKET, &keys );
  }

  /* parse the member -- an identifier can be a thing, category, or rule,
   * and things and categories may also be defined anonymously. */

  BSKClearBits( &keys, TT_IDENTIFIER, TT_PUNCT_LBRACE, TT_PUNCT_LPAREN, TT_NULL, 0 );
  switch( parser->currentToken.type ) {
    case TT_IDENTIFIER:
      sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
      if( sym == 0 ) {
        s_error( parser, PE_UNDECLARED_IDENTIFIER, parser->currentToken.data.identifier, 0 );
      } else {
        switch( sym->type ) {
          case ST_THING:
            member = (BSKCategoryMember*)BSKFindThing( parser->db, parser->currentToken.data.identifier );
            if( member == 0 ) {
              s_error( parser, PE_BUG_DETECTED, (BSKUI32)"thing defined as symbol, but not as thing (s_parseCategoryEntry)", 0 );
            } else {
              if( category != 0 ) {
                BSKAddToCategory( category, weight, member );
              }
            }
            break;
          case ST_CATEGORY:
            member = (BSKCategoryMember*)BSKFindCategory( parser->db, parser->currentToken.data.identifier );
            if( member == 0 ) {
              s_error( parser, PE_BUG_DETECTED, (BSKUI32)"category defined as symbol, but not as category (s_parseCategoryEntry)", 0 );
            } else {
              if( category != 0 ) {
                BSKAddToCategory( category, weight, member );
              }
            }
            break;
          case ST_RULE:
            member = (BSKCategoryMember*)BSKFindRule( parser->db->rules, parser->currentToken.data.identifier );
            if( member == 0 ) {
              s_error( parser, PE_BUG_DETECTED, (BSKUI32)"rule defined as symbol, but not as rule (s_parseCategoryEntry)", 0 );
            } else {
              if( category != 0 ) {
                BSKAddToCategory( category, weight, member );
              }
            }
            break;
          default:
            s_error( parser, PE_WRONG_TYPE, parser->currentToken.data.identifier, 0 );
        }
      }
      s_getToken( parser );
      break;
    case TT_NULL:
      if( category != 0 ) {
        BSKAddToCategory( category, weight, 0 );
      }
      s_getToken( parser );
      break;
    case TT_PUNCT_LBRACE:
      s_eatToken( parser, TT_PUNCT_LBRACE, &keys );
      if( category == 0 ) {
        thing = 0;
      } else {
        thing = BSKAddThingToDB( parser->db, BSKNewThing( BSKAddIdentifier( parser->db->idTable, 0 ) ) );
        member = (BSKCategoryMember*)thing;
        BSKAddToCategory( category, weight, member );
      }
      BSKSetBit( &keys, TT_PUNCT_RBRACE );
      s_parsePropertyList( parser, thing, &keys );
      BSKClearBit( &keys, TT_PUNCT_RBRACE );
      s_eatToken( parser, TT_PUNCT_RBRACE, &keys );
      break;
    case TT_PUNCT_LPAREN:
      s_getToken( parser );
      if( category == 0 ) {
        subCategory = 0;
      } else {
        subCategory = BSKAddCategoryToDB( parser->db, BSKNewCategory( BSKAddIdentifier( parser->db->idTable, 0 ) ) );
        member = (BSKCategoryMember*)subCategory;
        BSKAddToCategory( category, weight, member );
      }
      BSKSetBit( &keys, TT_PUNCT_RPAREN );
      s_parseCategoryBody( parser, subCategory, &keys );
      BSKClearBit( &keys, TT_PUNCT_RPAREN );
      s_eatToken( parser, TT_PUNCT_RPAREN, &keys );
      break;
    default:
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  }

  return 0;
}


static BSKI32 s_parseTemplateEntry( BSKParser* parser,
                                    BSKWeightedIdentList* categories,
                                    BSKIdentList* attributes,
                                    BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet tvalHead;
  BSKThing* thing;
  BSKWeightedIdentList* moreCategories;
  BSKIdentList* i;
  BSKWeightedIdentList* wi;
  BSKSymbolTableEntry* sym;
  BSKCategory* category;
  BSKValue value;
  BSKAttributeDef* attr;
      
  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IDENTIFIER, TT_PUNCT_LBRACE, TT_IN, TT_PUNCT_LPAREN, 0 );

  thing = 0;
  moreCategories = 0;

  /* get the identifier for the new thing */

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
    sym = BSKGetSymbol( parser->symbols, parser->currentToken.data.identifier );
    if( sym != 0 ) {
      if( sym->type != ST_THING ) {
        s_error( parser, PE_REDEFINED_IDENTIFIER, parser->currentToken.data.identifier, 0 );
      } else {
        thing = BSKFindThing( parser->db, parser->currentToken.data.identifier );
      }
      /* clear the flag in case it was declared "forward" */
      sym->flags = SF_NONE;
    } else {
      thing = BSKAddThingToDB( parser->db, BSKNewThing( parser->currentToken.data.identifier ) );
      BSKAddSymbol( parser->symbols, ST_THING, SF_NONE, thing->id );
    }
    s_getToken( parser );
  }

  /* if an LPAREN '(' is found, anticipate a list of category names to add
   * the thing to.  If a given symbol is not yet defined, create a new
   * category for that identifier.
   *
	 * Added 18 Feb 2001: if the keyword "in" is found, expect an lparen and a weighted
	 * identifier list immediately following it.
	 */

	BSKClearBits( &keys, TT_IN, TT_PUNCT_LPAREN, 0 );

	if( parser->currentToken.type == TT_IN ) {
		s_getToken( parser );
		if( parser->currentToken.type != TT_PUNCT_LPAREN ) {
			s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_LPAREN, &keys );
		}
	}

  if( parser->currentToken.type == TT_PUNCT_LPAREN ) {
    s_eatToken( parser, TT_PUNCT_LPAREN, &keys );
    BSKSetBit( &keys, TT_PUNCT_RPAREN );
    moreCategories = s_parseWeightedIdentList( parser, &keys );
    BSKClearBit( &keys, TT_PUNCT_RPAREN );
    s_eatToken( parser, TT_PUNCT_RPAREN, &keys );

		s_addMemberToCategories( parser, (BSKCategoryMember*)thing, moreCategories );
  }
  
  /* add the thing to each of the given categories; we could use
	 * s_addMemberToCategories for this, but that function also checks to make sure
	 * that each identifier is a valid category, and that's overhead that we've already
	 * done once when the list was first parsed (in s_parseTemplateDef).  This is faster.
	 */

  for( wi = categories; wi != 0; wi = wi->next ) {
    if( wi->id != 0 ) {
      category = BSKFindCategory( parser->db, wi->id );
      if( category != 0 ) {
        BSKAddToCategory( category, wi->weight, thing );
      }
    }
  }

  BSKClearBit( &keys, TT_PUNCT_LBRACE );
  s_eatToken( parser, TT_PUNCT_LBRACE, &keys );

  BSKClearBitSet( &tvalHead );
  BSKSetBits( &tvalHead, TT_PUNCT_DOLLAR,
                         hNUMBER,
                         hBOOLEAN,
                         TT_STRING,
                         TT_NUMBER,
                         TT_DICE,
                         TT_IDENTIFIER,
                         TT_PUNCT_LBRACE,
                         TT_PUNCT_LPAREN,
                         0 );
  BSKSetBit( &keys, TT_PUNCT_RBRACE );

  /* parse the property list for the thing -- making sure that the
   * number of properties does not exceed the number of attributes
   * defined in the template definition. */

  i = attributes;
  while( BSKTestBit( &tvalHead, parser->currentToken.type ) ) {
    if( i == 0 ) {
      s_error( parser, PE_TOO_MANY_ATTRIBUTES, 0, &keys );
      break;
    }

    if( parser->currentToken.type == TT_PUNCT_DOLLAR ) {
      s_getToken( parser );
    } else {
      attr = BSKFindAttributeDef( parser->db->attrDef, i->id );
      s_parseValue( parser, &value, ( attr != 0 ? attr->type : 0 ), &keys );
      if( attr != 0 ) {
        if( !BSKValueOfAttributeType( &value, attr->type ) ) {
          s_error( parser, PE_WRONG_TYPE, i->id, 0 );
        } else if( thing != 0 ) {
          BSKAddAttributeTo( thing, i->id, &value );
        }
      }
      BSKInvalidateValue( &value );
    }
    i = i->next;
  }

  BSKClearBit( &keys, TT_PUNCT_RBRACE );
  s_eatToken( parser, TT_PUNCT_RBRACE, &keys );

  s_destroyWeightedIdentList( moreCategories );

  return 0;
}


static BSKTokenId s_getToken( BSKParser* parser ) {
  
  /* grab the next token from the stream via the lexer */

  BSKNextToken( &(parser->lexer), parser->stream, &(parser->currentToken) );

  /* if we've gone to a new line and we are in the middle of a rule
   * definition, emit the new line number into the rule's code buffer
   * with a LINE operation */

  if( parser->currentToken.row != parser->line ) {
    parser->line = parser->currentToken.row;
    if( parser->currentRule != 0 ) {
      s_emitOp( parser->currentRule, OP_LINE );
      s_emitDWord( parser->currentRule, parser->line );
    }
  }

  return parser->currentToken.type;
}


static void s_error( BSKParser* parser, 
                     BSKI32 code, 
                     BSKUI32 data,
                     BSKBitSet* valid ) 
{
  if( parser->handler != 0 ) {
    parser->handler( code, 
                     parser->db,
                     ( parser->currentFile ? parser->currentFile : "" ),
                     &(parser->currentToken), 
                     data,
                     parser->userData );
  }
  
  /* if the set of valid tokens is not null, throw away all tokens until
   * one that is in the valid set is found */

  if( valid != 0 ) {
    s_throwAwayUntil( parser, valid );
  }

  parser->error = BSKTRUE;
}


static BSKI32 s_eatToken( BSKParser* parser, 
                          BSKTokenId type,
                          BSKBitSet* valid )
{
  /* expect the given token type -- if it is not that, throw an error */

  if( parser->currentToken.type != type ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, type, valid );
    return -1;
  }

  /* grab the next token from the stream */

  s_getToken( parser );
  return 0;
}


static void s_throwAwayUntil( BSKParser* parser, BSKBitSet* valid ) {
  while( !BSKTestBit( valid, parser->currentToken.type ) ) {
    s_getToken( parser );

    /* always stop at the end of the stream, regardless of whether EOF is
     * a valid token or not */

    if( parser->currentToken.type == TT_EOF ) {
      break;
    }
  }
}


static BSKIdentList* s_addToIdentList( BSKIdentList* list, BSKUI32 id ) {
  BSKIdentList* ident;

  ident = (BSKIdentList*)BSKMalloc( sizeof( BSKIdentList ) );
  ident->id = id;
  ident->next = list;

  return ident;
}


static BSKIdentList* s_reverseIdentList( BSKIdentList* list ) {
  BSKIdentList* tmp;
  BSKIdentList* i;
  BSKIdentList* n;

  tmp = 0;
  n = 0;
  for( i = list; i != 0; i = n ) {
    n = i->next;
    i->next = tmp;
    tmp = i;
  }

  return tmp;
}


static void s_destroyIdentList( BSKIdentList* list ) {
  BSKIdentList* i;
  BSKIdentList* n;

  i = list;
  while( i != 0 ) {
    n = i->next;
    BSKFree( i );
    i = n;
  }
}


static void s_destroyWeightedIdentList( BSKWeightedIdentList* list ) {
  BSKWeightedIdentList* i;
  BSKWeightedIdentList* n;

  i = list;
  while( i != 0 ) {
    n = i->next;
    BSKFree( i );
    i = n;
  }
}


static void s_emitOp( BSKRule* rule, BSKUI32 op ) {
  BSKUI8* code;

  /* this is NOT the most efficient implementation -- the code buffer
   * is reallocated every time a new operator or data is added to it. */

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + 1 );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  code[ rule->codeLength++ ] = op;

  rule->code = code;
}


static void s_emitID( BSKRule* rule, BSKUI32 id ) {
  BSKUI8* code;

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + sizeof( BSKUI32 ) );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  BSKMemCpy( &(code[rule->codeLength]), &id, sizeof( BSKUI32 ) );

  rule->codeLength += sizeof( BSKUI32 );
  rule->code = code;
}


static void s_emitDWord( BSKRule* rule, BSKUI32 dword ) {
  BSKUI8* code;

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + sizeof( BSKUI32 ) );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  BSKMemCpy( &(code[rule->codeLength]), &dword, sizeof( BSKUI32 ) );

  rule->codeLength += sizeof( BSKUI32 );
  rule->code = code;
}


static void s_emitWord( BSKRule* rule, BSKUI16 word ) {
  BSKUI8* code;

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + sizeof( BSKUI16 ) );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  BSKMemCpy( &(code[rule->codeLength]), &word, sizeof( BSKUI16 ) );

  rule->codeLength += sizeof( BSKUI16 );
  rule->code = code;
}


static void s_emitByte( BSKRule* rule, BSKUI8 byte ) {
  BSKUI8* code;

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + 1 );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  code[ rule->codeLength++ ] = byte;

  rule->code = code;
}


static void s_emitString( BSKRule* rule, BSKCHAR* string ) {
  BSKUI8*  code;
  BSKUI32  length;
  BSKUI32  i;

  if( rule == 0 ) {
    return;
  }

  length = BSKStrLen( string ) + 1;
  code = (BSKUI8*)BSKMalloc( rule->codeLength + length );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  
  for( i = 0; i < length; i++ ) {
    code[ rule->codeLength + i ] = string[ i ];
  }

  rule->codeLength += length;
  rule->code = code;
}


static void s_emitBytes( BSKRule* rule, BSKNOTYPE buffer, BSKUI32 len ) {
  BSKUI8*  code;
  BSKUI32  i;
  BSKUI8*  bbuf;

  if( rule == 0 ) {
    return;
  }

  code = (BSKUI8*)BSKMalloc( rule->codeLength + len );
  if( rule->codeLength > 0 ) {
    BSKMemCpy( code, rule->code, rule->codeLength );
    BSKFree( rule->code );
  }
  
  bbuf = (BSKUI8*)buffer;
  for( i = 0; i < len; i++ ) {
    code[ rule->codeLength + i ] = bbuf[ i ];
  }

  rule->codeLength += len;
  rule->code = code;
}


static BSKUI32 s_forwardJump( BSKRule* rule ) {
  BSKUI32 savePC;

  if( rule == 0 ) {
    return 0;
  }

  /* emit a blank dword value into the code stream and return it's
   * address */

  savePC = rule->codeLength;
  s_emitDWord( rule, 0 );

  return savePC;
}


static void s_backFix( BSKRule* rule, BSKUI32 at ) {
  if( rule != 0 ) {
    /* copy the current address into the requested address */
    BSKMemCpy( &(rule->code[at]), &(rule->codeLength), sizeof( BSKUI32 ) );
  }
}


static BSKI32 s_parseRuleStatement( BSKParser* parser,
                                    BSKRule* rule,
                                    BSKBitSet* follow )
{
  switch( parser->currentToken.type ) {
    case TT_IDENTIFIER:
      s_parseRuleStatement_Identifier( parser, rule, follow );
      break;
    case TT_IF:
      s_parseRuleStatement_If( parser, rule, follow );
      break;
    case TT_WHILE:
      s_parseRuleStatement_While( parser, rule, follow );
      break;
    case TT_DO:
      s_parseRuleStatement_Do( parser, rule, follow );
      break;
    case TT_FOR:
      s_parseRuleStatement_For( parser, rule, follow );
      break;
    case TT_EXIT:
      s_parseRuleStatement_Exit( parser, rule, follow );
      break;
    case TT_CASE:
      s_parseRuleStatement_Case( parser, rule, follow );
      break;
  }

  return 0;
}


static BSKI32 s_parseRuleStatement_Identifier( BSKParser* parser, 
                                               BSKRule* rule, 
                                               BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32 count;
  BSKUI32 id;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_PUNCT_EQ, TT_PUNCT_LBRACKET, TT_PUNCT_DOT, 
                     TT_PUNCT_LPAREN, TT_PUNCT_SEMICOLON, 0 );

  /* if IDENT then
   *   eat IDENT
   *   emit "LID id"
   *   recurse
   */
  if( parser->currentToken.type == TT_IDENTIFIER ) {
    id = parser->currentToken.data.identifier;
    if( rule != 0 ) {

      /* if the symbol is not defined, define it as a variable */

      if( BSKGetSymbol( rule->symbols, id ) == 0 ) {
        BSKAddSymbol( rule->symbols, ST_VARIABLE, SF_NONE, id );
      }

      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
    }

    /* get the next token and recurse */

    s_getToken( parser );
    s_parseRuleStatement_Identifier( parser, rule, &keys );
  } else if( parser->currentToken.type == TT_PUNCT_DOT ) {
    /* elsif DOT then
     *   eat DOT
     *   expect IDENT
     *   emit "LID id"
     *   emit "DREF"
     *   recurse
     */
    s_getToken( parser );
    if( parser->currentToken.type != TT_IDENTIFIER ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
    } else {
      /* make sure the identifier is an attribute */
      if( !s_isAttributeID( parser->db, parser->currentToken.data.identifier ) ) {
        s_error( parser, PE_WRONG_TYPE, parser->currentToken.data.identifier, 0 );
      } else {
        s_emitOp( rule, OP_LID );
        s_emitID( rule, parser->currentToken.data.identifier );
        s_emitOp( rule, OP_DREF );
      }
      s_getToken( parser );
    }
    s_parseRuleStatement_Identifier( parser, rule, &keys );
  } else if( parser->currentToken.type == TT_PUNCT_LPAREN ) {
    /* elsif LPAREN then
     *   eat LPAREN
     *   parse-parameter-list
     *   eat RPAREN
     *   emit "CALL x"
     *   if not SEMICOLON then
     *     recurse
     *   else
     *     emit "POP"
     *   end
     */
    s_getToken( parser );
    count = s_parseParameterList( parser, rule, &keys );
    s_eatToken( parser, TT_PUNCT_RPAREN, &keys );
    s_emitOp( rule, OP_CALL );
    s_emitByte( rule, (BSKUI8)count );
    if( parser->currentToken.type != TT_PUNCT_SEMICOLON ) {
      s_parseRuleStatement_Identifier( parser, rule, &keys );
    } else {
      s_emitOp( rule, OP_POP );
      s_getToken( parser );
    }
  } else if( parser->currentToken.type == TT_PUNCT_LBRACKET ) {
    /* elsif LBRACKET then
     *   eat LBRACKET
     *   parse-expression
     *   eat RBRACKET
     *   emit "ELEM"
     *   recurse
     */
    s_getToken( parser );
    s_parseLogicalExpr( parser, rule, &keys );
    s_eatToken( parser, TT_PUNCT_RBRACKET, &keys );
    s_emitOp( rule, OP_ELEM );
    s_parseRuleStatement_Identifier( parser, rule, &keys );
  } else if( parser->currentToken.type == TT_PUNCT_EQ ) {
    /* elsif EQ then
     *   eat EQ
     *   parse-expression
     *   eat SEMICOLON
     *   emit "PUT"
     */
    s_getToken( parser );
    BSKCopyBitSet( &keys, follow );
    BSKSetBit( &keys, TT_PUNCT_SEMICOLON );
    s_parseLogicalExpr( parser, rule, &keys );
    s_emitOp( rule, OP_PUT );
    BSKClearBit( &keys, TT_PUNCT_SEMICOLON );
    s_eatToken( parser, TT_PUNCT_SEMICOLON, &keys );
  } else {
    BSKCopyBitSet( &keys, follow );
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  }

  return 0;
}


static BSKI32 s_parseRuleStatement_Case( BSKParser* parser, 
                                         BSKRule* rule, 
                                         BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32   svEFixups;

  svEFixups = parser->eFixups;

  s_getToken( parser );

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_IS, TT_DEFAULT, TT_END, 0 );

  /* get the value to test */

  s_parseLogicalExpr( parser, rule, &keys );

  /* parse the case list */

  BSKClearBits( &keys, TT_IS, 0 );
  s_parseRuleStatement_CaseList( parser, rule, &keys );

  /* parse the default case, if it exists */

  BSKClearBits( &keys, TT_DEFAULT, 0 );
  if( parser->currentToken.type == TT_DEFAULT ) {
    /* pop the comparison value from the stack */
    s_emitOp( rule, OP_POP );
    s_getToken( parser );
    s_parseRuleStatementList( parser, rule, &keys );
  }

  /* back patch any of the case statements that jumped to the end */
  while( parser->eFixups > svEFixups ) {
    s_backFix( rule, parser->exitFixups[ --parser->eFixups ] );
  }

  s_eatToken( parser, TT_END, &keys );

  return 0;
}


static BSKI32 s_parseRuleStatement_CaseList( BSKParser* parser, 
                                             BSKRule* rule, 
                                             BSKBitSet* follow )
{
  BSKUI32 nextCase;
  BSKBOOL isNot;

  nextCase = 0;

  while( parser->currentToken.type == TT_IS ) {
    s_getToken( parser );
    
    /* duplicate the comparison value */
    s_emitOp( rule, OP_DUP );

    isNot = BSKFALSE;
    if( parser->currentToken.type == TT_NOT ) {
      isNot = BSKTRUE;
      s_getToken( parser );
    }

    /* get the value to test against */

    s_parseFactor2( parser, rule, follow );

    /* test for equivelence */

    s_emitOp( rule, OP_EQ );

    /* if this is a NOT test, then skip the code for this case
     * if the EQ test is true, otherwise skip it if it is false. */

    if( isNot ) {
      s_emitOp( rule, OP_JMPT );
    } else {
      s_emitOp( rule, OP_JMPF );
    }
    nextCase = s_forwardJump( rule );

    s_eatToken( parser, TT_THEN, follow );

    /* this is a matching case -- pop the comparison value since we
     * don't need it anymore */

    s_emitOp( rule, OP_POP );
    s_parseRuleStatementList( parser, rule, follow );

    /* skip the remaining cases by jumping to the end of the case
     * statement */

    s_emitOp( rule, OP_JUMP );
    parser->exitFixups[ parser->eFixups++ ] = s_forwardJump( rule );

    s_backFix( rule, nextCase );
  }

  return 0;
}


static BSKI32 s_parseRuleStatement_If( BSKParser* parser,
                                       BSKRule* rule, 
                                       BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32   nextFix;
  BSKUI32   ifFixupCnt;
  BSKUI32   ifFixups[64];

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_THEN, TT_ELSEIF, TT_ELSE, TT_END, 0 );

  ifFixupCnt = 0;

  nextFix = 0;
  do {
    s_getToken( parser );

    /* get the test -- if it evaluates to false, skip this condition */

    s_parseLogicalExpr( parser, rule, &keys );
    s_emitOp( rule, OP_JMPF );
    nextFix = s_forwardJump( rule );

    BSKClearBit( &keys, TT_THEN );
    s_eatToken( parser, TT_THEN, &keys );

    s_parseRuleStatementList( parser, rule, &keys );

    /* jump over the remaining conditions to the end of the statement */

    s_emitOp( rule, OP_JUMP );
    ifFixups[ ifFixupCnt++ ] = s_forwardJump( rule );

    s_backFix( rule, nextFix );
  } while( parser->currentToken.type == TT_ELSEIF );

  /* if the else statement exists, parse it now */

  BSKClearBits( &keys, TT_ELSEIF, TT_ELSE, 0 );
  if( parser->currentToken.type == TT_ELSE ) {
    s_getToken( parser );
    s_parseRuleStatementList( parser, rule, &keys );
  }

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  /* back patch any of the if statements that jumped to the end */
  while( ifFixupCnt > 0 ) {
    s_backFix( rule, ifFixups[ --ifFixupCnt ] );
  }

  return 0;
}


static BSKI32 s_parseRuleStatement_While( BSKParser* parser, 
                                          BSKRule* rule, 
                                          BSKBitSet* follow )
{
  BSKUI32 testPos;
  BSKUI32 endFix;
  BSKBitSet keys;
  BSKUI32 svEfixes;
  BSKBOOL svLoop;

  svEfixes = parser->eFixups;

  s_getToken( parser );

  /* get the address of the top of the loop */

  testPos = rule->codeLength;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_DO, TT_IDENTIFIER, TT_IF, TT_WHILE, TT_DO, TT_FOR, TT_EXIT, TT_END, 0 );

  /* parse the test condition -- if it evaluates to false, jump out of the
   * loop */

  s_parseLogicalExpr( parser, rule, &keys );
  s_emitOp( rule, OP_JMPF );
  endFix = s_forwardJump( rule );

  BSKClearBit( &keys, TT_DO );
  s_eatToken( parser, TT_DO, &keys );

  svLoop = parser->inLoop;
  parser->inLoop = BSKTRUE;
  BSKClearBits( &keys, TT_IF, TT_WHILE, TT_DO, TT_FOR, TT_EXIT, 0 );
  s_parseRuleStatementList( parser, rule, &keys );
  parser->inLoop = svLoop;

  /* jump back to the top of the loop, to test the loop condition again */
  s_emitOp( rule, OP_JUMP );
  s_emitDWord( rule, testPos );

  /* back patch any of the "exit loop" parts */
  s_backFix( rule, endFix );
  while( parser->eFixups > svEfixes ) {
    s_backFix( rule, parser->exitFixups[ --parser->eFixups ] );
  }

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  return 0;
}


static BSKI32 s_parseRuleStatement_Do( BSKParser* parser, 
                                       BSKRule* rule, 
                                       BSKBitSet* follow )
{
  BSKUI32 startPos;
  BSKBitSet keys;
  BSKUI32 svEfixes;
  BSKBOOL svLoop;

  svEfixes = parser->eFixups;

  /* skip "DO" */
  s_getToken( parser );
  startPos = rule->codeLength;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_LOOP, TT_PUNCT_SEMICOLON, 0 );

  svLoop = parser->inLoop;
  parser->inLoop = BSKTRUE;
  /* parse the statements in the loop */
  s_parseRuleStatementList( parser, rule, &keys );
  parser->inLoop = svLoop;

  /* eat "LOOP" and "WHILE" */
  BSKSetBit( &keys, TT_WHILE );
  BSKClearBit( &keys, TT_LOOP );
  s_eatToken( parser, TT_LOOP, &keys );
  BSKClearBit( &keys, TT_WHILE );
  s_eatToken( parser, TT_WHILE, &keys );

  /* parse the expression */
  s_parseLogicalExpr( parser, rule, &keys );

  /* if the expression evaluates to true, jump back to the top of the loop */
  s_emitOp( rule, OP_JMPT );
  s_emitDWord( rule, startPos );

  /* eat a semicolon */
  BSKClearBit( &keys, TT_PUNCT_SEMICOLON );
  s_eatToken( parser, TT_PUNCT_SEMICOLON, &keys );

  /* back patch any of the "exit loop" parts */
  while( parser->eFixups > svEfixes ) {
    s_backFix( rule, parser->exitFixups[ --parser->eFixups ] );
  }

  return 0;
}


static BSKI32 s_parseRuleStatement_For( BSKParser* parser, 
                                        BSKRule* rule, 
                                        BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32   id;
  BSKSymbolTableEntry* sym;
  BSKUI32   fjump;
  BSKUI32   nextLoopPos;
  BSKUI32   endFix;
  BSKUI32   svEfixes;
  BSKBOOL   svLoop;

  id = 0;
  fjump = nextLoopPos = endFix = svLoop = 0;
  svEfixes = parser->eFixups;

  /* skip "FOR" */
  s_getToken( parser );

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_PUNCT_EQ, TT_TO, TT_IN, TT_DO, TT_END, hLOGEXPR, 0 );

  /* get the loop variable that will hold the value of the current
   * iteration */

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
    id = parser->currentToken.data.identifier;
    if( rule != 0 ) {
      BSKAddSymbol( rule->symbols, ST_VARIABLE, SF_NONE, id );
      sym = BSKGetSymbol( rule->symbols, id );
      if( sym->type != ST_VARIABLE ) {
        s_error( parser, PE_WRONG_TYPE, id, 0 );
      }
    }
    s_getToken( parser );
  }

  BSKClearBits( &keys, TT_PUNCT_EQ, TT_IN, 0 );

  endFix = 0;

  /* if an EQ '=' is encountered, this is a numeric loop, otherwise it is
   * a loop over all members of a category */

  if( parser->currentToken.type == TT_PUNCT_EQ ) {
    s_getToken( parser );

    /* initialization section */

    if( rule != 0 ) {
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
    }

    s_parseExpression( parser, rule, &keys );

    if( rule != 0 ) {
      s_emitOp( rule, OP_PUT );
      s_emitOp( rule, OP_JUMP );
      fjump = s_forwardJump( rule );

      /* increment the identifier */
      nextLoopPos = rule->codeLength;
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
      s_emitOp( rule, OP_DUP );
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, 1 );
      s_emitOp( rule, OP_ADD );
      s_emitOp( rule, OP_PUT );

      s_backFix( rule, fjump );
    }

    /* condition test */
    BSKClearBit( &keys, TT_TO );
    s_eatToken( parser, TT_TO, &keys );

    if( rule != 0 ) {
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
    }

    s_parseExpression( parser, rule, &keys );

    if( rule != 0 ) {
      s_emitOp( rule, OP_LE );
      s_emitOp( rule, OP_JMPF );
      endFix = s_forwardJump( rule );
    }
  } else if( parser->currentToken.type == TT_IN ) {
    /* initialization */
    s_getToken( parser );
    s_parseLogicalExpr( parser, rule, &keys );

    if( rule != 0 ) {
      s_emitOp( rule, OP_FIRST );
      s_emitOp( rule, OP_DUP );
      s_emitOp( rule, OP_GET );
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
      s_emitOp( rule, OP_SWAP );
      s_emitOp( rule, OP_PUT );
      s_emitOp( rule, OP_JUMP );
      fjump = s_forwardJump( rule );

      /* increment the identifier */
      nextLoopPos = rule->codeLength;
      s_emitOp( rule, OP_NEXT );
      s_emitOp( rule, OP_DUP );
      s_emitOp( rule, OP_GET );
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
      s_emitOp( rule, OP_SWAP );
      s_emitOp( rule, OP_PUT );

      /* condition test */
      s_backFix( rule, fjump );
      s_emitOp( rule, OP_NULL );
      s_emitOp( rule, OP_LID );
      s_emitID( rule, id );
      s_emitOp( rule, OP_EQ );
      s_emitOp( rule, OP_JMPF );
      fjump = s_forwardJump( rule );
      s_emitOp( rule, OP_POP );
      s_emitOp( rule, OP_JUMP );
      endFix = s_forwardJump( rule );
      s_backFix( rule, fjump );
    }
  } else {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_EQ, &keys );
  }

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, hSTATEMENT, TT_END, 0 );
  s_eatToken( parser, TT_DO, &keys );

  BSKCopyBitSet( &keys, follow );
  BSKSetBit( &keys, TT_END );

  svLoop = parser->inLoop;
  parser->inLoop = BSKTRUE;
  s_parseRuleStatementList( parser, rule, &keys );
  parser->inLoop = svLoop;

  /* jump back up to test the loop condition */
  s_emitOp( rule, OP_JUMP );
  s_emitDWord( rule, nextLoopPos );

  /* fix the end of the loop */
  s_backFix( rule, endFix );

  /* back patch any of the "exit loop" parts */
  while( parser->eFixups > svEfixes ) {
    s_backFix( rule, parser->exitFixups[ --parser->eFixups ] );
  }

  BSKClearBit( &keys, TT_END );
  s_eatToken( parser, TT_END, &keys );

  return 0;
}

static BSKI32 s_parseRuleStatement_Exit( BSKParser* parser, 
                                         BSKRule* rule, 
                                         BSKBitSet* follow )
{
  BSKBitSet keys;

  BSKCopyBitSet( &keys, follow );
  BSKSetBit( &keys, TT_PUNCT_SEMICOLON );

  s_getToken( parser );

  if( parser->currentToken.type == TT_LOOP ) {
    if( !parser->inLoop ) {
      s_error( parser, PE_EXIT_LOOP_NOT_IN_LOOP, 0, 0 );
    } else {
      /* jump out of the loop */
      s_emitOp( rule, OP_JUMP );
      parser->exitFixups[ parser->eFixups++ ] = s_forwardJump( rule );
    }
    s_getToken( parser );
  } else if( parser->currentToken.type == TT_RULE ) {
    s_emitOp( rule, OP_END );
    s_getToken( parser );
  } else {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_RULE, &keys );
  }

  s_eatToken( parser, TT_PUNCT_SEMICOLON, follow );

  return 0;
}


static BSKIdentList* s_parseIdentHead( BSKParser* parser,
                                       BSKRule* rule,
                                       BSKBitSet* follow )
{
  BSKIdentList* list;
  BSKUI32       id;

  /* ident-head := ident                *
   *             | ident "." ident-head */

  list = 0;
  while( parser->currentToken.type == TT_IDENTIFIER ) {
    id = parser->currentToken.data.identifier;
    if( rule != 0 ) {
      if( BSKGetSymbol( rule->symbols, id ) == 0 ) {
        BSKAddSymbol( rule->symbols, ST_VARIABLE, SF_NONE, id );
      }
    }
    list = s_addToIdentList( list, id );
    s_getToken( parser );
    if( parser->currentToken.type == TT_PUNCT_DOT ) {
      s_getToken( parser );
      if( parser->currentToken.type != TT_IDENTIFIER ) {
        s_error( parser, PE_UNEXPECTED_TOKEN, parser->currentToken.type, follow );
      } else if( !s_isAttributeID( parser->db, parser->currentToken.data.identifier ) ) {
        s_error( parser, PE_WRONG_TYPE, parser->currentToken.data.identifier, 0 );
      }
    }
  }

  return s_reverseIdentList( list );
}


static void s_emitIdentHead( BSKRule* rule, BSKIdentList* list ) {
  BSKIdentList* i;
  BSKUI32 count;

  count = s_getIdentListLength( list );
  if( count == 1 ) {
    s_emitOp( rule, OP_LID );
    s_emitID( rule, list->id );
  } else if( count > 1 ) {
    s_emitOp( rule, OP_LID );
    s_emitID( rule, list->id );
    for( i = list->next; i != 0; i = i->next ) {
      s_emitOp( rule, OP_LID );
      s_emitID( rule, i->id );
      s_emitOp( rule, OP_DREF );
    }
  }
}


static BSKUI32 s_getIdentListLength( BSKIdentList* list ) {
  BSKIdentList* i;
  BSKUI32 count;

  for( count = 0, i = list; i != 0; i = i->next, count++ )
     /* loop */ ;

  return count;
}


static BSKUI32 s_parseParameterList( BSKParser* parser,
                                     BSKRule* rule,
                                     BSKBitSet* follow )
{
  BSKBitSet heads;
  BSKBitSet keys;
  BSKUI32 count;

  BSKClearBitSet( &heads );
  BSKSetBits( &heads, TT_IDENTIFIER,
                      TT_NUMBER,
                      TT_DICE,
                      TT_STRING,
                      hBOOLEAN,
                      TT_PUNCT_LPAREN,
                      TT_NOT,
                      TT_NULL,
                      TT_CATEGORY,
                      TT_RULE,
                      TT_THING,
                      TT_NUMBER_WORD,
                      TT_STRING_WORD,
                      TT_BOOLEAN_WORD,
                      0 );
  BSKOrBitSets( &keys, follow, &heads );

  count = 0;
  while( BSKTestBit( &heads, parser->currentToken.type ) ) {
    s_parseLogicalExpr( parser, rule, &keys );
    count++;
    if( parser->currentToken.type == TT_PUNCT_COMMA ) {
      s_getToken( parser );
    } else if( BSKTestBit( &heads, parser->currentToken.type ) ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_PUNCT_COMMA, &keys );
    } else {
      break;
    }
  }

  return count;
}


static BSKI32 s_parseLogicalExpr( BSKParser* parser,
                                 BSKRule* rule,
                                 BSKBitSet* follow )
{ 
  BSKBitSet keys;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKSetBit( &keys, TT_OR );
  op = 0;

  do {
    s_parseLogicalExpr2( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    if( parser->currentToken.type == TT_OR ) {
      s_getToken( parser );
      op = OP_OR;
    } else {
      break;
    }
  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseLogicalExpr2( BSKParser* parser,
                                   BSKRule* rule,
                                   BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKSetBit( &keys, TT_AND );

  op = 0;
  do {
    s_parseLogicalExpr3( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    if( parser->currentToken.type == TT_AND ) {
      s_getToken( parser );
      op = OP_AND;
    } else {
      break;
    }
  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseLogicalExpr3( BSKParser* parser,
                                   BSKRule* rule,
                                   BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet opHeads;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKClearBitSet( &opHeads );

  BSKSetBits( &opHeads, TT_EQ, TT_NE, TT_LT, TT_GT, TT_LE, TT_GE, TT_TYPEOF, 0 );
  BSKOrBitSets( &keys, &keys, &opHeads );

  op = 0;
  do {
    s_parseExpression( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    op = parser->currentToken.type;
    if( BSKTestBit( &opHeads, op ) ) {
      s_getToken( parser );

      switch( op ) {
        case TT_EQ: op = OP_EQ; break;
        case TT_NE: op = OP_NE; break;
        case TT_LT: op = OP_LT; break;
        case TT_GT: op = OP_GT; break;
        case TT_LE: op = OP_LE; break;
        case TT_GE: op = OP_GE; break;
        case TT_TYPEOF: op = OP_TYPEOF; break;
      }
    } else {
      break;
    }

  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseExpression( BSKParser* parser,
                                 BSKRule* rule,
                                 BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet opHeads;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKClearBitSet( &opHeads );

  BSKSetBits( &opHeads, TT_PUNCT_PLUS, TT_PUNCT_DASH, 0 );
  BSKOrBitSets( &keys, &keys, &opHeads );

  op = 0;
  do {
    s_parseTerm( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    op = parser->currentToken.type;
    if( BSKTestBit( &opHeads, op ) ) {
      s_getToken( parser );

      switch( op ) {
        case TT_PUNCT_PLUS: op = OP_ADD; break;
        case TT_PUNCT_DASH: op = OP_SUB; break;
      }
    } else {
      break;
    }
  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseTerm( BSKParser* parser,
                           BSKRule* rule,
                           BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet opHeads;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKClearBitSet( &opHeads );

  BSKSetBits( &opHeads, TT_PUNCT_STAR, TT_PUNCT_SLASH, TT_PUNCT_PERCENT, 0 );
  BSKOrBitSets( &keys, &keys, &opHeads );

  op = 0;
  do {
    s_parseFactor( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    op = parser->currentToken.type;
    if( BSKTestBit( &opHeads, op ) ) {
      s_getToken( parser );

      switch( op ) {
        case TT_PUNCT_STAR:    op = OP_MUL; break;
        case TT_PUNCT_SLASH:   op = OP_DIV; break;
        case TT_PUNCT_PERCENT: op = OP_MOD; break;
      }
    } else {
      break;
    }
  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseFactor( BSKParser* parser,
                             BSKRule* rule,
                             BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKBitSet opHeads;
  BSKUI32 op;

  BSKCopyBitSet( &keys, follow );
  BSKClearBitSet( &opHeads );

  BSKSetBit( &opHeads, TT_PUNCT_CARET );
  BSKOrBitSets( &keys, &keys, &opHeads );

  op = 0;
  do {
    s_parseFactor2( parser, rule, &keys );

    if( op != 0 ) {
      s_emitOp( rule, op );
    }

    op = parser->currentToken.type;
    if( BSKTestBit( &opHeads, op ) ) {
      s_getToken( parser );

      switch( op ) {
        case TT_PUNCT_CARET: op = OP_POW; break;
      }
    } else {
      break;
    }
  } while( BSKTRUE );

  return 0;
}


static BSKI32 s_parseFactor2( BSKParser* parser,
                              BSKRule* rule,
                              BSKBitSet* follow )
{
  switch( parser->currentToken.type ) {
    case TT_IDENTIFIER:
      s_parseFactor2_Identifier( parser, rule, follow );
      break;
    case TT_STRING:
      s_emitOp( rule, OP_LSTR );
      s_emitString( rule, parser->currentToken.data.strValue );
      s_getToken( parser );
      break;
    case TT_PUNCT_PLUS:
    case TT_PUNCT_DASH:
    case TT_NUMBER:
    case TT_DICE:
      s_parseFactor2_Number( parser, rule, follow );
      break;
    case TT_YES: case TT_ON: case TT_TRUE:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, 1 );
      s_getToken( parser );
      break;
    case TT_NO: case TT_OFF: case TT_FALSE:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, 0 );
      s_getToken( parser );
      break;
    case TT_PUNCT_LPAREN:
      s_getToken( parser );
      s_parseLogicalExpr( parser, rule, follow );
      s_eatToken( parser, TT_PUNCT_RPAREN, follow );
      break;
    case TT_NOT:
      s_getToken( parser );
      s_parseFactor( parser, rule, follow );
      s_emitOp( rule, OP_NOT );
      break;
    case TT_NULL:
      s_getToken( parser );
      s_emitOp( rule, OP_NULL );
      break;
    case TT_CATEGORY:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_CATEGORY );
      s_getToken( parser );
      break;
    case TT_RULE:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_RULE );
      s_getToken( parser );
      break;
    case TT_THING:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_THING );
      s_getToken( parser );
      break;
    case TT_NUMBER_WORD:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_NUMBER );
      s_getToken( parser );
      break;
    case TT_STRING_WORD:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_STRING );
      s_getToken( parser );
      break;
    case TT_BOOLEAN_WORD:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_BOOLEAN );
      s_getToken( parser );
      break;
    case TT_ARRAY:
      s_emitOp( rule, OP_LBYTE );
      s_emitByte( rule, TID_ARRAY );
      s_getToken( parser );
      break;
  }

  return 0;
}


static BSKI32 s_parseFactor2_Identifier( BSKParser* parser,
                                         BSKRule* rule,
                                         BSKBitSet* follow )
{
  BSKIdentList* list;
  BSKIdentList* i;
  BSKUI32 count;

  list = s_parseIdentHead( parser, rule, follow );

  if( parser->currentToken.type == TT_PUNCT_LPAREN ) {
    s_getToken( parser );
    s_emitIdentHead( rule, list );
    count = s_parseParameterList( parser, rule, follow );
    s_eatToken( parser, TT_PUNCT_RPAREN, follow );
    s_emitOp( rule, OP_CALL );
    s_emitByte( rule, (BSKUI8)count );
    s_parseFactor2_Identifier( parser, rule, follow );
  } else if( parser->currentToken.type == TT_PUNCT_LBRACKET ) {
    s_getToken( parser );
    s_emitIdentHead( rule, list );
    s_parseExpression( parser, rule, follow );
    s_eatToken( parser, TT_PUNCT_RBRACKET, follow );
    s_emitOp( rule, OP_ELEM );
    s_parseFactor2_Identifier( parser, rule, follow );
  } else if( parser->currentToken.type == TT_PUNCT_DOT ) {
    s_getToken( parser );
    list = s_parseIdentHead( parser, rule, follow );
    if( list == 0 ) {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, follow );
    } else {
      s_emitOp( rule, OP_LID );
      s_emitID( rule, list->id );
      s_emitOp( rule, OP_DREF );
      for( i = list->next; i != 0; i = i->next ) {
        s_emitOp( rule, OP_LID );
        s_emitID( rule, i->id );
        s_emitOp( rule, OP_DREF );
      }
    }
    s_parseFactor2_Identifier( parser, rule, follow );
  } else {
    s_emitIdentHead( rule, list );
  }

  s_destroyIdentList( list );

  return 0;
}


static BSKI32 s_parseFactor2_Number( BSKParser* parser,
                                     BSKRule* rule,
                                     BSKBitSet* follow )
{
  BSKI8    sign;
  BSKValue value;
  BSKBOOL  sawSign;

  sign = 1;
  sawSign = BSKFALSE;
  switch( parser->currentToken.type ) {
    case TT_PUNCT_PLUS: sawSign = BSKTRUE; sign = 1; s_getToken( parser ); break;
    case TT_PUNCT_DASH: sawSign = BSKTRUE; sign = -1; s_getToken( parser ); break;
  }

  switch( parser->currentToken.type ) {
    case TT_NUMBER:
      BSKSetValueNumber( &value, parser->currentToken.data.dblValue );
      switch( value.type ) {
        case VT_BYTE:
          s_emitOp( rule, OP_LBYTE );
          s_emitByte( rule, (BSKUI8)( sign * *(BSKUI8*)value.datum ) );
          break;
        case VT_WORD:
          s_emitOp( rule, OP_LWORD );
          s_emitWord( rule, (BSKUI16)( sign * *(BSKUI16*)value.datum ) );
          break;
        case VT_DWORD:
          s_emitOp( rule, OP_LDWORD );
          s_emitDWord( rule, (BSKUI32)( sign * *(BSKUI32*)value.datum ) );
          break;
        case VT_FLOAT:
          s_emitOp( rule, OP_LFLOAT );
          *(BSKFLOAT*)value.datum *= sign;
          s_emitBytes( rule, value.datum, sizeof( BSKFLOAT ) );
          break;
      }
      s_getToken( parser );
      BSKInvalidateValue( &value );
      break;
    case TT_DICE:
      s_emitOp( rule, OP_LDICE );
      s_emitWord( rule, (BSKUI16)( sign * parser->currentToken.data.dice.dCount ) );
      s_emitWord( rule, parser->currentToken.data.dice.dType );
      s_getToken( parser );
      break;
    case TT_IDENTIFIER:
      if( sawSign ) {
        s_parseFactor2( parser, rule, follow );
        if( sign == -1 ) {
          s_emitOp( rule, OP_NEG );
        }
      }
      break;        
  }

  return 0;
}


static BSKI32 s_metaDef( BSKParser* parser,
                         BSKBitSet* follow )
{
  BSKBitSet keys;

  BSKCopyBitSet( &keys, follow );

  s_eatToken( parser, TT_PUNCT_OCTOTHORP, &keys );
  if( parser->currentToken.type == TT_INCLUDE ) {
    s_getToken( parser );
    if( parser->currentToken.type == TT_STRING ) {
      BSKStream* subStream;
      BSKCHAR    streamName[512];
      BSKCHAR*   p;

      /* FIXME: make this more flexible so that the included stream may
       * or may not be a file. */

      if( !s_fileIsIncluded( parser, parser->currentToken.data.strValue ) ) {
        s_addFileToIncludedList( parser, parser->currentToken.data.strValue );
        p = s_findStreamInPaths( parser, 
                                 parser->currentToken.data.strValue,
                                 streamName,
                                 sizeof( streamName ) );
        if( p != NULL ) {
          subStream = BSKStreamOpenFile( streamName, "rt" );
          s_parseSubStream( parser, subStream, streamName );
          subStream->close( subStream );
        } else {
          s_error( parser, PE_CANNOT_OPEN_FILE, (BSKUI32)parser->currentToken.data.strValue, 0 );
        }
      }
      
      s_getToken( parser );
    } else {
      s_error( parser, PE_UNEXPECTED_TOKEN, TT_STRING, &keys );
    }
  } else {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_INCLUDE, &keys );
  }

  return 0;
}


static BSKI32 s_forwardDef( BSKParser* parser,
                            BSKBitSet* follow )
{
  BSKBitSet keys;
  BSKUI32   type;

  BSKCopyBitSet( &keys, follow );
  BSKSetBits( &keys, TT_THING, TT_CATEGORY, TT_RULE, TT_IDENTIFIER, 0 );

  s_eatToken( parser, TT_FORWARD, &keys );
  BSKClearBits( &keys, TT_THING, TT_CATEGORY, TT_RULE, 0 );

  type = parser->currentToken.type;
  if( ( type != TT_THING ) && ( type != TT_CATEGORY ) && ( type != TT_RULE ) ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_RULE, &keys );
  } else {  
    switch( type ) {
      case TT_THING: type = ST_THING; break;
      case TT_CATEGORY: type = ST_CATEGORY; break;
      case TT_RULE: type = ST_RULE; break;
    }
    s_getToken( parser );
  }

  if( parser->currentToken.type != TT_IDENTIFIER ) {
    s_error( parser, PE_UNEXPECTED_TOKEN, TT_IDENTIFIER, &keys );
  } else {
    BSKSymbolTableEntry* sym;
    BSKUI32              ident;
    BSKRule*             rule;

    /* if the symbol has already been defined, it is only an error if the
     * redefinition of the symbol has a different type than the original.
     * if the types are the same, we just exit.  If the symbol has never
     * been defined, then we just create an empty object and add it to the
     * database. */

    ident = parser->currentToken.data.identifier;
    sym = BSKGetSymbol( parser->symbols, ident );
    if( sym != 0 ) {
      if( type != sym->type ) {
        s_error( parser, PE_REDEFINED_IDENTIFIER, ident, 0 );
        ident = 0;
      }
    } else {
      BSKAddSymbol( parser->symbols, (BSKUI8)type, SF_FORWARD_DECL, ident );

      /* add a new, empty object to it's respective list */
      switch( type ) {
        case ST_THING:
          BSKAddThingToDB( parser->db, BSKNewThing( ident ) );
          break;
        case ST_CATEGORY:
          BSKAddCategoryToDB( parser->db, BSKNewCategory( ident ) );
          break;
        case ST_RULE:
          rule = BSKAddRuleToDB( parser->db, BSKNewRule( ident ) );
          rule->symbols = BSKNewSymbolTable();
          rule->symbols->parent = parser->symbols;
          break;
      }
    }

    s_getToken( parser );
  }

  return 0;
}


static BSKBOOL s_isAttributeID( BSKDatabase* db, BSKUI32 id ) {
  BSKSymbolTableEntry* sym;

  sym = BSKGetSymbol( db->symbols, id );
  if( sym == 0 ) {
    return BSKFALSE;
  }

  return ( sym->type == ST_ATTRIBUTE );
}


static BSKBOOL s_fileIsIncluded( BSKParser* parser, BSKCHAR* name ) {
  int i;

  /* this does a case-sensitive comparison */

  for( i = 0; i < parser->includedCount; i++ ) {
    if( BSKStrCmp( parser->includedFiles[ i ], name ) == 0 ) {
      return BSKTRUE;
    }
  }

  return BSKFALSE;
}


static void s_addFileToIncludedList( BSKParser* parser, BSKCHAR* name ) {
  BSKCHAR** temp;

  temp = (BSKCHAR**)BSKMalloc( ( parser->includedCount + 1 ) * sizeof( BSKCHAR* ) );

  if( parser->includedCount > 0 ) {
    BSKMemCpy( temp, parser->includedFiles, 
               sizeof( BSKCHAR* ) * parser->includedCount );
    BSKFree( parser->includedFiles );
  }

  parser->includedFiles = temp;
  parser->includedFiles[ parser->includedCount++ ] = BSKStrDup( name );
}


static BSKCHAR* s_findStreamInPaths( BSKParser* parser,
                                     BSKCHAR* name,
                                     BSKCHAR* dest,
                                     BSKUI32 destLen )
{
  BSKCHAR* p;
  BSKCHAR* sep;
  BSKUI32  sepLen;
  BSKUI32  len;

  p = parser->searchPaths;
  sep = FILE_SEPARATOR;
  sepLen = BSKStrLen( sep );

  /* loop over all the paths in the search path list */
  while( *p != '\0' ) {

    /* build the filename to look for */
    BSKStrNCpy( dest, p, destLen );
    len = BSKStrLen( dest );
    if( len+sepLen < destLen ) {
      if( BSKStrCmp( &(dest[len-sepLen]), sep ) != 0 ) {
        BSKStrCat( dest, sep );
        len = BSKStrLen( dest );
      }
    }
    if( len + BSKStrLen( name ) + 1 < destLen ) {
      BSKStrCat( dest, name );
      len = BSKStrLen( dest );
    }

    /* look to see if the file exists */
    if( s_fileExists( dest ) ) {
      return dest;
    }

    /* if not, get the next search path */
    while( *p != '\0' ) {
      p++;
    }
    p++;
  }

  return 0;
}


static BSKBOOL s_fileExists( BSKCHAR* file ) {
#ifndef __PALM__
  FILE* f;

  f = fopen( file, "r" );
  if( f == NULL ) {
    return BSKFALSE;
  }
  fclose( f );
  return BSKTRUE;
#else
  return BSKFALSE;
#endif
}


static void s_addMemberToCategories( BSKParser* parser,
		                                 BSKCategoryMember* member,
																		 BSKWeightedIdentList* list )
{
	BSKWeightedIdentList* i;
	BSKCategory* cat;
	BSKSymbolTableEntry* sym;

	for( i = list; i != 0; i = i->next ) {
		/* has the symbol been declared already? */
		sym = BSKGetSymbol( parser->symbols, i->id );
		if( sym == 0 ) { /* if not, add it */
			BSKAddSymbol( parser->symbols, ST_CATEGORY, SF_NONE, i->id );
			cat = BSKAddCategoryToDB( parser->db, BSKNewCategory( i->id ) );
			BSKAddToCategory( cat, i->weight, member );
		} else if( sym->type != ST_CATEGORY ) {
			s_error( parser, PE_WRONG_TYPE, i->id, 0 );
		} else {
			sym->flags = SF_NONE; /* mark it as parsed, if this was previously declared "forward" */
			cat = BSKFindCategory( parser->db, i->id );
			if( cat == 0 ) {
				s_error( parser, PE_BUG_DETECTED, (BSKUI32)"category symbol exists in symbol table, but not in category list (s_parseThingDef)", 0 );
			} else {
				BSKAddToCategory( cat, i->weight, member );
			}
		}
	}
}

