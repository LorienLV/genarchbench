###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT:=..
FOLDER_BUILD_PATH:=$(FOLDER_ROOT)/$(FOLDER_BUILD)
FOLDER_BIN_PATH:=$(FOLDER_ROOT)/$(FOLDER_BIN)

###############################################################################
# Tools
###############################################################################
TOOLS=generate_dataset align_benchmark
TOOLS_SRC=$(addsuffix .c, $(TOOLS))

###############################################################################
# Profiling
###############################################################################

# Perf
PERF_ANALYSIS=0

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	INCLUDES+=$(VTUNE_INCLUDES)
	LIBS+=$(VTUNE_LDFLAGS)
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

# Dynamorio
DYNAMORIO_ANALYSIS=0

# PWR
PWR=0

ifeq ($(PWR),1)
	INCLUDES+=$(PWR_INCLUDES)
	LIBS+=$(PWR_LDFLAGS)
endif

# RAPL_STOPWATCH
RAPL_STOPWATCH=0

ifeq ($(RAPL_STOPWATCH),1)
	INCLUDES+=$(RAPL_STOPWATCH_INCLUDES)
	LIBS+=$(RAPL_STOPWATCH_LDFLAGS)
endif

###############################################################################
# Rules
###############################################################################
OBJS=$(FOLDER_BUILD_PATH)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: $(FOLDER_BUILD_PATH)/*.o align_benchmark.c
	$(CC) $(FLAGS) -I$(FOLDER_ROOT) $(INCLUDES) -DPERF_ANALYSIS=$(PERF_ANALYSIS) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) -DDYNAMORIO_ANALYSIS=$(DYNAMORIO_ANALYSIS) -DPWR=$(PWR) -DRAPL_STOPWATCH=$(RAPL_STOPWATCH) align_benchmark.c $(OBJS) -o $(FOLDER_BIN_PATH)/align_benchmark $(LIBS)

generate_dataset: generate_dataset.c
	$(CC) $(CC_FLAGS) generate_dataset.c -o $(FOLDER_BIN_PATH)/generate_dataset $(LD_FLAGS)
