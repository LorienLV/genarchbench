###############################################################################
# Flags & Folders
###############################################################################
CC=gcc
CPP=g++
LD_FLAGS=-lm -lrt
CC_FLAGS=-Wall # -g

FOLDER_BIN=bin
FOLDER_BUILD=build

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

###############################################################################
# Compile rules
###############################################################################
SUBDIRS=benchmark \
        bitpal \
        edit \
        system \
        utils

all: CC_FLAGS+=-O3 $(ARCH_FLAGS)
all: MODE=all
all: setup
all: $(SUBDIRS) tools

debug: setup
debug: MODE=all
debug: $(SUBDIRS) tools

setup: 
	@mkdir -p $(FOLDER_BIN) $(FOLDER_BUILD)

clean:
	rm -rf $(FOLDER_BIN) $(FOLDER_BUILD)
	
###############################################################################
# Subdir rule
###############################################################################
export
$(SUBDIRS):
	make --directory=$@ all
	
tools:
	make --directory=$@ $(MODE)

.PHONY: $(SUBDIRS) tools

