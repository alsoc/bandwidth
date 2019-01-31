
ARCH_FLAGS= -march=native


$(shell mkdir -p obj)

bandwidth$(SUFFIX): obj/allocation$(SUFFIX).o obj/bandwidth$(SUFFIX).o obj/main$(SUFFIX).o obj/timer$(SUFFIX).o
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) obj/allocation$(SUFFIX).o obj/bandwidth$(SUFFIX).o obj/main$(SUFFIX).o obj/timer$(SUFFIX).o -o bandwidth$(SUFFIX)

obj/allocation$(SUFFIX).o: src/allocation.cpp include/allocation.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/allocation.cpp -o obj/allocation$(SUFFIX).o
obj/bandwidth$(SUFFIX).o: src/bandwidth.cpp include/bandwidth.h include/stream.h include/omp-helper.h include/simd.h include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/bandwidth.cpp -o obj/bandwidth$(SUFFIX).o
obj/main$(SUFFIX).o: src/main.cpp include/bandwidth.h include/allocation.h include/omp-helper.h include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/main.cpp -o obj/main$(SUFFIX).o
obj/timer$(SUFFIX).o: src/timer.cpp include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/timer.cpp -o obj/timer$(SUFFIX).o

clean:
	rm -rf obj/allocation$(SUFFIX).o obj/bandwidth$(SUFFIX).o obj/main$(SUFFIX).o obj/timer$(SUFFIX).o

.PHONY: clean
