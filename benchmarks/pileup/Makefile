# Set compiler
CC:=gcc
CXX:=g++

# Set flags.
CFLAGS:=-g -Wall -pthread -fPIC -std=c99 -O3 -fopenmp
CPPFLAGS:=-D_FORTIFY_SOURCE=2 -DPRINT_OUTPUT=1
DEPFLAGS:=-MD -MP # Autogenerate dependencies.
LDFLAGS:=-lz -lm -lbz2 -llzma -lpthread -lcurl -lcrypto -ldl
# INCLUDES=

# ifneq ($(VTUNE_HOME),)
# CFLAGS += -DVTUNE_ANALYSIS=1
# LDFLAGS += -L${VTUNE_HOME}/lib64 -littnotify
# INCLUDES += -I${VTUNE_HOME}/include
# endif

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
	CC='$(CC)' CXX='$(CXX)' ./configure --prefix=`pwd`/../ --enable-bz2=no --enable-lzma=no --with-libdeflate=no --enable-libcurl=no  --enable-gcs=no --enable-s3=no && \
	$(MAKE) && \
	$(MAKE) install

# Files.
SRCS:=$(shell find $(SRC_DIR) -name "*.c")
OBJS:=$(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:%.c=%.o))
DEPS:=$(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:%.c=%.d))

#
# Executables.
#
PILEUP:=$(BUILD_DIR)/pileup
PILEUP_OBJS:=medaka_bamiter.o medaka_common.o medaka_counts.o
PILEUP_OBJS:=$(addprefix $(OBJ_DIR)/, $(PILEUP_OBJS))

#
# Generate executables
#
$(PILEUP): $(HTS_LIB) $(PILEUP_OBJS)
	$(CC) $(CFLAGS) $(PILEUP_OBJS) $(LDFLAGS) -o $@

#
# Generate objects.
#
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Include dependencies.
-include $(DEPS)

# The default goal is the next goal.
.DEFAULT_GOAL:=
.PHONY: all
all: PILEUP

PILEUP: $(PILEUP)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*
