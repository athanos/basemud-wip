/*
 * mccp.c - support functions for the Mud Client Compression Protocol
 *
 * see http://www.randomly.org/projects/MCCP/
 *
 * Copyright (c) 1999, Oliver Jowett <oliver@randomly.org>
 *
 * This code may be freely distributed and used if this copyright
 * notice is retained intact.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "mud.h"

/* local functions */
bool  processCompressed       ( SOCKET_DATA *sock );

const unsigned char enable_compress  [] = { IAC, SB, TELOPT_COMPRESS, WILL, SE, 0 };
const unsigned char enable_compress2 [] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, 0 };

/*
 * Memory management - zlib uses these hooks to allocate and free memory
 * it needs
 */
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
  return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
  free(address);
}

/*
 * Begin compressing data on `desc'
 */
bool compressStart(SOCKET_DATA *sock, unsigned char teleopt)
{
  z_stream *s;

  /* already compressing */
  if (sock->out_compress)
    return TRUE;

  /* allocate and init stream, buffer */
  s = (z_stream *) malloc(sizeof(*s));
  sock->out_compress_buf = (unsigned char *) malloc(COMPRESS_BUF_SIZE);

  s->next_in    =  NULL;
  s->avail_in   =  0;
  s->next_out   =  sock->out_compress_buf;
  s->avail_out  =  COMPRESS_BUF_SIZE;
  s->zalloc     =  zlib_alloc;
  s->zfree      =  zlib_free;
  s->opaque     =  NULL;

  if (deflateInit(s, 9) != Z_OK)
  {
    free(sock->out_compress_buf);
    free(s);
    return FALSE;
  }

  /* version 1 or 2 support */
  if (teleopt == TELOPT_COMPRESS)
    text_to_socket(sock, (char *) enable_compress);
  else if (teleopt == TELOPT_COMPRESS2)
    text_to_socket(sock, (char *) enable_compress2);
  else
  {
    bug("Bad teleoption %d passed", teleopt);
    free(sock->out_compress_buf);
    free(s);
    return FALSE;
  }

  /* now we're compressing */
  sock->compressing = teleopt;
  sock->out_compress = s;

  /* success */
  return TRUE;
}

/* Cleanly shut down compression on `desc' */
bool compressEnd(SOCKET_DATA *sock, unsigned char teleopt, bool forced)
{
  unsigned char dummy[1];

  if (!sock->out_compress)
    return TRUE;

  if (sock->compressing != teleopt)
    return FALSE;

  sock->out_compress->avail_in = 0;
  sock->out_compress->next_in = dummy;
  sock->top_output = 0;

  /* No terminating signature is needed - receiver will get Z_STREAM_END */
  if (deflate(sock->out_compress, Z_FINISH) != Z_STREAM_END && !forced)
    return FALSE;

  /* try to send any residual data */
  if (!processCompressed(sock) && !forced)
    return FALSE;

  /* reset compression values */
  deflateEnd(sock->out_compress);
  free(sock->out_compress_buf);
  free(sock->out_compress);
  sock->compressing      = 0;
  sock->out_compress     = NULL;
  sock->out_compress_buf = NULL;

  /* success */
  return TRUE;
}

/* Try to send any pending compressed-but-not-sent data in `desc' */
bool processCompressed(SOCKET_DATA *sock)
{
  int iStart, nBlock, nWrite, len;

  if (!sock->out_compress)
    return TRUE;
    
  len = sock->out_compress->next_out - sock->out_compress_buf;
  if (len > 0)
  {
    for (iStart = 0; iStart < len; iStart += nWrite)
    {
      nBlock = UMIN (len - iStart, 4096);
      if ((nWrite = write(sock->control, sock->out_compress_buf + iStart, nBlock)) < 0)
      {
        if (errno == EAGAIN || errno == ENOSR)
          break;

        /* write error */
        return FALSE;
      }
      if (nWrite <= 0)
        break;
    }

    if (iStart)
    {
      if (iStart < len)
        memmove(sock->out_compress_buf, sock->out_compress_buf+iStart, len - iStart);

      sock->out_compress->next_out = sock->out_compress_buf + len - iStart;
    }
  }

  /* success */
  return TRUE;
}
