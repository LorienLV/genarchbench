#!/bin/bash

#PJM -N WFA.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

if [[ "$GENARCH_BENCH_CLUSTER" != "CTEARM" ]]; then
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
            make CC=gcc CPP=g++ arch=-march=armv8-a+sve \
            FOLDER_BUILD=build_gcc FOLDER_BIN=bin_gcc
            ;;
        fcc)
            module load fuji
            make CC='fcc -Nclang' CPP='FCC -Nclang' arch=-march=armv8-a+sve \
            FOLDER_BUILD=build_fcc FOLDER_BIN=bin_fcc \
            FAPP_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
