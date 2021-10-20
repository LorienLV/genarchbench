#!/bin/bash

# The folder that contains the inputs for this app.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/wfa/large"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

output_folder="$(pwd)/out"
mkdir "$output_folder"

scriptfolder="$(dirname $(realpath $0))"
binaries_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="WFA-DETAILED-PROFILING"

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

# Nodes, MPI ranks and OMP theads used to execute with each command.
# parallelism=(
#     'nodes=1, mpi=1, omp=1'
#     'nodes=1, mpi=1, omp=2'
#     'nodes=1, mpi=1, omp=4'
#     'nodes=1, mpi=1, omp=8'
#     'nodes=1, mpi=1, omp=12'
#     'nodes=1, mpi=1, omp=24'
#     'nodes=1, mpi=1, omp=36'
#     'nodes=1, mpi=1, omp=48'
# )

# job additional parameters.
# job_options=(
#     # '--exclusive'
#     # '--time=00:00:01'
#     # '--qos=debug'
# )

case "$GENARCH_BENCH_CLUSTER" in
CTEARM)
  commands=(
    "module load fuji; $scriptfolder/../../ctearm_fapp_profiling.sh -e pa1-pa17 $binaries_path/bin_fcc/align_benchmark"
  )

  job_options=(
    '-L rscgrp=large'
  )

  parallelism=(
    'nodes=1, mpi=1, omp=1'
    # 'nodes=1, mpi=1, omp=2'
    # 'nodes=1, mpi=1, omp=4'
    # 'nodes=1, mpi=1, omp=8'
    # 'nodes=1, mpi=1, omp=12'
    # 'nodes=1, mpi=1, omp=24'
    # 'nodes=1, mpi=1, omp=36'
    # 'nodes=1, mpi=1, omp=48'
  )
  ;;
*)
  echo "Cluster not supported"
  exit 1
  ;;
esac

# Additional arguments to pass to the commands.
command_opts="-i \"$inputs_path/input.n100K.l1K.seq\" -o checksum.file -t \$OMP_NUM_THREADS"

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

  # We assume that the result is correct.

  job_output_folder="$output_folder/$job_name"
  mkdir "$job_output_folder"

  cp *.csv "$job_output_folder"
  if [[ $? -ne 0 ]]; then
    echo "There has been a problem during the execution of the profiling, check the job stage folder: \"$job_name\""
    return 1 # Failure
  fi

  echo "The .csv files can be found in $job_output_folder"

  return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
