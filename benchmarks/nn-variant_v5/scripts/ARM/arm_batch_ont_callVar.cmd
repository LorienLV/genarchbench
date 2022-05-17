#!/bin/bash
#------ pjsub option --------#
#PJM -L "rscgrp=small"
# Name of the resource group (= queue) to submit the job
#PJM -N ont-threads
# Name of the job (optional)
#PJM -o ont_callvar_threads_%j.out
# The name of the file to collect the standard output (stdout) of the job.
#PJM -e ont_callvar_threads_%j.err
# The name of the file to collect the standard error output (stderr) of the job.
#PJM -L node=1
# Specify the number of required nodes
#PJM -L elapse=24:00:00
# Specify the maximum duration of a job
#PJM --mpi "proc=48,max-proc-per-node=48"
#------- Program execution -------#


module load fuji/1.2.26b tensorflow/2.1.0-dnnl
source /apps/TENSORFLOW/2.1.0-dnnl/FUJI/VENV/fccbuild_v210/bin/activate
#module load ANACONDA/2021.11
#source activate clair3
module load parallel samtools


uname -n 
numactl -H 

PLATFORM="ont"
WORKING_DIR="/fefs/scratch/bsc18/bsc18926/Bioinformatics/"
INPUT_DIR=${WORKING_DIR}"/content/${PLATFORM}-data/"
MODELS_DIR=${WORKING_DIR}"/models/"
CLAIR3_DIR=${WORKING_DIR}"/Clair3/"

REF="GRCh38_no_alt_chr20.fa"
BAM="HG003_chr20_demo.bam"

CONTIGS="chr20"
START_POS=100000
END_POS=300000
echo -e "${CONTIGS}\t${START_POS}\t${END_POS}" > ${INPUT_DIR}/quick_demo.bed

# run Clair3 using one command
for t in 1 4 8 12 24 48
do
	THREADS=${t}
	OUTPUT_DIR=${WORKING_DIR}"/content/clair3_${PLATFORM}_demo_${THREADS}"
	mkdir -p ${OUTPUT_DIR}
	date
	\time -v ${CLAIR3_DIR}/callVar.sh \
	  --bam_fn=${INPUT_DIR}/${BAM} \
	  --ref_fn=${INPUT_DIR}/${REF} \
	  --threads=${THREADS} \
	  --platform=${PLATFORM} \
	  --model_path="${MODELS_DIR}/${PLATFORM}" \
	  --output=${OUTPUT_DIR} \
	  --bed_fn=${INPUT_DIR}/quick_demo.bed
	date
done



