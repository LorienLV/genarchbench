# Set compiler
CXX:=g++

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

# Set flags.
CXXFLAGS:=-Wall -O2 -fopenmp -g $(ARCH_FLAGS)
DEPFLAGS:=-MD -MP # Autogenerate dependencies.
LDFLAGS:=-lcurl -lcrypto -lz -lm -lbz2 -llzma -lpthread -ldl -fopenmp
# INCLUDES=

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

CPPFLAGS+=-DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) -DDYNAMORIO_ANALYSIS=$(DYNAMORIO_ANALYSIS) -DPWR=$(PWR) -DRAPL_STOPWATCH=$(RAPL_STOPWATCH)

# Directories.
BUILD_DIR:=build
SRC_DIR:=src
OBJ_DIR:=$(BUILD_DIR)/obj

# Htslib
HTS_LIB_BUILD_DIR:=$(BUILD_DIR)/libhts
HTS_LIB:=$(HTS_LIB_BUILD_DIR)/lib/libhts.a
INCLUDES+=-I$(HTS_LIB_BUILD_DIR)/include
# If the library is appended instead of prepended there is
# an undefined reference error.
LDFLAGS:=$(HTS_LIB) $(LDFLAGS)

#
# Htslib
#
$(HTS_LIB):
	mkdir -p $(@D)
	cp htslib.tar.bz2 $(HTS_LIB_BUILD_DIR)
	tar -xf $(HTS_LIB_BUILD_DIR)/htslib.tar.bz2 -C $(HTS_LIB_BUILD_DIR)
	rm -f $(HTS_LIB_BUILD_DIR)/htslib.tar.bz2
	cd $(HTS_LIB_BUILD_DIR)/htslib && \
	CC='$(CC) $(ARCH_FLAGS)' CXX='$(CXX) $(ARCH_FLAGS)' ./configure --prefix=`pwd`/../ --enable-bz2=no --enable-lzma=no --with-libdeflate=no --enable-libcurl=no  --enable-gcs=no --enable-s3=no && \
	$(MAKE) && \
	$(MAKE) install

# Files.
SRCS:=$(shell find $(SRC_DIR) -name "*.cpp")
OBJS:=$(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:%.cpp=%.o))
DEPS:=$(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:%.cpp=%.d))

#
# Executables.
#
DBG:=$(BUILD_DIR)/dbg
DBG_OBJS:=debruijn.o common.o
DBG_OBJS:=$(addprefix $(OBJ_DIR)/, $(DBG_OBJS))

#
# Generate executables
#
$(DBG): $(HTS_LIB) $(DBG_OBJS)
	$(CXX) $(CXXFLAGS) $(DBG_OBJS) $(LDFLAGS) -o $@

#
# Generate objects.
#
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(INCLUDES) $(DEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Include dependencies.
-include $(DEPS)

# The default goal is the next goal.
.DEFAULT_GOAL:=
.PHONY: all
all: dbg

dbg: $(DBG)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*
