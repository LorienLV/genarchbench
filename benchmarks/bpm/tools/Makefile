###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT=..
FOLDER_BUILD=../build_$(CC)
FOLDER_BIN=../bin_$(CC)
FOLDER_RESOURCES=../resources_$(CC)

###############################################################################
# Version
###############################################################################
VERSION=$(shell if [ -d $(ROOT_PATH)/.git ]; then git describe --abbrev=4 --dirty --always; else cat $(ROOT_PATH)/VERSION; fi)

###############################################################################
# Tools
###############################################################################
TOOLS=generate_datasets align_benchmark
TOOLS_SRC=$(addsuffix .c, $(TOOLS))
TOOLS_BIN=$(addprefix $(FOLDER_BIN)/, $(GEM_TOOLS))

###############################################################################
# Rules
###############################################################################
OBJS=$(FOLDER_BUILD)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: $(FOLDER_BUILD)/*.o align_benchmark.c
	$(CC) $(FLAGS) -fopenmp -I$(FOLDER_ROOT) align_benchmark.c $(OBJS) -o $(FOLDER_BIN)/align_benchmark $(LIBS)
	
generate_datasets: generate_datasets.c
	$(CC) $(CC_FLAGS) generate_datasets.c -o $(FOLDER_BIN)/generate_datasets $(LD_FLAGS)
