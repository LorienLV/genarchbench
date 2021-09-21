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

FMI=		fmi
#CXX=		icpx
CC= gcc
CXX=		g++
#VTUNE_HOME= /opt/intel/oneapi/vtune/2021.1.1

BWAMEM2_PATH=./bwa-mem2
TARGET_ARCH:=$(shell arch)
ifeq ($(TARGET_ARCH),x86_64)
    BSW_SRC=src/x86_64
else ifeq ($(TARGET_ARCH),aarch64)
    BSW_SRC=src/sve
else
    $(error Architecture $(TARGET_ARCH) not supported)
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

CXXFLAGS=	-std=c++11 -fopenmp $(ARCH_FLAGS) #-mtune=native -march=native
#CPPFLAGS=	-DPRINT_OUTPUT -DENABLE_PREFETCH -DBWA_OTHER_ELE=0
CPPFLAGS=	-DENABLE_PREFETCH -DBWA_OTHER_ELE=0
INCLUDES=	-I$(BWAMEM2_PATH)/src -I$(BWAMEM2_PATH)/ext/safestringlib/include -I$(BWAMEM2_PATH)/$(BSW_SRC) # -DENABLE_PARSEC_HOOKS -I/romol/hooks/include
LIBS=		-L$(BWAMEM2_PATH) -L$(BWAMEM2_PATH)/ext/safestringlib -lsafestring -fopenmp -lz -lbwa -ldl # -L/romol/hooks/lib -lhooks

ifneq ($(VTUNE_HOME),)
CPPFLAGS+=	-DVTUNE_ANALYSIS=1
INCLUDES+=	-I${VTUNE_HOME}/include
LIBS+=-L${VTUNE_HOME}/lib64 -littnotify
endif

.PHONY:all clean depend
.SUFFIXES:.cpp .o

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:$(FMI)

$(FMI):fmi.o $(BWAMEM2_PATH)/libbwa.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

$(BWAMEM2_PATH)/libbwa.a:
	cd $(BWAMEM2_PATH) && \
	make CXX=$(CXX) CC=$(CC) BSW_SRC=$(BSW_SRC) arch=$(arch) all

clean:
	rm -fr *.o $(FMI)
	cd $(BWAMEM2_PATH) && $(MAKE) clean

# DO NOT DELETE

fmi.o: $(BWAMEM2_PATH)/src/FMI_search.h $(BWAMEM2_PATH)/src/bntseq.h $(BWAMEM2_PATH)/src/read_index_ele.h
fmi.o: $(BWAMEM2_PATH)/src/bwa.h $(BWAMEM2_PATH)/src/bwt.h $(BWAMEM2_PATH)/src/utils.h $(BWAMEM2_PATH)/$(BSW_SRC)/macro.h