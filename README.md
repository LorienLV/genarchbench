# GenarchBench

A port of [GenomicsBench](https://github.com/arun-sub/genomicsbench) to A64FX. This project contains every benchmark included in GenomicsBench plus three additional genomics benchmarks.

## Benchmarks

| #  | Benchmark  | Description                               | Language | Status                                          | Next Step         |
|----|------------|-------------------------------------------|----------|-------------------------------------------------|-------------------|
| 1  | abea       | Adaptive Banded Signal to Event Alignment | C/C++    | <span style="color:green">PORTED</span>         | Tuning            |
| 2  | bpm        | Bit-Parallel Myers Alignment              | C        | <span style="color:green">PORTED</span>         | Tuning            |
| 3  | bsw        | Banded Smith-Waterman                     | C++      | <span style="color:green">PORTED</span>         | ---               |
| 4  | chain      | Seed Chaining                             | C++      | <span style="color:green">PORTED</span>         | Tuning            |
| 5  | fast-chain | SVE version of Chain (fast-chain)         | C++      | <span style="color:orange">PENDING</span>       | Write SVE version |
| 6  | dbg        | De-Bruijn Graph Construction              | C/C++    | <span style="color:green">PORTED</span>         | Tuning            |
| 7  | fmi        | FM-Index                                  | C++      | <span style="color:green">PORTED</span>         | Tuning            |
| 8  | grm        | Genomic Relationship Matrix               | C/C++    | <span style="color:orange">PENDING</span>       | Compile in A64FX  |
| 9  | kmer-cnt   | K-mer Counting                            | C++      | <span style="color:green">PORTED</span>         | Tuning            |
| 10 | nn-base    | Neural Network-based Base Calling         | Python   | <span style="color:green">PORTED</span>         | Tuning  |
| 11 | nn-variant | Neural Network-based Variant Calling      | Python   | <span style="color:orange">PENDING</span>       | Compile in A64FX  |
| 12 | pairHMM    | Pairwise Hidden Markov Model              | C++/Java | <span style="color:orange">PENDING</span>       | Compile in A64FX  |
| 13 | pileup     | Pileup Counting                           | C        | <span style="color:green">PORTED</span>         | Tuning            |
| 14 | poa        | Partial-Order Alignment                   | C++      | <span style="color:green">PORTED</span>         | Tuning            |
| 15 | wfa        | Wavefront Alignment Algorithm             | C        | <span style="color:green">PORTED</span>         | Tuning            |

## Working with the Benchmarks

There is one folder for each benchmark under ([benchmarks](benchmarks)). The folder of each <span style="color:green">PORTED</span> benchmark contains the source code of the benchmark, its dependent libraries, and scripts to compile, execute and profile the benchmark in MareNostrum4, CTE-ARM (A64FX), and in a local Linux environment.

### Set Up the Environment

If you are not working on MN4 or in CTE-ARM, you will need to download the input folder from any of the two clusters:
- Inputs in CTE-ARM: `/fefs/scratch/bsc18/bsc18248/genarch-inputs`
- Inputs in MN4: `/gpfs/scratch/bsc18/bsc18248/genarch-inputs`

#### Setup the environment in CTE-ARM
```
source benchmarks/setup_ctearm.sh
```

#### Setup the environment in MN4
```
source benchmarks/setup_ctearm.sh
```

#### Setup the environment in Linux
```
# Modify the GENARCH_BENCH_INPUTS_ROOT variable in benchmarks/setup_local.sh to point to your local inputs folder
source benchmarks/setup_local.sh
```

The scripts will prepare the environment to be able to use the compilation, execution, and profiling scripts of the benchmarks.

:exclamation: The following sections explain how to compile, execute and profile a benchmark. Some benchmarks, like ABEA, require an initial setup. If you are working outside of CTE-ARM or MN4, please check the README of the benchmark you are working with. 

### Compile a Benchmark

#### Compile a Benchmark in CTE-ARM

```
cd benchmarks/BENCHMARK
pjsub --interact
# Optionally you can run a job to compile:
# pjsub scripts/compile_ctearm.sh
bash scripts/compile_ctearm.sh
exit # To exit the interactive session
```

The script will compile the benchmark using two different compilers: GCC-10.2.0 and FCC-4.2.0.

#### Compile a Benchmark in MN4

```
cd benchmarks/BENCHMARK
bash scripts/compile_mn4.sh
# Optionally you can run a job to compile:
# sbatch scripts/compile_ctearm.sh
```

The script will compile the benchmark using GCC-10.1.0.

#### Compile a Benchmark in Linux

```
cd benchmarks/BENCHMARK
bash scripts/compile_local.sh
```

The script will compile the benchmark using GCC. Modify it to use other compiler.

:exclamation: Only <span style="color:green">PORTED</span> benchmarks have scripts to compile. If you are working with a <span style="color:orange">PENDING</span> benchmark, you will have to manually compile. Check the README of the benchmark to find some help.

### Execute a Benchmark

If you have followed the [Set Up the Environment](#set-up-the-environment) section, you can use the `regression_small.sh` and `regression_large.sh` scripts of a benchmark to run it. The scripts automatically detect the job scheduler of the cluster you are working on (only MN4, CTE-ARM, and local Linux) and run the benchmark with different combinations of threads and compilers. The scripts also perform a sanity check to ensure that the execution is correct and output the execution time of the benchmark. Example:

```
cd benchmarks/BENCHMARK
bash scripts/regression_small.sh # Run a small test-case with different compilers/number of threads
bash scripts/regression_large.sh # Run a large test-case with different compilers/number of threads
```

If you want to execute a benchmark manually, you can check its README or take a look at one of the regression scripts to find out what command to use.

:exclamation: Only <span style="color:green">PORTED</span> benchmarks have scripts to execute. If you are working with a <span style="color:orange">PENDING</span> benchmark, you will have to manually execute. Check the README of the benchmark to find some help.

### Profile a Benchmark

If you have followed the [Set Up the Environment](#set-up-the-environment) section, you can use the `detailed_profiling.sh` script of a benchmark to profile it. The script automatically detects the job scheduler of the cluster you are working on (only MN4, CTE-ARM) and runs the serial version of the benchmark with FAPP if you are working in CTE-ARM or VTune if you are working on MN4. The profiling results will be stored in the `out` folder of the working directory.

:exclamation: Only <span style="color:green">PORTED</span> benchmarks have scripts to profile. If you are working with a <span style="color:orange">PENDING</span> benchmark, you will have to manually profile it.
