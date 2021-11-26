###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT=..

###############################################################################
# Version
###############################################################################
VERSION=$(shell if [ -d $(ROOT_PATH)/.git ]; then git describe --abbrev=4 --dirty --always; else cat $(ROOT_PATH)/VERSION; fi)

###############################################################################
# Tools
###############################################################################
TOOLS=generate_datasets align_benchmark
TOOLS_SRC=$(addsuffix .c, $(TOOLS))
TOOLS_BIN=$(addprefix ../$(FOLDER_BIN)/, $(GEM_TOOLS))

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
OBJS=../$(FOLDER_BUILD)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: ../$(FOLDER_BUILD)/*.o align_benchmark.c
	$(CC) -I$(FOLDER_ROOT) $(VTUNE_INCLUDES) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) $(FLAGS) -fopenmp align_benchmark.c $(OBJS) -o ../$(FOLDER_BIN)/align_benchmark $(LIBS) $(VTUNE_LDFLAGS)
	
generate_datasets: generate_datasets.c
	$(CC) $(CC_FLAGS) generate_datasets.c -o ../$(FOLDER_BIN)/generate_datasets $(LD_FLAGS)
