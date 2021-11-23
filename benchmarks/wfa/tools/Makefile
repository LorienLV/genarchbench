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

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	# VTUNE_HOME # Path to VTUNE-HOME directory
	VTUNE_INCLUDES+=-I$(VTUNE_HOME)/include
	VTUNE_LDFLAGS+=-L$(VTUNE_HOME)/lib64 -littnotify -lpthread -ldl
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

###############################################################################
# Rules
###############################################################################
OBJS=$(FOLDER_BUILD_PATH)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: $(FOLDER_BUILD_PATH)/*.o align_benchmark.c
	$(CC) $(FLAGS) -I$(FOLDER_ROOT) $(VTUNE_INCLUDES) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) align_benchmark.c $(OBJS) -o $(FOLDER_BIN_PATH)/align_benchmark $(LIBS) $(VTUNE_LDFLAGS)
	
generate_dataset: generate_dataset.c
	$(CC) $(CC_FLAGS) generate_dataset.c -o $(FOLDER_BIN_PATH)/generate_dataset $(LD_FLAGS)
