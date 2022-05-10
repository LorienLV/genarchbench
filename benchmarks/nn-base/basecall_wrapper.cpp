#include <string>
#include <vector>
#include <iostream>

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

int main(int argc, char const *argv[]) {
    std::vector<std::string> args;
    args.assign(argv + 1, argv + argc);
    std::string command;
    for (const auto& arg : args) {
        command += "\"" + arg + "\" ";
    }
    command = "python3 " + command;

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

    system(command.c_str());

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
    return EXIT_SUCCESS;
}
