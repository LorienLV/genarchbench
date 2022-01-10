#!/bin/bash

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/nn-variant"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
benchmark_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=0

# The name of the job.
job="NN-VARIANT-REGRESSION-SMALL"

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
        "python3 $benchmark_path/prediction.py"
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
        "module load gcc/11.0.0-3503git python/3.6.8; python3 $benchmark_path/prediction.py"
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
        "python3 $benchmark_path/prediction.py"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        # 'nodes=1, mpi=1, omp=2'
        # 'nodes=1, mpi=1, omp=4'
        # 'nodes=1, mpi=1, omp=8'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="--chkpnt_fn \"$inputs_path/model\" --sampleName chr20 \
--threads \$OMP_NUM_THREADS --qual 100 \
--input_fn \"$inputs_path/small/prediction_input.h5\" \
--output_fn \"$inputs_path/small/prediction_output.h5\""

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

    wall_time="$(tac "$job_name.out" | grep -m 1 "Time taken:" | cut -d ' ' -f 3,4)"

    echo "Time taken: $wall_time"

    # TODO: Sanity check?

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
