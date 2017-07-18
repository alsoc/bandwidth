#ifndef BANDWIDTH_H
#define BANDWIDTH_H
#ifndef restrict
#define restrict __restrict__
#endif
#include "timer.h"


struct bandwidth {
  // kernel size
  int kern = 0;
  // versions
  double (*read_f )(const float *restrict A,                                             long long n, int repeat, int tries) noexcept = nullptr;
  double (*write_f)(      float *restrict A,                                             long long n, int repeat, int tries) noexcept = nullptr;
  double (*copy_f )(const float *restrict A,       float *restrict B,                    long long n, int repeat, int tries) noexcept = nullptr;
  double (*scale_f)(const float *restrict A,       float *restrict B,                    long long n, int repeat, int tries) noexcept = nullptr;
  double (*add_f  )(const float *restrict A, const float *restrict B, float *restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  double (*triad_f)(const float *restrict A, const float *restrict B, float *restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  double (*read_d )(const double*restrict A,                                             long long n, int repeat, int tries) noexcept = nullptr;
  double (*write_d)(      double*restrict A,                                             long long n, int repeat, int tries) noexcept = nullptr;
  double (*copy_d )(const double*restrict A,       double*restrict B,                    long long n, int repeat, int tries) noexcept = nullptr;
  double (*scale_d)(const double*restrict A,       double*restrict B,                    long long n, int repeat, int tries) noexcept = nullptr;
  double (*add_d  )(const double*restrict A, const double*restrict B, double*restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  double (*triad_d)(const double*restrict A, const double*restrict B, double*restrict C, long long n, int repeat, int tries) noexcept = nullptr;

  // overloads
  double read(const float *restrict A, long long n, int repeat, int tries) const noexcept {
    return read_f(A, n, repeat, tries);
  }
  double read(const double*restrict A, long long n, int repeat, int tries) const noexcept {
    return read_d(A, n, repeat, tries);
  }
  double write(float *restrict A, long long n, int repeat, int tries) const noexcept {
    return write_f(A, n, repeat, tries);
  }
  double write(double*restrict A, long long n, int repeat, int tries) const noexcept {
    return write_d(A, n, repeat, tries);
  }
  double copy(const float *restrict A, float *restrict B, long long n, int repeat, int tries) const noexcept {
    return copy_f(A, B, n, repeat, tries);
  }
  double copy(const double*restrict A, double*restrict B, long long n, int repeat, int tries) const noexcept {
    return copy_d(A, B, n, repeat, tries);
  }
  double scale(const float *restrict A, float *restrict B, long long n, int repeat, int tries) const noexcept {
    return scale_f(A, B, n, repeat, tries);
  }
  double scale(const double*restrict A, double*restrict B, long long n, int repeat, int tries) const noexcept {
    return scale_d(A, B, n, repeat, tries);
  }
  double add(const float *restrict A, const float *restrict B, float *restrict C, long long n, int repeat, int tries) const noexcept {
    return add_f(A, B, C, n, repeat, tries);
  }
  double add(const double*restrict A, const double*restrict B, double*restrict C, long long n, int repeat, int tries) const noexcept {
    return add_d(A, B, C, n, repeat, tries);
  }
  double triad(const float *restrict A, const float *restrict B, float *restrict C, long long n, int repeat, int tries) const noexcept {
    return triad_f(A, B, C, n, repeat, tries);
  }
  double triad(const double*restrict A, const double*restrict B, double*restrict C, long long n, int repeat, int tries) const noexcept {
    return triad_d(A, B, C, n, repeat, tries);
  }
};

extern bandwidth bandwidth_benches[];


#endif // BANDWIDTH_H
