/* ---------------------------------------------------------------------- *
 * See the LICENSE file for redistribution information.
 * Copyright (c) 2001
 *   Jamis Buck.  All rights reserved.
 * ---------------------------------------------------------------------- *
 * bskstream.h
 *
 * The BSKStream object encapsulates stream I/O, whether it be from a file
 * or a buffer in memory.  If other streams are needed as well, it would
 * not be difficult to make a new stream object (see bskstream.c for more
 * info).
 * 
 * A BSKStream object has the following methods:
 *   BSKI32 close( BSKStream* stream ) - closes (and disposes of) the 
 *       stream.  The given stream object is no longer valid after the
 *       stream is closed.  The method returns 0 on success.
 *   BSKI32 read( BSKStream* stream, BSKNOTYPE buffer, BSKUI32 len ) -
 *       reads up to 'len' bytes from 'stream' and puts them in 'buffer'.
 *       The method returns the number of bytes read, or -1 if there
 *       was an error, or the end of file was reached before any bytes
 *       could be read.
 *   BSKI32 write( BSKStream* stream, BSKNOTYPE buffer, BSKUI32 len ) -
 *       writes len bytes to stream from buffer.  The method returns the
 *       number of bytes actually written.
 *   BSKI32 getch( BSKStream* stream ) - returns the next character on
 *       the stream, or -1 if there was an error or EOF was reached.
 *   BSKI32 putch( BSKStream* stream, BSKI32 c ) - puts the character c
 *       back into the stream, making it the character that will be
 *       read by the next call to getch.  Returns 0 if successful.
 *
 * Any new stream object must define all these methods.
 *
 * Original Author: Jamis Buck (minam@rpgplanet.com)
 * ---------------------------------------------------------------------- */

#ifndef __BSKSTREAM_H__
#define __BSKSTREAM_H__

/* ---------------------------------------------------------------------- *
 * Dependencies
 * ---------------------------------------------------------------------- */

#include "bskenv.h"
#include "bsktypes.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *
 * Type definitions
 * ---------------------------------------------------------------------- */

typedef struct __bskstream__ BSKStream;

/* ---------------------------------------------------------------------- *
 * Structure definitions
 * ---------------------------------------------------------------------- */

struct __bskstream__ {
  BSKNOTYPE stream;  /* stream-specific data */
  
  /* stream methods */

  BSKI32    (*close)( BSKStream* );

  BSKI32    (*read)( BSKStream*, BSKNOTYPE, BSKUI32 );
  BSKI32    (*write)( BSKStream*, BSKNOTYPE, BSKUI32 );
  BSKI32    (*getch)( BSKStream* );
  BSKI32    (*putch)( BSKStream*, BSKI32 );
};

/* ---------------------------------------------------------------------- *
 * C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- *
 * Functions
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * BSKStreamOpenFile
   *
   * Opens the file with the given name, with the given mode (these
   * parameters are identical to those passed to the fopen function).
   * If the file could not be opened, BSKStreamOpenFile returns null (0),
   * otherwise, it returns a pointer to the new stream.
   * -------------------------------------------------------------------- */
BSKStream* BSKStreamOpenFile( const BSKCHAR* filename, 
                              const BSKCHAR* mode );

  /* -------------------------------------------------------------------- *
   * BSKStreamOpenBuffer
   *
   * Creates a new stream using the given pointer as the buffer.  If the
   * stream is writeable, writes will reallocate the given buffer to fit
   * the written data.
   * -------------------------------------------------------------------- */
BSKStream* BSKStreamOpenBuffer( BSKCHAR** bufPtr );

/* ---------------------------------------------------------------------- *
 * Close C++ wrapper for C routines
 * ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __BSKSTREAM_H__ */
