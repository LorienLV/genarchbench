#!/bin/bash

#
# A script to execute a set of regression tests under different job-schedulers.
#
# Supported job-schedulers: Local, SLURM and PJM.
#
# Before executing this script you have to define this variables:
#
#    Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
#    clean=1
#
#    The name of the job.
#    job="NAME-OF-THE-JOB"
#
#    Commands to run.
#    You can access the number of mpi-ranks using the environment variable
#    MPI_RANKS and the number of omp-threads using the environment variable
#    OMP_NUM_THREADS:
#    commands=(
#       'command $MPI_RANKS $OMP_NUM_THREADS'
#    )
#    OR
#    commands=(
#       "command \$MPI_RANKS \$OMP_NUM_THREADS"
#    )
#
#    commands=(
#          "sleep 5"
#          "sleep 10"
#     )
#
#    Additional arguments to pass to the commands.
#    command_opts="-s 1"
#
#    Nodes, MPI ranks and OMP theads used to execute with each command.
#    parallelism=(
#        'nodes=1, mpi=1, omp=1'
#        'nodes=1, mpi=1, omp=2'
#        'nodes=1, mpi=1, omp=4'
#        'nodes=1, mpi=1, omp=8'
#        'nodes=1, mpi=1, omp=12'
#        'nodes=1, mpi=1, omp=24'
#        'nodes=1, mpi=1, omp=36'
#        'nodes=1, mpi=1, omp=48'
#    )
#
# Optional variables:
#    
#    Maximum allowed execution time of each command.
#    time="hh:mm:ss"
#    
#    Run the commands in exclusive mode.
#    exclusive=1
#
#    Scheduler additional parameters.
#    job_options=(
#        '--qos=debug'
#    )
#
# You also have to define this two functions:
#
#    This function is executed before launching a job. You can use this function to
#    prepare the stage folder of the job.
#
#    before_run() (
#        job_name="$1"
#    )
#
#    This function is executed when a job has finished. You can use this function to
#    perform a sanity check and to output something in the report of a job.
#
#    echo: The message that you want to output in the report of the job.
#    return: 0 if the run is correct, 1 otherwise.
#
#    after_run() (
#        job_name="$1"
#    
#        echo "Message"
#        return 1 # Failure
#        return 0 # OK
#    )
#
# WARNING: ALWAYS CALL THIS SCRIPT USING SOURCE AS YOU CAN NOT EXPORT ARRAYS
# IN BASH.
#
#    source "regression.sh"
#

################################################################################

# Detect the job-scheduler
job_scheduler="NONE"
if [[ $(which sbatch) || $? -eq 0 ]]; then
    job_scheduler="SLURM"
elif [[ $(which pjsub) || $? -eq 0 ]]; then
    job_scheduler="PJM"
fi
# else -> No job scheduler

# Trap ctrl_c -> Cancel the jobs.
ctrl_c_trap() (
    for job_id in "${jobs_id[@]}"; do
        if [[ "$job_scheduler" == "SLURM" ]]; then
            scancel "$job_id" >/dev/null 2>&1
        elif [[ "$job_scheduler" == "PJM" ]]; then
            pjdel "$job_id" >/dev/null 2>&1
        fi
    done
)

# Trap ctrl_c -> Cancel the jobs.
trap ctrl_c_trap EXIT

#
# Starting the jobs.
#

scriptpath="$(realpath $0)"
workfolder="$(pwd)"

njobs=$((${#commands[@]} * ${#parallelism[@]}))

echo "[Setup]"
echo "    job scheduler:     '$job_scheduler'"
echo "    working directory: '$workfolder'"
echo "    script path:       '$scriptpath'"
echo ""
echo "[==========] Running $njobs job(s)"
echo "[==========] Started on $(date)"
echo ""

nfailed_jobs=0
jobs_status=()
jobs_id=()
jobs_name=()
jobs_nodes=()
jobs_mpi=()
jobs_omp=()
for command in "${commands[@]}"; do
    for par in "${parallelism[@]}"; do
        nodes="$(echo "$par" | sed -n -E 's/.*nodes[ ]*=[ ]*([0-9]+).*/\1/p')"
        mpi="$(echo "$par" | sed -n -E 's/.*mpi[ ]*=[ ]*([0-9]+).*/\1/p')"
        omp="$(echo "$par" | sed -n -E 's/.*omp[ ]*=[ ]*([0-9]+).*/\1/p')"

        jobs_nodes+=("$nodes")
        jobs_mpi+=("$mpi")
        jobs_omp+=("$omp")

        if [[ -z "$nodes" || "$nodes" -lt 1 ]]; then
            nodes=1
            echo "\033[33mWarning: The number of nodes is $nodes, which is invalid, using 1 instead\e[0m"
        fi
        if [[ -z "$mpi" || "$mpi" -lt 1 ]]; then
            mpi=1
            echo "\033[33mWarning: The number of MPI ranks is $mpi, which is invalid, using 1 instead\e[0m"
        fi
        if [[ -z "$omp" || "$omp" -lt 1 ]]; then
            omp=1
            echo "\033[33mWarning: The number of OpenMP threads is $omp, which is invalid, using 1 instead\e[0m"
        fi

        command_trilled="$(echo "$command" | tr -dc '[:alnum:]\n\r')"
        date_compact="$(date '+%Y%m%d_%H%M%S')"
        job_name="${job}_${command_trilled}_nodes_${nodes}_mpi_${mpi}_omp_${omp}_${date_compact}"
        job_id="-1"

        mkdir "$job_name" >/dev/null 2>&1

        # Create the job script
        jobscript="#!/bin/bash\n"

        if [[ "$job_scheduler" == "SLURM" ]]; then
            jobscript+="#SBATCH --job-name=$job_name\n"
            jobscript+="#SBATCH --nodes=$nodes\n"
            jobscript+="#SBATCH --ntasks=$mpi\n"
            jobscript+="#SBATCH --cpus-per-task=$omp\n"
            jobscript+="#SBATCH --output=${job_name}.out\n"
            jobscript+="#SBATCH --error=${job_name}.err\n"

            if [[ -n "$time" ]]; then
                jobscript+="#SBATCH --time=${job_name}.err\n"
            fi

            if [[ -n "$exclusive" && $exclusive == 1 ]]; then
                jobscript+="#SBATCH --exclusive\n"
            fi

            for job_option in "${job_options[@]}"; do
                jobscript+="#SBATCH $job_option\n"
            done
        elif [[ "$job_scheduler" == "PJM" ]]; then
            # jobscript+="#PJM -N $job_name\n"
            jobscript+="#PJM -L node=$nodes\n"
            jobscript+="#PJM --mpi proc=$mpi\n"
            jobscript+="#PJM -o ${job_name}.out\n"
            jobscript+="#PJM -e ${job_name}.err\n"

            if [[ -n "$time" ]]; then
                jobscript+="#PJM -L elapse=${job_name}.err\n"
            fi

            # if [[ -n "$exclusive" && $exclusive == 1 ]]; then
            #     jobscript+="#PJM --exclusive\n"
            # fi

            for job_option in "${job_options[@]}"; do
                jobscript+="#PJM $job_option\n"
            done
        fi

        jobscript+="export MPI_RANKS=$mpi\n"
        jobscript+="export OMP_NUM_THREADS=$omp\n"

        jobscript+="$command $command_opts"

        # cd to the stage folder of the job.
        current_folder="$(pwd)"
        cd "$job_name"
        echo -e "$jobscript" >"${job_name}.sh"

        # Call before run function
        before_run_out="$(before_run "$job_name")"

        # Run the job.
        run_out=""
        if [[ "$job_scheduler" == "SLURM" ]]; then
            run_out=$(sbatch "${job_name}.sh" 2>&1)
        elif [[ "$job_scheduler" == "PJM" ]]; then
            run_out="$(pjsub "${job_name}.sh" 2>&1)"
        else
            # Execute and wait until finished.
            run_out="$(bash "${job_name}.sh" 1>"$job_name.out" 2>"$job_name.err")"
        fi

        if [ "$?" -ne 0 ]; then
            echo "[----------]"
            echo -e "[ \033[31m FAILED \e[0m ] Could not start processing $job_name"
            echo "[----------]"
            echo ""
            echo "$run_out"

            nfailed_jobs=$((nfailed_jobs + 1))
            jobs_status+=("F") # Failed
        else
            if [[ "$job_scheduler" == "SLURM" ]]; then
                job_id="$(echo "$run_out" | cut -d ' ' -f4)"
            elif [[ "$job_scheduler" == "PJM" ]]; then
                job_id="$(echo "$run_out" | cut -d ' ' -f6)"
            fi

            echo "[----------]"
            echo -e "[ \033[32m RUN \e[0m    ] Started processing $job_name"
            echo "[----------]"
            echo ""

            jobs_status+=("R") # Ready
        fi

        jobs_id+=("$job_id")
        jobs_name+=("$job_name")

        cd "$current_folder"
    done
done

echo "==================== Waiting for spawned jobs to finish ======================"
echo ""

#
# Wait for jobs to finish.
#
for i in "${!jobs_id[@]}"; do
    status="${jobs_status[i]}"

    if [[ "$status" == "F" ]]; then
        # Already failed, do not wait.
        continue
    fi

    job_id="${jobs_id[i]}"
    job_name="${jobs_name[i]}"

    nodes="${jobs_nodes[i]}"
    mpi="${jobs_mpi[i]}"
    omp="${jobs_omp[i]}"

    # Wait for job to finish
    job_state=""
    while :; do
        sleep 1
        if [[ "$job_scheduler" == "SLURM" ]]; then
            job_state="$(sacct -p -n -j $job_id | grep "^$job_id|" | cut -d '|' -f 6)"
            if [[ -n "$job_state" && "$job_state" != "PENDING" && "$job_state" != "RUNNING" && \
                "$job_state" != "REQUEUED" && "$job_state" != "RESIZING" && \
                "$job_state" != "SUSPENDED" && "$job_state" != "REVOKED" ]]; then
                break
            fi
        elif [[ "$job_scheduler" == "PJM" ]]; then
            job_state="$(pjstat -H -S $job_id | grep "^[ ]*STATE[ ]*:[ ]*" | tr -s ' ' | cut -d ' ' -f 4)"
            if [[ -n "$job_state" ]]; then
                # Has finished
                break
            fi
        else
            # No job scheduler
            job_state="OK"
            break
        fi
    done

    status="$job_state"
    after_run_out=""

    if [[ ("$job_scheduler" == "SLURM" && "$job_state" == "COMPLETED") || \
           ("$job_scheduler" == "PJM" && "$job_state" == "EXT") || \
            "$job_state" == "OK" ]]; then
        #
        # Sanity check
        #
        current_folder="$(pwd)"
        cd "$job_name"
        after_run_out="$(after_run "$job_name" 2>&1)"
        if [[ $? -eq 0 ]]; then
            status="OK"

            jobs_status[$i]='V' # Valid
        else
            status="SANITY CHECK FAILED"

            jobs_status[$i]='F' # Failed
            nfailed_jobs=$((nfailed_jobs + 1))
        fi
        cd "$current_folder"

    else
        nfailed_jobs=$((nfailed_jobs + 1))
    fi

    status_string=""
    if [[ "$status" == "OK" ]]; then
        status_string="\033[32m${status}\e[0m"
    else
        status_string="\033[31m${status}\e[0m"
    fi

    echo "------------------------------------------------------------------------------"
    echo "$job_name"
    echo "    - Nodes: $nodes"
    echo "    - MPI ranks: $mpi"
    echo "    - OMP threads: $omp"
    echo -e "    - Status: $status_string"
    echo "Message:"
    echo "$after_run_out"
done

echo ""
echo "[==========]"

if [[ $nfailed_jobs -eq 0 ]]; then
    # All jobs passed
    echo -e "[\033[32m  PASSED  \e[0m] $nfailed_jobs/$njobs jobs have failed"
else
    # Some job has failed
    echo -e "[\033[31m  FAILED  \e[0m] $nfailed_jobs/$njobs jobs have failed"
fi

echo "[==========] Finished on $(date)"

# Clean all the stage directories.
if [[ $clean -eq 1 ]]; then
    for i in "${!jobs_id[@]}"; do

        status="${jobs_status[i]}"
        job_name="${jobs_name[i]}"

        # Only remove the directory if the execution was OK.
        if [[ "$status" == 'V' ]]; then
            rm -rf "$job_name"
        fi
    done
fi
