#include <iostream>
#include "bandwidth.h"
#include "stream.h"
#include "omp-helper.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {
  template <class F>
  float64_t bench(F&& f, int repeat = 1, int tries = 1) noexcept {
    using counter_t = Timer::counter_t;
    using diff_t = Timer::diff_t;

    static diff_t dmin = -1, dmax = 0;
    OMP(master) {
      dmin = -1;
      dmax = 0;
    }
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
    return static_cast<float64_t>(dmin) / (repeat * Timer::frequency);
  }

  template <int N, bool nt>
  struct Bandwidth {
    template <class T>
    static float64_t read(const T*restrict A, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return sizeof(T) * n / bench([A, n]{ stream<N, nt>::read(A, n); }, repeat, tries);
    }
    template <class T>
    static float64_t write(T*restrict A, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return sizeof(T) * n / bench([A, n]{ stream<N, nt>::write(A, n); }, repeat, tries);
    }
    template <class T>
    static float64_t copy(const T*restrict A, T*restrict B, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return 2*sizeof(T) * n / bench([A, B, n]{ stream<N, nt>::copy(A, B, n); }, repeat, tries);
    }
    template <class T>
    static float64_t incr(T*restrict A, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return 2*sizeof(T) * n / bench([A, n]{ stream<N, nt>::incr(A, n); }, repeat, tries);
    }
    template <class T>
    static float64_t scale(const T*restrict A, T*restrict B, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      T scalar = 1.2345;
      return 2*sizeof(T) * n / bench([A, B, n, scalar]{ stream<N, nt>::scale(scalar, A, B, n); }, repeat, tries);
    }
    template <class T>
    static float64_t add(const T*restrict A, const T*restrict B, T*restrict C, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      return 3*sizeof(T) * n / bench([A, B, C, n]{ stream<N, nt>::add(A, B, C, n); }, repeat, tries);
    }
    template <class T>
    static float64_t triad(const T*restrict A, const T*restrict B, T*restrict C, long long n, int repeat = 1, int tries = 1) noexcept {
      if (n == 0) return 0.;
      T scalar = 1.2345;
      return 3*sizeof(T) * n / bench([A, B, C, n, scalar]{ stream<N, nt>::triad(scalar, A, B, C, n); }, repeat, tries);
    }

    operator bandwidth() const noexcept {
      bandwidth b;
      b.kern = N;
      b.nontemporal = nt;
      b.read_f32 = &read;
      b.read_f64 = &read;
      b.write_f32 = &write;
      b.write_f64 = &write;
      b.copy_f32 = &copy;
      b.copy_f64 = &copy;
      b.incr_f32 = &incr;
      b.incr_f64 = &incr;
      b.scale_f32 = &scale;
      b.scale_f64 = &scale;
      b.add_f32 = &add;
      b.add_f64 = &add;
      b.triad_f32 = &triad;
      b.triad_f64 = &triad;
      return b;
    }
  };
}


bandwidth bandwidth_benches[] = {
  // Temporal stores
  Bandwidth<  1, false>{},
  Bandwidth<  2, false>{},
  Bandwidth<  4, false>{},
  Bandwidth<  8, false>{},
  Bandwidth< 16, false>{},
  Bandwidth< 32, false>{},
  Bandwidth< 64, false>{},
  Bandwidth<128, false>{},
  Bandwidth<256, false>{},
  Bandwidth<512, false>{},
  // Non-Temporal stores
  //Bandwidth<  1, true>{}, // scalar doesn't have non temporal stores
  Bandwidth<  2, true>{},
  Bandwidth<  4, true>{},
  Bandwidth<  8, true>{},
  Bandwidth< 16, true>{},
  Bandwidth< 32, true>{},
  Bandwidth< 64, true>{},
  Bandwidth<128, true>{},
  Bandwidth<256, true>{},
  Bandwidth<512, true>{},
  bandwidth{}
};
