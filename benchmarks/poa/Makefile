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

CXXFLAGS=-O3 $(ARCH_FLAGS) -fopenmp

# Directories.
SRC_DIR=src
BUILD_DIR=build_$(CC)

# Includes
CXXFLAGS+=-I$(BUILD_DIR)/spoa/include

#
# Libraries
#

# Spoa library
SPOA_LIB=$(BUILD_DIR)/spoa/build/lib/libspoa.a
CXXFLAGS+=-I$(BUILD_DIR)/spoa/include/spoa
LDFLAGS+=$(SPOA_LIB)

#VTUNE_HOME= /opt/intel/vtune_profiler

ifneq ($(VTUNE_HOME),)
CXXFLAGS+=-DVTUNE_ANALYSIS=1 -I${VTUNE_HOME}/include
LDFLAGS+=-L${VTUNE_HOME}/lib64 -littnotify
endif

LDFLAGS+=-ldl

# Files.
SRCS:=$(shell find $(SRC_DIR) -name "*.cpp")
OBJS:=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:%.cpp=%.o))
DEPS:=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:%.cpp=%.d))

#
# Executables.
#
MSA_SPOA_OMP=msa_spoa_omp_$(CC)
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
	$(CXX) $(CXXFLAGS) -c $< -o $@

#
# Compile SPOA library
#
$(SPOA_LIB):
	mkdir -p $(@D)
	cp spoa.tar.bz2 $(BUILD_DIR)
	tar -xf $(BUILD_DIR)/spoa.tar.bz2 -C $(BUILD_DIR)
	rm -f $(BUILD_DIR)/spoa.tar.bz2
	cd $(BUILD_DIR)/spoa/build && \
	cmake -DCMAKE_BUILD_TYPE=Release .. && \
	make

all: $(MSA_SPOA_OMP)

.PHONY: clean

clean:
	rm msa_spoa_omp_$(CC)
	rm -rf build_$(CC)
