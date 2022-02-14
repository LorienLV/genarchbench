# Linux/OS X Makefile for PLINK 2.00.
#
# Compilation options (leave blank after "=" to disable, put "= 1" to enable):
#   Do not use AVX2 instructions: NO_AVX2
#   Do not use SSE4.2 instructions: NO_SSE42
#   Print clear error message if SSE42/AVX2 needed but missing: CPU_CHECK
#   Do not link to LAPACK: NO_LAPACK
#   Use cblas_f77 instead of cblas: FORCE_CBLAS_F77
#   Use only -O2 optimization for zstd (may be necessary for gcc 4.x): ZSTD_O2
#   Statically link zlib: STATIC_ZLIB
#   Statically link zstd: STATIC_ZSTD
#   Link to MKL with 64-bit indexes (dynamically): DYNAMIC_MKL
#     (this also requires MKLROOT and MKL_IOMP5_DIR to be defined, and
#     LD_LIBRARY_PATH to include the appropriate directories)
#   32-bit binary (also sets STATIC_ZLIB and ZSTD_O2):
#     FORCE_32BIT (warning: you may need to add a zconf.h symlink to make that
#     work)
#   Debug symbols: set DEBUG to -g
# NO_AVX2 =
# NO_SSE42 =
CPU_CHECK = 1
# NO_LAPACK =
BLAS_FLAG = -lblas -lcblas
LAPACK_FLAG = -llapack 
ATLAS_FLAG = -latlas
ZSTD_O2 = 1
# STATIC_ZLIB =
STATIC_ZSTD = 1
# DYNAMIC_MKL =
MKLROOT = #/home/ubuntu/intel/mkl
MKL_IOMP5_DIR = #/home/ubuntu/intel/compilers_and_libraries_2017.2.174/linux/compiler/lib/intel64
FORCE_32BIT =
DEBUG =
CC ?= gcc
CXX ?= g++

BASEFLAGS=-DZSTD_MULTITHREAD
# ***** end configuration *****

BASEFLAGS += ${DEBUG}

include ../Makefile.src

LINKFLAGS=-lm -lpthread ${DEBUG}
ZLIB=
ARCH32=
CPUCHECK_FLAGS = ${DEBUG}

# ARCH flags.
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

BASEFLAGS+=$(ARCH_FLAGS)

CFLAGS=-O2 -std=gnu99
# zstd appears to be seriously targeted at -O3; see 26 Jul 2016 entry at
# cbloom.com/rants.html
ifdef ZSTD_O2
  ZCFLAGS=-O2 -std=gnu99
else
  ZCFLAGS=-O3 -std=gnu99
endif
# this actually needs to be named "CXXFLAGS"
CXXFLAGS += -O2

BLASFLAGS=$(LAPACK_FLAG) $(BLAS_FLAG) $(ATLAS_FLAG)

ifdef STATIC_ZLIB
  BASEFLAGS += -DSTATIC_ZLIB
  LINKFLAGS += -L. ../../zlib-1.2.11/libz.a
else
  LINKFLAGS += -lz
endif

SKIP_STATIC_ZSTD =
ifdef STATIC_ZSTD
  BASEFLAGS += -DSTATIC_ZSTD
else
  ZCSRC2 =
  ZSSRC2 =
  SKIP_STATIC_ZSTD = echo
  OBJ = $(CSRC:.c=.o) $(CCSRC:.cc=.o)
  OBJ2 = $(notdir $(OBJ))
  LINKFLAGS += -lzstd
endif

LIBTOOL=ar
STATIC=rcs
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
  ifdef FORCE_32BIT
    $(error 32-bit OS X builds are not supported)
  endif
  ifdef DYNAMIC_MKL
    $(error MKL is not currently supported on OS X)
  endif
  BLASFLAGS=-framework Accelerate
  CFLAGS += -I/usr/local/include
  CXXFLAGS += -I/usr/local/include -stdlib=libc++
  LINKFLAGS += -L/usr/local/lib
  LIBTOOL=libtool
  STATIC=-static
else
  ifdef DYNAMIC_MKL
    ifdef NO_LAPACK
      $(error DYNAMIC_MKL and NO_LAPACK conflict)
    endif
    ifdef FORCE_32BIT
      $(error DYNAMIC_MKL + FORCE_32BIT not supported)
    endif
    BASEFLAGS += -DDYNAMIC_MKL -DLAPACK_ILP64 -I${MKLROOT}/include
    BLASFLAGS = -L${MKLROOT}/lib/intel64 -L${MKL_IOMP5_DIR} -Wl,--no-as-needed -lmkl_intel_ilp64 -lmkl_intel_thread -lmkl_core -liomp5
    LINKFLAGS += -ldl
  endif
endif

ifdef NO_LAPACK
  BASEFLAGS += -DNOLAPACK
  BLASFLAGS=
endif

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	# VTUNE_HOME # Path to VTUNE-HOME directory
	BASEFLAGS+=-I$(VTUNE_HOME)/include
	LINKFLAGS+=-L$(VTUNE_HOME)/lib64 -littnotify -lpthread -ldl
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

BASEFLAGS+=-DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS)

CFLAGS += ${BASEFLAGS} ${CWARN2} ${CINCLUDE2}
ZCFLAGS += ${BASEFLAGS} ${CWARN2} ${ZSTD_INCLUDE2}
CXXFLAGS += ${BASEFLAGS} ${CXXWARN2} ${CXXINCLUDE2}

ifdef FORCE_32BIT
  CXXFLAGS += -Wno-sign-compare
endif

SFX ?= ""

all: plink2$(SFX) pgen_compress$(SFX)

plink2$(SFX): $(CSRC2) $(ZCSRC2) $(ZSSRC2) $(CCSRC2) ../plink2_cpu.cc
	$(CC) $(CFLAGS) $(CSRC2) -c
	$(SKIP_STATIC_ZSTD) $(CC) $(ZCFLAGS) $(ZCSRC2) $(ZSSRC2) -c
	$(CXX) $(CXXFLAGS) $(CCSRC2) -c
	$(CXX) $(CPUCHECK_FLAGS) ../plink2_cpu.cc -c
	$(CXX) $(OBJ2) plink2_cpu.o $(ARCH32) -o $@ $(BLASFLAGS) $(LINKFLAGS)

pgen_compress$(SFX): $(PGCSRC2)
	$(CXX) $(CXXFLAGS) $(PGCSRC2) -o $@

pgenlib.a: $(PGENLIB_CCSRC2)
	$(CXX) $(CXXFLAGS) $(PGENLIB_CCSRC2) -c
	$(LIBTOOL) $(STATIC) -o $@ $(PGENLIB_OBJ2)

plink2lib.a: $(CSRC2) $(ZCSRC2) $(ZSSRC2) $(PLINK2LIB_CCSRC2)
	$(CC) $(CFLAGS) $(CSRC2) -c
	$(CC) $(ZCFLAGS) $(ZCSRC2) $(ZSSRC2) -c
	$(CXX) $(CXXFLAGS) $(PLINK2LIB_CCSRC2) -c
	$(LIBTOOL) $(STATIC) -o $@ $(PLINK2LIB_OBJ2)

static_pgenlib_test: pgenlib.a ../pgen_compress.cc
	$(CXX) $(CXXFLAGS) -o $@ ../pgen_compress.cc -L. pgenlib.a

static_plink2lib_test: plink2lib.a ../pgen_compress.cc
	$(CXX) $(CXXFLAGS) -o $@ ../pgen_compress.cc -L. plink2lib.a

.PHONY: clean
clean:
	rm -f *.o
	rm -f plink2
	rm -f pgen_compress
	rm -f pgenlib.a
	rm -f plink2lib.a
	rm -f static_pgenlib_test
	rm -f static_plink2lib_test
