#include <stdio.h>
#include "bskenv.h"
#include "bskdb.h"

#include "bskparse.h"
#include "bskexec.h"
#include "bskrule.h"

#include "bskcallbacks.h"


BSKI32 myConsole( BSKCHAR* msg,
                  BSKExecutionEnvironment* env,
                  BSKNOTYPE userData )
{
  printf( "%s", msg );
  return 0;
}


BSKI32 myRTEHandler( BSKI32 code,
                     BSKCHAR* msg,
                     BSKExecutionEnvironment* env,
                     BSKNOTYPE userData )
{
  BSKRule* rule;
  BSKCHAR name[100];
  BSKStackFrame* frame;

  printf( "\n" );
  switch( code ) {
    case RTE_SUCCESS:
      printf( "*** no error" );
      break;
    case RTE_INVALID_RULE:
      printf( "*** invalid rule" );
      break;
    case RTE_BUG:
      printf( "*** bug detected" );
      break;
    case RTE_STACK_UNDERFLOW:
      printf( "*** stack underflow" );
      break;
    case RTE_INVALID_OPERANDS:
      printf( "*** invalid operands" );
      break;
    case RTE_DIVIDE_BY_ZERO:
      printf( "*** divide by zero" );
      break;
    case RTE_UNKNOWN_INSTRUCTION:
      printf( "*** unknown instruction" );
      break;
    case RTE_WRONG_PARAM_COUNT:
      printf( "*** wrong parameter count" );
      break;
    case RTE_DOMAIN_ERROR:
      printf( "*** domain error" );
      break;
    case RTE_WRONG_UNITS:
      printf( "*** wrong units" );
      break;
    case RTE_CALL_OF_NONFUNCTION:
      printf( "*** call of non-function" );
      break;
  }

  frame = BSKExecEnvironmentGetCurrentFrame( env );

  printf( " (" );
  if( frame == 0 ) {
    printf( "main: " );
  } else {
    rule = BSKFindRule( BSKExecEnvironmentGetDB( env )->rules, BSKStackFrameGetRuleID( frame ) );
    BSKGetIdentifier( BSKExecEnvironmentGetDB( env )->idTable, BSKRuleGetID( rule ), 
                      name, sizeof( name ) );
    if( rule->file != 0 ) {
      printf( "%s:", BSKRuleGetSourceFile( rule ) );
    }
    printf( "%s:", name );
  }
  printf( "%ld)", BSKExecEnvironmentGetCurrentLine( env ) );

  if( msg != 0 ) {
    printf( " (%s)", msg );
  }
  printf( "\n" );

  printStackTrace( env );
  printf( "\n" );

  return 0;
}


BSKI32 myErrorHandler( BSKI32 code,
                       BSKDatabase* db,
                       BSKCHAR* file,
                       BSKToken* token,
                       BSKUI32 data,
                       BSKNOTYPE userData )
{
  BSKCHAR buffer1[ 128 ];
  BSKCHAR buffer2[ 128 ];

  switch( code ) {
    case PE_NOERROR:
      printf( "*** no error\n" );
      break;
    case PE_UNEXPECTED_TOKEN:
      BSKGetTokenDescription( (BSKTokenId)data, buffer1, sizeof( buffer1 ) );
      BSKGetTokenDescription( token->type, buffer2, sizeof( buffer2 ) );
      printf( "*** [%s:%ld,%ld] expected %s, found %s\n", file, token->row, token->col, buffer1, buffer2 );
      break;
    case PE_REDEFINED_IDENTIFIER:
      BSKGetIdentifier( db->idTable, data, buffer1, sizeof( buffer1 ) );
      printf( "*** [%s:%ld,%ld] identifier '%s' redefined\n", file, token->row, token->col, buffer1 );
      break;
    case PE_UNDECLARED_IDENTIFIER:
      BSKGetIdentifier( db->idTable, data, buffer1, sizeof( buffer1 ) );
      printf( "*** [%s:%ld,%ld] identifier '%s' undeclared\n", file, token->row, token->col, buffer1 );
      break;
    case PE_WRONG_TYPE:
      BSKGetIdentifier( db->idTable, data, buffer1, sizeof( buffer1 ) );
      printf( "*** [%s:%ld,%ld] identifier '%s' of wrong type\n", file, token->row, token->col, buffer1 );
      break;
    case PE_BUG_DETECTED:
      printf( "*** [%s:%ld,%ld] bug detected (%s)\n", file, token->row, token->col, (BSKCHAR*)data );
      break;
    case PE_TOO_MANY_ATTRIBUTES:
      printf( "*** [%s:%ld,%ld] too many attributes\n", file, token->row, token->col );
      break;
    case PE_EXIT_LOOP_NOT_IN_LOOP:
      printf( "*** [%s:%ld,%ld] \"exit loop\" may only be used in a loop context\n", file, token->row, token->col );
      break;
    case PE_CANNOT_OPEN_FILE:
      printf( "*** [%s:%ld,%ld] cannot open file '%s'\n", file, token->row, token->col, (BSKCHAR*)data );
      break;
    case PE_FORWARD_NOT_DEFINED:
      BSKGetIdentifier( db->idTable, data, buffer1, sizeof( buffer1 ) );
      printf( "*** forwarded identifier '%s' never defined\n", buffer1 );
      break;
    default:
      printf( "*** [%s:%ld,%ld] unknown error %ld\n", file, token->row, token->col, code );
  }

  return 0;
}


void displayRuleCode( BSKDatabase* db, BSKRule* rule ) {
  BSKUI32 pc;
  BSKUI32 ival;
  BSKCHAR buffer[128];
  BSKUI8* code;

  code = BSKRuleGetCode( rule );

  for( pc = 0; pc < BSKRuleGetCodeLength( rule ); pc++ ) {
    printf( "   %03lX: ", pc );
    switch( code[pc] ) {
      case OP_NOOP: printf( "NOOP\n" ); break;
      case OP_LSTR:
        printf( "LSTR   \"" );
        pc++;
        while( code[pc] != 0 ) {
          putc( code[pc], stdout );
          pc++;
        }
        printf( "\"\n" );
        break;
      case OP_LBYTE:
        pc++;
        printf( "LBYTE  %ld\n", (BSKI32)code[pc] );
        break;
      case OP_LWORD:
        pc++;
        printf( "LWORD  %ld\n", (BSKI32)*(BSKI16*)(&code[pc]) );
        pc += sizeof( BSKI16 ) - 1;
        break;
      case OP_LDWORD:
        pc++;
        printf( "LDWORD %ld\n", *(BSKI32*)(&code[pc]) );
        pc += sizeof( BSKI32 ) - 1;
        break;
      case OP_LFLOAT:
        pc++;
        printf( "LFLOAT %g\n", *(BSKFLOAT*)(&code[pc]) );
        pc += sizeof( BSKFLOAT ) - 1;
        break;
      case OP_LDICE:
        {
          BSKI16 count;
          BSKUI16 type;

          pc++;
          count = *(BSKI16*)(&code[pc]);
          pc += sizeof( BSKI16 );
          type = *(BSKUI16*)(&code[pc]);
          pc += sizeof( BSKUI16 ) - 1;
          printf( "LDICE  %dd%d\n", count, type );
        }
        break;
      case OP_LID:
        pc++;
        BSKMemCpy( &ival, &(code[pc]), sizeof( ival ) );
        pc += sizeof( ival )-1;
        BSKGetIdentifier( db->idTable, ival, buffer, sizeof( buffer ) );
        printf( "LID    %s\n", buffer );
        break;
      case OP_NULL: printf( "NULL\n" ); break;
      case OP_CALL:
        pc++;
        printf( "CALL   %ld\n", (BSKUI32)code[pc] );
        break;
      case OP_ADD: printf( "ADD\n" ); break;
      case OP_SUB: printf( "SUB\n" ); break;
      case OP_MUL: printf( "MUL\n" ); break;
      case OP_DIV: printf( "DIV\n" ); break;
      case OP_POW: printf( "POW\n" ); break;
      case OP_NEG: printf( "NEG\n" ); break;
      case OP_DREF: printf( "DREF\n" ); break;
      case OP_LT: printf( "LT\n" ); break;
      case OP_GT: printf( "GT\n" ); break;
      case OP_LE: printf( "LE\n" ); break;
      case OP_GE: printf( "GE\n" ); break;
      case OP_EQ: printf( "EQ\n" ); break;
      case OP_NE: printf( "NE\n" ); break;
      case OP_TYPEOF: printf( "TYPEOF\n" ); break;
      case OP_NOT: printf( "NOT\n" ); break;
      case OP_JUMP:
        pc++;
        BSKMemCpy( &ival, &(code[pc]), sizeof( ival ) );
        pc += sizeof(ival) - 1;
        printf( "JUMP   %03lX\n", ival );
        break;
      case OP_JMPT:
        pc++;
        BSKMemCpy( &ival, &(code[pc]), sizeof( ival ) );
        pc += sizeof(ival) - 1;
        printf( "JMPT   %03lX\n", ival );
        break;
      case OP_JMPF:
        pc++;
        BSKMemCpy( &ival, &(code[pc]), sizeof( ival ) );
        pc += sizeof(ival) - 1;
        printf( "JMPF   %03lX\n", ival );
        break;
      case OP_END: printf( "END\n" ); break;
      case OP_GET: printf( "GET\n" ); break;
      case OP_PUT: printf( "PUT\n" ); break;
      case OP_AND: printf( "AND\n" ); break;
      case OP_OR: printf( "OR\n" ); break;
      case OP_MOD: printf( "MOD\n" ); break;
      case OP_FIRST: printf( "FIRST\n" ); break;
      case OP_NEXT: printf( "NEXT\n" ); break;
      case OP_DUP: printf( "DUP\n" ); break;
      case OP_POP: printf( "POP\n" ); break;
      case OP_ELEM: printf( "ELEM\n" ); break;
      case OP_SWAP: printf( "SWAP\n" ); break;
      case OP_LINE:
        pc++;
        printf( "LINE   %ld\n", *(BSKUI32*)(&code[pc]) );
        pc += sizeof( BSKUI32 ) - 1;
        break;
      default:
        printf( "** UNKNOWN OPCODE %ld **\n", (BSKUI32)code[pc] );
    }
  }
}


void dumpThing( BSKDatabase* db, BSKThing* thing ) {
  BSKAttribute* attribute;
  BSKCHAR       buffer[ 128 ];
  BSKNOTYPE     ptr;
  BSKValue*     value;

  if( BSKThingGetID( thing ) < 1 ) {
    strcpy( buffer, "{anonymous}" );
  } else {
    BSKGetIdentifier( db->idTable, BSKThingGetID( thing ), buffer, sizeof( buffer ) );
  }
  printf( " - %s (", buffer );

  for( attribute = BSKThingGetAttributeList( thing ); attribute != 0; attribute = attribute->next ) {
    BSKGetIdentifier( db->idTable, BSKThingAttributeGetID( attribute ), buffer, sizeof( buffer ) );
    printf( "%s{", buffer );
    value = BSKThingAttributeGetValue( attribute );
    switch( BSKValueGetType( value ) ) {
      case VT_NULL:
        printf( "none" );
        break;
      case VT_BYTE:
        printf( "%d", *(BSKI8*)BSKValueGetData( value ) );
        ptr = ((BSKI8*)BSKValueGetData( value ))+1;
        break;
      case VT_WORD:
        printf( "%d", *(BSKI16*)BSKValueGetData( value ) );
        ptr = ((BSKI16*)BSKValueGetData( value ))+1;
        break;
      case VT_DWORD:
        printf( "%ld", *(BSKI32*)BSKValueGetData( value ) );
        ptr = ((BSKI32*)BSKValueGetData( value ))+1;
        break;
      case VT_FLOAT:
        printf( "%g", *(BSKFLOAT*)BSKValueGetData( value ) );
        ptr = ((BSKFLOAT*)BSKValueGetData( value ) )+1;
        break;
      case VT_DICE:
        {
          BSKI16 count;
          BSKUI16 type;
          BSKCHAR op;
          BSKI16 modifier;

          BSKGetDiceParts( value, &count, &type, &op, &modifier );
          printf( "%dd%d", count, type );
          if( ( BSKValueGetFlags( value ) & VF_MODIFIER ) != 0 ) {
            if( ( BSKValueGetFlags( value ) & VF_MULTIPLICATIVE ) != 0 ) {
              printf( "*%d", modifier );
            } else {
              printf( "%+d", modifier );
            }
          }
        }
        break;
      case VT_STRING:
        printf( "%s", BSKValueGetString( value ) );
        break;
      case VT_THING:
        {
          BSKThing* thing;
          thing = BSKValueGetThing( value );
          if( BSKThingGetID( thing ) < 1 ) {
            printf( "--anonymous--" );
          } else {
            BSKGetIdentifier( db->idTable, BSKThingGetID( thing ), buffer, sizeof( buffer ) );
            printf( "%s", buffer );
          }
        }
        break;
      case VT_CATEGORY:
        {
          BSKCategory* category;
          category = BSKValueGetCategory( value );
          BSKGetIdentifier( db->idTable, BSKCategoryGetID( category ), buffer, sizeof( buffer ) );
          printf( "%s", buffer );
        }
        break;
      case VT_RULE:
        {
          BSKRule* rule;
          rule = BSKValueGetRule( value );
          BSKGetIdentifier( db->idTable, BSKRuleGetID( rule ), buffer, sizeof( buffer ) );
          printf( "%s", buffer );
        }
        break;
      default:
        printf( "-- error(%d) --", BSKValueGetType( value ) );
    }
    printf( "}" );
    if( attribute->next != 0 ) {
      printf( " " );
    }
  }
  printf( ")\n" );
}

void dumpRules( BSKDatabase* db ) {
  BSKRule* rule;
  BSKCHAR buffer[1024];
  BSKUI32 i;
  BSKUI32* parms;

  printf( "\n*** Rules:\n" );
  for( rule = db->rules; rule != 0; rule = rule->next ) {
    BSKGetIdentifier( db->idTable, BSKRuleGetID( rule ), buffer, sizeof( buffer ) );
    printf( " - %s(", buffer );

    parms = BSKRuleGetParameterIDs( rule );
    for( i = 0; i < BSKRuleGetParameterCount( rule ); i++ ) {
      BSKGetIdentifier( db->idTable, parms[ i ], buffer, sizeof( buffer ) );
      printf( "%s", buffer );
      if( i+1 < BSKRuleGetParameterCount( rule ) ) {
        printf( " " );
      }
    }
    printf( ") -- code length: %ld\n", BSKRuleGetCodeLength( rule ) );
    displayRuleCode( db, rule );
    printf( "\n" );
  }
}

void dumpDB( BSKDatabase* db ) {
  BSKCategory* category;
  BSKUnitDefinition* unit;
  BSKAttributeDef* attr;
  BSKCHAR buffer[1024];
  BSKCategoryEntry* entry;
  BSKThing* thing;

  printf( "*** Units:\n" );
  for( unit = db->unitDef; unit != 0; unit = unit->next ) {
    BSKGetIdentifier( db->idTable, BSKUnitGetID( unit ), buffer, sizeof( buffer ) );
    printf( " - %s", buffer );
    if( BSKUnitGetReference( unit ) > 0 ) {
      BSKGetIdentifier( db->idTable, BSKUnitGetReference( unit ), buffer, sizeof( buffer ) );
      printf( " = %g %s\n", BSKUnitGetUnits( unit ), buffer );
    } else {
      printf( "\n" );
    }
  }

  printf( "\n*** Attributes:\n" );
  for( attr = db->attrDef; attr != 0; attr = attr->next ) {
    BSKGetIdentifier( db->idTable, BSKAttributeGetID( attr ), buffer, sizeof( buffer ) );
    printf( " - %s (", buffer );
    switch( BSKAttributeGetType( attr ) ) {
      case AT_NONE:     printf( "-- none --" ); break;
      case AT_NUMBER:   printf( "number" ); break;
      case AT_STRING:   printf( "string" ); break;
      case AT_BOOLEAN:  printf( "boolean" ); break;
      case AT_THING:    printf( "thing" ); break;
      case AT_CATEGORY: printf( "category" ); break;
      case AT_RULE:     printf( "rule" ); break;
      default:          printf( "-- unknown(%d) --", BSKAttributeGetType( attr ) );
    }
    printf( ")\n" );
  }

  printf( "\n*** Categories:\n" );
  for( category = db->categories; category != 0; category = category->next ) {
    BSKGetIdentifier( db->idTable, BSKCategoryGetID( category ), buffer, sizeof( buffer ) );
    printf( " - %s [%ld] (", buffer, BSKCategoryGetTotalWeight( category ) );
    for( entry = BSKCategoryGetFirstMember( category ); entry != 0; entry = entry->next ) {
      if( BSKCategoryEntryGetMember( entry ) == 0 ) {
        printf( "{null}" );
      } else if( BSKCategoryMemberGetID( BSKCategoryEntryGetMember( entry ) ) < 1 ) {
        printf( "{anonymous}" );
      } else {
        BSKGetIdentifier( db->idTable, 
                          BSKCategoryMemberGetID( BSKCategoryEntryGetMember( entry ) ),
                          buffer, sizeof( buffer ) );
        printf( "%s", buffer );
      }
      if( entry->next != 0 ) {
        printf( " " );
      }
    }
    printf( ")\n" );
  }

  printf( "\n*** Things:\n" );
  for( thing = db->things; thing != 0; thing = thing->next ) {
    dumpThing( db, thing );
  }

  dumpRules( db );
}


void printStackTrace( BSKExecutionEnvironment* env ) {
  BSKCHAR name[100];
  BSKStackFrame* frame;

  for( frame = BSKExecEnvironmentGetCurrentFrame(env); frame != 0; frame = frame->next ) {
    BSKGetIdentifier( BSKExecEnvironmentGetDB( env )->idTable, 
                      BSKStackFrameGetRuleID( frame ), 
                      name, sizeof( name ) );
    printf( " *** %s\n", name );
  }
}


