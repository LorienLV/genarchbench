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
	INCLUDES+=$(VTUNE_INCLUDES)
	LIBS+=$(VTUNE_LDFLAGS)
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

# Dynamorio
DYNAMORIO_ANALYSIS=0

ifeq ($(DYNAMORIO_ANALYSIS),1)
	INCLUDES+=$(DYNAMORIO_INCLUDES)
	LIBS+=$(DYNAMORIO_LDFLAGS)
endif

# PWR
PWR=0

ifeq ($(PWR),1)
	INCLUDES+=$(PWR_INCLUDES)
	LIBS+=$(PWR_LDFLAGS)
endif

###############################################################################
# Rules
###############################################################################
OBJS=../$(FOLDER_BUILD)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: ../$(FOLDER_BUILD)/*.o align_benchmark.c
	$(CC) -I$(FOLDER_ROOT) $(INCLUDES) -DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS) -DDYNAMORIO_ANALYSIS=$(DYNAMORIO_ANALYSIS) -DPWR=$(PWR) $(FLAGS) -fopenmp align_benchmark.c $(OBJS) -o ../$(FOLDER_BIN)/align_benchmark $(LIBS)
	
generate_datasets: generate_datasets.c
	$(CC) $(CC_FLAGS) generate_datasets.c -o ../$(FOLDER_BIN)/generate_datasets $(LD_FLAGS)
