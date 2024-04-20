#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

static inline Timer::counter_t read_time(void) noexcept {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (Timer::counter_t) t.tv_nsec + 1000000000ull * (Timer::counter_t) t.tv_sec;
}


#ifdef __x86_64__
#include <unistd.h>
#include <x86intrin.h>

static inline float64_t get_nominal_frequency() {
  Timer::counter_t t0, t1, c0, c1;
  float64_t dt, dc;

  t0 = read_time();
  c0 = _rdtsc();

  usleep(100000); // wait for 0.1 sec

  t1 = read_time();
  c1 = _rdtsc();

  dt = (Timer::diff_t) t1 - (Timer::diff_t) t0;
  dt *= 1e-9;
  dc = (Timer::diff_t) c1 - (Timer::diff_t) c0;

  return dc / dt;
}

__attribute((noinline)) Timer::counter_t Timer::read(void) noexcept {
  return _rdtsc();
}

float64_t Timer::frequency = get_nominal_frequency();
bool Timer::low_overhead = true;

#else
__attribute((noinline)) Timer::counter_t Timer::read(void) noexcept {
  return read_time();
}

float64_t Timer::frequency = 1e9;
bool Timer::low_overhead = false;

#endif

static inline Timer::diff_t compute_overhead(int tries) noexcept {
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

Timer::diff_t Timer::overhead = compute_overhead(65536) * 0.8;
