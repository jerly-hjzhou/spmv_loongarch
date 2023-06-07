CXX = g++
CXXFLAGS = -O3 -Iinclude 
SRCDIR = src
TARGETS = eigen spmv spmv2 spmv3 spmv4 spmv5

all: $(TARGETS)

$(TARGETS): %: $(SRCDIR)/%.cpp spmv_double.S 
	$(CXX) -fopenmp $(CXXFLAGS) -o $@ $< spmv_double.S

.PHONY: clean
clean:
	rm -f $(TARGETS)
