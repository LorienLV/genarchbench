# Set compiler
CXX:=g++

# Set flags.
CXXFLAGS:=-Wall -O2 -fopenmp -g
DEPFLAGS:=-MD -MP # Autogenerate dependencies.
LDFLAGS:=-lcurl -lcrypto -lz -lm -lbz2 -llzma -lpthread -ldl -fopenmp
# INCLUDES=

#VTUNE_HOME= /opt/intel/vtune_profiler
# ifneq ($(VTUNE_HOME),)
# CXXFLAGS+= -DVTUNE_ANALYSIS=1
# INC+=	-I${VTUNE_HOME}/include
# LIBS+=-L${VTUNE_HOME}/lib64 -littnotify
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
