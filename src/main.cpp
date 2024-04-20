#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include "allocation.h"
#include "bandwidth.h"
#include "omp-helper.h"
#include "types.h"

#define OPTPARSE_API static
#define OPTPARSE_IMPLEMENTATION
#include "optparse/optparse.h"

#ifdef _OPENMP
#include "omp.h"
#endif

/* GLOBALS */
bool bytes_power_1024 = false;
bool verbose = false;
bool CSV = false;
bool first = true;
#if !defined(__SSE2__)
bool temporal = true;
#else
bool temporal = false;
#endif

template <class T>
constexpr const char* name() noexcept {
  return __PRETTY_FUNCTION__;
}
#ifdef F16
template <> constexpr const char* name<float16_t>() noexcept { return "f16"; }
#endif
template <> constexpr const char* name<float32_t>() noexcept { return "f32"; }
template <> constexpr const char* name<float64_t>() noexcept { return "f64"; }

struct bytes {
  float64_t n = 0;
  bytes() = default;
  bytes(float64_t n) : n(n) {}
  bytes(const char* s) {
    char* p = nullptr;
    n = std::strtod(s, &p);
    if (!p) return;
    while (*p == ' ') ++p;
    float64_t power = 1000.;
    if (*p != 0 && p[1] == 'i') power = 1024.;
    switch (*p) {
      case 'f':
        n /= power;
        [[fallthrough]];
      case 'p':
        n /= power;
        [[fallthrough]];
      case 'n':
        n /= power;
        [[fallthrough]];
      case 'u':
        n /= power;
        [[fallthrough]];
      case 'm':
        n /= power;
        break;
      case 'E':
        n *= power;
        [[fallthrough]];
      case 'P':
        n *= power;
        [[fallthrough]];
      case 'T':
        n *= power;
        [[fallthrough]];
      case 'G':
        n *= power;
        [[fallthrough]];
      case 'M':
        n *= power;
        [[fallthrough]];
      case 'k':
      case 'K':
        n *= power;
    }
  }
  friend std::ostream& operator<<(std::ostream& out, bytes B) {
    float64_t b = B.n;
    static const char letters[] = "\00fpnum KMGTPE\00";
    const char* letter = letters+6;
    float64_t power = bytes_power_1024 ? 1024. : 1000.;
    while (letter[0] && letter[-1] && std::abs(b) < 1.) {
      b *= power;
      --letter;
    }
    while (letter[0] && letter[+1] && std::abs(b) > 999.) {
      b /= power;
      ++letter;
    }
    if (std::abs(b) < 1e-4) {
      out << "0 ";
      if (bytes_power_1024) out << ' ';
      out << 'B';
      return out;
    }
    out << b << ' ' << *letter;
    if (bytes_power_1024) {
      out << (*letter != ' ' ? 'i' : ' ');
    }
    out << 'B';
    return out;
  }

  operator float64_t() const {
    return n;
  }
};


template <class T>
bool cannot_be_fast(int N) {
#if defined(__AVX512F__) || defined(__KNC__)
  constexpr int width = 512;
  constexpr int regn = 32;
#elif defined(__AVX__)
  constexpr int width = 256;
  constexpr int regn = 16;
#else
  constexpr int width = 128;
#if defined(__aarch64__)
  constexpr int regn = 32;
#else
  constexpr int regn = 16;
#endif
#endif
  constexpr int card = width / (8 * sizeof(T));

  return N != 1 && (N < card || N > card*regn);
}

template <class T, class F>
float64_t max_bandwidth(F&& f) {
  float64_t max_bandwidth = -1./0.;
  for (const bandwidth* b = bandwidth_benches; b->kern != 0; ++b) {
    if (cannot_be_fast<T>(b->kern)) continue;
    if (temporal && b->nontemporal) continue;
    float64_t cur_bandwidth = f(b);
    max_bandwidth = std::max(max_bandwidth, cur_bandwidth);
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
void test(const std::vector<long long>& sizes, float64_t cost) {
  if (CSV) {
    if (first) {
      std::cout << "type,size,read,write,copy,incr,scale,add,triad" << std::endl;
      first = false;
    }
  } else {
    std::cout << "Testing bandwidth with type: " << name<T>() << std::endl;
  }
  std::cout << std::setprecision(3);
  const int min_tries = 2, min_repeat = 1;

  int k = get_num_threads();
  
  for (long long size : sizes) {

    long long n = size / sizeof(T) / k;
    int repeat = 1;
    int tries = 1;

    float64_t cost_ratio = cost / static_cast<float64_t>(n);
    repeat = std::sqrt(cost_ratio) / 2.;

    float64_t l = std::log2(cost_ratio);
    if (l < 1.) l = 1.;
    if (repeat < 1)          repeat = 1;
    tries = cost_ratio / repeat;
    repeat *= l;
    if (tries  < 1)          tries  = 1;
    if (tries  < min_tries)  tries  = min_tries;
    //if (tries  > max_tries)  tries  = max_tries;
    if (repeat < min_repeat) repeat = min_repeat;
    //if (repeat > max_repeat) repeat = max_repeat;

    if (CSV) {
      std::cout << name<T>() << ',' << static_cast<float64_t>(n*k*sizeof(T));
    } else {
      std::cout << "  size: "     << std::setw(6) << bytes(n*k*sizeof(T));
      if (verbose) {
        std::cout << "  repeat: " << std::setw(4) << repeat;
        std::cout << "  tries: "  << std::setw(4) << tries;
      }
      std::cout << std::flush;
    }

    OMP(parallel firstprivate(n, repeat, tries, k)) {
      T *buffer = allocate<T>(n + 0x3000 / sizeof(T), 0x1000);
      if (!buffer) {
        std::cerr << "Error: Allocation failed. Aborting." << std::endl;
        abort();
      }
      for (long long i = 0; i < (long long)(n + 0x3000 / sizeof(T)); ++i) {
        buffer[i] = 0;
      }
      T *A1 = buffer, *A2 = buffer, *A3 = buffer;
      T *B2 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(A2 + (n+1)/2), 0x1000));
      T *B3 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(A3 + (n+2)/3), 0x1000));
      T *C3 = reinterpret_cast<T*>(round_up(reinterpret_cast<unsigned long long>(B3 + (n+2)/3), 0x1000));

      float64_t read_b = k*max_bandwidth<T>([A1, n, repeat, tries](const bandwidth* b){ return b->read(A1, round_down(n, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(read_b);
        } else {
          std::cout << "  \tread: "  << std::setw(6) << bytes(read_b) << "/s" << std::flush;
        }
      }

      float64_t write_b = k*max_bandwidth<T>([A1, n, repeat, tries](const bandwidth* b){ return b->write(A1, round_down(n, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(write_b);
        } else {
          std::cout << "  \twrite: " << std::setw(6) << bytes(write_b) << "/s" << std::flush;
        }
      }

      float64_t copy_b = k*max_bandwidth<T>([A2, B2, n, repeat, tries](const bandwidth* b){ return b->copy(A2, B2, round_down(n/2, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(copy_b);
        } else {
          std::cout << "  \tcopy: "  << std::setw(6) << bytes( copy_b) << "/s" << std::flush;
        }
      }

      float64_t incr_b = k*max_bandwidth<T>([A2, n, repeat, tries](const bandwidth* b){ return b->incr(A2, round_down(n/2, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(incr_b);
        } else {
          std::cout << "  \tincr: "  << std::setw(6) << bytes(incr_b) << "/s" << std::flush;
        }
      }

      float64_t scale_b = k*max_bandwidth<T>([A2, B2, n, repeat, tries](const bandwidth* b){ return b->scale(A2, B2, round_down(n/2, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(scale_b);
        } else {
          std::cout << "  \tscale: " << std::setw(6) << bytes(scale_b) << "/s" << std::flush;
        }
      }

      float64_t add_b = k*max_bandwidth<T>([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->add(A3, B3, C3, round_down(n/3, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(add_b);
        } else {
          std::cout << "  \tadd: "   << std::setw(6) << bytes(add_b) << "/s" << std::flush;
        }
      }

      float64_t triad_b = k*max_bandwidth<T>([A3, B3, C3, n, repeat, tries](const bandwidth* b){ return b->triad(A3, B3, C3, round_down(n/3, b->kern), repeat, tries); });
      OMP(master) {
        if (CSV) {
          std::cout << ',' << static_cast<float64_t>(triad_b);
        } else {
          std::cout << "  \ttriad: " << std::setw(6) << bytes(triad_b) << "/s" << std::flush;
        }
      }

      deallocate(buffer);
    }

    std::cout << std::endl;
  }
}

/* CLI DEFAULTS */
float64_t default_cost = 1e6;
long long default_min = bytes("4 KiB");
long long default_max = bytes("512 MiB");
float64_t default_density = 2;

const char* program_name = "bandwidth";
void help(std::ostream& out) {
  bytes_power_1024 = true;
  out << "USAGE: " << program_name << " [options]\n";
  out << "  measure the bandwidth of your memory (both caches and main memory)\n";
  out << "\n";
  out << "  options:\n";
  out << "    -h, --help            Prints this message\n";
  out << "    -v, --verbose         Verbose output\n";
  out << "    -C, --csv             CSV output\n";
  out << "    -m, --min size        sets the minimum buffer size to \"size\" (default: NPROC * " << bytes(default_min) << ")\n";
  out << "    -M, --max size        sets the maximum buffer size to \"size\" (default: " << bytes(default_max) << ")\n";
  out << "    -d, --density d       sets the density of sizes to tests (default: " << default_density << " per octave)\n";
  out << "    -n, --n   n           sets the number of buffer size being tested to \"n\" (default: 1 + density * log2(max / min) )\n";
  out << "    -c, --cost cost       sets the goal cost of the tests: higher means more retries per test (default: " << default_cost << ")\n";
  out << "    -s, --size list       sets the buffer size being tested to a specific list "
                                    "(default: n sizes logarithmically spaced from min to max)\n";
  out << "    -i, --binary-prefix   uses binary prefixes (eg: KiB, MiB) for the output\n";
  out << "    -T, --temporal        does not use any non-temporal store instructions";
  if (temporal) out << " (always ON: non-temporal stores not supported on this architecture)";
  out << "\n";
  out << std::flush;
	return;
}

int main(int argc, char *argv[]) {
  program_name = argv[0];
  int opt, longindex;
  struct optparse options;
  struct optparse_long longopts[] = {
    {"help",          'h', OPTPARSE_NONE},
    {"verbose",       'v', OPTPARSE_NONE},
    {"csv",           'C', OPTPARSE_NONE},
    {"min",           'm', OPTPARSE_REQUIRED},
    {"max",           'M', OPTPARSE_REQUIRED},
    {"density",       'd', OPTPARSE_REQUIRED},
    {"n",             'n', OPTPARSE_REQUIRED},
    {"cost",          'c', OPTPARSE_REQUIRED},
    {"size",          's', OPTPARSE_REQUIRED},
    {"type",          't', OPTPARSE_REQUIRED},
    {"binary-prefix", 'i', OPTPARSE_NONE},
    {"temporal",      'T', OPTPARSE_NONE},
    {0, 0, OPTPARSE_NONE}
  };
  optparse_init(&options, argv);
  int k = get_num_threads();
  options.permute = 0;

  long long min_size = 0, max_size = 0;

  // If a machine has a really lot of cores
  if (8 * min_size > max_size) {
    max_size = 8*min_size;
  }

  long long n = 0;
  float64_t cost = default_cost;
  float64_t density = default_density;
  std::vector<long long> sizes;

  while (options.optind < argc) {
    if ((opt = optparse_long(&options, longopts, &longindex)) != -1) {
      switch (opt) {
        case 'h': // help
          help(std::cout);
          exit(0);
          break;
        case 'v': // verbose
          verbose = true;
          break;
        case 'C': // CSV output
          CSV = true;
          break;
        case 'm': // min
          min_size = bytes(options.optarg);
          break;
        case 'M': // max
          max_size = bytes(options.optarg);
          break;
        case 'd': // number of sizes per octave
          density = std::stod(options.optarg);
          break;
        case 'n': // number of sizes
          n = std::stod(options.optarg);
          break;
        case 'c': // goal cost
          cost = std::stod(options.optarg);
          break;
        case 's': // sizes
          {
            const char *p = options.optarg;
            sizes.clear();
            while (*p) {
              sizes.push_back(bytes(p));
              while (*p && *p != ',') ++p;
              if (*p) ++p;
            }
          }
          break;
        case 't': // types
          std::cerr << "error: -t,--type option not yet implemented\n";
          help(std::cerr);
          exit(1);
          break;
        case 'i': // binary-prefix
          bytes_power_1024 = true;
          break;
        case 'T': // binary-prefix
          temporal = true;
          break;
        case '?':
          std::cerr << "error: unrecognized option\n";
          help(std::cerr);
          exit(1);
          break;
      }
    } else {
      std::cerr << "error: no command line argument expected\n";
      help(std::cerr);
      exit(1);
    }
  }

  if (min_size < 1) {
    min_size = k * default_min;
  }
  if (max_size < 1) {
    max_size = default_max;

    // If a machine has a really lot of cores
    if (8 * k * default_min > max_size) {
      max_size = 8*k*default_min;
    }
    if (max_size < min_size) {
      max_size = min_size;
    }
  }

  if (min_size > max_size) {
    std::cerr << "error: min (" << bytes(min_size) << ") should not be larger than max (" << bytes(max_size) << ")" << std::endl;
    help(std::cerr);
    exit(1);
  }

  if (n < 1) {
    n = 1 + std::ceil(density * std::log2(static_cast<float64_t>(max_size) / static_cast<float64_t>(min_size)));
    if (n < 1) n = 1;
  }

  float64_t granularity = k*bytes("1 KiB");

  if (sizes.size() == 0) {
    sizes.resize(n);
    if (n == 1) {
      sizes.front() = min_size;
    } else {
      float64_t dmin = std::log2(float64_t(min_size)), dmax = std::log2(float64_t(max_size)), rN = 1. / float64_t(n-1);

      sizes.front() = min_size;
      for (int i = 1; i < n-1; ++i) {
        float64_t s = std::exp2(dmin + i * (dmax - dmin) * rN);
        sizes[i] = granularity * std::round(s / granularity);
        sizes[i] = std::min(max_size, sizes[i]);
        sizes[i] = std::max(min_size, sizes[i]);
      }
      sizes.back() = max_size;

      sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    }
  }

  if (verbose) {
#ifdef _OPENMP
    OMP(parallel) {
      OMP(master)
      std::cerr << omp_get_num_threads() << " threads required";
    }
    std::cerr << "\t" << k << " active threads";
    std::cerr << std::endl;
#else
    std::cerr << "OPENMP disabled\n";
    std::cerr << "1 thread required\t1 active thread" << std::endl;
#endif

    std::cerr << "min: " << bytes(min_size) << "\tmax: " << bytes(max_size) << "\tcost: " << cost << "\tn: " << n << " (" << sizes.size() << ")\tgranularity: " << bytes(granularity) << std::endl;
  }

#ifdef F16
  test<float16_t>(sizes, cost);
#endif
  test<float32_t>(sizes, cost);
  test<float64_t>(sizes, cost);


  return 0;
}
