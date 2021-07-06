###############################################################################
# Flags & Folders
###############################################################################
FOLDER_BIN=bin
FOLDER_BUILD=build

CC=gcc
CPP=g++
LD_FLAGS=-lm -lrt
CC_FLAGS=-Wall -g

###############################################################################
# Compile rules
###############################################################################
SUBDIRS=benchmark \
        bitpal \
        edit \
        system \
        utils

all: CC_FLAGS+=-O3
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
	$(MAKE) --directory=$@ all
	
tools:
	$(MAKE) --directory=$@ $(MODE)

.PHONY: $(SUBDIRS) tools

