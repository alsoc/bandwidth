#include <iostream>
#include "bandwidth.h"
#include "stream.h"
#include "omp-helper.h"
#include <omp.h>

namespace {
  template <class F>
  double bench(F&& f, int repeat = 1, int tries = 1) noexcept {
    using counter_t = Timer::counter_t;
    using diff_t = Timer::diff_t;

    static diff_t dmin = -1, dmax = 0;
    OMP(master) {
      dmin = -1;
      dmax = 0;
    }
    counter_t t0, t1;
    for (int i = 0; i < tries; i++) {
      Timer::reset();
      OMP(barrier);
      asm volatile ("");
      counter_t t0 = Timer::read();
      for (int j = 0; j < repeat; j++) {
        asm volatile ("");
        f();
        asm volatile ("");
      }
      counter_t t1 = Timer::read();
      asm volatile ("");
      diff_t d = Timer::diff(t0, t1);
      OMP(critical) {
        dmax = (dmax < 0 || d > dmax) ? d : dmax;
      }
      OMP(barrier);
      OMP(master) {
        dmin = (dmin < 0 || dmax < dmin) ? dmax : dmin;
        dmax = 0;
      }
    }
    return static_cast<double>(dmin) / (repeat * Timer::frequency);
  }

  template <int N>
  struct Bandwidth {
    template <class T>
    static double read(const T*restrict A, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return sizeof(T) * n / bench([A, n]{ stream<N>::read(A, n); }, repeat, tries);
    }
    template <class T>
    static double write(T*restrict A, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return sizeof(T) * n / bench([A, n]{ stream<N>::write(A, n); }, repeat, tries);
    }
    template <class T>
    static double copy(const T*restrict A, T*restrict B, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return 2*sizeof(T) * n / bench([A, B, n]{ stream<N>::copy(A, B, n); }, repeat, tries);
    }
    template <class T>
    static double scale(const T*restrict A, T*restrict B, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      T scalar = 1.2345;
      return 2*sizeof(T) * n / bench([A, B, n, scalar]{ stream<N>::scale(scalar, A, B, n); }, repeat, tries);
    }
    template <class T>
    static double add(const T*restrict A, const T*restrict B, T*restrict C, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return 3*sizeof(T) * n / bench([A, B, C, n]{ stream<N>::add(A, B, C, n); }, repeat, tries);
    }
    template <class T>
    static double triad(const T*restrict A, const T*restrict B, T*restrict C, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      T scalar = 1.2345;
      return 3*sizeof(T) * n / bench([A, B, C, n, scalar]{ stream<N>::triad(scalar, A, B, C, n); }, repeat, tries);
    }

    operator bandwidth() const noexcept {
      bandwidth b;
      b.kern = N;
      b.read_f = &read;
      b.read_d = &read;
      b.write_f = &write;
      b.write_d = &write;
      b.copy_f = &copy;
      b.copy_d = &copy;
      b.scale_f = &scale;
      b.scale_d = &scale;
      b.add_f = &add;
      b.add_d = &add;
      b.triad_f = &triad;
      b.triad_d = &triad;
      return b;
    }
  };
}


bandwidth bandwidth_benches[] = {
  Bandwidth<  1>{},
  Bandwidth<  2>{},
  Bandwidth<  4>{},
  Bandwidth<  8>{},
  Bandwidth< 16>{},
  Bandwidth< 32>{},
  Bandwidth< 64>{},
  Bandwidth<128>{},
  Bandwidth<256>{},
  Bandwidth<512>{},
  bandwidth{}
};
