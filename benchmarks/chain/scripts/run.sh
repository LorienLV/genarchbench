#!/bin/bash

#PJM -N CHAIN.RUN
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

inputs_path=""
input_size="small"
compiler="gcc"

usage() {
    echo "usage: ./$0 options"
    echo "    -i, --input"
    echo "        The input folder, e.g: /inputs/abea"
    echo "    -s, --size"
    echo "        The input size, small or large [default=small]"
    echo "    -c, --compiler"
    echo "        The compiler to use, GCC or FCC [default=gcc]"
    echo "    -h, --help"
    echo "        Show usage"
}

while [ "$1" != "" ]; do
    case $1 in
    -i | --input)
        shift
        inputs_path="$1"
        ;;
    -s | --size)
        shift
        input_size="$1"
        if [[ $input_size != "small" && $input_size != "large" ]]; then
            echo "ERROR: The input size can only be small or large"
            exit 1
        fi
        ;;
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

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

echo "Executing..."

if [[ $input_size == "small" ]]; then
    ./chain_"$compiler" -i "$inputs_path"/small/in-1k.txt -o out-small.txt
else
    ./chain_"$compiler" -i "$inputs_path"/large/c_elegans_40x.10k.txt -o out-large.txt
fi

if [ $? -eq 0 ]; then
    echo "SUCCESS"
    exit 0
else
    echo "ERROR"
    exit 1
fi
