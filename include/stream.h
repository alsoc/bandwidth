#ifndef STREAM_H
#define STREAM_H
#include "simd.h"
#include <stdio.h>
#ifndef restrict
#define restrict __restrict__
#endif

template <int N = 1, bool nt = false>
struct stream {
  constexpr static int kern = N;

  template <class T>
  static void read(const T*restrict A, long long n) {
    using vec = simd<T, N>;
    long long i;

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

    if (nt) {
      for (i = 0; i < n; i += N) {
        vstorent(&A[i], a);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vstore(&A[i], a);
      }
    }
  }

  template <class T>
  static void copy(const T*restrict A, T*restrict B, long long n) {
    using vec = simd<T, N>;
    long long i;

    if (nt) {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = a;
        vstorent(&B[i], b);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = a;
        vstore(&B[i], b);
      }
    }
  }

  template <class T>
  static void incr(T*restrict A, long long n) {
    using vec = simd<T, N>;
    vec vone(static_cast<T>(1));
    long long i;

    if (nt) {
      for (i = 0; i < n; i += N) {
        vec a1 = vload(&A[i]);
        vec a2 = vadd(a1, vone);
        vstorent(&A[i], a2);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vec a1 = vload(&A[i]);
        vec a2 = vadd(a1, vone);
        vstore(&A[i], a2);
      }
    }
  }

  template <class T>
  static void scale(T scalar, const T*restrict A, T*restrict B, long long n) {
    using vec = simd<T, N>;
    vec vscalar(scalar);
    long long i;

    if (nt) {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vmul(vscalar, a);
        vstorent(&B[i], b);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vmul(vscalar, a);
        vstore(&B[i], b);
      }
    }
  }

  template <class T>
  static void add(const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    using vec = simd<T, N>;
    long long i;

    if (nt) {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vload(&B[i]);
        vec c = vadd(a, b);
        vstorent(&C[i], c);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vload(&B[i]);
        vec c = vadd(a, b);
        vstore(&C[i], c);
      }
    }
  }

  template <class T>
  static void triad(T scalar, const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    using vec = simd<T, N>;
    vec vscalar(scalar);
    long long i;

    if (nt) {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vload(&B[i]);
        vec c = vfma(vscalar, a, b);
        vstorent(&C[i], c);
      }
    } else {
      for (i = 0; i < n; i += N) {
        vec a = vload(&A[i]);
        vec b = vload(&B[i]);
        vec c = vfma(vscalar, a, b);
        vstore(&C[i], c);
      }
    }
  }
};


template <>
struct stream<1, false> {
  constexpr static int kern = 1;

  template <class T>
  static void read(const T*restrict A, long long n) {
    for (long long i = 0; i < n; ++i) {
      T a = A[i];
      asm volatile ("" :"+X"(a));
    }
  }

  template <class T>
  static void write(T*restrict A, long long n) {
    T a = 0;
    for (long long i = 0; i < n; ++i) {
      A[i] = a;
    }
  }

  template <class T>
  static void copy(const T*restrict A, T*restrict B, long long n) {
    for (long long i = 0; i < n; ++i) {
      B[i] = A[i];
    }
  }

  template <class T>
  static void incr(T*restrict A, long long n) {
    for (long long i = 0; i < n; ++i) {
      A[i] = A[i] + 1;
    }
  }

  template <class T>
  static void scale(T scalar, const T*restrict A, T*restrict B, long long n) {
    for (long long i = 0; i < n; ++i) {
      B[i] = scalar * A[i];
    }
  }

  template <class T>
  static void add(const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    for (long long i = 0; i < n; ++i) {
      C[i] = A[i] + B[i];
    }
  }

  template <class T>
  static void triad(T scalar, const T*restrict A, const T*restrict B, T*restrict C, long long n) {
    for (long long i = 0; i < n; ++i) {
      C[i] = scalar * A[i] + B[i];
    }
  }
};

#endif // STREAM_H
