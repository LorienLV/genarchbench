#!/bin/bash

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/nn-variant"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

output_folder="$(pwd)/out"
mkdir "$output_folder"

scriptfolder="$(dirname $(realpath $0))"
benchmark_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=0

# The name of the job.
job="NN-VARIANT-DETAILED-PROFILING"

contig="chr20"
contig_len=64444167 # Mbps
start_pos=0
end_pos=10000000

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
        "$scriptfolder/../../mn4_vtune_profiling.sh $benchmark_path/variantcaller_wrapper_gcc"
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

    job_options=(
        '--exclusive'
        '--time=00:30:00'
    )
    ;;
CTEARM)
    before_command+="source \"$scriptfolder/../../setup_ctearm.sh\"; \
                     module load fuji/1.2.26b tensorflow/2.1.0-dnnl; \
                     source /apps/TENSORFLOW/2.1.0-dnnl/FUJI/VENV/fccbuild_v210/bin/activate; \
                     module load parallel samtools;"

    commands=(
        "$scriptfolder/../../ctearm_fapp_profiling.sh -e pa1-pa17 $benchmark_path/variantcaller_wrapper_fcc"
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

    job_options=(
        '-L rscgrp=large'
    )
    ;;
*)
    commands=(
        "$scriptfolder/../../dynamorio_imix.sh $benchmark_path/variantcaller_wrapper_gcc"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="\"$benchmark_path/Clair3/callVar.sh\" \
	          --bam_fn=\"$inputs_path/HG002_GRCh38_ONT-UL_GIAB_20200122_chr20_0_10000000.phased.bam\" \
	          --ref_fn=\"$inputs_path/hg38_chr20.fa\" \
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

    # We assume that the result is correct.

    job_output_folder="$output_folder/$job_name"
    mkdir "$job_output_folder"

    case "$GENARCH_BENCH_CLUSTER" in
    MN4)
        cp -r *.out *.err *runsa "$job_output_folder"
        ;;
    CTEARM)
        cp -r *.out *.err *.csv "$job_output_folder"
        ;;
    *)
        cp -r *.out *.err *runsa "$job_output_folder"
        ;;
    esac

    echo "The profiling files can be found in $job_output_folder"

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
