#!/bin/bash

#PJM -N CHAIN.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

compiler="gcc"

usage() {
    echo "usage: ./$0 options"
    echo "    -c, --compiler"
    echo "        The compiler to use, GCC or FCC [default=gcc]"
    echo "    -h, --help"
    echo "        Show usage"
}

while [ "$1" != "" ]; do
    case $1 in
    -c | --compiler)
        shift
        compiler=$1
        if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
            echo "ERROR: The compiler must be gcc or fcc"
            exit 1
        fi
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        usage
        exit 1
        ;;
    esac
    shift
done

if [[ $compiler == "gcc" ]]; then
    module load gcc/10.2.0
    make -f Makefile.gcc
elif [[ $compiler == "fcc" ]]; then
    module load fuji
    make -f Makefile.fcc
fi

if [ $? -eq 0 ]; then
    echo "SUCCESS"
    exit 0
else
    echo "ERROR"
    exit 1
fi
