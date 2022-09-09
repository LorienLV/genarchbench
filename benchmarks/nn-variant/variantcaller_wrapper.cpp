#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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

const char *prof_pipe = "prof_pipe";

/**
 * C++ wrapper to use FAPP, and the power libraries.
 */

int wait_and_profile() {
    int pipe_fd = open(prof_pipe, O_RDWR);
    if (pipe_fd == -1) {
        fprintf(stderr, "ERROR opening the fifo pipe\n");
        return EXIT_FAILURE;
    }

    char c;

    // Wait until someone begins executing (b is written to the pipe)
    bool keep_reading = true;
    while (keep_reading) {
        read(pipe_fd, &c, 1);
        switch (c) {
            case 'b':
                keep_reading = false;
                break;
            case 'e':
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Character different from 'b' and 'e' read from fifo pipe\n");
                return EXIT_FAILURE;
        }
    }

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
    fapp_start("variant", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
    __DR_START_TRACE();
#endif
    auto time_start = std::chrono::high_resolution_clock::now();

    // Wait until everyone ends (e is written to the pipe)
    keep_reading = true;
    while (keep_reading) {
        read(pipe_fd, &c, 1);
        switch (c) {
            case 'b':
                break;
            case 'e':
                keep_reading = false;
                break;
            default:
                fprintf(stderr, "Character different from 'b' and 'e' read from fifo pipe\n");
                return EXIT_FAILURE;
        }
    }

    auto time_end = std::chrono::high_resolution_clock::now();
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
    fapp_stop("variant", 1, 0);
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

    std::chrono::duration<double> e_time = time_end - time_start;
    std::cerr << "VariantCalling execution time: " << e_time.count() << " s\n";

    close(pipe_fd);

    return EXIT_SUCCESS;
}

int main(int argc, char *const argv[]) {
    // Create a named pipe to communicate with the python program.
    if (mkfifo(prof_pipe, 0666) != 0) {
        fprintf(stderr, "ERROR while creating the fifo pipe in c++ wrapper\n");
        return EXIT_FAILURE;
    }

    int rvalue = EXIT_SUCCESS;
    int cpid = fork();
    if (cpid == 0) { // Child.
        execvp(argv[1], &argv[1]); // Execute callVar.sh.
    }
    else { // Parent.
        // Create a thread to read from the fifo pipe.
        auto profile_thread = std::async(wait_and_profile);

        // Wait until callVar finishes.
        int wstatus;
        waitpid(cpid, &wstatus, 0);
        rvalue = WEXITSTATUS(wstatus);

        // Write to the pipe to release the thread in case callVar has crashed.
        int pipe_fd = open(prof_pipe, O_RDWR);
        if (pipe_fd == -1) {
            fprintf(stderr, "ERROR opening the fifo pipe\n");
            return EXIT_FAILURE;
        }
        write(pipe_fd, "ee", 2);

        rvalue = (rvalue != EXIT_SUCCESS) ? rvalue : profile_thread.get();
    }

    return rvalue;
}
