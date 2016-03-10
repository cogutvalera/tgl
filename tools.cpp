/* 
    This file is part of tgl-library

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright Vitaly Valtman 2013-2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crypto/rand.h"
#include <zlib.h>
#include <time.h>
#include <sys/time.h>

#include "tools.h"

extern "C" {
#include "mtproto-common.h"
}

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#endif

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
int vasprintf(char ** __restrict__ ret,
                      const char * __restrict__ format,
                      va_list ap) {
  int len;
  /* Get Length */
  len = _vsnprintf(NULL,0,format,ap);
  if (len < 0) return -1;
  /* +1 for \0 terminator. */
  *ret = malloc(len + 1);
  /* Check malloc fail*/
  if (!*ret) return -1;
  /* Write String */
  _vsnprintf(*ret,len+1,format,ap);
  /* Terminate explicitly */
  (*ret)[len] = '\0';
  return len;
}

int clock_gettime(int ignored, struct timespec *spec)      
{
  __int64 wintime;
  GetSystemTimeAsFileTime((FILETIME*)&wintime);
  wintime      -= 116444736000000000;  //1jan1601 to 1jan1970
  spec->tv_sec  = wintime / 10000000;           //seconds
  spec->tv_nsec = wintime % 10000000 *100;      //nano-seconds
  return 0;
}
#endif

#ifdef VALGRIND_FIXES
#include "valgrind/memcheck.h"
#endif

void logprintf (const char *format, ...) __attribute__ ((format (printf, 1, 2), weak));
void logprintf (const char *format, ...) {
  va_list ap;
  va_start (ap, format);
  vfprintf (stdout, format, ap);
  va_end (ap);
}

int tgl_snprintf (char *buf, int len, const char *format, ...) {
  va_list ap;
  va_start (ap, format);
  int r = vsnprintf (buf, len, format, ap);
  va_end (ap);
  assert (r <= len && "tsnprintf buffer overflow");
  return r;
}

int tgl_asprintf (char **res, const char *format, ...) {
  va_list ap;
  va_start (ap, format);
  int r = vasprintf (res, format, ap);
  assert (r >= 0);
  va_end (ap);
  void *rs = talloc (strlen (*res) + 1);
  memcpy (rs, *res, strlen (*res) + 1);
  free (*res);
  *res = rs;
  return r;
}

void tgl_free_debug (void *ptr, int size __attribute__ ((unused))) {
  if (!ptr) {
    assert (!size);
    return;
  }
  total_allocated_bytes -= size;
  ptr -= RES_PRE;
  if (size != (int)((*(int *)ptr) ^ 0xbedabeda)) {
    logprintf ("size = %d, ptr = %d\n", size, (*(int *)ptr) ^ 0xbedabeda);
  }
  assert (*(int *)ptr == (int)((size) ^ 0xbedabeda));
  assert (*(int *)(ptr + RES_PRE + size) == (int)((size) ^ 0x7bed7bed));
  assert (*(int *)(ptr + 4) == size);
  int block_num = *(int *)(ptr + 4 + RES_PRE + size);
  if (block_num >= used_blocks) {
    logprintf ("block_num = %d, used = %d\n", block_num, used_blocks);
  }
  assert (block_num < used_blocks);
  if (block_num < used_blocks - 1) {
    void *p = blocks[used_blocks - 1];
    int s = (*(int *)p) ^ 0xbedabeda;
    *(int *)(p + 4 + RES_PRE + s) = block_num;
    blocks[block_num] = p;
  }
  blocks[--used_blocks] = 0;
  memset (ptr, 0, size + RES_PRE + RES_AFTER);
  *(int *)ptr = size + 12;
  free_blocks[free_blocks_cnt ++] = ptr;
}

void tgl_free_release (void *ptr, int size) {
  total_allocated_bytes -= size;
  memset (ptr, 0, size);
  free (ptr);
}

void *tgl_realloc_debug (void *ptr, size_t old_size __attribute__ ((unused)), size_t size) {
  void *p = talloc (size);
  memcpy (p, ptr, size >= old_size ? old_size : size); 
  if (ptr) {
    tfree (ptr, old_size);
  } else {
    assert (!old_size);
  }
  return p;
}

void *tgl_realloc_release (void *ptr, size_t old_size __attribute__ ((unused)), size_t size) {
  total_allocated_bytes += (size - old_size);
  void *p = realloc (ptr, size);
  ensure_ptr (p);
  return p;
}

void *tgl_alloc_release (size_t size) {
  total_allocated_bytes += size;
  void *p = malloc (size);
  ensure_ptr (p);
  return p;
}

char *tgl_strdup (const char *s) {
  int l = strlen (s);
  char *p = (char*)malloc (l + 1);
  memcpy (p, s, l + 1);
  return p;
}

char *tgl_strndup (const char *s, size_t n) {
  size_t l = 0;
  for (l = 0; l < n && s[l]; l++) { }
  char *p = (char*)malloc (l + 1);
  memcpy (p, s, l);
  p[l] = 0;
  return p;
}

void *tgl_memdup (const void *s, size_t n) {
  void *r = malloc (n);
  memcpy (r, s, n);
  return r;
}


int tgl_inflate (void *input, int ilen, void *output, int olen) {
  z_stream strm;
  memset (&strm, 0, sizeof (strm));
  assert (inflateInit2 (&strm, 16 + MAX_WBITS) == Z_OK);
  strm.avail_in = ilen;
  strm.next_in = (Bytef*)input;
  strm.avail_out = olen ;
  strm.next_out = (Bytef*)output;
  int err = inflate (&strm, Z_FINISH); 
  int total_out = strm.total_out;

  if (err != Z_OK && err != Z_STREAM_END) {
    logprintf ( "inflate error = %d\n", err);
    logprintf ( "inflated %d bytes\n", (int) strm.total_out);
    total_out = 0;
  }
  inflateEnd (&strm);
  return total_out;
}

double tglt_get_double_time (void) {
  struct timespec tv;
  tgl_my_clock_gettime (CLOCK_REALTIME, &tv);
  return tv.tv_sec + 1e-9 * tv.tv_nsec;
}

void tglt_secure_random (unsigned char *s, int l) {
  if (TGLC_rand_bytes (s, l) <= 0) {
    /*if (allow_weak_random) {
      TGLC_rand_pseudo_bytes (s, l);
    } else {*/
      assert (0 && "End of random. If you want, you can start with -w");
    //}
  } else {
    #ifdef VALGRIND_FIXES
      VALGRIND_MAKE_MEM_DEFINED (s, l);
      VALGRIND_CHECK_MEM_IS_DEFINED (s, l);
    #endif
  }
}

struct tgl_allocator tgl_allocator_release = {
  .alloc = tgl_alloc_release,
  .realloc = tgl_realloc_release,
  .free = tgl_free_release
};

long long tgl_get_allocated_bytes (void) {
  return total_allocated_bytes;
}
struct tgl_allocator *tgl_allocator = &tgl_allocator_release;
