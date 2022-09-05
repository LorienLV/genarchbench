/* To complile:
   g++ -O3 msa_spoa_omp.cpp -fopenmp -std=c++11 -I include/ -L build/lib/ -lspoa -o msa_spoa
  */

#include <getopt.h>
#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <assert.h>
#include <exception>
#include <getopt.h>
#include <omp.h>
#include <fcntl.h>
#include <unistd.h>

#include "spoa.hpp"
#include "graph.hpp"
#include "alignment_engine.hpp"
// #include <x86intrin.h>

#define CLMUL 8

// #define ENABLE_SORT 1

#if RAPL_STOPWATCH
	#include <rapl_stopwatch.h>
#endif
#if PWR
	#include <pwr.h>
#endif
#if VTUNE_ANALYSIS
    #include <ittnotify.h>
#endif
#if FAPP_ANALYSIS
    #include <fj_tool/fapp.h>
#endif
#if DYNAMORIO_ANALYSIS
    #if defined(__x86_64__) || defined(_M_X64)
        #define __DR_START_TRACE() { asm volatile ("nopw 0x24"); }
        #define __DR_STOP_TRACE() { asm volatile ("nopw 0x42"); }
    #else
        #error invalid TARGET
    #endif
#endif

using namespace std;
using Alignment = std::vector<std::pair<std::int32_t, std::int32_t>>;

// #define DEBUG_FILE_READ
#define PRINT_OUTPUT 1

typedef struct {
    int id;
    vector<string> seqs;
    string consensus_seq;
    uint8_t padding[24];
} Batch;

struct SortBySize
{
    bool operator()( const Batch& lx, const Batch& rx ) const {
        return lx.seqs.size() > rx.seqs.size();
    }
};

struct SortById
{
    bool operator()( const Batch& lx, const Batch& rx ) const {
        return lx.id < rx.id;
    }
};

double get_realtime()
{
	struct timeval tp;
	struct timezone tzp;
	gettimeofday(&tp, &tzp);
	return tp.tv_sec*1e6 + tp.tv_usec;
}

long peakrss(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
#ifdef __linux__
	return r.ru_maxrss * 1024;
#else
	return r.ru_maxrss;
#endif
}

void readFile(ifstream& in_file, vector<Batch>& batches) {
    string seq;
    if (!in_file.eof()) {
        getline(in_file, seq); // header line
    }
    int count = 0;
    while (!in_file.eof()) { // loop across batches
        if (seq[1] == '0') {
            Batch b;
            b.id = count++;
            while (!in_file.eof()) { // loop across sequences in batch
                getline(in_file, seq); // sequence line
                b.seqs.push_back(seq);
                getline(in_file, seq); // header line
                if (seq[1] == '0') {
                    batches.push_back(b);
                    break;
                }
            }
            if (in_file.eof()) {
                batches.push_back(b);
            }
        }
    }

#ifdef DEBUG_FILE_READ    
    for (int i = 0; i < batches.size(); i++) {
        cout << "Batch " << i << endl;
        for (int j = 0; j < batches[i].seqs.size(); j++) {
            cout << batches[i].seqs[j] << endl;
        }
    }
#endif

}


void help() {
    std::cout <<
        "\n"
        "usage: ./poa -s input.fasta -t <num_threads> > cons.fasta\n"
        "\n"
        "    options:\n"
        "        -m <int>\n"
        "            default: 2\n"
        "            score for matching bases\n"
        "        -x <int>\n"
        "            default: 4\n"
        "            penalty for mismatching bases\n"
        "        -o <int(,int)>\n"
        "            default: gap_open1 = 4, gap_open2 = 24\n"
        "            gap opening penalty (must be non-negative)\n"
        "        -e <int(,int)>\n"
        "            default: gap_ext1 = 2, gap_ext2 = 1\n"
        "            gap extension penalty (must be non-negative)\n"
        "        -s <file>\n"
        "            default: seq.fa\n"
        "            the input sequence set\n"
        "        -n <int>\n"
        "            default: 10\n"
        "            number of sequences in each set\n"
        "        -t <int>\n"
        "            default: 1\n"
        "            number of CPU threads\n"
        "        -h \n"
        "            prints the usage\n";
}

int main(int argc, char** argv) {
    string seq_file = "seq.fa";

    std::uint8_t algorithm = 1;
    std::int8_t m = 2;
    std::int8_t x = -4;
    std::int8_t o1 = -4;
    std::int8_t e1 = -2;
    std::int8_t o2 = -24;
    std::int8_t e2 = -1;

    if (argc == 1) {
        fprintf(stderr, "argc = %d\n", argc);
        help();
        exit(EXIT_FAILURE);
    }

    char *s; 
    int n_seqs = 0;
    int numThreads = 1;
    while(1) {
        int opt = getopt(argc, argv, "m:x:o:e:n:s:t:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'm': m = atoi(optarg); break;
            case 'x': x = 0-atoi(optarg); break;
            case 'o': o1 = 0-strtol(optarg, &s, 10); if (*s == ',') o2 = 0-strtol(s+1, &s, 10); break;
            case 'e': e1 = 0-strtol(optarg, &s, 10); if (*s == ',') e2 = 0-strtol(s+1, &s, 10); break;
            case 'n': n_seqs = atoi(optarg); break;
            case 's': seq_file = optarg; break;
            case 't': numThreads = atoi(optarg); break;
            case 'h': help(); return 0;
            default: help(); fprintf(stderr, "%c\n", (char)opt); return 1;
        }
    }

    std::int8_t oe1=o1+e1, oe2=o2+e2;

    std::unique_ptr<spoa::AlignmentEngine> alignment_engine[numThreads];
    for (int i = 0; i < numThreads; i++) {
        try {
            alignment_engine[i] = spoa::createAlignmentEngine(
                    static_cast<spoa::AlignmentType>(algorithm), m, x, oe1, e1, oe2, e2);
        } catch(std::invalid_argument& exception) {
            std::cerr << exception.what() << std::endl;
            return 1;
        }
    }

    ifstream fp_seq;
    fp_seq.open(seq_file, ios::in);
    assert(fp_seq.is_open());

    vector<Batch> batches;

    struct timeval start_time, end_time, t_start, t_end;
    double runtime = 0; int seq_i; string seq;
    double realtime = 0, real_start, real_end;
    double graphCreationTime = 0, alignTime = 0, addToGraphTime = 0, generateConsensusTime = 0;

#pragma omp parallel num_threads(numThreads) 
{
    int tid = omp_get_thread_num();
    if (tid == 0) {
        fprintf(stderr, "Running with threads: %d\n", numThreads);
    }
}

    readFile(fp_seq, batches);
    fprintf(stderr, "Number of batches: %lu, Size of batch struct %d\n", batches.size(), sizeof(Batch));
    // int64_t workTicks[CLMUL * numThreads];
    // std::memset(workTicks, 0, CLMUL * numThreads * sizeof(int64_t));
    gettimeofday(&start_time, NULL); real_start = get_realtime();
#if RAPL_STOPWATCH
	int err = rapl_stopwatch_api_init();
	if (err) {
		fprintf(stderr, "Error initializing the RAPL-stopwatch API\n");
	}

	rapl_stopwatch_t rapl_sw;
	rapl_stopwatch_init(&rapl_sw);

	rapl_stopwatch_play(&rapl_sw);
#endif
#if PWR
	// 1. Initialize Power API
	PWR_Cntxt pwr_cntxt = NULL;
	PWR_CntxtInit(PWR_CNTXT_FX1000, PWR_ROLE_APP, "app", &pwr_cntxt);
	// 2. Get Object (In this step, get an Object that indicates the entire compute node.)
	PWR_Obj pwr_obj = NULL;
	PWR_CntxtGetObjByName(pwr_cntxt, "plat.node", &pwr_obj);
	// 3. Get electric energy at the start.
	double energy0 = 0.0;
	PWR_ObjAttrGetValue(pwr_obj, PWR_ATTR_MEASURED_ENERGY, &energy0, NULL);
#endif
#if PERF_ANALYSIS
	const char *perf_pipe = "perf_ctl.fifo";
    const char *perf_enable = "enable";
    const char *perf_disable = "disable";

    int perf_pipe_fd = open(perf_pipe, O_WRONLY);
    if (perf_pipe_fd == -1) {
        fprintf(stderr, "ERROR opening the Perf pipe\n");
    }
    write(perf_pipe_fd, perf_enable, strlen(perf_enable));
#endif
#if VTUNE_ANALYSIS
    __itt_resume();
#endif
#if FAPP_ANALYSIS
    fapp_start("alignment", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
    __DR_START_TRACE();
#endif

#ifdef ENABLE_SORT
    std::sort(batches.begin(), batches.end(), SortBySize());
#endif

#pragma omp parallel num_threads(numThreads)
{
    int tid = omp_get_thread_num();
    #pragma omp for schedule(dynamic, 1)
        for (int i = 0; i < batches.size(); i++) {
            // int64_t st1 = __rdtsc();
            // gettimeofday(&t_start, NULL);
            auto graph = spoa::createGraph();
            // gettimeofday(&t_end, NULL);
            // graphCreationTime += (t_end.tv_sec - t_start.tv_sec)*1e6 + t_end.tv_usec - t_start.tv_usec;
            for (int j = 0; j < batches[i].seqs.size(); j++) {
                // gettimeofday(&t_start, NULL);
                auto alignment = alignment_engine[tid]->align(batches[i].seqs[j], graph);
                // gettimeofday(&t_end, NULL);
                // alignTime += (t_end.tv_sec - t_start.tv_sec)*1e6 + t_end.tv_usec - t_start.tv_usec;

                // gettimeofday(&t_start, NULL);
                graph->add_alignment(alignment, batches[i].seqs[j]);
                // gettimeofday(&t_end, NULL);
                // addToGraphTime += (t_end.tv_sec - t_start.tv_sec)*1e6 + t_end.tv_usec - t_start.tv_usec;
            }
            // gettimeofday(&t_start, NULL);
            batches[i].consensus_seq = graph->generate_consensus();
            // gettimeofday(&t_end, NULL);
            // generateConsensusTime += (t_end.tv_sec - t_start.tv_sec)*1e6 + t_end.tv_usec - t_start.tv_usec;
            // int64_t et1 = __rdtsc();
            // workTicks[CLMUL * tid] += (et1 - st1);
        }
    // printf("%d] workTicks = %ld\n", tid, workTicks[CLMUL * tid]);

}
#ifdef ENABLE_SORT
    std::sort(batches.begin(), batches.end(), SortById());
#endif

#if DYNAMORIO_ANALYSIS
    __DR_STOP_TRACE();
#endif
#if PERF_ANALYSIS
    write(perf_pipe_fd, perf_disable, strlen(perf_disable));
    close(perf_pipe_fd);
#endif
#if VTUNE_ANALYSIS
    __itt_pause();
#endif
#if FAPP_ANALYSIS
    fapp_stop("alignment", 1, 0);
#endif
#if PWR
	// 3. Get electric energy at the end.
	double energy1 = 0.0;
	PWR_ObjAttrGetValue(pwr_obj, PWR_ATTR_MEASURED_ENERGY, &energy1, NULL);
	// 4. Terminate processing of Power API
	PWR_CntxtDestroy(pwr_cntxt);

	fprintf(stderr, "Energy consumption: %0.4lf J\n", energy1 - energy0);
#endif
#if RAPL_STOPWATCH
	rapl_stopwatch_pause(&rapl_sw);

	uint64_t count = 0;
	err = rapl_stopwatch_get_mj(&rapl_sw, RAPL_NODE, &count);
	if (err) {
		fprintf(stderr, "Error reading the RAPL-stopwatch counter\n");
	}

	fprintf(stderr, "Energy consumption: %0.4lf J\n", (double)count / 1E3);

	rapl_stopwatch_destroy(&rapl_sw);
	rapl_stopwatch_api_destroy();
#endif

    gettimeofday(&end_time, NULL); real_end = get_realtime();
    runtime += (end_time.tv_sec - start_time.tv_sec)*1e6 + end_time.tv_usec - start_time.tv_usec;
    realtime += (real_end-real_start);

    int64_t sumTicks = 0;
    int64_t maxTicks = 0;
    for(int i = 0; i < numThreads; i++)
    {
        // sumTicks += workTicks[CLMUL * i];
        // if(workTicks[CLMUL * i] > maxTicks) maxTicks = workTicks[CLMUL * i];
    }
    double avgTicks = (sumTicks * 1.0) / numThreads;
    // printf("avgTicks = %lf, maxTicks = %ld, load imbalance = %lf\n", avgTicks, maxTicks, maxTicks/avgTicks);
#ifdef PRINT_OUTPUT
    for (int i = 0; i < batches.size(); i++) {
        cout << ">Consensus_sequence" << endl;
        cout << batches[i].consensus_seq.c_str() << endl;
    }
#endif

    fprintf(stderr, "Runtime: %.2f, GraphCreate: %.2f, Align: %.2f, AddSeqGraph: %.2f, Consensus %.2f %.2f %.3f \n", runtime*1e-6, graphCreationTime*1e-6, alignTime*1e-6, addToGraphTime*1e-6, generateConsensusTime*1e-6, realtime*1e-6, peakrss()/1024.0/1024.0);

    fp_seq.close();
    return 0;
}
