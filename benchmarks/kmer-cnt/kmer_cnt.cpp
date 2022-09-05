//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmath>
#include <execinfo.h>
#include <fcntl.h>

#include "vertex_index.h"
#include "sequence_container.h"
#include "config.h"
#include "logger.h"
#include "utils.h"
#include "memory_info.h"
#include "time.h"
#include "sys/time.h"

#include <getopt.h>

// #define VTUNE_ANALYSIS 1

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

bool parseArgs(int argc, char** argv, std::string& readsFasta, 
			   std::string& logFile,
			   int& kmerSize, bool& debug, size_t& numThreads, int& minOverlap, 
			   std::string& configPath, int& minReadLength, bool& unevenCov)
{
	auto printUsage = []()
	{
		std::cerr << "Usage: kmer-cnt "
				  << " --reads path --out-asm path --config path [--genome-size size]\n"
				  << "\t\t[--min-read length] [--log path] [--treads num] [--extra-params]\n"
				  << "\t\t[--kmer size] [--meta] [--min-ovlp size] [--debug] [-h]\n\n"
				  << "Required arguments:\n"
				  << "  --reads path\tcomma-separated list of read files\n"
				  << "  --config path\tpath to the config file\n\n"
				  << "Optional arguments:\n"
				  << "  --kmer size\tk-mer size [default = 15] \n"
				  << "  --min-ovlp size\tminimum overlap between reads "
				  << "[default = 5000] \n"
				  << "  --debug \t\tenable debug output "
				  << "[default = false] \n"
				  << "  --log log_file\toutput log to file "
				  << "[default = not set] \n"
				  << "  --threads num_threads\tnumber of parallel threads "
				  << "[default = 1] \n";
	};
	
	int optionIndex = 0;
	static option longOptions[] =
	{
		{"reads", required_argument, 0, 0},
		{"config", required_argument, 0, 0},
		{"min-read", required_argument, 0, 0},
		{"log", required_argument, 0, 0},
		{"threads", required_argument, 0, 0},
		{"kmer", required_argument, 0, 0},
		{"min-ovlp", required_argument, 0, 0},
		{"debug", no_argument, 0, 0},
		{0, 0, 0, 0}
	};

	int opt = 0;
	while ((opt = getopt_long(argc, argv, "h", longOptions, &optionIndex)) != -1)
	{
		switch(opt)
		{
		case 0:
			if (!strcmp(longOptions[optionIndex].name, "kmer"))
				kmerSize = atoi(optarg);
			else if (!strcmp(longOptions[optionIndex].name, "min-read"))
				minReadLength = atoi(optarg);
			else if (!strcmp(longOptions[optionIndex].name, "threads"))
				numThreads = atoi(optarg);
			else if (!strcmp(longOptions[optionIndex].name, "min-ovlp"))
				minOverlap = atoi(optarg);
			else if (!strcmp(longOptions[optionIndex].name, "log"))
				logFile = optarg;
			else if (!strcmp(longOptions[optionIndex].name, "debug"))
				debug = true;
			else if (!strcmp(longOptions[optionIndex].name, "meta"))
				unevenCov = true;
			else if (!strcmp(longOptions[optionIndex].name, "reads"))
				readsFasta = optarg;
			else if (!strcmp(longOptions[optionIndex].name, "config"))
				configPath = optarg;
			break;

		case 'h':
			printUsage();
			exit(0);
		}
	}
	if (readsFasta.empty() ||
		configPath.empty())
	{
		printUsage();
		return false;
	}

	return true;
}
const char *__parsec_roi_begin(const char *s, int *beg, int *end)
{
    const char *colon = strrchr(s, ':');
    if (colon == NULL) {
        *beg = 0; *end = 0x7fffffff;
        return s + strlen(s);
    }
    return NULL;
}

const char *__parsec_roi_end(const char *s, int *beg, int *end)
{
    const char *colon = strrchr(s, ':');
    if (colon == NULL) {
        *beg = 0; *end = 0x7fffffff;
        return s + strlen(s);
    }
    return NULL;
}
int main(int argc, char** argv)
{
	#ifdef NDEBUG
	signal(SIGSEGV, segfaultHandler);
	std::set_terminate(exceptionHandler);
	#endif

	int kmerSize = -1;
	int minReadLength = 0;
	size_t genomeSize = 0;
	int minOverlap = 5000;
	bool debugging = false;
	bool unevenCov = false;
	size_t numThreads = 1;
	std::string readsFasta;
	std::string logFile;
	std::string configPath;

	if (!parseArgs(argc, argv, readsFasta, logFile,
				   kmerSize, debugging, numThreads, minOverlap, configPath, 
				   minReadLength, unevenCov)) return 1;

	Logger::get().setDebugging(debugging);
	if (!logFile.empty()) Logger::get().setOutputFile(logFile);
	Logger::get().debug() << "Build date: " << __DATE__ << " " << __TIME__;
	std::ios::sync_with_stdio(false);

	Logger::get().debug() << "Total RAM: " 
		<< getMemorySize() / 1024 / 1024 / 1024 << " Gb";
	Logger::get().debug() << "Available RAM: " 
		<< getFreeMemorySize() / 1024 / 1024 / 1024 << " Gb";
	Logger::get().debug() << "Total CPUs: " << std::thread::hardware_concurrency();

	Config::load(configPath);

	if (kmerSize == -1)
	{
		kmerSize = Config::get("kmer_size");
	}
	Parameters::get().numThreads = numThreads;
	Parameters::get().kmerSize = kmerSize;
	Parameters::get().minimumOverlap = minOverlap;
	Parameters::get().unevenCoverage = unevenCov;
	Logger::get().debug() << "Running with k-mer size: " << 
		Parameters::get().kmerSize; 
	// Logger::get().debug() << "Running with minimum overlap " << minOverlap;
	// Logger::get().debug() << "Metagenome mode: " << "NY"[unevenCov];

	//TODO: unify minimumOverlap ad safeOverlap concepts
	Parameters::get().minimumOverlap = 1000;

	SequenceContainer readsContainer;
	std::vector<std::string> readsList = splitString(readsFasta, ',');
	Logger::get().info() << "Reading sequences";
	try
	{
		//only use reads that are longer than minOverlap,
		//or a specified threshold (used for downsampling)
		minReadLength = std::max(minReadLength, minOverlap);
		for (auto& readsFile : readsList)
		{
			readsContainer.loadFromFile(readsFile, minReadLength);
		}
	}
	catch (SequenceContainer::ParseException& e)
	{
		Logger::get().error() << e.what();
		return 1;
	}
	readsContainer.buildPositionIndex();
	VertexIndex vertexIndex(readsContainer, 
							(int)Config::get("assemble_kmer_sample"));
	vertexIndex.outputProgress(false);

	/*int64_t sumLength = 0;
	for (auto& seq : readsContainer.iterSeqs())
	{
		sumLength += seq.sequence.length();
	}
	int coverage = sumLength / 2 / genomeSize;
	Logger::get().debug() << "Expected read coverage: " << coverage;*/

	// const int MIN_FREQ = 2;
	// static const float SELECT_RATE = Config::get("meta_read_top_kmer_rate");
	// static const int TANDEM_FREQ = Config::get("meta_read_filter_kmer_freq");

	//Building index
	struct timeval start_time, end_time;
	double runtime = 0;
	gettimeofday(&start_time, NULL);
	const char *roi_q;
	int roi_i, roi_j;
	char roi_s[20] = "chr22:0-5";
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
    fapp_start("computing", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
    __DR_START_TRACE();
#endif
	roi_q = __parsec_roi_begin(roi_s, &roi_i, &roi_j);
	bool useMinimizers = Config::get("use_minimizers");
	if (useMinimizers)
	{
		const int minWnd = Config::get("minimizer_window");
		vertexIndex.buildIndexMinimizers(/*min freq*/ 1, minWnd);
	}
	else	//indexing using solid k-mers
	{
		vertexIndex.countKmers();
		// vertexIndex.buildIndexUnevenCoverage(MIN_FREQ, SELECT_RATE, 
		//									 TANDEM_FREQ);
	}
	roi_q = __parsec_roi_end(roi_s, &roi_i, &roi_j);
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
    fapp_stop("computing", 1, 0);
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
	gettimeofday(&end_time, NULL);
	runtime += (end_time.tv_sec - start_time.tv_sec)*1e6 + end_time.tv_usec - start_time.tv_usec;
	Logger::get().debug() << "Peak RAM usage: " 
		<< getPeakRSS() / 1024 / 1024 / 1024 << " Gb";
	fprintf(stderr, "Kernel time: %.3f sec\n", runtime * 1e-6);
	return 0;
}
