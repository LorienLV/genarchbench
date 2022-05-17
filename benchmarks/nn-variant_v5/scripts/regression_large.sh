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
clean=1

# The name of the job.
job="NN-VARIANT-REGRESSION-LARGE"

contig="chr20"
contig_len=64444167 # Mbps
start_pos=100000
end_pos=300000

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
    before_command+="module load python/3-intel-2019.2; \
                     source /apps/INTEL/PYTHON/2019.2.066/intelpython3/bin/init; \
                     conda activate tf2-bio; export KERAS_BACKEND=tensorflow;"

    commands=(
        "$benchmark_path/basecall_wrapper_gcc"
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
    before_command+="source \"$scriptfolder/../../setup_ctearm.sh\"; \
                     module load fuji/1.2.26b tensorflow/2.1.0-dnnl; \
                     source /apps/TENSORFLOW/2.1.0-dnnl/FUJI/VENV/fccbuild_v210/bin/activate; \
                     module load parallel samtools;"

    commands=(
        "$benchmark_path/basecall_wrapper_fcc"
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
        "$benchmark_path/basecall_wrapper_gcc"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="\"$benchmark_path/Clair3/callVar.sh\" \
	          --bam_fn=\"$inputs_path/HG003_chr20_demo.bam\" \
	          --ref_fn=\"$inputs_path/GRCh38_no_alt_chr20.fa\" \
	          --threads=\$OMP_NUM_THREADS \
	          --platform="ont" \
	          --model_path=\"$benchmark_path/models/r941_prom_hac_g360+g422\" \
	          --bed_fn=region.bed \
              --output=. \
              --chunk_size=\$((${contig_len}/\${OMP_NUM_THREADS}+1))"

#
# This function is executed before launching a job. You can use this function to
# prepare the stage folder of the job.
#
before_run() (
    job_name="$1"

    echo -e "${contig}\t${start_pos}\t${end_pos}" > region.bed
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

    # wall_time="$(tac "$job_name.err" | grep -m 1 "duration:" | cut -d ' ' -f 3)"

    # echo "Duration: $wall_time"

    # diff --brief out.fastq "$inputs_path/reference.out.fastq" &>/dev/null
    # if [[ $? -ne 0 ]]; then
    #     echo "The output file is not identical to the reference file"
    #     return 1 # Failure
    # fi

    # cat "$job_name.err" | grep "Energy consumption:"
    
    cat "$job_name.out" | grep "Total time elapsed:"

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
