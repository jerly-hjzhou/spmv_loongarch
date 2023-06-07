CXX = g++
CXXFLAGS = -O3 -Iinclude 
SRCDIR = src
TARGETS = eigen spmv spmv2 spmv3 spmv4 spmv5

all: $(TARGETS)

$(TARGETS): %: $(SRCDIR)/%.cpp spmv_float.S 
	$(CXX) -fopenmp $(CXXFLAGS) -o $@ $< spmv_float.S

.PHONY: clean
clean:
	rm -f $(TARGETS)
