#!/bin/bash

#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

if [[ "$GENARCH_BENCH_CLUSTER" != "MN4" ]]; then
    echo "ERROR: Run 'source setup_mn4.sh' before using this script"
    exit 1
fi

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.1.0_binutils
            make CC=gcc CXX=g++ arch='-march=skylake-avx512' \
            BUILD_DIR=build_gcc \
            VTUNE_ANALYSIS=1 DYNAMORIO_ANALYSIS=1 RAPL_STOPWATCH=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
