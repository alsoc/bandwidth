#ifndef TIMER_H
#define TIMER_H

#include "types.h"

struct Timer {
  public:
    using counter_t = unsigned long long int;
    using diff_t = signed long long int;
  public:
    static void reset(void) noexcept {}
    static counter_t read(void) noexcept;
  private:
    static diff_t overhead;
  public:
    static float64_t frequency;
    static bool low_overhead;
    static diff_t diff(counter_t t0, counter_t t1) noexcept {
      diff_t d = (diff_t) t1 - (diff_t) t0 - overhead;
      return (d <= 0) ? 1 : d;
    }
};



#endif // TIMER_H
