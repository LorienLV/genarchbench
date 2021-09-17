CXX= g++
#CXX		 = icpc

EXE=kmer-cnt

LIBS= -lz -lm -lbz2 -llzma -ldl -fopenmp
CXXFLAGS=-O3 -fopenmp -std=c++11
INC = -Ilibcuckoo

ifneq ($(VTUNE_HOME),)
#VTUNE_HOME= /opt/intel/vtune_profiler
CPPFLAGS+=-DVTUNE_ANALYSIS=1
LIBS += -L${VTUNE_HOME}/lib64 -littnotify
INC += -I${VTUNE_HOME}/include
endif


all: sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) sequence_container.cpp sequence.cpp vertex_index.cpp kmer_cnt.cpp $(INC) $(LIBS) -o $(EXE)

.PHONY: clean

clean: 
	rm -f $(EXE)

sequence_container.cpp: sequence_container.h sequence.h logger.h
sequence.cpp: sequence.h
vertex_index.cpp: vertex_index.h parallel.h memory_info.h logger.h config.h sequence_container.h kmer.h
kmer-cnt.cpp: sequence_container.h sequence.h vertex_index.h memory_info.h logger.h utils.h config.h parallel.h
