#ifndef ALLOCATION_H
#define ALLOCATION_H

void* allocate(unsigned long long int n, unsigned long long int alignment = 1);
void deallocate(void* ptr);

template <class T>
T* allocate(unsigned long long int n, unsigned long long int alignment = 1){
  if (alignment < alignof(T)) alignment = alignof(T);
  return (T*) allocate(n * sizeof(T), alignment);
}
template <class T>
void deallocate(T* ptr) {
  deallocate((void*) ptr);
}

#endif // ALLOCATION_H
