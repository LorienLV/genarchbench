#include <cstdio>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include "omp.h"
#include "host_data_io.h"
#include "host_data.h"
#include "host_kernel.h"

#define PRINT_OUTPUT 1

#if VTUNE_ANALYSIS
    #include <ittnotify.h>
#endif
#if FAPP_ANALYSIS
    #include "fj_tool/fapp.h"
#endif

void help() {
    std::cout <<
        "\n"
        "usage: ./chain [options ...]\n"
        "\n"
        "    options:\n"
        "        -i <input file>\n"
        "            default: NULL\n"
        "            the input anchor set\n"
        "        -o <output file>\n"
        "            default: NULL\n"
        "            the output scores, best predecessor set\n"
        "        -t <int>\n"
        "            default: 1\n"
        "            number of CPU threads\n"
        "        -h \n"
        "            prints the usage\n";
}


int main(int argc, char **argv) {
#if VTUNE_ANALYSIS
    __itt_pause();
#endif
    FILE *in, *out;
    std::string inputFileName, outputFileName;

    int opt, numThreads = 1;
    while ((opt = getopt(argc, argv, ":i:o:t:h")) != -1) {
        switch (opt) {
            case 'i': inputFileName = optarg; break;
            case 'o': outputFileName = optarg; break;
            case 't': numThreads = atoi(optarg); break;
            case 'h': help(); return 0;
            default: help(); return 1;
        }
    }

    if (argc == 1 || argc != optind) {
        help();
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Input file: %s\n", inputFileName.c_str());
    fprintf(stderr, "Output file: %s\n", outputFileName.c_str());

    in = fopen(inputFileName.c_str(), "r");
    out = fopen(outputFileName.c_str(), "w");

    std::vector<call_t> calls;
    std::vector<return_t> rets;

    for (call_t call = read_call(in);
            call.n != ANCHOR_NULL;
            call = read_call(in)) {
        calls.push_back(call);
    }

    rets.resize(calls.size());

#pragma omp parallel num_threads(numThreads)
{
    int tid = omp_get_thread_num();
    if (tid == 0) {
        fprintf(stderr, "Running with threads: %d\n", numThreads);
    }
}

    struct timeval start_time, end_time;
    double runtime = 0;

    gettimeofday(&start_time, NULL);
#if VTUNE_ANALYSIS
    __itt_resume();
#endif
#if FAPP_ANALYSIS
    fapp_start("host_chain_kernel", 1, 0);
#endif
    host_chain_kernel(calls, rets, numThreads);
#if VTUNE_ANALYSIS
    __itt_pause();
#endif
#if FAPP_ANALYSIS
    fapp_stop("host_chain_kernel", 1, 0);
#endif
    gettimeofday(&end_time, NULL);

    runtime += (end_time.tv_sec - start_time.tv_sec) * 1e6 + (end_time.tv_usec - start_time.tv_usec);
    
#if PRINT_OUTPUT
    for (auto it = rets.begin(); it != rets.end(); it++) {
        print_return(out, *it);
    }
#endif

    fprintf(stderr, "Time in kernel: %.2f sec\n", runtime * 1e-6);

    fclose(in);
    fclose(out);

    return 0;
}
