#!/bin/bash

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/nn-base/small"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
benchmark_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="NN-BASE-REGRESSION-SMALL"

# Commands to run.
# You can access the number of mpi-ranks using the environment variable
# MPI_RANKS and the number of omp-threads using the environment variable
# OMP_NUM_THREADS:
# commands=(
#    'command $MPI_RANKS $OMP_NUM_THREADS'
#)
# OR
# commands=(
#    "command \$MPI_RANKS \$OMP_NUM_THREADS"
#)

# job additional parameters.
# job_options=(
#     '--exclusive'
#     '--time=00:03:00'
# )

# Everything you want to do before executing the commands.
before_command="export OMP_PROC_BIND=true;"

case "$GENARCH_BENCH_CLUSTER" in
MN4)
    commands=(
        "python3 $benchmark_path/bonito/basecall.py"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
        'nodes=1, mpi=1, omp=8'
        'nodes=1, mpi=1, omp=12'
        'nodes=1, mpi=1, omp=24'
        'nodes=1, mpi=1, omp=36'
        'nodes=1, mpi=1, omp=48'
    )

    job_options=(
        '--exclusive'
        '--time=00:50:00'
    )
    ;;
CTEARM)
    commands=(
        "module load gcc/11.0.0-3503git python/3.6.8; python3 $benchmark_path/bonito/basecall.py"
        # "module load fuji; $benchmark_path/build_gcc/dbg_fcc"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
        'nodes=1, mpi=1, omp=8'
        'nodes=1, mpi=1, omp=12'
        'nodes=1, mpi=1, omp=24'
        'nodes=1, mpi=1, omp=36'
        'nodes=1, mpi=1, omp=48'
    )

    job_options=(
        '-L rscgrp=large'
    )
    ;;
*)
    commands=(
        "python3 $benchmark_path/bonito/basecall.py"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="\"$benchmark_path/models/bonito_dna_r941\" \"$inputs_path\" --chunksize 3000 --fastq --maxreads 1 > out.fastq"

#
# This function is executed before launching a job. You can use this function to
# prepare the stage folder of the job.
#
before_run() (
    job_name="$1"
)

#
# This function is executed when a job has finished. You can use this function to
# perform a sanity check and to output something in the report of a job.
#
# echo: The message that you want to output in the report of the job.
# return: 0 if the run is correct, 1 otherwise.
#
after_run() (
    job_name="$1"

    wall_time="$(tac "$job_name.err" | grep -m 1 "duration:" | cut -d ' ' -f 3)"

    echo "Duration: $wall_time"

    diff --brief out.fastq "$inputs_path/reference.out.fastq" &>/dev/null
    if [[ $? -ne 0 ]]; then
        echo "The output file is not identical to the reference file"
        return 1 # Failure
    fi

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
