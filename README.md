# Bandwidth

## Purpose

`bandwidth` is a benchmark intended to measure the memory bandwidth of 
single-core and multi-core CPUs. It is inspired by the famous
[`stream`](https://www.cs.virginia.edu/stream/) benchmark from the University of 
Virginia.

Compared to [`stream`](https://www.cs.virginia.edu/stream/), `bandwidth` is 
written in C++17 and it takes advantage of C++ meta-programming capabilities.

Main advantages of `bandwidth` over 
[`stream`](https://www.cs.virginia.edu/stream/):
 
- Ability to benchmark buffers of different memory sizes without re-compile the 
  code
- Benchmark different datatypes (single and double precision)
- The code is explicitly vectorized (over SIMD intrinsic calls), thus giving a 
  more realistic memory bandwidth peak

Known limitations:

- The code works "only" for AltiVec, SSE, AVX, AVX-512 or NEON capable CPUs
  * It represents most of the available SIMD CPUs of the market
  * However, for now, **SVE is not supported**
- The code only compiles with the C++ GNU Compiler (`g++`)

## Features

`bandwidth` measures the memory bandwidth over 7 micro-benchmarks:

- read: `x = A[i]`
- write: `A[i] = x`
- copy: `B[i] = A[i]`
- incr: `A[i] = A[i] + 1`
- scale: `B[i] = x * A[i]`
- add: `C[i] = A[i] + B[i]`
- triad: `C[i] = x * A[i] + B[i]`

## Installation and Execution

`bandwidth` depends on [`optparse`](https://github.com/skeeto/optparse) for
the CLI management:
```bash
git submodule update --init --recursive
```

`cmake` is used to generate the `Makefile`:
```bash
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-march=native" -DENABLE_OMP=ON -DENABLE_F16=ON
make -j4
```

Now you can run the code:
```bash
./bin/bandwidth
```

This will produce an output that looks like the following:
```
Testing bandwidth with type: float
  size:     41 KB  	read:   1.44 TB/s  	write:    978 GB/s  	copy:   1.89 TB/s  	scale:   1.89 TB/s  	add:   2.58 TB/s  	triad:   2.58 TB/s
  size:   61.4 KB  	read:   1.64 TB/s  	write:    956 GB/s  	copy:   1.64 TB/s  	scale:   1.64 TB/s  	add:   2.15 TB/s  	triad:   2.15 TB/s
  size:   81.9 KB  	read:   1.54 TB/s  	write:    979 GB/s  	copy:   1.54 TB/s  	scale:   1.54 TB/s  	add:   1.88 TB/s  	triad:   1.88 TB/s
  [...]
  size:    273 MB  	read:    135 GB/s  	write:    269 GB/s  	copy:    206 GB/s  	scale:    152 GB/s  	add:    146 GB/s  	triad:    165 GB/s
  size:    383 MB  	read:    131 GB/s  	write:    195 GB/s  	copy:    177 GB/s  	scale:    146 GB/s  	add:    172 GB/s  	triad:    206 GB/s
  size:    537 MB  	read:    125 GB/s  	write:    200 GB/s  	copy:    169 GB/s  	scale:    196 GB/s  	add:    150 GB/s  	triad:    181 GB/s
Testing bandwidth with type: double
  size:     41 KB  	read:   1.59 TB/s  	write:    964 GB/s  	copy:   1.59 TB/s  	scale:   1.59 TB/s  	add:   1.86 TB/s  	triad:   1.77 TB/s
  size:   61.4 KB  	read:   1.59 TB/s  	write:    937 GB/s  	copy:   1.59 TB/s  	scale:   1.59 TB/s  	add:    2.2 TB/s  	triad:   1.84 TB/s
  size:   81.9 KB  	read:   1.48 TB/s  	write:    918 GB/s  	copy:   1.48 TB/s  	scale:   1.32 TB/s  	add:   1.92 TB/s  	triad:   1.92 TB/s
  [...]
  size:    273 MB  	read:    132 GB/s  	write:    205 GB/s  	copy:    173 GB/s  	scale:    181 GB/s  	add:    161 GB/s  	triad:    173 GB/s
  size:    383 MB  	read:    138 GB/s  	write:    207 GB/s  	copy:    173 GB/s  	scale:    171 GB/s  	add:    155 GB/s  	triad:    157 GB/s
  size:    537 MB  	read:    141 GB/s  	write:    197 GB/s  	copy:    180 GB/s  	scale:    154 GB/s  	add:    152 GB/s  	triad:    152 GB/s
```
In this example, the code has been run on a Apple M1 Pro CPU. Of course, the 
bandwidth will be different on different architectures.

Note that by default the benchmark run on all the cores. If you want to bench 
single-core memory bandwidth you can do:
```
OMP_NUM_THREADS=1 ./bin/bandwidth
```

## Plotting the Results

A Python3 script is given (`plot.py`). It allows to quickly plot the results of 
`bandwidth`. Here is an example of use (from the `build` dir):
```bash
OMP_NUM_THREADS=1 ./bin/bandwidth -M 2GiB -C > bandwidth_output.csv
../plot.py bandwidth_output.csv -o plot.png
../plot.py bandwidth_output.csv -o plot_with_caches.png --L1 131072 --L2 12582912
```

![Example of plot (Single-core M1 Pro CPU).](img/plot.png "Example of plot (Single-core M1 Pro CPU).")
![Example of plot with cache sizes (Single-core M1 Pro CPU).](img/plot_with_caches.png "Example of plot with cache sizes (Single-core M1 Pro CPU).")

## Contributors

This code has been developed at 
[Sorbonne University](https://www.sorbonne-universite.fr) from Paris in the 
[LIP6](https://www.lip6.fr) laboratory.

Here are the people involved in the project:

- Florian LEMAITRE: **main contributor**
- Adrien CASSAGNE: enthusiastic contributor
- Lionel LACASSAGNE: development supervisor
