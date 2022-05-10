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

#if PWR
	#include "pwr.h"
#endif
#if VTUNE_ANALYSIS
    #include <ittnotify.h>
#endif
#if FAPP_ANALYSIS
    #include "fj_tool/fapp.h"
#endif
#if DYNAMORIO_ANALYSIS
    #include <dr_api.h>
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
#if VTUNE_ANALYSIS
    __itt_resume();
#endif
#if FAPP_ANALYSIS
    fapp_start("host_chain_kernel", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
    dr_app_setup_and_start();
#endif
    host_chain_kernel(calls, rets, numThreads);
#if DYNAMORIO_ANALYSIS
    dr_app_stop_and_cleanup();
#endif
#if VTUNE_ANALYSIS
    __itt_pause();
#endif
#if FAPP_ANALYSIS
    fapp_stop("host_chain_kernel", 1, 0);
#endif
#if PWR
	// 3. Get electric energy at the end.
	double energy1 = 0.0;
	PWR_ObjAttrGetValue(pwr_obj, PWR_ATTR_MEASURED_ENERGY, &energy1, NULL);
	// 4. Terminate processing of Power API
	PWR_CntxtDestroy(pwr_cntxt);

	fprintf(stderr, "Energy consumption: %0.4lf J\n", energy1 - energy0);
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
