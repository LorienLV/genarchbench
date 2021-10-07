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
# Rules
###############################################################################
OBJS=$(FOLDER_BUILD_PATH)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_benchmark: $(FOLDER_BUILD_PATH)/*.o align_benchmark.c
	$(CC) $(FLAGS) -I$(FOLDER_ROOT) align_benchmark.c $(OBJS) -o $(FOLDER_BIN_PATH)/align_benchmark $(LIBS)
	
generate_dataset: generate_dataset.c
	$(CC) $(CC_FLAGS) generate_dataset.c -o $(FOLDER_BIN_PATH)/generate_dataset $(LD_FLAGS)
