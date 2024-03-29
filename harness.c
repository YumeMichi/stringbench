/*
 * Copyright (c) 2011, Linaro Limited
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Linaro nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

/** A simple harness that times how long a string function takes to
 * run.
 */

/* PENDING: Add EPL */

#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <assert.h>

#define NUM_ELEMS(_x) (sizeof(_x) / sizeof((_x)[0]))

#ifndef VERSION
#define VERSION "(unknown version)"
#endif

/** Make sure a function is called by using the return value */
#define SPOIL(_x)  volatile long x = (long)(_x); (void)x

/** Type of functions that can be tested */
typedef void (*stub_t)(void *dest, void *src, size_t n);

/** Meta data about one test */
struct test
{
  /** Test name */
  const char *name;
  /** Function to test */
  stub_t stub;
};

/** Flush the cache by reading a chunk of memory */
static void empty(volatile char *against)
{
  /* We know that there's a 16 k cache with 64 byte lines giving
     a total of 256 lines.  Read randomly from 256*5 places should
     flush everything */
  int offset = (1024 - 256)*1024;

  for (int i = offset; i < offset + 16*1024*3; i += 64)
    {
      against[i];
    }
}

/** Stub that does nothing.  Used for calibrating */
static void xbounce(void *dest __unused, void *src __unused, size_t n __unused)
{
  SPOIL(0);
}

/** Stub that calls memcpy */
static void xmemcpy(void *dest, void *src, size_t n)
{
  SPOIL(memcpy(dest, src, n));
}

/** Stub that calls memset */
static void xmemset(void *dest, void *src __unused, size_t n)
{
  SPOIL(memset(dest, 0, n));
}

/** Stub that calls strcpy */
static void xstrcpy(void *dest, void *src, size_t n __unused)
{
  SPOIL(strcpy(dest, src));
}

/** Stub that calls strlen */
static void xstrlen(void *dest, void *src __unused, size_t n __unused)
{
  SPOIL(strlen(dest));
}

/** Stub that calls strcmp */
static void xstrcmp(void *dest, void *src, size_t n __unused)
{
  SPOIL(strcmp(dest, src));
}

/** Stub that calls strchr */
static void xstrchr(void *dest __unused, void *src, size_t n)
{
  /* Put the character at the end of the string and before the null */
  ((char *)src)[n-1] = 32;
  SPOIL(strchr(src, 32));
}

/** Stub that calls memchr */
static void xmemchr(void *dest __unused, void *src, size_t n)
{
  /* Put the character at the end of the block */
  ((char *)src)[n-1] = 32;
  SPOIL(memchr(src, 32, n));
}

/** All functions that can be tested */
static const struct test tests[] =
  {
    { "bounce", xbounce },
    { "memchr", xmemchr },
    { "memcpy", xmemcpy },
    { "memset", xmemset },
    { "strchr", xstrchr },
    { "strcmp", xstrcmp },
    { "strcpy", xstrcpy },
    { "strlen", xstrlen },
    { NULL, NULL }
  };

/** Show basic usage */
static void usage(const char* name)
{
  printf("%s %s: run a string related benchmark.\n"
         "usage: %s [-c block-size] [-l loop-count] [-a alignment] [-f] [-t test-name]\n"
         , name, VERSION, name);

  printf("Tests:");

  for (const struct test *ptest = tests; ptest->name != NULL; ptest++)
    {
      printf(" %s", ptest->name);
    }

  printf("\n");

  exit(-1);
}

/** Find the test by name */
static const struct test *find_test(const char *name)
{
  if (name == NULL)
    {
      return tests + 0;
    }
  else
    {
      for (const struct test *p = tests; p->name != NULL; p++)
	{
          if (strcmp(p->name, name) == 0)
	    {
              return p;
	    }
	}
    }

  return NULL;
}

/** Take a pointer and ensure that the lower bits == alignment */
static char *realign(char *p, int alignment)
{
  if (alignment < 0)
    {
      return p;
    }

  uintptr_t pp = (uintptr_t)p;
  pp = (pp + 255) & ~255;
  pp += alignment;

  return (char *)pp;
}

/** Setup and run a test */
int main(int argc, char **argv)
{
  /* Buffers to read and write from */
  char *src = calloc(1024, 1024);
  char *dest = calloc(1024, 1024);

  assert(src != NULL && dest != NULL);

  /* Number of bytes per call */
  int count = 31;
  /* Number of times to run */
  int loops = 10000000;
  /* True to flush the cache each time */
  int flush = 0;
  /* Name of the test */
  const char *name = NULL;
  /* Alignment of both buffers */
  int alignment = -1;

  int opt;

  while ((opt = getopt(argc, argv, "c:l:ft:hva:")) > 0)
    {
      switch (opt)
	{
	case 'c':
          count = atoi(optarg);
          break;
	case 'l':
          loops = atoi(optarg);
          break;
	case 'a':
          alignment = atoi(optarg);
          break;
	case 'f':
          flush = 1;
          break;
	case 't':
          name = strdup(optarg);
          break;
	case 'h':
          usage(argv[0]);
          break;
	default:
          usage(argv[0]);
          break;
	}
    }

  /* Find the test by name */
  const struct test *ptest = find_test(name);

  if (ptest == NULL)
    {
      usage(argv[0]);
    }

  src = realign(src, alignment);
  dest = realign(dest, alignment);

  /* Fill the first 16 k with non-zero, reproducable random data */
  srandom(1539);

  for (int i = 0; i < 16*1024; i++)
    {
      src[i] = (char)random() | 1;
      dest[i] = src[i];
    }

  /* Make sure the buffers are null terminated for any string tests */
  src[count] = 0;
  dest[count] = 0;

  struct timespec start, end;
  int err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  assert(err == 0);

  /* Preload */
  stub_t stub = ptest->stub;

  /* Run two variants to reduce the cost of testing for the flush */
  if (flush == 0)
    {
      for (int i = 0; i < loops; i++)
	{
	  (*stub)(dest, src, count);
	}
    }
  else
    {
      for (int i = 0; i < loops; i++)
	{
	  (*stub)(dest, src, count);
	  empty(dest);
	}
    }

  err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
  assert(err == 0);

  /* Drop any leading path and pull the variant name out of the executable */
  char *variant = strrchr(argv[0], '/');

  if (variant == NULL)
    {
      variant = argv[0];
    }

  variant = strstr(variant, "try-");
  assert(variant != NULL);

  double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
  /* Estimate the bounce time.  Measured on a Panda. */
  double bounced = 0.448730 * loops / 50000000;

  /* Dump both machine and human readable versions */
  printf("%s:%s:%u:%u:%d:%.6f: took %.6f s for %u calls to %s of %u bytes.  ~%.3f MB/s corrected.\n", 
         variant + 4, ptest->name,
	 count, loops, alignment,
	 elapsed,
         elapsed, loops, ptest->name, count,
         (double)loops*count/(elapsed - bounced)/(1024*1024));

  return 0;
}
