#include <time.h>
#include <ctype.h>
#ifdef _WIN32
  #ifdef __MWERKS__
    // CodeWarrior
    #include <types.h>
  #else
    // VC++, etc.
    #include <sys\types.h>
  #endif
  #include <direct.h>
#else
  #include <unistd.h>
#endif

#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"
#include "bskdb.h"
#include "bskparse.h"
#include "bsktokens.h"
#include "bskrule.h"
#include "bskvalue.h"
#include "bskexec.h"
#include "bskutil.h"
#include "bskarray.h"

#include "bskcallbacks.h"


#define GENERATE_BY_LEVEL  "rGenerateByELAndDisplay"
#define GENERATE_BY_TYPE   "rGenerateByTypeAndDisplay"
#define DISPLAY_RULE       "rDisplayTreasureHoard"
#define INIT_OPTIONS       "rInitializeOptions"

#define SEARCH_PATHS       "dat/standard\0dat/snfist\0dat/scitadel\0\0"


/* function prototypes */

BSKI32 formattedConsole( BSKCHAR* msg,
                         BSKExecutionEnvironment* env,
                         BSKNOTYPE userData );

BSKI32 doRuleExec( BSKDatabase* db, 
                   BSKUI32 id, 
                   BSKValue** parameters, BSKUI32 parmCount, 
                   BSKValue* retVal );

void initOptions( BSKDatabase* db, BSKThing* options );

void doByLevel( BSKDatabase* db, 
                BSKUI32 level,
                BSKFLOAT pctCoins, BSKFLOAT pctGoods, BSKFLOAT pctItems );

void doByType( BSKDatabase* db, BSKUI32 count, BSKCHAR* magnitudes, BSKCHAR* itemTypes );

void usage();

int main( int argc, char* argv[] );

/* function implementations */

BSKBOOL doUC = 0;

BSKI32 formattedConsole( BSKCHAR* msg,
                         BSKExecutionEnvironment* env,
                         BSKNOTYPE userData )
{
  int    i;

  for( i = 0; msg[ i ] != 0; i++ ) {
    if( msg[i] == '~' ) {
      i++;
      switch( msg[i] ) {
        case 'n':
          putc( '\n', stdout );
          break;
        case 'B':
          doUC = 1;
          break;
        case 'b':
          doUC = 0;
          break;
        case 'I':
        case 'i':
          break;
        case 'C':
        case 'G':
          while( msg[i] != 0 && msg[i] != ']' ) {
            i++;
          }
          break;
        case 'c':
        case 'g':
          break;
        default:
          if( doUC ) {
            putc( toupper( msg[i] ), stdout );
          } else {
            putc( msg[i], stdout );
          }
          break;
      }
    } else {
      if( doUC ) {
        putc( toupper( msg[i] ), stdout );
      } else {
        putc( msg[i], stdout );
      }
    }
  }
  return 0;
}


BSKI32 doRuleExec( BSKDatabase* db, 
                   BSKUI32 id, 
                   BSKValue** parameters, BSKUI32 parmCount, 
                   BSKValue* retVal )
{
  BSKI32 rc;
  BSKExecOpts opts;
  BSKBOOL halt;
  BSKValue *rval;
  BSKValue temp;
	BSKCallbackData cbdata;

	cbdata.consoleOut = stdout;
	cbdata.errorOut = stderr;

  rval = ( retVal != 0 ? retVal : &temp );

  opts.db = db;
  opts.ruleId = id;
  opts.parameterCount = parmCount;
  opts.parameters = parameters;
  opts.rval = rval;
  opts.errorHandler = BSKDefaultRuntimeErrorHandler;
  opts.console = formattedConsole;
  opts.userData = &cbdata;

  halt = BSKFALSE;
  opts.halt = &halt;

  rc = BSKExec( &opts );

  if( retVal == 0 ) {
    BSKInvalidateValue( rval );
  }

  return rc;
}


void initOptions( BSKDatabase* db, BSKThing* options ) {
  FILE *optFile;
  char  line[1024];
  char* value;
  char* name;
  char* unit;
  BSKAttribute* attr;
  BSKAttributeDef* attrDef;
  BSKSymbolTableEntry* sym;
  BSKUI32 id;
  BSKUI32 unitId;
  BSKValue valItem;
  int i;

  optFile = fopen( "treasure.ini", "rt" );
  if( optFile == 0 ) {
    return;
  }

  while( fgets( line, sizeof( line ), optFile ) != 0 ) {
    name = line;
    while( isspace( *name ) ) {
      name++;
    }
    if( *name == '#' || *name == 0 ) {
      continue;
    }
    value = strchr( name, '=' );
    if( value == 0 ) {
      fprintf( stderr, "invalid format for configuration file\n" );
      break;
    }
    *value = 0;
    value++;

    i = strlen( value ) - 1;
    while( ( i >= 0 ) && ( isspace(value[i]) ) ) {
      value[i] = 0;
      i--;
    }

    id = BSKFindIdentifier( db->idTable, name );
    if( id < 0 ) {
      fprintf( stderr, "property '%s' is not a valid identifier in the database\n", name );
    } else {
      sym = BSKGetSymbol( db->symbols, id );
      if( ( sym == 0 ) || ( BSKSymbolGetType( sym ) != ST_ATTRIBUTE ) ) {
        fprintf( stderr, "property '%s' is not an attribute in the database\n", name );
      } else {
        attrDef = BSKFindAttributeDef( db->attrDef, id );

        switch( attrDef->type ) {
          case AT_NUMBER:
            unitId = 0;
            unit = strchr( value, ' ' );
            if( unit != 0 ) {
              *unit = 0;
              unit++;
              while( isspace( *unit ) ) unit++;
              unitId = BSKFindIdentifier( db->idTable, unit );
              if( unitId < 0 ) {
                fprintf( stderr, "'%s' is not a declared unit\n", unit );
                unitId = 0;
              } else {
                sym = BSKGetSymbol( db->symbols, unitId );
                if( ( sym == 0 ) || ( BSKSymbolGetType( sym ) != ST_UNIT ) ) {
                  fprintf( stderr, "'%s' is not a unit identifier\n", unit );
                  unitId = 0;
                }
              }
            }
            BSKSetValueNumberU( &valItem, BSKAtoF( value ), unitId );
            break;
          case AT_STRING:
            BSKSetValueString( &valItem, value );
          case AT_BOOLEAN:
            if( BSKStrCaseCmp( value, "true" ) == 0 || 
                BSKStrCaseCmp( value, "yes" ) == 0 ||
                BSKStrCaseCmp( value, "1" ) == 0 ||
                BSKStrCaseCmp( value, "t" ) == 0 ||
                BSKStrCaseCmp( value, "on" ) == 0 )
            {
              BSKSetValueNumber( &valItem, 1 );
            } else if( BSKStrCaseCmp( value, "false" ) == 0 || 
                       BSKStrCaseCmp( value, "no" ) == 0 ||
                       BSKStrCaseCmp( value, "0" ) == 0 ||
                       BSKStrCaseCmp( value, "f" ) == 0 ||
                       BSKStrCaseCmp( value, "off" ) == 0 )
            {
              BSKSetValueNumber( &valItem, 0 );
            } else {
              fprintf( stderr, "'%s' is not a boolean value\n", value );
            }
            break;
          default:
            valItem.type = VT_NULL;
        }

        attr = BSKGetAttributeOf( options, id );
        if( attr != 0 ) {
          BSKInvalidateValue( BSKThingAttributeGetValue( attr ) );
          BSKCopyValue( BSKThingAttributeGetValue( attr ), &valItem );
        } else {
          BSKAddAttributeTo( options, id, &valItem );
        }

        BSKInvalidateValue( &valItem );
      }
    }
  }

  fclose( optFile );
}


void doByLevel( BSKDatabase* db, 
                BSKUI32 level,
                BSKFLOAT pctCoins, BSKFLOAT pctGoods, BSKFLOAT pctItems )
{
  BSKI32 id;
  BSKI32 displayId;
  BSKSymbolTableEntry* sym;
  BSKI32 rc;
  BSKValue retVal;
  BSKValue parm1;
  BSKValue parm2;
  BSKValue parm3;
  BSKValue parm4;
  BSKValue parm5;
  BSKValue parm6;
  BSKValue* parameters[6];
  BSKThing* options;

  printf( "\n\n" );

  options = BSKNewThing( 0 );
  id = BSKFindIdentifier( db->idTable, INIT_OPTIONS );
  if( id >= 0 ) {
    sym = BSKGetSymbol( db->symbols, id );
    if( ( sym != 0 ) && ( BSKSymbolGetType( sym ) == ST_RULE ) ) {
      parm1.type = VT_THING;
      parm1.datum = options;
      parameters[0] = &parm1;
      doRuleExec( db, id, parameters, 1, 0 );
      BSKInvalidateValue( &parm1 );
    }
  }
  initOptions( db, options );

  id = BSKFindIdentifier( db->idTable, GENERATE_BY_LEVEL );
  if( id < 0 ) {
    printf( "No \"%s\" defined -- no rules executed.\n", GENERATE_BY_LEVEL );
    return;
  }

  sym = BSKGetSymbol( db->symbols, id );
  if( ( sym == 0 ) || ( BSKSymbolGetType( sym ) != ST_RULE ) ) {
    printf( "\"%s\" is not a rule -- cannot execute.\n", GENERATE_BY_LEVEL );
    return;
  }

  BSKSetValueNumber( &parm1, level );
  parameters[0] = &parm1;

  BSKSetValueNumber( &parm2, pctCoins );
  parameters[1] = &parm2;

  BSKSetValueNumber( &parm3, pctGoods );
  parameters[2] = &parm3;

  BSKSetValueNumber( &parm4, pctItems );
  parameters[3] = &parm4;

  displayId = BSKFindIdentifier( db->idTable, DISPLAY_RULE );
  parm5.type = VT_RULE;
  parm5.datum = BSKFindRule( db->rules, displayId );
  parameters[4] = &parm5;

  parm6.type = VT_THING;
  parm6.datum = options;
  parameters[5] = &parm6;

  rc = doRuleExec( db, id, parameters, 6, &retVal );

  BSKInvalidateValue( &parm1 );
  BSKInvalidateValue( &parm2 );
  BSKInvalidateValue( &parm3 );
  BSKInvalidateValue( &parm4 );
  BSKInvalidateValue( &parm5 );
  BSKInvalidateValue( &parm6 );
  BSKCleanupReturnValue( &retVal );
  BSKDestroyThing( options );

  if( rc != 0 ) {
    printf( "run-time error (%ld):  ", rc );
    switch( rc ) {
      case RTE_INVALID_RULE: printf( "invalid rule\n" ); break;
      case RTE_BUG: printf( "bug detected\n" ); break;
      case RTE_STACK_UNDERFLOW: printf( "stack underflow\n" ); break;
      case RTE_INVALID_OPERANDS: printf( "invalid operands\n" ); break;
      case RTE_DIVIDE_BY_ZERO: printf( "divide by zero\n" ); break;
      case RTE_UNKNOWN_INSTRUCTION: printf( "unknown instruction\n" ); break;
      case RTE_WRONG_PARAM_COUNT: printf( "wrong parameter count\n" ); break;
      case RTE_DOMAIN_ERROR: printf( "domain error\n" ); break;
      case RTE_WRONG_UNITS: printf( "wrong units in conversion\n" ); break;
      case RTE_CALL_OF_NONFUNCTION: printf( "call of non-function\n" ); break;
      default: printf( "unknown error\n" ); break;
    }
  }
}


void doByType( BSKDatabase* db, BSKUI32 count, BSKCHAR* magnitudes, BSKCHAR* itemTypes ) {
  BSKI32 id;
  BSKI32 displayId;
  BSKSymbolTableEntry* sym;
  BSKI32 rc;
  BSKValue parm1;
  BSKValue parm2;
  BSKValue parm3;
  BSKValue parm4;
  BSKValue parm5;
  BSKValue retVal;
  BSKValue* parameters[5];
  BSKArray* magsArray;
  BSKArray* itemsArray;
  BSKCHAR* p;
  BSKUI32  i;
  BSKThing* options;

  printf( "\n\n" );

  options = BSKNewThing( 0 );
  id = BSKFindIdentifier( db->idTable, INIT_OPTIONS );
  if( id >= 0 ) {
    sym = BSKGetSymbol( db->symbols, id );
    if( ( sym != 0 ) && ( BSKSymbolGetType( sym ) == ST_RULE ) ) {
      parm1.type = VT_THING;
      parm1.datum = options;
      parameters[0] = &parm1;
      doRuleExec( db, id, parameters, 1, 0 );
      BSKInvalidateValue( &parm1 );
    }
  }
  initOptions( db, options );

  id = BSKFindIdentifier( db->idTable, GENERATE_BY_TYPE );
  if( id < 0 ) {
    printf( "No \"%s\" defined -- no rules executed.\n", GENERATE_BY_TYPE );
    return;
  }

  sym = BSKGetSymbol( db->symbols, id );
  if( ( sym == 0 ) || ( BSKSymbolGetType( sym ) != ST_RULE ) ) {
    printf( "\"%s\" is not a rule -- cannot execute.\n", GENERATE_BY_TYPE );
    return;
  }

  BSKSetValueNumber( &parm1, count );
  parameters[0] = &parm1;

  magsArray = BSKNewArray( 0 );
  itemsArray = BSKNewArray( 0 );

  i = 0;
  p = strtok( magnitudes, "|" );
  while( p != 0 ) {
    BSKValue val;

    if( *p > ' ' ) {
      BSKSetValueString( &val, p );
      BSKPutElement( magsArray, i++, &val );
    }

    p = strtok( 0, "|" );
  }

  parm2.type = VT_ARRAY;
  parm2.datum = magsArray;
  parameters[1] = &parm2;

  i = 0;
  p = strtok( itemTypes, "|" );
  while( p != 0 ) {
    BSKValue val;

    if( *p > ' ' ) {
      BSKSetValueString( &val, p );
      BSKPutElement( itemsArray, i++, &val );
    }

    p = strtok( 0, "|" );
  }

  parm3.type = VT_ARRAY;
  parm3.datum = itemsArray;
  parameters[2] = &parm3;

  displayId = BSKFindIdentifier( db->idTable, DISPLAY_RULE );
  parm4.type = VT_RULE;
  parm4.datum = BSKFindRule( db->rules, displayId );
  parameters[3] = &parm4;

  parm5.type = VT_THING;
  parm5.datum = options;
  parameters[4] = &parm5;

  rc = doRuleExec( db, id, parameters, 5, &retVal );

  BSKInvalidateValue( &parm1 );
  BSKInvalidateValue( &parm2 );
  BSKInvalidateValue( &parm3 );
  BSKInvalidateValue( &parm4 );
  BSKInvalidateValue( &parm5 );

  BSKDestroyArray( magsArray );
  BSKDestroyArray( itemsArray );
  BSKDestroyThing( options );

  BSKCleanupReturnValue( &retVal );

  if( rc != 0 ) {
    printf( "run-time error (%ld):  ", rc );
    switch( rc ) {
      case RTE_INVALID_RULE: printf( "invalid rule\n" ); break;
      case RTE_BUG: printf( "bug detected\n" ); break;
      case RTE_STACK_UNDERFLOW: printf( "stack underflow\n" ); break;
      case RTE_INVALID_OPERANDS: printf( "invalid operands\n" ); break;
      case RTE_DIVIDE_BY_ZERO: printf( "divide by zero\n" ); break;
      case RTE_UNKNOWN_INSTRUCTION: printf( "unknown instruction\n" ); break;
      case RTE_WRONG_PARAM_COUNT: printf( "wrong parameter count\n" ); break;
      case RTE_DOMAIN_ERROR: printf( "domain error\n" ); break;
      case RTE_WRONG_UNITS: printf( "wrong units in conversion\n" ); break;
      case RTE_CALL_OF_NONFUNCTION: printf( "call of non-function\n" ); break;
      default: printf( "unknown error\n" ); break;
    }
  }
}


void usage() {
  printf( "BSKTreasure Usage:\n" );
  printf( "  bsktreasure <file> t[ype] <count> <magnitudes> <itemtypes> ( <seed> )\n" );
  printf( "  bsktreasure <file> l[evel] <level> <coins> <goods> <items> ( <seed> )\n" );
  printf( "\n" );
  printf( "  <file> may be either a Basilisk script file, or\n" );
  printf( "         a compiled version of a Basilisk script file\n" );
  printf( "  <count> is the number of items to generate.\n" );
  printf( "  <magnitudes> is a '|' delimited list of magnitudes.\n" );
  printf( "  <itemtypes> is a '|' delimited list of item types.\n" );
  printf( "  <level> is the encounter level of the treasure (1-30).\n" );
  printf( "  <coins> is the multiplier of coins to generate (default is 1)\n" );
  printf( "  <goods> is the multiplier of goods to generate (default is 1)\n" );
  printf( "  <items> is the multiplier of items to generate (default is 1)\n" );
  printf( "  <seed> is the random seed to use\n" );
  printf( "\n" );
}


int main( int argc, char* argv[] ) {
  BSKStream* stream;
  BSKDatabase* db;
  BSKI32 rc;
  BSKCHAR* ptr;
  BSKCHAR* type;
  BSKUI32 seed;
  BSKBOOL isCompiled;
  BSKUI32 start;
  BSKUI32 end;
	BSKCallbackData cbdata;

  if( argc < 4 ) {
    usage();
    return 0;
  }

  type = argv[2];
  if( ( *type != 't' ) && ( *type != 'l' ) ) {
    usage();
    return 0;
  }

  seed = time(0);
  if( *type == 't' ) {
    if( argc > 6 ) {
      seed = atol( argv[6] );
    }
  } else {
    if( argc > 7 ) {
      seed = atol( argv[7] );
    }
  }

  printf( "random seed: %ld\n", seed );
  BSKSRand( seed );

  stream = BSKStreamOpenFile( argv[1], "rb" );
  if( stream == NULL ) {
    printf( "Could not open file '%s'\n", argv[1] );
    return 0;
  }

  isCompiled = BSKFALSE;
  ptr = argv[1] + strlen( argv[1] );
  while( ( *ptr != '.' ) && ( ptr > argv[1] ) ) {
    ptr--;
  }

  if( *ptr == '.' ) {
    ptr++;
    if( BSKStrCaseCmp( ptr, "bdb" ) == 0 ) {
      isCompiled = BSKTRUE;
    }
  }

  ptr = argv[1] + strlen( argv[1] );
  while( ( *ptr != '/' ) && ( *ptr != '\\' ) && ( ptr > argv[1] ) ) {
    ptr--;
  }

  if( ( *ptr == '/' ) || ( *ptr == '\\' ) ) {
    *ptr = 0;
    chdir( argv[1] );
  }

  if( !isCompiled ) {
		cbdata.consoleOut = stdout;
		cbdata.errorOut = stderr;

    db = BSKNewDatabase();
    rc = BSKParse( stream,
                   db,
                   SEARCH_PATHS,
                   BSKDefaultParseErrorHandler,
                   &cbdata );
  } else {
    db = BSKSerializeDatabaseIn( stream );
    rc = ( db == 0 );
  }

  if( rc != 0 ) {
    printf( "Parse terminated with errors.\n" );
  } else {
    printf( "Parse was successful.\n" );
    start = clock();
    for( rc = 0; rc < 1; rc++ ) {
      if( *type == 't' ) {
        doByType( db, atoi( argv[3] ), argv[4], argv[5] );
      } else {
        doByLevel( db, atoi( argv[3] ), atof( argv[4] ), atof( argv[5] ), atof( argv[6] ) );
      }
    }
    end = clock();
    printf( "\nexecution time: %g sec.\n", 1.0 * ( end - start ) / CLOCKS_PER_SEC );
  }

  BSKDestroyDatabase( db );

  stream->close( stream );

  printf( "\nrandom seed: %ld\n", seed );
  return 0;
}
