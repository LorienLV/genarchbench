#!/bin/bash

#PJM -N CHAIN.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

if [[ "$GENARCH_BENCH_CLUSTER" != "CTEARM" ]]; then
    echo "ERROR: Run 'source setup_ctearm.sh' before using this script"
    exit 1
fi

export OMP_NUM_THREADS=1

# The compiler to use.
compilers=(
    'fcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        fcc)
            module load fuji
            make CC='fcc -Nclang' CXX='FCC -Nclang' \
            BIN_NAME=variantcaller_wrapper_fcc \
            FAPP_ANALYSIS=1 DYNAMORIO_ANALYSIS=0 PWR=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
