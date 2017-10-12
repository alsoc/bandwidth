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

unsigned long long round_down(unsigned long long n, int r) {
  return (n/r) * r;
}
unsigned long long round_up(unsigned long long n, int r) {
  return round_down(n-1, r)+r;
}

int get_num_threads() {
#ifdef _OPENMP
  int k = 0;
  OMP(parallel) {
    OMP(atomic) ++k;
  }
#else
  int k = 1;
#endif
  return k;
}

template <class T>
void test(long long max_size, const long long* sizes) {
  long long N = max_size / sizeof(T);

  std::cout << "Testing bandwidth with type: " << name<T>() << std::endl;
  std::cout << std::setprecision(3);
  const int min_tries = 2, min_repeat = 1;

  int k = get_num_threads();
  
  while (*sizes != 0) {
    if (*sizes > max_size) continue;


    long long n = *sizes / sizeof(T) / k;
    int repeat = 1;
    int tries = 1;

    double cost = 2e6;
    double cost_ratio = cost / static_cast<double>(n);
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

    //tries = 50;

    std::cout << "  size: "    << std::setw(6) << bytes(n*k*sizeof(T));
    //std::cout << "  repeat: "    << std::setw(4) << repeat;
    //std::cout << "  tries: "    << std::setw(4) << tries;
    std::cout << std::flush;

    OMP(parallel firstprivate(n, repeat, tries, k)) {
      T *buffer = allocate<T>(n + 0x3000 / sizeof(T), 0x1000);
      if (!buffer) {
        std::cerr << "Error: Allocation failed. Aborting." << std::endl;
        abort();
      }
      for (long long i = 0; i < n + 0x3000 / sizeof(T); ++i) {
        buffer[i] = 0;
      }
      T *A1 = buffer, *A2 = buffer, *A3 = buffer;
      T *B2 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(A2 + (n+1)/2), 0x1000));
      T *B3 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(A3 + (n+2)/3), 0x1000));
      T *C3 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(B3 + (n+2)/3), 0x1000));

      double  read_b = k*max_bandwidth([A1, n, repeat, tries](const bandwidth* b){ return b->read(A1, round_down(n, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \tread: "  << std::setw(6) << bytes( read_b) << "/s" << std::flush;
      }

      double write_b = k*max_bandwidth([A1, n, repeat, tries](const bandwidth* b){ return b->write(A1, round_down(n, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \twrite: " << std::setw(6) << bytes(write_b) << "/s" << std::flush;
      }

      double  copy_b = k*max_bandwidth([A2, B2, n, repeat, tries](const bandwidth* b){ return b->copy(A2, B2, round_down(n/2, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \tcopy: "  << std::setw(6) << bytes( copy_b) << "/s" << std::flush;
      }

      double scale_b = k*max_bandwidth([A2, B2, n, repeat, tries](const bandwidth* b){ return b->scale(A2, B2, round_down(n/2, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \tscale: " << std::setw(6) << bytes(scale_b) << "/s" << std::flush;
      }

      double   add_b = k*max_bandwidth([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->add(A3, B3, C3, round_down(n/3, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \tadd: "   << std::setw(6) << bytes(  add_b) << "/s" << std::flush;
      }

      double triad_b = k*max_bandwidth([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->triad(A3, B3, C3, round_down(n/3, b->kern), repeat, tries); });
      OMP(master) {
        std::cout << "  \ttriad: " << std::setw(6) << bytes(triad_b) << "/s" << std::flush;
      }

      deallocate(buffer);
    }

    std::cout << std::endl;

    ++sizes;
  }
}

int main() {
#ifdef _OPENMP
  std::cerr << omp_get_num_threads() << " threads required";
  std::cerr << "\t" << get_num_threads() << " active threads";
  std::cerr << std::endl;
#else
  std::cerr << "OPENMP disabled\n";
  std::cerr << "1 thread required\t1 active thread" << std::endl;
#endif
  static long long sizes[] = {
    0x00001000ll, //   4 KB
    0x00001800ll, //   6 KB
    0x00002000ll, //   8 KB
    0x00003000ll, //  12 KB
    0x00004000ll, //  16 KB
    0x00006000ll, //  24 KB
    0x00008000ll, //  32 KB // L1
    0x0000C000ll, //  48 KB
    0x00010000ll, //  64 KB
    0x00018000ll, //  96 KB
    0x00020000ll, // 128 KB
    0x00030000ll, // 192 KB
    0x00040000ll, // 256 KB // L2
    0x00060000ll, // 384 KB // L2
    0x00080000ll, // 512 KB // L2
    0x000C0000ll, // 768 KB
    0x00100000ll, //   1 MB
    0x00180000ll, // 1.5 MB
    0x00200000ll, //   2 MB
    0x00300000ll, //   3 MB
    0x00400000ll, //   4 MB // L3
    0x00600000ll, //   6 MB // L3
    0x00800000ll, //   8 MB // L3
    0x00C00000ll, //  12 MB // L3
    0x01000000ll, //  16 MB // L3
    0x01800000ll, //  24 MB
    0x02000000ll, //  32 MB
    0x03000000ll, //  48 MB
    0x04000000ll, //  64 MB
    0x06000000ll, //  96 MB
    0x08000000ll, // 128 MB
    0x0C000000ll, // 192 MB
    0x10000000ll, // 256 MB
    0x18000000ll, // 384 MB
    0x20000000ll, // 512 MB
    0x30000000ll, // 768 MB
    0x40000000ll, //   1 GB
    0x60000000ll, // 1.5 GB
    0x80000000ll, //   2 GB
    0xC0000000ll, //   3 GB
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
