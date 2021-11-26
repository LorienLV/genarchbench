#!/bin/bash

#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

if [[ "$GENARCH_BENCH_CLUSTER" != "MN4" ]]; then
    echo "ERROR: Run 'source setup_mn4.sh' before using this script"
    exit 1
fi

export OMP_NUM_THREADS=1

# The compilers to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.1.0
            make CC=gcc CXX=g++ FOLDER_BUILD=build_gcc FOLDER_BIN=bin_gcc \
            VTUNE_ANALYSIS=1 ADVISOR_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
