#include "timer.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <time.h>
__attribute((noinline)) Timer::counter_t Timer::read(void) noexcept {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (Timer::counter_t) t.tv_nsec + (Timer::counter_t) 1000000000 * (Timer::counter_t) t.tv_sec;
}

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

Timer::diff_t Timer::overhead = compute_overhead(128);
