CXX = g++
CXXFLAGS = -O3
TARGETS = eigen spmv spmv2 spmv3 spmv4 spmv5

all: $(TARGETS)

$(TARGETS): %: %.cpp spmv_float.S spmv.hpp
	$(CXX) -fopenmp $(CXXFLAGS) -o $@ $< spmv_float.S

.PHONY: clean
clean:
	rm -f $(TARGETS)
