#include "bskenv.h"
#include "bsktypes.h"
#include "bskstream.h"

/* ---------------------------------------------------------------------- *
 * Private Type Declarations
 * ---------------------------------------------------------------------- */

typedef struct __bskstream_file__       BSKStream_File;
typedef struct __bskstream_buffer__     BSKStream_Buffer;

/* ---------------------------------------------------------------------- *
 * Private Structure Definitions
 * ---------------------------------------------------------------------- */

struct __bskstream_file__ {
  FILE* fptr;
};


struct __bskstream_buffer__ {
  BSKCHAR** bufPtr;
  BSKUI32   length;
  BSKUI32   position;
};

/* ---------------------------------------------------------------------- *
 * Private Function Declarations
 * ---------------------------------------------------------------------- */

  /* -------------------------------------------------------------------- *
   * s_streamCloseFile
   *
   * The "close" method for file streams -- closes and deallocates the
   * given stream.  Returns whatever 'fclose' returns.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamCloseFile( BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_streamReadFile
   *
   * The "read" method for file streams.  Reads up to 'length' bytes
   * into the given buffer.  Returns the number of bytes read, or -1 if
   * an error or EOF is reached.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamReadFile( BSKStream* stream, 
                                BSKNOTYPE buffer,
                                BSKUI32 length );

  /* -------------------------------------------------------------------- *
   * s_streamWriteFile
   *
   * The "write" method for file streams.  Writes up to 'length' bytes
   * from the given buffer.  Returns the number of bytes written.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamWriteFile( BSKStream* stream, 
                                 BSKNOTYPE buffer,
                                 BSKUI32 length );

  /* -------------------------------------------------------------------- *
   * s_streamGetcFile
   *
   * The "getch" method for file streams.  Reads one byte from the stream
   * and returns it (or -1 if EOF is reached).
   * -------------------------------------------------------------------- */
static BSKI32 s_streamGetcFile( BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_streamPutcFile
   *
   * The "putch" method for file streams.  Writes the given byte to the
   * stream, and returns what 'fputc' returns.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamPutcFile( BSKStream* stream, BSKI32 ch );

  /* -------------------------------------------------------------------- *
   * s_streamCloseBuffer
   *
   * The "close" method for memory streams.  Closes and deallocates the
   * stream.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamCloseBuffer( BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_streamReadBuffer
   *
   * The "read" method for memory streams.  Reads up to 'length' bytes
   * into the given buffer, and returns the number of bytes actually
   * read, or (-1 on EOF).
   * -------------------------------------------------------------------- */
static BSKI32 s_streamReadBuffer( BSKStream* stream, 
                                  BSKNOTYPE bufPtr,
                                  BSKUI32 length );

  /* -------------------------------------------------------------------- *
   * s_streamWriteBuffer
   *
   * The "write" method for memory streams.  Writes up to 'length' bytes
   * into the given buffer, and returns the number of bytes actually
   * written.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamWriteBuffer( BSKStream* stream, 
                                   BSKNOTYPE bufPtr,
                                   BSKUI32 length );

  /* -------------------------------------------------------------------- *
   * s_streamGetcBuffer
   *
   * The "getch" method for memory streams.  Reads one byte from the
   * stream and returns it, or -1 on EOF.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamGetcBuffer( BSKStream* stream );

  /* -------------------------------------------------------------------- *
   * s_streamPutcBuffer
   *
   * The "putch" method for memory streams.  Writes one byte to the
   * stream and returns 0.
   * -------------------------------------------------------------------- */
static BSKI32 s_streamPutcBuffer( BSKStream* stream, BSKI32 ch );

/* ---------------------------------------------------------------------- *
 * Public Function Definitions
 * ---------------------------------------------------------------------- */

BSKStream* BSKStreamOpenFile( const BSKCHAR* filename, 
                              const BSKCHAR* mode )
{
  BSKStream_File* file;
  BSKStream*      stream;

  /* allocate the stream's data object */

  file = (BSKStream_File*)BSKMalloc( sizeof( BSKStream_File ) );
  if( file == NULL ) {
    return NULL;
  }

  BSKMemSet( file, 0, sizeof( *file ) );

  /* open the file */

  file->fptr = fopen( filename, mode );
  if( file->fptr == NULL ) {
    BSKFree( file );
    return NULL;
  }

  /* allocate the new stream object */

  stream = (BSKStream*)BSKMalloc( sizeof( BSKStream ) );
  if( stream == NULL ) {
    fclose( file->fptr );
    BSKFree( file );
    return NULL;
  }

  /* set up the stream's methods and data */

  stream->stream = file;
  stream->close  = s_streamCloseFile;
  stream->read   = s_streamReadFile;
  stream->write  = s_streamWriteFile;
  stream->getch  = s_streamGetcFile;
  stream->putch  = s_streamPutcFile;

  return stream;
}


BSKStream* BSKStreamOpenBuffer( BSKCHAR** bufPtr ) {
  BSKStream_Buffer* buffer;
  BSKStream*        stream;

  /* allocate the stream's data object */

  buffer = (BSKStream_Buffer*)BSKMalloc( sizeof( BSKStream_Buffer ) );
  if( buffer == NULL ) {
    return NULL;
  }

  /* if the buffer passed in points to a NULL, it is assumed that the
   * buffer is empty.  Otherwise, all bytes up to a null byte are considered
   * part of the existing buffer */

  BSKMemSet( buffer, 0, sizeof( *buffer ) );
  buffer->bufPtr = bufPtr;
  buffer->length = ( *bufPtr == NULL ? 0 : BSKStrLen( *bufPtr ) );
  buffer->position = 0;

  stream = (BSKStream*)BSKMalloc( sizeof( BSKStream ) );
  if( stream == NULL ) {
    BSKFree( buffer );
    return NULL;
  }

  /* set up the stream's methods */

  stream->stream = buffer;
  stream->close  = s_streamCloseBuffer;
  stream->read   = s_streamReadBuffer;
  stream->write  = s_streamWriteBuffer;
  stream->getch  = s_streamGetcBuffer;
  stream->putch  = s_streamPutcBuffer;

  return stream;
}

/* ---------------------------------------------------------------------- *
 * Private Function Definitions
 * ---------------------------------------------------------------------- */

static BSKI32 s_streamCloseFile( BSKStream* stream ) {
  BSKStream_File* file;
  BSKI32         rc;

  file = (BSKStream_File*)stream->stream;
  rc = fclose( file->fptr );

  BSKFree( file );
  BSKFree( stream );

  return rc;
}


static BSKI32 s_streamReadFile( BSKStream* stream,
                                BSKNOTYPE buffer,
                                BSKUI32 length )
{
  BSKStream_File* file;
  BSKI32         rc;

  file = (BSKStream_File*)stream->stream;
  rc = fread( buffer, 1, length, file->fptr );

  return rc;
}


static BSKI32 s_streamWriteFile( BSKStream* stream, 
                                 BSKNOTYPE buffer,
                                 BSKUI32 length )
{
  BSKStream_File* file;
  BSKI32         rc;

  file = (BSKStream_File*)stream->stream;
  rc = fwrite( buffer, 1, length, file->fptr );

  return rc;
}


static BSKI32 s_streamGetcFile( BSKStream* stream ) {
  BSKStream_File* file;

  file = (BSKStream_File*)stream->stream;

  return fgetc( file->fptr );
}


static BSKI32 s_streamPutcFile( BSKStream* stream, BSKI32 ch ) {
  BSKStream_File* file;

  file = (BSKStream_File*)stream->stream;

  return fputc( ch, file->fptr );
}


static BSKI32 s_streamCloseBuffer( BSKStream* stream ) {
  BSKStream_Buffer* buffer;

  /* free the stream and its data object, but not the buffer that was
   * being written to (that belongs to the calling routine) */

  buffer = (BSKStream_Buffer*)stream->stream;
  BSKFree( buffer );
  BSKFree( stream );

  return 0;
}


static BSKI32 s_streamReadBuffer( BSKStream* stream, 
                                  BSKNOTYPE bufPtr,
                                  BSKUI32 length )
{
  BSKStream_Buffer* buffer;
  BSKCHAR* tmp;

  buffer = (BSKStream_Buffer*)stream->stream;
  
  if( buffer->position >= buffer->length ) {
    return 0;
  }

  if( buffer->position + length > buffer->length ) {
    length = buffer->length - buffer->position;
  }

  tmp = *(buffer->bufPtr);
  BSKMemCpy( bufPtr, &( tmp[ buffer->position ] ), length );
  buffer->position += length;

  return length;
}


static BSKI32 s_streamWriteBuffer( BSKStream* stream, 
                                   BSKNOTYPE bufPtr,
                                   BSKUI32 length )
{
  BSKStream_Buffer* buffer;
  BSKCHAR*          tmp;

  buffer = (BSKStream_Buffer*)stream->stream;
  
  /* if the buffer is empty (length <= 0), allocate it for the first time
   * and copy the data into it. */

  if( buffer->length <= 0 ) {
    tmp = (BSKCHAR*)BSKMalloc( length+1 );
    if( tmp == NULL ) {
      return 0;
    }

    BSKMemCpy( tmp, bufPtr, length );
    *(buffer->bufPtr) = tmp;
    buffer->length = length;
    buffer->position = length;

    return length;
  }

  /* if the buffer needs to be grown, grow it */

  if( buffer->position + length + 1 > buffer->length ) {
    tmp = (BSKCHAR*)BSKMalloc( buffer->position + length + 1 );

    BSKMemCpy( tmp, *(buffer->bufPtr), buffer->position );
    BSKMemCpy( &(tmp[buffer->position]), bufPtr, length );
    BSKFree( *(buffer->bufPtr) );
    
    *(buffer->bufPtr) = tmp;
    buffer->length = buffer->position = buffer->position + length;
  } else {
    tmp = *(buffer->bufPtr);
    BSKMemCpy( &(tmp[ buffer->position ]),
               bufPtr,
               length );
    buffer->position += length;
  }

  (*(buffer->bufPtr))[ buffer->length-1 ] = 0;
  return length;
}


static BSKI32 s_streamGetcBuffer( BSKStream* stream ) {
  BSKStream_Buffer* buffer;
  BSKCHAR*          tmp;

  buffer = (BSKStream_Buffer*)stream->stream;
  if( buffer->position >= buffer->length ) {
    return EOF;
  }

  tmp = *(buffer->bufPtr);

  return tmp[ buffer->position++ ];
}


static BSKI32 s_streamPutcBuffer( BSKStream* stream, BSKI32 ch ) {
  BSKStream_Buffer* buffer;
  BSKCHAR*          tmp;

  buffer = (BSKStream_Buffer*)stream->stream;

  if( buffer->length < 0 ) {
    tmp = (BSKCHAR*)BSKMalloc( 2 );
    *tmp = ch;
    buffer->length = buffer->position = 1;
    *(buffer->bufPtr) = tmp;
    return 0;
  }

  if( buffer->position >= buffer->length ) {
    tmp = (BSKCHAR*)BSKMalloc( buffer->length + 2 );
    BSKMemCpy( tmp, *(buffer->bufPtr), buffer->length+1 );
    BSKFree( *(buffer->bufPtr) );
    *(buffer->bufPtr) = tmp;
    buffer->length++;
  } else {
    tmp = *(buffer->bufPtr);
  }

  tmp[ buffer->length-1 ] = ch;
  tmp[ buffer->length ] = 0;
  buffer->position++;

  return 0;
}
