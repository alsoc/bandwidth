
ARCH_FLAGS= -mcpu=native


$(shell mkdir -p obj)

bandwidth: obj/allocation.o obj/bandwidth.o obj/main.o obj/timer.o
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) obj/allocation.o obj/bandwidth.o obj/main.o obj/timer.o -o bandwidth

obj/allocation.o: src/allocation.cpp include/allocation.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/allocation.cpp -o obj/allocation.o
obj/bandwidth.o: src/bandwidth.cpp include/bandwidth.h include/stream.h include/omp-helper.h include/simd.h include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/bandwidth.cpp -o obj/bandwidth.o
obj/main.o: src/main.cpp include/bandwidth.h include/allocation.h include/omp-helper.h include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/main.cpp -o obj/main.o
obj/timer.o: src/timer.cpp include/timer.h
	$(CXX) -std=c++11 -O3 -fopenmp $(ARCH_FLAGS) -Iinclude -c src/timer.cpp -o obj/timer.o

clean:
	rm -rf obj/allocation.o obj/bandwidth.o obj/main.o obj/timer.o

.PHONY: clean
