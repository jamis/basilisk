#ifndef __BSKCALLBACKS_H__
#define __BSKCALLBACKS_H__

#include "bsktypes.h"
#include "bskenv.h"
#include "bskdb.h"

#include "bsktokens.h"
#include "bskexec.h"

BSKI32 myConsole( BSKCHAR* msg,
                  BSKExecutionEnvironment* env,
                  BSKNOTYPE userData );

BSKI32 myRTEHandler( BSKI32 code,
                     BSKCHAR* msg,
                     BSKExecutionEnvironment* env,
                     BSKNOTYPE userData );

BSKI32 myErrorHandler( BSKI32 code,
                       BSKDatabase* db,
                       BSKCHAR* file,
                       BSKToken* token,
                       BSKUI32 data,
                       BSKNOTYPE userData );

void displayRuleCode( BSKDatabase* db, BSKRule* rule );
void dumpThing( BSKDatabase* db, BSKThing* thing );
void dumpRules( BSKDatabase* db );
void dumpDB( BSKDatabase* db );

void printStackTrace( BSKExecutionEnvironment* env );

#endif

