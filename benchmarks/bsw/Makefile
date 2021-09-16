##/*************************************************************************************
##                           The MIT License
##
##   BWA-MEM2  (Sequence alignment using Burrows-Wheeler Transform),
##   Copyright (C) 2019  Vasimuddin Md, Sanchit Misra, Intel Corporation, Heng Li.
##
##   Permission is hereby granted, free of charge, to any person obtaining
##   a copy of this software and associated documentation files (the
##   "Software"), to deal in the Software without restriction, including
##   without limitation the rights to use, copy, modify, merge, publish,
##   distribute, sublicense, and/or sell copies of the Software, and to
##   permit persons to whom the Software is furnished to do so, subject to
##   the following conditions:
##
##   The above copyright notice and this permission notice shall be
##   included in all copies or substantial portions of the Software.
##
##   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
##   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
##   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
##   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
##   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
##   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
##   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
##   SOFTWARE.
##
##Contacts: Vasimuddin Md <vasimuddin.md@intel.com>; Sanchit Misra <sanchit.misra@intel.com>;
##                                Heng Li <hli@jimmy.harvard.edu> 
##*****************************************************************************************/

CC=gcc
CXX=g++
EXE=bsw_$(CXX)

BUILDDIR=build_$(CC)
OBJDIR=$(BUILDDIR)/obj

SHELLARCH:=$(shell arch)
ifeq ($(SHELLARCH),x86_64)
    SRCDIR=src/x86_64
else ifeq ($(SHELLARCH),aarch64)
    SRCDIR=src/sve
else
    $(error Architecture $(SHELLARCH) not supported)
endif

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

CXXFLAGS=-DSORT_PAIRS -DENABLE_PREFETCH -DBWA_OTHER_ELE=0 -O3 -std=c++11 -fopenmp $(ARCH_FLAGS)
# VTUNE_HOME= /opt/intel/oneapi/vtune/2021.1.1
INCLUDES=
LIBS=-fopenmp -lz -ldl

ifneq ($(VTUNE_HOME),)
CXXFLAGS+=-DVTUNE_ANALYSIS=1
INCLUDES+=-I${VTUNE_HOME}/include
LIBS+=-L${VTUNE_HOME}/lib64 -littnotify
endif

CXXFLAGS+=-MD -MP # Autogenerate dependencies. MUST NOT BE DELETED.

# Files.
SRCS:=$(shell find $(SRCDIR) -name "*.cpp")
OBJS:=$(subst $(SRCDIR),$(OBJDIR),$(SRCS:%.cpp=%.o))
DEPS:=$(subst $(SRCDIR),$(OBJDIR),$(SRCS:%.cpp=%.d))

#
# Executables.
#
MAIN_BSW=$(BUILDDIR)/main_bsw
MAIN_BSW_OBJS=$(OBJDIR)/main_banded.o \
			  $(OBJDIR)/bandedSWA.o

$(info $(MAIN_BSW_OBJS))

.PHONY: all
all: main_bsw

main_bsw: $(MAIN_BSW)

$(MAIN_BSW): $(MAIN_BSW_OBJS)
	$(CXX) $^ $(LIBS) -o $@

#
# Generate objects.
#
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Include dependencies.
-include $(DEPS)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)