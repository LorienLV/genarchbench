# Set compiler.
CC=gcc
CXX=g++
#CXX=icpc

# Set flags.
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

CXXFLAGS=-O3 -fopenmp $(ARCH_FLAGS)

# Directories.
SRC_DIR=src
BUILD_DIR=build

#
# Libraries
#

LDFLAGS+=-fopenmp -ldl

# Spoa library
SPOA_LIB=$(BUILD_DIR)/spoa/build/lib/libspoa.a
INCLUDES+=-I$(BUILD_DIR)/spoa/include/spoa
LDFLAGS+=-L$(BUILD_DIR)/spoa/build/lib/ -lspoa

# Perf
PERF_ANALYSIS=0

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	INCLUDES+=$(VTUNE_INCLUDES)
	LDFLAGS+=$(VTUNE_LDFLAGS)
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

# Dynamorio
DYNAMORIO_ANALYSIS=0

# PWR
PWR=0

ifeq ($(PWR),1)
	INCLUDES+=$(PWR_INCLUDES)
	LDFLAGS+=$(PWR_LDFLAGS)
endif

# RAPL_STOPWATCH
RAPL_STOPWATCH=0

ifeq ($(RAPL_STOPWATCH),1)
	INCLUDES+=$(RAPL_STOPWATCH_INCLUDES)
	LDFLAGS+=$(RAPL_STOPWATCH_LDFLAGS)
endif

CPPFLAGS+=-DPERF_ANALYSIS=$(PERF_ANALYSIS) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) -DDYNAMORIO_ANALYSIS=$(DYNAMORIO_ANALYSIS) -DPWR=$(PWR) -DRAPL_STOPWATCH=$(RAPL_STOPWATCH)

# Files.
SRCS:=$(shell find $(SRC_DIR) -name "*.cpp")
OBJS:=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:%.cpp=%.o))
DEPS:=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:%.cpp=%.d))

#
# Executables.
#
MSA_SPOA_OMP=msa_spoa_omp
MSA_SPOA_OMP_OBJS=$(BUILD_DIR)/msa_spoa_omp.o

#
# Generate executables
#
$(MSA_SPOA_OMP): $(SPOA_LIB) $(MSA_SPOA_OMP_OBJS)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

#
# Generate objects.
#
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

#
# Compile SPOA library
#
$(SPOA_LIB):
	mkdir -p $(@D)
	cp spoa.tar.bz2 $(BUILD_DIR)
	tar -xf $(BUILD_DIR)/spoa.tar.bz2 -C $(BUILD_DIR)
	rm -f $(BUILD_DIR)/spoa.tar.bz2
	cd $(BUILD_DIR)/spoa/build && \
	CC='$(CC)' CXX='$(CXX)' CXXFLAGS='$(ARCH_FLAGS)' \
	cmake -Dspoa_optimize_for_native=OFF -DCMAKE_BUILD_TYPE=Release .. && \
	$(MAKE)

all: $(MSA_SPOA_OMP)

msa_spoa_omp: $(MSA_SPOA_OMP)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(MSA_SPOA_OMP)
