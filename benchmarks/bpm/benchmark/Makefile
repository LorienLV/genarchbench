###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT:=..

###############################################################################
# Modules
###############################################################################
MODULES=benchmark_bitpal \
        benchmark_edit \
        benchmark_utils
        
SRCS=$(addsuffix .c, $(MODULES))
OBJS=$(addprefix ../$(FOLDER_BUILD)/, $(SRCS:.c=.o))

###############################################################################
# Rules
###############################################################################
all: $(OBJS) 

# General building rule
../$(FOLDER_BUILD)/%.o : %.c
	$(CC) $(CC_FLAGS) -I$(FOLDER_ROOT) -c $< -o $@
	
