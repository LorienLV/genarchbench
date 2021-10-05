#!/bin/bash

#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

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
            make CC=gcc CXX=g++ FMI=fmi_gcc arch='-march=skylake-avx512' TARGET_ARCH=x86_64
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
