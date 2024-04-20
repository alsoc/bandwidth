#ifndef TYPES_H
#define TYPES_H

using float32_t = float;
using float64_t = double;

#ifdef __ARM_NEON
#include <arm_neon.h> // float16_t definition
#define F16 1
#define F16_MEM_OPS 1
#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#define F16_ARI_OPS 1
#endif
#endif

#endif // TYPES_H
