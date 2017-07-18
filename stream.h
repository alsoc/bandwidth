#ifndef STREAM_H
#define STREAM_H
#include "simd.h"
#ifndef restrict
#define restrict __restrict__
#endif
#include "omp-helper.h"

template <int N = 1>
struct stream {
  constexpr static int kern = N;

  template <class T>
  static void read(const T*restrict A, long long n) {
    using vec = simd<T, N>;
    long long i;

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vec a = vload(&A[i]);
      vkeep(a);
    }
  }

  template <class T>
  static void write(T*restrict A, long long n) {
    using vec = simd<T, N>;
    long long i;
    vec a(0);

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vkeep(a);
      vstore(&A[i], a);
    }
  }

  template <class T>
  static void copy(const T*restrict A, T*restrict B, long long n) {
    using vec = simd<T, N>;
    long long i;

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vec a = vload(&A[i]);
      vec b = a;
      vstore(&B[i], b);
    }
  }

  template <class T>
  static void scale(T scalar, const T*restrict A, T*restrict B, long long n) {
    using vec = simd<T, N>;
    vec vscalar(scalar);
    long long i;

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vec a = vload(&A[i]);
      vec b = vmul(vscalar, a);
      vstore(&B[i], b);
    }
  }

  template <class T>
  static void add(const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    using vec = simd<T, N>;
    long long i;

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vec a = vload(&A[i]);
      vec b = vload(&B[i]);
      vec c = vadd(a, b);
      vstore(&C[i], c);
    }
  }

  template <class T>
  static void triad(T scalar, const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    using vec = simd<T, N>;
    vec vscalar(scalar);
    long long i;

    OMP(for schedule(static) nowait)
    for (i = 0; i < n; i += N) {
      vec a = vload(&A[i]);
      vec b = vload(&B[i]);
      vec c = vfma(vscalar, a, b);
      vstore(&C[i], c);
    }
  }
};


template <>
struct stream<1> {
  constexpr static int kern = 1;

  template <class T>
  static void read(const T*restrict A, long long n) {
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      T a = A[i];
      asm volatile ("" :"+X"(a));
    }
  }

  template <class T>
  static void write(T*restrict A, long long n) {
    T a = 0;
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      asm volatile ("" :"+X"(a));
      A[i] = a;
    }
  }

  template <class T>
  static void copy(const T*restrict A, T*restrict B, long long n) {
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      B[i] = A[i];
    }
  }

  template <class T>
  static void scale(T scalar, const T*restrict A, T*restrict B, long long n) {
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      B[i] = scalar * A[i];
    }
  }

  template <class T>
  static void add(const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      C[i] = A[i] + B[i];
    }
  }

  template <class T>
  static void triad(T scalar, const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    OMP(for schedule(static) nowait)
    for (long long i = 0; i < n; ++i) {
      C[i] = scalar * A[i] + B[i];
    }
  }
};

#endif // STREAM_H
