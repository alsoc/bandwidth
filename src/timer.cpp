#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"

// Buffer length should be > size+1
static inline char* seek_after(FILE* fp, char delim, char* buffer, int size) {
  char* end = buffer + size;
  char* first = (char*) memchr(buffer, delim, size);
  int n = 0, i = 0;

  while (!first) {
    n = fread(buffer, 1, size, fp);
    buffer[n] = 0;
    if (n == 0) return NULL;
    first = (char*) memchr(buffer, delim, size);
  }


  i = (first - buffer) + 1;


  memmove(buffer, buffer + i, size - i);
  n = fread(end - i, 1, i, fp);
  end[n-i] = 0;

  return buffer;
}

static inline double get_nominal_frequency() {
  char buffer[128+1];
  FILE* fp;
  const char *pattern = "";
  char* line;
  char* end = &buffer[128];
  int n = 0;

  // safety
  *end = 0;

  fp = fopen("freq", "r");
  if (!fp) {
    pattern = "model name";
    fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
      fprintf(stderr, "Could not open /proc/cpuinfo\n");
      return 1e9;
    }
  }
  int pattern_length = strlen(pattern);

  n = fread(buffer, 1, 128, fp);
  buffer[n] = 0;
  line = n ? buffer : NULL;
  while (line && *line) {
    // If we found the good line
    if (strncmp(pattern, line, pattern_length) == 0) {
      line = seek_after(fp, '@', buffer, 128);

      if (!line || !*line) break;

      char* after;
      double f = strtod(buffer, &after);

      if (after && after != line && (after = (char*) memchr(after, 'H', end - after)) && after[1] == 'z') {
        switch (after[-1]) {
          case 'G':
            f *= 1e3;
            [[fallthrough]];
          case 'M':
            f *= 1e3;
            [[fallthrough]];
          case 'k':
          case 'K':
            f *= 1e3;
        }
        return f;
      }
    }

    // Retrieve next line
    line = seek_after(fp, '\n', buffer, 128);
  }

  fclose(fp);
  fprintf(stderr, "Could not find nominal frequency\n");
  return -1.;
}






#ifdef __x86_64__
#include <x86intrin.h>
__attribute((noinline)) Timer::counter_t Timer::read(void) noexcept {
  return _rdtsc();
}

double Timer::frequency = get_nominal_frequency();
bool Timer::low_overhead = true;

#else
__attribute((noinline)) Timer::counter_t Timer::read(void) noexcept {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (Timer::counter_t) t.tv_nsec + (Timer::counter_t) 1000000000 * (Timer::counter_t) t.tv_sec;
}

double Timer::frequency = 1e9;
bool Timer::low_overhead = false;

#endif

namespace {
  static Timer::diff_t compute_overhead(int tries) noexcept {
    int i;
    Timer::counter_t t0, t1;
    Timer::diff_t d, dmin = -1;
    for (i = 0; i < tries; ++i) {
      Timer::reset();
      t0 = Timer::read();
      t1 = Timer::read();
      d = (Timer::diff_t) t1 - (Timer::diff_t) t0;
      dmin = (d > 0 && (dmin < 0 || d < dmin)) ? d : dmin;
    }
    return dmin;
  }
}

Timer::diff_t Timer::overhead = compute_overhead(65536) * 0.8;
