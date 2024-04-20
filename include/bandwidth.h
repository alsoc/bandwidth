#ifndef BANDWIDTH_H
#define BANDWIDTH_H
#ifndef restrict
#define restrict __restrict__
#endif
#include "timer.h"
#include "types.h"


struct bandwidth {
  // kernel size
  int kern = 0;
  bool nontemporal = false;
  // versions
  float64_t (*read_f32 )(const float32_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*write_f32)(      float32_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*copy_f32 )(const float32_t *restrict A,       float32_t *restrict B,                        long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*incr_f32 )(      float32_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*scale_f32)(const float32_t *restrict A,       float32_t *restrict B,                        long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*add_f32  )(const float32_t *restrict A, const float32_t *restrict B, float32_t *restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*triad_f32)(const float32_t *restrict A, const float32_t *restrict B, float32_t *restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*read_f64 )(const float64_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*write_f64)(      float64_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*copy_f64 )(const float64_t *restrict A,       float64_t *restrict B,                        long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*incr_f64 )(      float64_t *restrict A,                                                     long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*scale_f64)(const float64_t *restrict A,       float64_t *restrict B,                        long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*add_f64  )(const float64_t *restrict A, const float64_t *restrict B, float64_t *restrict C, long long n, int repeat, int tries) noexcept = nullptr;
  float64_t (*triad_f64)(const float64_t *restrict A, const float64_t *restrict B, float64_t *restrict C, long long n, int repeat, int tries) noexcept = nullptr;

  // overloads
  float64_t read(const float32_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return read_f32(A, n, repeat, tries);
  }
  float64_t read(const float64_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return read_f64(A, n, repeat, tries);
  }
  float64_t write(float32_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return write_f32(A, n, repeat, tries);
  }
  float64_t write(float64_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return write_f64(A, n, repeat, tries);
  }
  float64_t copy(const float32_t *restrict A, float32_t *restrict B, long long n, int repeat, int tries) const noexcept {
    return copy_f32(A, B, n, repeat, tries);
  }
  float64_t copy(const float64_t *restrict A, float64_t *restrict B, long long n, int repeat, int tries) const noexcept {
    return copy_f64(A, B, n, repeat, tries);
  }
  float64_t incr(float32_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return incr_f32(A, n, repeat, tries);
  }
  float64_t incr(float64_t *restrict A, long long n, int repeat, int tries) const noexcept {
    return incr_f64(A, n, repeat, tries);
  }
  float64_t scale(const float32_t *restrict A, float32_t *restrict B, long long n, int repeat, int tries) const noexcept {
    return scale_f32(A, B, n, repeat, tries);
  }
  float64_t scale(const float64_t *restrict A, float64_t *restrict B, long long n, int repeat, int tries) const noexcept {
    return scale_f64(A, B, n, repeat, tries);
  }
  float64_t add(const float32_t *restrict A, const float32_t *restrict B, float32_t *restrict C, long long n, int repeat, int tries) const noexcept {
    return add_f32(A, B, C, n, repeat, tries);
  }
  float64_t add(const float64_t *restrict A, const float64_t *restrict B, float64_t*restrict C, long long n, int repeat, int tries) const noexcept {
    return add_f64(A, B, C, n, repeat, tries);
  }
  float64_t triad(const float32_t *restrict A, const float32_t *restrict B, float32_t *restrict C, long long n, int repeat, int tries) const noexcept {
    return triad_f32(A, B, C, n, repeat, tries);
  }
  float64_t triad(const float64_t *restrict A, const float64_t *restrict B, float64_t*restrict C, long long n, int repeat, int tries) const noexcept {
    return triad_f64(A, B, C, n, repeat, tries);
  }
};

extern bandwidth bandwidth_benches[];


#endif // BANDWIDTH_H
