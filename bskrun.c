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

#include "bskcallbacks.h"

/* function prototypes */

void runMain( BSKDatabase* db );

void showCounts( BSKDatabase* db );

int main( int argc, char* argv[] );


/* function implementations */

void runMain( BSKDatabase* db ) {
  BSKI32 id;
  BSKSymbolTableEntry* sym;
  BSKI32 rc;
  BSKValue retVal;
  BSKExecOpts opts;
  BSKBOOL halt;

  printf( "\n\n" );

  id = BSKFindIdentifier( db->idTable, "main" );
  if( id < 0 ) {
    printf( "No \"main\" defined -- no rules executed.\n" );
    return;
  }

  sym = BSKGetSymbol( db->symbols, id );
  if( ( sym == 0 ) || ( sym->type != ST_RULE ) ) {
    printf( "\"main\" is not a rule -- cannot execute.\n" );
    return;
  }

  opts.db = db;
  opts.ruleId = id;
  opts.parameterCount = 0;
  opts.parameters = 0;
  opts.rval = &retVal;
  opts.errorHandler = BSKDefaultRuntimeErrorHandler;
  opts.console = BSKDefaultConsole;
  opts.userData = 0;

  halt = BSKFALSE;
  opts.halt = &halt;

  rc = BSKExec( &opts );
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
  } else {
    printf( "return value:  " );
    switch( retVal.type ) {
      case VT_NULL: printf( "null\n" ); break;
      case VT_BYTE: printf( "%d\n", *(BSKI8*)retVal.datum ); break;
      case VT_WORD: printf( "%d\n", *(BSKI16*)retVal.datum ); break;
      case VT_DWORD: printf( "%ld\n", *(BSKI32*)retVal.datum ); break;
      case VT_FLOAT: printf( "%g\n", *(BSKFLOAT*)retVal.datum ); break;
      case VT_DICE: printf( "dice\n" ); break;
      case VT_STRING: printf( "\"%s\"\n", (BSKCHAR*)retVal.datum ); break;
      case VT_THING: dumpThing( db, (BSKThing*)retVal.datum ); break;
      case VT_CATEGORY: printf( "category\n" ); break;
      case VT_RULE: printf( "rule\n" ); break;
      default: printf( "other: %04X\n", retVal.type );
    }
    BSKCleanupReturnValue( &retVal );
  }
}


void showCounts( BSKDatabase* db ) {
  BSKUI32 count;
  BSKThing* thing;
  BSKCategory* cat;
  BSKRule* rule;

  printf( "Database objects:\n" );

  count = 0;
  for( thing = db->things; thing != 0; thing = thing->next ) count++;
  printf( "  things: %ld\n", count );
  count = 0;
  for( cat = db->categories; cat != 0; cat = cat->next ) count++;
  printf( "  categories: %ld\n", count );
  count = 0;
  for( rule = db->rules; rule != 0; rule = rule->next ) count++;
  printf( "  rules: %ld\n", count );
}


int main( int argc, char* argv[] ) {
  BSKStream* stream;
  BSKDatabase* db;
  BSKI32 rc;
  BSKCHAR* ptr;
  BSKUI32 seed;
  BSKBOOL isCompiled;
  BSKUI32 start;
  BSKUI32 end;

  if( argc < 2 ) {
    printf( "BSKRun Usage:\n" );
    printf( "  bskrun <file> ( <option> ) ( <seed> )\n" );
    printf( "\n" );
    printf( "  <file> may be either a Basilisk script file, or\n" );
    printf( "         a compiled version of a Basilisk script file\n" );
    printf( "  <option> may be either:\n" );
    printf( "         r - display all rules and their compiled code\n" );
    printf( "         s - show all entities in the database\n" );
    printf( "  <seed> is the random seed to use\n" );
    printf( "\n" );
    return 0;
  }

  seed = time(0);
  if( argc > 2 ) {
    if( *argv[2]=='s' || *argv[2]=='r' ) {
      if( argc > 3 ) {
        seed = atol( argv[3] );
      }
    } else {
      seed = atol( argv[2] );
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
    db = BSKNewDatabase();
    rc = BSKParse( stream,
                   db,
                   "\0\0",
                   BSKDefaultParseErrorHandler,
                   0 );
  } else {
    db = BSKSerializeDatabaseIn( stream );
    rc = ( db == 0 );
  }

  if( rc != 0 ) {
    printf( "Parse terminated with errors.\n" );
  } else {
    printf( "Parse was successful.\n" );
    if( argc > 2 ) {
      if( *argv[2] == 's' ) {
        dumpDB( db );
      } else if( *argv[2] == 'r' ) {
        dumpRules( db );
      }
    }
    start = clock();
    runMain( db );
    end = clock();
    printf( "\nexecution time: %g sec.\n", 1.0 * ( end - start ) / CLOCKS_PER_SEC );
  }

  printf( "\n" );
  showCounts( db );

  BSKDestroyDatabase( db );

  stream->close( stream );

#ifdef _BSKDEBUG
  printf( "\n" );
  printf( "[bytes unallocated: %6ld]\n", g_currentAllocation );
  printf( "[maximum allocated: %6ld]\n", g_maximumAllocation );
#endif

  BSKDumpAllocations( stdout );

  printf( "\nrandom seed: %ld\n", seed );
  return 0;
}
