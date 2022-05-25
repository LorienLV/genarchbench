#include <string>
#include <vector>
#include <iostream>

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
    #include <dr_api.h>
#endif

/**
 * C++ wrapper to use FAPP, and the power libraries.
 */

int main(int argc, char *const argv[]) {
    int rvalue = EXIT_SUCCESS;
    int cpid = fork();
    if (cpid == 0) { // Child.
        execvp(argv[1], argv); // Execute Clair3.
    }
    else { // Parent.
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
#if VTUNE_ANALYSIS
		__itt_resume();
#endif
#if FAPP_ANALYSIS
		fapp_start("basecall", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
		dr_app_setup_and_start();
#endif

        // Wait until bonito finishes.
        int wstatus;
        waitpid(cpid, &wstatus, 0);
        rvalue = WEXITSTATUS(wstatus);

#if DYNAMORIO_ANALYSIS
		dr_app_stop_and_cleanup();
#endif
#if VTUNE_ANALYSIS
		__itt_pause();
#endif
#if FAPP_ANALYSIS
		fapp_stop("basecall", 1, 0);
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
    }

    return rvalue;
}
