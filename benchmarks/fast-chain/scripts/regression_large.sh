#!/bin/bash

# The folder that contains the inputs for this app.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/chain/large"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
binaries_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=0

# The name of the job.
job="CHAIN-REGRESSION-LARGE"

# job additional parameters.
job_options=(
    # '--exclusive'
    # '--time=00:00:01'
    # '--qos=debug'
)

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
commands=(
    "$binaries_path/chain_gcc"
    # "$binaries_path/chain_fcc"
)

# Additional arguments to pass to the commands.
command_opts="-i \"$inputs_path/c_elegans_40x.10k.in\" -o out.txt -t \$OMP_NUM_THREADS"

# Nodes, MPI ranks and OMP theads used to execute with each command.
parallelism=(
    'nodes=1, mpi=1, omp=1'
    # 'nodes=1, mpi=1, omp=2'
    # 'nodes=1, mpi=1, omp=4'
)

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

    wall_time="$(tac "$job_name.err" | grep -m 1 "Time in kernel" | cut -d ' ' -f 4)"

    echo "Time in kernel: $wall_time s"

    # Check if the output file is identical to the reference
    diff --brief "out.txt" "$inputs_path/out-reference.txt" > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "The output file is not identical to the reference file"
        return 1 # Failure
    fi

    return 0 # OK
)

source "$scriptfolder/../../regression.sh"
