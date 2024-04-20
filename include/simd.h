#ifndef SIMD_H
#define SIMD_H

#include "types.h"

#ifdef __SSE2__
#include <immintrin.h>
#endif
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#ifdef __ALTIVEC__
#include <altivec.h>
#endif

template <class T>
struct load_addr {
  const T* p;
};

template <class T>
load_addr<T> vload(const T* p) {
  return {p};
}

template <class T, int N>
class simd {
  private:
    simd<T, N/2> low, high;
    simd(simd<T, N/2> low, simd<T, N/2> high) noexcept : low(low), high(high) {}
  public:
    explicit simd(T val) noexcept : low(val), high(val) {}
    simd(load_addr<T> la) noexcept : low(la), high(vload(la.p + N/2)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(T* p, simd v) noexcept {
      vstore(p, v.low);
      vstore(p + N/2, v.high);
    }
    friend void vstorent(T* p, simd v) noexcept {
      vstorent(p, v.low);
      vstorent(p + N/2, v.high);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return {vadd(a.low, b.low), vadd(a.high, b.high)};
    }
    friend simd vmul(simd a, simd b) noexcept {
      return {vmul(a.low, b.low), vmul(a.high, b.high)};
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return {vfma(a.low, b.low, c.low), vfma(a.high, b.high, c.high)};
    }
    friend inline __attribute((always_inline)) void vkeep(simd a) noexcept {
      vkeep(a.low);
      vkeep(a.high);
    }
};

template <class T>
class simd<T, 0> {
  static_assert(sizeof(T) == 0, "Empty SIMD is not allowed");
};
template <class T>
class simd<T, 1> {
  private:
    T inner = 0;
  public:
    explicit simd(T val) noexcept : inner(val) {}
    simd(load_addr<T> la) noexcept : inner(*la.p) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(T* p, simd v) noexcept {
      *p = v.inner;
    }
    friend void vstorent(T* p, simd v) noexcept {
      *p = v.inner;
    }

    friend simd vadd(simd a, simd b) noexcept {
      return simd(a.inner + b.inner);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return simd(a.inner * b.inner);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return simd(a.inner * b.inner + c.inner);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+X"(a.inner));
    }
};

#ifdef __SSE2__
template <>
class simd<float32_t, 2> {
  private:
    __m128 inner = _mm_setzero_ps();
    simd(__m128 v) noexcept : inner(v) {}
    operator __m128() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(_mm_set1_ps(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      _mm_storel_pi((__m64*)p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      _mm_storel_pi((__m64*)p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return _mm_add_ps(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm_mul_ps(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return _mm_add_ps(_mm_mul_ps(a, b), c);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
template <>
class simd<float32_t, 4> {
  private:
    __m128 inner = _mm_setzero_ps();
    simd(__m128 v) noexcept : inner(v) {}
    operator __m128() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(_mm_set1_ps(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(_mm_load_ps(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      _mm_store_ps(p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      _mm_stream_ps(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return _mm_add_ps(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm_mul_ps(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return _mm_add_ps(_mm_mul_ps(a, b), c);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
template <>
class simd<float64_t, 2> {
  private:
    __m128d inner = _mm_setzero_pd();
    simd(__m128d v) noexcept : inner(v) {}
    operator __m128d() const noexcept {
      return inner;
    }
  public:
    explicit simd(float64_t val) noexcept : inner(_mm_set1_pd(val)) {}
    simd(load_addr<float64_t> la) noexcept : inner(_mm_load_pd(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float64_t* p, simd v) noexcept {
      _mm_store_pd(p, v);
    }
    friend void vstorent(float64_t* p, simd v) noexcept {
      _mm_stream_pd(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return _mm_add_pd(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm_mul_pd(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return _mm_add_pd(_mm_mul_pd(a, b), c);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
#endif
#ifdef __AVX__
template <>
class simd<float32_t, 8> {
  private:
    __m256 inner = _mm256_setzero_ps();
    simd(__m256 v) noexcept : inner(v) {}
    operator __m256() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(_mm256_set1_ps(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(_mm256_load_ps(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      _mm256_store_ps(p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      _mm256_stream_ps(p, v);
    }
    friend simd vadd(simd a, simd b) noexcept {
      return _mm256_add_ps(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm256_mul_ps(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
#ifdef __FMA__
      return _mm256_fmadd_ps(a, b, c);
#else
      return _mm256_add_ps(_mm256_mul_ps(a, b), c);
#endif
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
template <>
class simd<float64_t, 4> {
  private:
    __m256d inner = _mm256_setzero_pd();
    simd(__m256d v) noexcept : inner(v) {}
    operator __m256d() const noexcept {
      return inner;
    }
  public:
    explicit simd(float64_t val) noexcept : inner(_mm256_set1_pd(val)) {}
    simd(load_addr<float64_t> la) noexcept : inner(_mm256_load_pd(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float64_t* p, simd v) noexcept {
      _mm256_store_pd(p, v);
    }
    friend void vstorent(float64_t* p, simd v) noexcept {
      _mm256_stream_pd(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return _mm256_add_pd(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm256_mul_pd(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
#ifdef __FMA__
      return _mm256_fmadd_pd(a, b, c);
#else
      return _mm256_add_pd(_mm256_mul_pd(a, b), c);
#endif
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
#endif
#ifdef __AVX512F__
template <>
class simd<float32_t, 16> {
  private:
    __m512 inner = _mm512_setzero_ps();
    simd(__m512 v) noexcept : inner(v) {}
    operator __m512() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(_mm512_set1_ps(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(_mm512_load_ps(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      _mm512_store_ps(p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      _mm512_stream_ps(p, v);
    }
    friend simd vadd(simd a, simd b) noexcept {
      return _mm512_add_ps(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm512_mul_ps(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return _mm512_fmadd_ps(a, b, c);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
template <>
class simd<float64_t, 8> {
  private:
    __m512d inner = _mm512_setzero_pd();
    simd(__m512d v) noexcept : inner(v) {}
    operator __m512d() const noexcept {
      return inner;
    }
  public:
    explicit simd(float64_t val) noexcept : inner(_mm512_set1_pd(val)) {}
    simd(load_addr<float64_t> la) noexcept : inner(_mm512_load_pd(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float64_t* p, simd v) noexcept {
      _mm512_store_pd(p, v);
    }
    friend void vstorent(float64_t* p, simd v) noexcept {
      _mm512_stream_pd(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return _mm512_add_pd(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return _mm512_mul_pd(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return _mm512_fmadd_pd(a, b, c);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
#endif
#ifdef __ARM_NEON
template <>
class simd<float32_t, 2> {
  private:
    float32x2_t inner = vdup_n_f32(0.f);
    simd(float32x2_t v) noexcept : inner(v) {}
    operator float32x2_t() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(vdup_n_f32(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(vld1_f32(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      vst1_f32(p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      vst1_f32(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return vadd_f32(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return vmul_f32(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return vmla_f32(c, a, b);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+x"(a.inner));
    }
};
template <>
class simd<float32_t, 4> {
  private:
    float32x4_t inner = vdupq_n_f32(0.f);
    simd(float32x4_t v) noexcept : inner(v) {}
    operator float32x4_t() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner(vdupq_n_f32(val)) {}
    simd(load_addr<float32_t> la) noexcept : inner(vld1q_f32(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
      vst1q_f32(p, v);
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
      vst1q_f32(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return vaddq_f32(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return vmulq_f32(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return vmlaq_f32(c, a, b);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+w"(a.inner));
    }
};
#ifdef __aarch64__
template <>
class simd<float64_t, 2> {
  private:
    float64x2_t inner = vdupq_n_f64(0.f);
    simd(float64x2_t v) noexcept : inner(v) {}
    operator float64x2_t() const noexcept {
      return inner;
    }
  public:
    explicit simd(float64_t val) noexcept : inner(vdupq_n_f64(val)) {}
    simd(load_addr<float64_t> la) noexcept : inner(vld1q_f64(la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float64_t* p, simd v) noexcept {
      vst1q_f64(p, v);
    }
    friend void vstorent(float64_t* p, simd v) noexcept {
      vst1q_f64(p, v);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return vaddq_f64(a, b);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return vmulq_f64(a, b);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return vmlaq_f64(c, a, b);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+w"(a.inner));
    }
};
#endif
#endif
#ifdef __ALTIVEC__
//template <>
//class simd<float32_t, 2> {
//  private:
//    float32x2_t inner = vdup_n_f32(0.f);
//    simd(float32x2_t v) noexcept : inner(v) {}
//    operator float32x2_t() const noexcept {
//      return inner;
//    }
//  public:
//    explicit simd(float32_t val) noexcept : inner(vdup_n_f32(val)) {}
//    simd(load_addr<float32_t> la) noexcept : inner(vld1_f32(la.p)) {}
//    simd() = default;
//    simd(const simd&) = default;
//    simd& operator=(const simd&) = default;
//    ~simd() = default;
//
//    friend void vstore(float32_t* p, simd v) noexcept {
//      vst1_f32(p, v);
//    }
//    friend void vstorent(float32_t* p, simd v) noexcept {
//      vst1_f32(p, v);
//    }
//
//    friend simd vadd(simd a, simd b) noexcept {
//      return vadd_f32(a, b);
//    }
//    friend simd vmul(simd a, simd b) noexcept {
//      return vmul_f32(a, b);
//    }
//    friend simd vfma(simd a, simd b, simd c) noexcept {
//      return vmla_f32(c, a, b);
//    }
//    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
//      asm volatile ("" : "+x"(a.inner));
//    }
//};
template <>
class simd<float32_t, 4> {
  private:
    vector float32_t inner = (vector float32_t) vec_splat_u32(0);
    simd(vector float32_t v) noexcept : inner(v) {}
    operator vector float32_t() const noexcept {
      return inner;
    }
  public:
    explicit simd(float32_t val) noexcept : inner{val, val, val, val} {}
#ifndef __VSX__
    simd(load_addr<float32_t> la) noexcept : inner(vec_ld(0, la.p)) {}
#else
    simd(load_addr<float32_t> la) noexcept : inner(vec_vsx_ld(0, la.p)) {}
#endif
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float32_t* p, simd v) noexcept {
#ifndef __VSX__
      vec_st(v.inner, 0, p);
#else
      vec_vsx_st(v.inner, 0, p);
#endif
    }
    friend void vstorent(float32_t* p, simd v) noexcept {
#ifndef __VSX__
      vec_st(v.inner, 0, p);
#else
      vec_vsx_st(v.inner, 0, p);
#endif
    }

    friend simd vadd(simd a, simd b) noexcept {
      return vec_add(a.inner, b.inner);
    }
    friend simd vmul(simd a, simd b) noexcept {
#ifndef __VSX__
      return vec_madd(a.inner, b.inner, (vector float32_t) vec_splat_u32(0));
#else
      return vec_mul(a.inner, b.inner);
#endif
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return vec_madd(a.inner, b.inner, c.inner);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
#ifndef __VSX__
      asm volatile ("" : "+v"(a.inner));
#else
      asm volatile ("" : "+wa"(a.inner));
#endif
    }
};
#ifdef __VSX__
template <>
class simd<float64_t, 2> {
  private:
    vector float64_t inner = (vector float64_t) vec_splat_u32(0);
    simd(vector float64_t v) noexcept : inner(v) {}
    operator vector float64_t() const noexcept {
      return inner;
    }
  public:
    explicit simd(float64_t val) noexcept : inner{val, val} {}
    simd(load_addr<float64_t> la) noexcept : inner(vec_vsx_ld(0, la.p)) {}
    simd() = default;
    simd(const simd&) = default;
    simd& operator=(const simd&) = default;
    ~simd() = default;

    friend void vstore(float64_t* p, simd v) noexcept {
      vec_vsx_st(v.inner, 0, p);
    }
    friend void vstorent(float64_t* p, simd v) noexcept {
      vec_vsx_st(v.inner, 0, p);
    }

    friend simd vadd(simd a, simd b) noexcept {
      return vec_add(a.inner, b.inner);
    }
    friend simd vmul(simd a, simd b) noexcept {
      return vec_mul(a.inner, b.inner);
    }
    friend simd vfma(simd a, simd b, simd c) noexcept {
      return vec_madd(a.inner, b.inner, c.inner);
    }
    friend inline __attribute((always_inline)) void vkeep(simd& a) noexcept {
      asm volatile ("" : "+wa"(a.inner));
    }
};
#endif
#endif

template <int N, class T>
simd<T, N> vload(const T* p) {
  return vload(p);
}


#endif
