#include <iostream>
#include <iomanip>
#include <cmath>
#include "allocation.h"
#include "bandwidth.h"
#include "omp-helper.h"

#ifdef _OPENMP
#include "omp.h"
#endif

template <class T>
constexpr const char* name() noexcept {
  return __PRETTY_FUNCTION__;
}
template <> constexpr const char* name<float >() noexcept { return "float" ; }
template <> constexpr const char* name<double>() noexcept { return "double"; }

struct bytes {
  double n = 0;
  bytes() = default;
  bytes(double n) : n(n) {}
  friend std::ostream& operator<<(std::ostream& out, bytes B) {
    double b = B.n;
    static const char letters[] = "\00fpnum KMGTPE\00";
    const char* letter = letters+6;
    while (letter[0] && letter[-1] && std::abs(b) < 1.) {
      b *= 1024.;
      --letter;
    }
    while (letter[0] && letter[+1] && std::abs(b) > 999.) {
      b /= 1024.;
      ++letter;
    }
    out << b << ' ' << *letter << 'B';
    return out;
  }
};

template <class F>
double max_bandwidth(F&& f) {
  double max_bandwidth = -1./0.;
  const bandwidth* b = bandwidth_benches;
  while (b->kern != 0) {
    double cur_bandwidth = f(b);
    max_bandwidth = std::max(max_bandwidth, cur_bandwidth);
    ++b;
  }
  return max_bandwidth;
}

long long round(long long n, int r) {
  return (n/r) * r;
}

template <class T>
void test(long long max_size, const long long* sizes) {
  long long N = max_size / sizeof(T);
  T *A1 = allocate<T>(N, 0x1000);
  T *A2 = allocate<T>(N/2, 0x1000);
  T *B2 = allocate<T>(N/2, 0x1000);
  T *A3 = allocate<T>(N/3, 0x1000);
  T *B3 = allocate<T>(N/3, 0x1000);
  T *C3 = allocate<T>(N/3, 0x1000);
  if (!A1 || !A2 || !B2 || !A3 || !B3 || !C3) {
    std::cerr << "Error: Allocation failed. Aborting." << std::endl;
    abort();
  }

  OMP(parallel for schedule(static) shared(A1))
  for (long long i = 0; i < N; ++i) {
    A1[i] = 0;
  }
  OMP(parallel for schedule(static) shared(A2,B2))
  for (long long i = 0; i < N/2; ++i) {
    A2[i] = 0;
    B2[i] = 0;
  }
  OMP(parallel for schedule(static) shared(A3,B3,C3))
  for (long long i = 0; i < N/3; ++i) {
    A3[i] = 0;
    B3[i] = 0;
    C3[i] = 0;
  }

  std::cout << "Testing bandwidth with type: " << name<T>() << std::endl;
  std::cout << std::setprecision(3);
  const int min_tries = 4, min_repeat = 1;
  
  while (*sizes != 0) {
    if (*sizes > max_size) continue;
    long long n = *sizes / sizeof(T);
    int repeat = 1;
    int tries = 1;

    double cost = 1e8;
#ifdef _OPENMP
    int k = 0;
#pragma omp parallel 
    {
#pragma omp atomic
      ++k;
    }
    cost *= k;
#endif
    double cost_ratio = cost / static_cast<double>(*sizes);
#ifdef _OPENMP
    repeat = std::sqrt(cost_ratio) / 2.;
#else
    repeat = std::sqrt(cost_ratio) /  5.;
#endif
    double l = std::log2(cost_ratio);
    if (l < 1.) l = 1.;
    if (repeat < 1)          repeat = 1;
    tries = cost_ratio / repeat;
    repeat *= l;
    if (tries  < 1)          tries  = 1;
    if (tries  < min_tries)  tries  = min_tries;
    //if (tries  > max_tries)  tries  = max_tries;
    if (repeat < min_repeat) repeat = min_repeat;
    //if (repeat > max_repeat) repeat = max_repeat;


    std::cout << "  size: "    << std::setw(6) << bytes(*sizes);
    //std::cout << "  repeat: "    << std::setw(4) << repeat;
    //std::cout << "  tries: "    << std::setw(4) << tries;
    std::cout << std::flush;

    double  read_b = max_bandwidth([A1, n, repeat, tries](const bandwidth* b){ return b->read(A1, round(n, b->kern), repeat, tries); });
    std::cout << "  \tread: "  << std::setw(6) << bytes( read_b) << "/s" << std::flush;
    double write_b = max_bandwidth([A1, n, repeat, tries](const bandwidth* b){ return b->write(A1, round(n, b->kern), repeat, tries); });
    std::cout << "  \twrite: " << std::setw(6) << bytes(write_b) << "/s" << std::flush;
    double  copy_b = max_bandwidth([A2, B2, n, repeat, tries](const bandwidth* b){ return b->copy(A2, B2, round(n/2, b->kern), repeat, tries); });
    std::cout << "  \tcopy: "  << std::setw(6) << bytes( copy_b) << "/s" << std::flush;
    double scale_b = max_bandwidth([A2, B2, n, repeat, tries](const bandwidth* b){ return b->scale(A2, B2, round(n/2, b->kern), repeat, tries); });
    std::cout << "  \tscale: " << std::setw(6) << bytes(scale_b) << "/s" << std::flush;
    double   add_b = max_bandwidth([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->add(A3, B3, C3, round(n/3, b->kern), repeat, tries); });
    std::cout << "  \tadd: "   << std::setw(6) << bytes(  add_b) << "/s" << std::flush;
    double triad_b = max_bandwidth([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->triad(A3, B3, C3, round(n/3, b->kern), repeat, tries); });
    std::cout << "  \ttriad: " << std::setw(6) << bytes(triad_b) << "/s" << std::flush;

    std::cout << std::endl;

    ++sizes;
  }

  deallocate(A1);
  deallocate(A2);
  deallocate(B2);
  deallocate(A3);
  deallocate(B3);
  deallocate(C3);
}

int main() {
#ifdef _OPENMP
#pragma omp parallel 
  {
#pragma omp master
    std::cerr << omp_get_num_threads() << " threads required";
  }
  int k = 0;
#pragma omp parallel 
  {
#pragma omp atomic
    ++k;
  }
  std::cerr << "\t" << k << " active threads";
  std::cerr << std::endl;
#else
  std::cerr << "OPENMP disabled\n";
  std::cerr << "1 thread required\t1 active thread" << std::endl;
#endif
  static long long sizes[] = {
    0x0001000, //   4 KB
    0x0001800, //   6 KB
    0x0002000, //   8 KB
    0x0003000, //  12 KB
    0x0004000, //  16 KB
    0x0006000, //  24 KB
    0x0008000, //  32 KB // L1
    0x000C000, //  48 KB
    0x0010000, //  64 KB
    0x0018000, //  96 KB
    0x0020000, // 128 KB
    0x0030000, // 192 KB
    0x0040000, // 256 KB // L2
    0x0060000, // 384 KB
    0x0080000, // 512 KB
    0x00C0000, // 768 KB
    0x0100000, //   1 MB
    0x0180000, // 1.5 MB
    0x0200000, //   2 MB
    0x0300000, //   3 MB
    0x0400000, //   4 MB // L3
    0x0600000, //   6 MB
    0x0800000, //   8 MB
    0x0C00000, //  12 MB
    0x1000000, //  16 MB
    0x1800000, //  24 MB
    0x2000000, //  32 MB
    0x3000000, //  48 MB
    0x4000000, //  64 MB
    0x6000000, //  96 MB
    0x8000000, // 128 MB
    //0xC000000, // 192 MB
    //0x10000000, // 256 MB
    //0x18000000, // 384 MB
    //0x20000000, // 512 MB
    0
  };
  long long max = 0;
  const long long *size = sizes;
  while (*size) {
    max = (max < *size) ? *size : max;
    ++size;
  }

  test<float >(max, sizes);
  test<double>(max, sizes);

  return 0;
}
