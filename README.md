# GenArchBench

GenArchBench is a Genomics benchmark suite targeting the Arm architecture. It comprises 13 multithreaded CPU kernels from the most widely used genomics tools covering the most important genome sequencing steps. GenArchBench includes 10 kernels from [GenomicsBench](https://github.com/arun-sub/genomicsbench) and three additional kernels: the Bit-Parallel Myers algorithm, the Wavefront Alignment algorithm, and a SIMD accelerated version of minimap2's chaining implementation (FAST-CHAIN). The kernels have been optimized for Arm and tunned in two Arm processors: the A64FX and Graviton3.

## Dataset

GenArchBench includes two inputs for each kernel, one small with a target execution time of less than a minute and one large with a target execution time of a couple of minutes. Additionally, we include the expected output for each benchmark and input. To download the dataset (~90 GB):

```
mkdir genarch-temp
cd genarch-temp
wget https://b2drop.bsc.es/index.php/s/Nyg7TXDRpkL5zTn/download
unzip download
rm -r download
cat inputs*/genarch-inputs.tar.gz* > genarch-inputs-merged.bz
rm -r inputs*
tar -xvjf genarch-inputs-merged.bz
mv genarch-inputs ../
cd ..
rm -rf genarch-temp
```

## Working with the benchmarks

Each benchmark under the [benchmark](benchmarks) folder includes a Makefile for compilation and a README that explains how to execute it and some additional information. Additionally, in the `scripts` folder of each benchmark, you will find a wrapper to compile the benchmark using different compilers (`scripts/compile.sh`) and two scripts to automatically run it using its two inputs with different thread counts and automatic output check (`scripts/regression_small.sh` and `scripts/regression_large.sh`). In order to use the scripts of each benchmark, you must set the environment variables in [setup.sh](benchmarks/setup.sh), and then run the script: `source benchmarks/setup.sh`.

### Compile

Use the Makefile of each benchmark or the wrapper inside the scripts folder of each benchmark (scripts/compile.sh). The wrapper can be used to compile each kernel with different compilers (by default GCC) and show the environment variables that can be set to profile the kernel (see [Profiling](#profiling)).

```
cd benchmarks/X
make
# OR
bash scripts/compile.sh
```

### Run

You can follow the instructions of the README inside each benchmark's folder or use our automatic regression tests. 

The regression tests inside the scripts folder of each benchmark, `scripts/regression_small.sh` and `scripts/regression_large.sh`, run the benchmarks using the small and large input, respectively. Note that you need to set the required environment variables in [setup.sh](benchmarks/setup.sh) to use these scripts. 

By default, the regression tests will run the benchmarks three times, each with a different number of threads: 1, 2, and 4. You can change the thread counts by modifying the script. Before finishing, the obtained output will be compared with the expected output to check the correctness of the execution (the output of DGB is not checked by default, take a look at its regression tests). The execution time taken with each thread count and the status of the correctness check will be reported at the end of the execution. The execution time reported only corresponds to the region of interest of the kernels, not the total execution time. 

The regression tests use [run_wrapper.sh](benchmarks/run_wrapper.sh) to automatically detect the job scheduler of the system and run the benchmarks using it. For now, it only supports SLURM and PJM (the job scheduler of the A64FX), but it should be easy to add any other. If [run_wrapper.sh](benchmarks/run_wrapper.sh) does not detect any job-scheduler, it will run the benchmarks without using any.

Example of full workflow:
```
source benchmarks/setup.sh
cd benchmarks/X
bash scripts/compile.sh
bash scripts/regression_small.sh
bash scripts/regression_large.sh
```

### Profiling

We have annotated the code of all benchmarks to isolate their region of interest when running different profilers and tools, i.e., only the region of interest is taken into account when using such profilers and tools. 

We support the following profilers and tools: [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html), [Perf](https://perf.wiki.kernel.org/index.php/Main_Page), our modified version of [DynamoRIO](https://github.com/LorienLV/dynamorio), the Fujitsu Advanced Performance Profiler, the Fujitsu PWR library, and the [RAPL-Stopwatch](https://github.com/LorienLV/rapl_stopwatch) library.

#### Intel VTune

To profile a benchmark using [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html) follow the next steps.

1. Set the following environment variable in [setup.sh](benchmarks/setup.sh):
    ```
    export VTUNE_HOME="THE_PATH_TO_THE_VTUNE_DIRECTORY_THAT_CONTAINS_THE_INCLUDE_FOLDER"
    ```
2. Compile the benchmark setting `VTUNE_ANALYSIS=1`:
    ```
    cd benchmarks/X
    make VTUNE_ANALYSIS=1
    ```
    This will link the benchmark against the VTune library and active the code annotations to isolate the region of interest.
3. Run the benchmark with VTune. You can do this by adding VTune's command before the benchmark's command in the `commands` variable of the regressions tests (`regression_small.sh` and `regression_large.sh`). IMPORTANT: You need to use the `-start-paused` flag of VTune to only take into account the region of interest of the benchmark. 

#### Perf

To profile a benchmark using [Perf](https://perf.wiki.kernel.org/index.php/Main_Page) follow the next steps.

1. Compile the benchmark setting `PERF_ANALYSIS=1`:
    ```
    cd benchmarks/X
    make PERF_ANALYSIS=1
    ```
    This will active the code annotations to isolate the region of interest.
2. Create a fifo file named `perf_ctl.fifo` to communicate with the benchmark
    ```
    perf_fifo="perf_ctl.fifo"
    test -p ${perf_fifo} && unlink ${perf_fifo}
    mkfifo ${perf_fifo}
    ```
3. Run the benchmark with Perf. You can do this by adding Perf's command (such as `perf stat`) before the benchmark's command in the `commands` variable of the regressions tests (`regression_small.sh` and `regression_large.sh`). IMPORTANT: You need to use the `-D -1` flag of Perf to only take into account the region of interest of the benchmark.

#### DynamoRIO (MOD)

To use our modified version of [DynamoRIO](https://github.com/LorienLV/dynamorio) to compute the instruction mix of an application follow the next steps.

DynamoRIO does not work yet with Arm SVE instructions.

1. Set the following environment variable in [setup.sh](benchmarks/setup.sh):
    ```
    export DYNAMORIO_INSTALL_PATH="THE_PATH_TO_DYNAMORIOS_BUILD_DIRECTORY"
    ```
2. Compile the benchmark setting `DYNAMORIO_ANALYSIS=1`:
    ```
    cd benchmarks/X
    make DYNAMORIO_ANALYSIS=1
    ```
    This will active the code annotations to isolate the region of interest.
3. Run the benchmark with DynamoRIO - libopcodes. You can do this by adding DynamoRIO's command (`"${DYNAMORIO_INSTALL_PATH}/bin64/drrun" -c "${DYNAMORIO_INSTALL_PATH}/api/bin/libopcodes.so" -- BENCHMARK`) before the benchmark's command in the `commands` variable of the regressions tests (`regression_small.sh` and `regression_large.sh`).

#### Fujitsu Advanced Performance Profiler (FAPP)

To profile a benchmark using the Fujitsu Advanced Performance Profiler (FAPP) follow the next steps.

1. Compile the benchmark setting `FAPP_ANALYSIS=1`:
    ```
    cd benchmarks/X
    make FAPP_ANALYSIS=1
    ```
    This will active the code annotations to isolate the region of interest. IMPORTANT: In the A64FX, this step only works when using the Fujitsu compiler (FCC).
2. Run the benchmark with FAPP. You can do this by adding FAPP's command before the benchmark's command in the `commands` variable of the regressions tests (`regression_small.sh` and `regression_large.sh`).

#### Fujitsu PWR library

To measure the energy consumption of a benchmark using the Fujitsu PWR library follow the next steps.

1. Set the following environment variables in [setup.sh](benchmarks/setup.sh):
    ```
    export PWR_INCLUDES="-IPATH_TO_PWRS_INCLUDE_DIRECTORY"
    export PWR_LDFLAGS="-LPATH_TO_PWRS_lib64_DIRECTORY -lpwr"
    ```
2. Compile the benchmark setting `PWR=1`:
    ```
    cd benchmarks/X
    make PWR=1
    ```
    This will link the benchmark against the PWR library and active the code annotations to isolate the region of interest.
3. Run the benchmark normally. The output will contain a line of text with the energy consumption of the benchmark's region of interest energy consumption in Joules.

#### RAPL-Stopwatch library

To measure the energy consumption of a benchmark using the [RAPL-Stopwatch](https://github.com/LorienLV/rapl_stopwatch) library


1. Set the following environment variables in [setup.sh](benchmarks/setup.sh):
    ```
    export RAPL_STOPWATCH_INCLUDES="-IPATH_TO_RAPL_STOPWATCHS_INCLUDE_DIRECTORY/rapl_stopwatch"
    export RAPL_STOPWATCH_LDFLAGS="-LPATH_TO_RAPL_STOPWATCHS_lib64_DIRECTORY -lrapl_stopwatch"
    ```
2. Compile the benchmark setting `RAPL_STOPWATCH=1`:
    ```
    cd benchmarks/X
    make RAPL_STOPWATCH=1
    ```
    This will link the benchmark against the RAPL-Stopwatch library and active the code annotations to isolate the region of interest.

## Licensing

Each benchmark is individually licensed according to the tool it is extracted from.
