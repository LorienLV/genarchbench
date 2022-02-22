/*************************************************************************************
                           The MIT License

   BWA-MEM2  (Sequence alignment using Burrows-Wheeler Transform),
   Copyright (C) 2019 Intel Corporation, Heng Li.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

Contacts: Vasimuddin Md <vasimuddin.md@intel.com>; Sanchit Misra <sanchit.misra@intel.com>;
                                Heng Li <hli@jimmy.harvard.edu> 
*****************************************************************************************/

// ----------------------------------
#include "main.h"
#if PERF
#include "perf.h"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "2.0"
#endif


// ----------------------------------
uint64_t proc_freq, tprof[LIM_R][LIM_C], prof[LIM_R];
// ----------------------------------

int usage()
{
    fprintf(stderr, "Usage: bwa-mem2 <command> <arguments>\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  index         create index\n");
    fprintf(stderr, "  mem           alignment\n");
    fprintf(stderr, "  version       print version number\n");
    return 1;
}

int main(int argc, char* argv[])
{
        
    // ---------------------------------    
    uint64_t tim = __rdtsc();
    sleep(1);
    proc_freq = __rdtsc() - tim;

    int ret = -1;
    if (argc < 2) return usage();

    if (strcmp(argv[1], "index") == 0)
    {
         uint64_t tim = __rdtsc();
         ret = bwa_index(argc-1, argv+1);
         fprintf(stderr, "Total time taken: %0.4lf\n", (__rdtsc() - tim)*1.0/proc_freq);
         return ret;
    }
    else if (strcmp(argv[1], "mem") == 0)
    {
        tprof[MEM][0] = __rdtsc();
        kstring_t pg = {0,0,0};
        extern char *bwa_pg;

        fprintf(stderr, "-----------------------------\n");
        fprintf(stderr, "Executing in SVE mode!!\n");
        fprintf(stderr, "-----------------------------\n");

        #if SA_COMPRESSION
        fprintf(stderr, "* SA compression enabled with xfactor: %d\n", 0x1 << SA_COMPX);
        #endif
        
        ksprintf(&pg, "@PG\tID:bwa-mem2\tPN:bwa-mem2\tVN:%s\tCL:%s", PACKAGE_VERSION, argv[0]);

        for (int i = 1; i < argc; ++i) ksprintf(&pg, " %s", argv[i]);
        ksprintf(&pg, "\n");
        bwa_pg = pg.s;

#if PERF
        // cpu cycles, cache misses, stalled cycles backend, DTLB read misses
        int events[PERF_COUNTERS] = {0,-1,-1,-1};//0,3,8,20};
        perf_start_1(events,0);
        perf_start_2(0);
        //perf_start();
        for (int i = 0; i < 48*3; i++) {
            perf_stopwatch_restart(i);
        }
#endif
        ret = main_mem(argc-1, argv+1);
#if PERF
        for (int i = 0; i < 3; i++) {
            perf_stopwatch_print_all_counters(i*48,(i+1)*48);
        }
        perf_stop_1(0);
        perf_stop_2(0);
#endif

        free(bwa_pg);
        
        /** Enable this return to avoid printing of the runtime profiling **/
        //return ret;
    }
    else if (strcmp(argv[1], "version") == 0)
    {
        puts(PACKAGE_VERSION);
        return 0;
    } else {
        fprintf(stderr, "ERROR: unknown command '%s'\n", argv[1]);
        return 1;
    }
        
    fprintf(stderr, "\nImportant parameter settings: \n");
    fprintf(stderr, "\tBATCH_SIZE: %d\n", BATCH_SIZE);
    fprintf(stderr, "\tMAX_SEQ_LEN_REF: %d\n", MAX_SEQ_LEN_REF);
    fprintf(stderr, "\tMAX_SEQ_LEN_QER: %d\n", MAX_SEQ_LEN_QER);
    fprintf(stderr, "\tMAX_SEQ_LEN8: %d\n", MAX_SEQ_LEN8);
    fprintf(stderr, "\tSEEDS_PER_READ: %d\n", SEEDS_PER_READ);
    fprintf(stderr, "\tSIMD_WIDTH8 X: %lu\n", SIMD_WIDTH8);
    fprintf(stderr, "\tSIMD_WIDTH16 X: %lu\n", SIMD_WIDTH16);
    fprintf(stderr, "\tAVG_SEEDS_PER_READ: %d\n", AVG_SEEDS_PER_READ);
    
    return 0;
}
