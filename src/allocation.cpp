#define _POSIX_C_SOURCE 200112L
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstdlib>
#include <iostream>
#include "allocation.h"


void* allocate(unsigned long long int n, unsigned long long int alignment) {
  void* ptr = nullptr;
  if (alignment < sizeof(void*)) alignment = sizeof(void*);
  if (posix_memalign(&ptr, alignment, n) != 0) {
    std::cerr << "Error: Failed to allocate " << n << " bytes (alignment: " << alignment << ")" << std::endl;
    return nullptr;
  }
  return ptr;
}
void deallocate(void* ptr) {
  free(ptr);
}
