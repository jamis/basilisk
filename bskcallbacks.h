#ifndef __BSKCALLBACKS_H__
#define __BSKCALLBACKS_H__

#include "bsktypes.h"
#include "bskenv.h"
#include "bskdb.h"

#include "bsktokens.h"
#include "bskexec.h"

#include <stdio.h>


BSKI32 BSKDefaultConsole( BSKCHAR* msg,
                          BSKExecutionEnvironment* env,
                          BSKNOTYPE userData );

BSKI32 BSKDefaultRuntimeErrorHandler( BSKI32 code,
                                      BSKCHAR* msg,
                                      BSKExecutionEnvironment* env,
                                      BSKNOTYPE userData );

BSKI32 BSKDefaultParseErrorHandler( BSKI32 code,
                                    BSKDatabase* db,
                                    BSKCHAR* file,
                                    BSKToken* token,
                                    BSKUI32 data,
                                    BSKNOTYPE userData );

void BSKPrintStackTrace( BSKExecutionEnvironment* env, FILE* f );

void displayRuleCode( BSKDatabase* db, BSKRule* rule );
void dumpThing( BSKDatabase* db, BSKThing* thing );
void dumpRules( BSKDatabase* db );
void dumpDB( BSKDatabase* db );

#endif

