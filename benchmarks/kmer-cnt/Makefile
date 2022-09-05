CXX= g++
#CXX		 = icpc

EXE=kmer-cnt

ifeq ($(arch),sse41)
	ARCH_FLAGS=-msse4.1
else ifeq ($(arch),avx2)
	ifeq ($(CXX), icpc)
		ARCH_FLAGS=-march=core-avx2 #-xCORE-AVX2
	else	
		ARCH_FLAGS=-mavx2
	endif
else ifeq ($(arch),avx512)
	ifeq ($(CXX), icpc)
		ARCH_FLAGS=-xCORE-AVX512
	else	
		ARCH_FLAGS=-mavx512bw
	endif
else ifeq ($(arch),native)
		ARCH_FLAGS=-march=native
else ifneq ($(arch),)
	## To provide a different architecture flag like -march=core-avx2.
		ARCH_FLAGS=$(arch)
endif

LIBS= -lz -lm -lbz2 -llzma -ldl -fopenmp
CXXFLAGS=-O3 -fopenmp -std=c++11 $(ARCH_FLAGS) 
INC = -Ilibcuckoo

# Perf
PERF_ANALYSIS=0

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	INC+=$(VTUNE_INCLUDES)
	LIBS+=$(VTUNE_LDFLAGS)
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

# Dynamorio
DYNAMORIO_ANALYSIS=0

# PWR
PWR=0

ifeq ($(PWR),1)
	INC+=$(PWR_INCLUDES)
	LIBS+=$(PWR_LDFLAGS)
endif

# RAPL_STOPWATCH
RAPL_STOPWATCH=0

ifeq ($(RAPL_STOPWATCH),1)
	INC+=$(RAPL_STOPWATCH_INCLUDES)
	LIBS+=$(RAPL_STOPWATCH_LDFLAGS)
endif

CPPFLAGS+=-DPERF_ANALYSIS=$(PERF_ANALYSIS) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) -DDYNAMORIO_ANALYSIS=$(DYNAMORIO_ANALYSIS) -DPWR=$(PWR) -DRAPL_STOPWATCH=$(RAPL_STOPWATCH)

all: sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp $(INC) $(LIBS) -o $(EXE)

.PHONY: clean

clean: 
	rm -f $(EXE)

sequence_container.cpp: sequence_container.h sequence.h logger.h
sequence.cpp: sequence.h
vertex_index.cpp: vertex_index.h parallel.h memory_info.h logger.h config.h sequence_container.h kmer.h
kmer-cnt.cpp: sequence_container.h sequence.h vertex_index.h memory_info.h logger.h utils.h config.h parallel.h
