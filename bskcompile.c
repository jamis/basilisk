#include <time.h>
#ifdef _WIN32
  #ifdef __MWERKS__
    // CodeWarrior
    #include <types.h>
  #else
    // VC++, etc.
    #include <sys/types.h>
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
#include "bskutil.h"

#include "bskcallbacks.h"


int main( int argc, char* argv[] ) {
  BSKStream* stream;
  BSKStream* ostream;
  BSKDatabase* db;
  BSKI32 rc;
  BSKCHAR compiled[512];
  BSKCHAR searchPath[512];
  BSKCHAR* ptr;
  BSKCHAR* p2;
  BSKCHAR* p3;

  if( argc < 2 || argc > 3 ) {
    printf( "bskcompile usage:\n\n" );
    printf( "  bskcompile <root-file> ( <search-paths> )\n\n" );
    printf( "where <root-file> is the name of the file that you wish to\n" );
    printf( "compile (and may #include other files to compile as well)\n" );
    printf( "and <search-paths> is a '|' delimited list of paths that\n" );
    printf( "should be searched for files (except the root-file)\n" );
    return 0;
  }

  stream = BSKStreamOpenFile( argv[1], "rt" );
  if( stream == NULL ) {
    printf( "Could not open file '%s'\n", argv[1] );
    return 0;
  }

  strcpy( compiled, argv[1] );
  ptr = compiled + strlen( compiled );
  while( ( ptr > compiled ) && ( *ptr != '.' ) ) {
    ptr--;
  }

  if( *ptr == '.' ) {
    ptr++;
    if( BSKStrCaseCmp( ptr, "bsk" ) == 0 ) {
      strcpy( ptr, "bdb" );
    } else {
      strcat( compiled, ".bdb" );
    }
  }

  ostream = BSKStreamOpenFile( compiled, "wb" );
  if( ostream == NULL ) {
    printf( "Could not create output file '%s'\n", compiled );
    stream->close( stream );
    return 0;
  }

  searchPath[0] = 0;
  searchPath[1] = 0;
  if( argc > 2 ) {
    p2 = argv[2];
    p3 = searchPath;
    while( ( ptr = strchr( p2, '|' ) ) != 0 ) {
      *ptr = 0;
      strcpy( p3, p2 );
      p3 += strlen( p3 ) + 1;
      *p3 = 0;
      p2 = ptr + 1;
    }
    strcpy( p3, p2 );
    p3 += strlen( p3 ) + 1;
    p3 = 0;
  }

  printf( "Parsing...\n" );

  db = BSKNewDatabase();
  rc = BSKParse( stream,
                 db,
                 searchPath,
                 myErrorHandler,
                 0 );

  if( rc != 0 ) {
    printf( "Parse terminated with errors.\n" );
  } else {
    printf( "Parse was successful -- compiling...\n" );
    BSKSerializeDatabaseOut( db, ostream );
    printf( "Done!\n" );
  }

  BSKDestroyDatabase( db );

  stream->close( stream );
  ostream->close( ostream );

#ifdef _DEBUG
  printf( "\n" );
  printf( "[bytes unallocated: %6ld]\n", g_currentAllocation );
  printf( "[maximum allocated: %6ld]\n", g_maximumAllocation );
#endif

  BSKDumpAllocations( stdout );

  return 0;
}
