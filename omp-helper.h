#ifndef OMP_HELPER_H
#define OMP_HELPER_H

#ifdef _OPENMP
#define __STR(x) #x
#define STR(x) __STR(x)
#define OMP(x) _Pragma(STR(omp x))
#else // _OPENMP
#define OMP(x)
#endif // _OPENMP

#endif // OMP_HELPER_H
