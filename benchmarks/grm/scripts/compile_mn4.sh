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

    build_dir="2.0/build_dynamic_$compiler"
    mkdir "$build_dir"
    cp -r "2.0/build_dynamic/." "$build_dir"

    pushd "$build_dir"

    case "$compiler" in
        gcc)
            module load gcc/10.1.0_binutils
            module load intel/2020.1
            module load mkl/2021.3

            make CC=gcc CXX=g++ arch='-march=native' \
            DYNAMIC_MKL=1 MKL_ROOT='/apps/INTEL/oneapi/2021.3/mkl/2021.3.0' \
            MKL_IOMP5_DIR='/apps/INTEL/oneapi/2021.3/compiler/2021.3.0/linux/compiler/lib/intel64_lin'
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac

    popd
done
