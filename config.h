/*
 * config.h -- common includes and defines
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>
#include <ctype.h>
#ifdef unix
  #include <unistd.h>
#elif _WIN32
  #ifdef __MWERKS__
    // CodeWarrior
    #include <types.h>
  #else
    // VC++, etc.
    #include <sys\types.h>
  #endif
  #include <direct.h>
#else
/* Someone who knows about the Palm port needs to put the appropriate
   define here. --bko XXX */
  #error unsupported platform
#endif

#endif /* CONFIG_H */
