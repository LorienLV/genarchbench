#!/bin/bash

#PJM -N DBG.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

if [[ "$GENARCH_BENCH_CLUSTER" != "CTE_ARM" ]]; then
    echo "ERROR: Run 'source setup_ctearm.sh' before using this script"
    exit 1
fi

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    'fcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0
            make CC=gcc CXX=g++ BUILD_DIR=build_gcc
            ;;
        fcc)
            module load fuji
            make CC='fcc -Nclang' CXX='FCC -Nclang' BUILD_DIR=build_fcc \
            FAPP_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
