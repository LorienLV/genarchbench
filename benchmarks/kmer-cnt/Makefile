CXX= g++
#CXX		 = icpc

EXE=kmer-cnt

LIBS= -lz -lm -lbz2 -llzma -ldl -fopenmp
CXXFLAGS=-O3 -fopenmp -std=c++11
INC = -Ilibcuckoo

# Intel VTune Profiler
VTUNE_ANALYSIS=0

ifeq ($(VTUNE_ANALYSIS),1)
	# VTUNE_HOME # Path to VTUNE-HOME directory
	INC+=-I$(VTUNE_HOME)/include
	LIBS+=-L$(VTUNE_HOME)/lib64 -littnotify -lpthread -ldl
endif

# Fujitsu Advanced Profiler
FAPP_ANALYSIS=0

CPPFLAGS+=-DVTUNE_ANALYSIS=$(VTUNE_ANALYSIS) -DFAPP_ANALYSIS=$(FAPP_ANALYSIS)

all: sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp $(INC) $(LIBS) -o $(EXE)

.PHONY: clean

clean: 
	rm -f $(EXE)

sequence_container.cpp: sequence_container.h sequence.h logger.h
sequence.cpp: sequence.h
vertex_index.cpp: vertex_index.h parallel.h memory_info.h logger.h config.h sequence_container.h kmer.h
kmer-cnt.cpp: sequence_container.h sequence.h vertex_index.h memory_info.h logger.h utils.h config.h parallel.h
