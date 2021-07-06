###############################################################################
# Definitions
###############################################################################
FOLDER_BUILD=../build

CC=gcc
CC_FLAGS=-Wall -O3 -march=native -Wno-unused-variable -Wno-unused-but-set-variable -Wno-char-subscripts

###############################################################################
# Modules
###############################################################################
MODULES=bitpal.m0.x1.g1 \
        bitpal.m1.x4.g2
        
SRCS=$(addsuffix .c, $(MODULES))
OBJS=$(addprefix $(FOLDER_BUILD)/, $(SRCS:.c=.o))

###############################################################################
# Rules
###############################################################################
all: $(OBJS)

# General building rule
$(FOLDER_BUILD)/%.o : %.c
	$(CC) $(CC_FLAGS) -c $< -o $@