#!/bin/bash

#PJM -N ABEA.RUN
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

inputs_path="/fefs/scratch/bsc18/bsc18248/genarch-inputs/abea/" # The folder that contains the inputs.
input_size="large" # The size of the input, "small" or "large".
compiler="fcc" # The compiler to use, "gcc" or "fcc".

if [[ $input_size != "small" && $input_size != "large" ]]; then
    echo "ERROR: The input size can only be small or large"
    exit 1
fi

if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
    echo "ERROR: The compiler must be gcc or fcc"
    exit 1
fi

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

echo "Executing: compiler = $compiler, input = $input_size"

if [[ $input_size == "small" ]]; then
    ./f5c_"$compiler" eventalign -b "$inputs_path"/small/1000reads.bam -g "$inputs_path"/humangenome.fa -r "$inputs_path"/1000reads.fastq -B 3.7M > events-small-$compiler.tsv
else
    ./f5c_"$compiler" eventalign -b "$inputs_path"/large/10000reads.bam -g "$inputs_path"/humangenome.fa -r "$inputs_path"/10000reads.fastq -B 3.7M > events-large-$compiler.tsv
fi

if [ $? -eq 0 ]; then
    echo "SUCCESS"
    exit 0
else
    echo "ERROR"
    exit 1
fi
