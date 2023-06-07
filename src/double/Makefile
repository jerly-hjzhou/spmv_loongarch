CXX = g++
CXXFLAGS = -O3 -Iinclude 
TARGETS = eigen spmv spmv2 spmv3 spmv4 spmv5

all: $(TARGETS)

$(TARGETS): %: %.cpp spmv_double.S 
	$(CXX) -fopenmp $(CXXFLAGS) -o ../../$@ $< spmv_double.S

.PHONY: clean
clean:
	rm -f $(TARGETS)
