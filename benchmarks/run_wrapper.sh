#!/bin/bash

#
# A script to run a set of commands under different job-schedulers.
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
# Optional variables:
#
#    Everything you want to do before executing the commands. For example, you
#    may want to load some module or to prepend /usr/time to the commands.
#    before_command="module load cmake; /usr/time"
#
#    Additional arguments to pass to the commands.
#    command_opts="-s 1"
#
#    Everything you want to do after executing the commands. For example, you
#    may want to append something to the commands or to process the output of
#    the commands inside the compute node.
#    IMPORTANT: If you do not want to append anything to the command, the first
#    character of this variable must be ';' or '\n'.
#    after_command=""
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
#    source "run_wrapper.sh"
#

################################################################################

# Define output colors.
# Check if stdout is a terminal.
if test -t 1; then
    # See if it supports colors.
    ncolors=$(tput colors)

    if test -n "$ncolors" && test $ncolors -ge 8; then
        RED='\033[01;31m'
        GREEN='\033[01;32m'
        YELLOW='\033[01;33m'
        COLOR_RESTORE='\033[0m'
    fi
fi

# Detect the job-scheduler
job_scheduler="NONE"
if which sbatch &>/dev/null; then
    job_scheduler="SLURM"
elif which pjsub &>/dev/null; then
    job_scheduler="PJM"
fi
# else -> No job scheduler

# Trap ctrl_c -> Cancel the jobs.
ctrl_c_trap() (
    for job_id in "${jobs_id[@]}"; do
        case "$job_scheduler" in
        SLURM)
            scancel "$job_id" &>/dev/null
            ;;
        PJM)
            pjdel "$job_id" &>/dev/null
            ;;
        esac
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
            echo -e "${YELLOW}Warning: The number of nodes ($nodes) is invalid, using 1 instead${COLOR_RESTORE}"
        fi
        if [[ -z "$mpi" || "$mpi" -lt 1 ]]; then
            mpi=1
            echo -e "${YELLOW}Warning: The number of MPI ranks ($mpi) is invalid, using 1 instead${COLOR_RESTORE}"
        fi
        if [[ -z "$omp" || "$omp" -lt 1 ]]; then
            omp=1
            echo -e "${YELLOW}Warning: The number of OpenMP threads ($omp) is invalid, using 1 instead${COLOR_RESTORE}"
        fi

        command_trilled="$(echo "$command" | tr -dc '[:alnum:]\n\r')"
        date_compact="$(date '+%Y%m%d_%H%M%S')"
        job_name="${job}_${command_trilled}_nodes_${nodes}_mpi_${mpi}_omp_${omp}_${date_compact}"
        job_id="-1"

        mkdir "$job_name" &>/dev/null

        # Create the job script
        jobscript="#!/bin/bash\n"

        case "$job_scheduler" in
        SLURM)
            jobscript+="#SBATCH --job-name=\"$job_name\"\n"
            jobscript+="#SBATCH --nodes=$nodes\n"
            jobscript+="#SBATCH --ntasks=$mpi\n"
            jobscript+="#SBATCH --cpus-per-task=$omp\n"
            jobscript+="#SBATCH --output=\"${job_name}.out\"\n"
            jobscript+="#SBATCH --error=\"${job_name}.err\"\n"

            if [[ -n "$time" ]]; then
                jobscript+="#SBATCH --time=${job_name}.err\n"
            fi

            if [[ -n "$exclusive" && $exclusive == 1 ]]; then
                jobscript+="#SBATCH --exclusive\n"
            fi

            for job_option in "${job_options[@]}"; do
                jobscript+="#SBATCH $job_option\n"
            done
            ;;
        PJM)
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
            ;;
        esac

        jobscript+="export MPI_RANKS=$mpi\n"
        jobscript+="export OMP_NUM_THREADS=$omp\n"

        jobscript+="$before_command $command $command_opts $after_command"

        # cd to the stage folder of the job.
        pushd "$job_name" &>/dev/null
        echo -e "$jobscript" >"${job_name}.sh"

        # Call before run function
        before_run_out="$(before_run "$job_name")"

        # Run the job.
        error=0
        run_out=""
        case "$job_scheduler" in
        SLURM)
            run_out=$(sbatch "${job_name}.sh" 2>&1)
            error=$?
            ;;
        PJM)
            run_out="$(pjsub "${job_name}.sh" 2>&1)"
            error=$?
            ;;
        *) # Local
            # Execute and wait until finished.
            run_out="$(bash "${job_name}.sh" 1>"$job_name.out" 2>"$job_name.err")"
            error=$?
            ;;
        esac

        if [ $error -ne 0 ]; then
            echo "[----------]"
            echo -e "[ ${RED} FAILED ${COLOR_RESTORE} ] Could not start processing $job_name"
            echo "[----------]"
            echo ""
            echo "$run_out"

            nfailed_jobs=$((nfailed_jobs + 1))
            jobs_status+=("F") # Failed
        else
            case "$job_scheduler" in
            SLURM)
                job_id="$(echo "$run_out" | cut -d ' ' -f4)"
                ;;
            PJM)
                job_id="$(echo "$run_out" | cut -d ' ' -f6)"
                ;;
            esac

            echo "[----------]"
            echo -e "[ ${GREEN} RUN ${COLOR_RESTORE}    ] Started processing $job_name"
            echo "[----------]"
            echo ""

            jobs_status+=("R") # Ready
        fi

        jobs_id+=("$job_id")
        jobs_name+=("$job_name")

        # Return to the initial directory.
        popd &>/dev/null
    done
done

echo "==================== Waiting for spawned jobs to finish ======================"
echo ""

#
# Wait for jobs to finish.
#
for i in ${!jobs_id[@]}; do

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
    job_energy="-"
    job_state=""
    while :; do
        sleep 1

        case "$job_scheduler" in
        SLURM)
            job_info="$(sacct -p -n -j $job_id 2>/dev/null | grep "^$job_id|")"
            job_state="$(echo "$job_info" | cut -d '|' -f 6)"

            if [[ -n "$job_state" && "$job_state" != "PENDING" && "$job_state" != "RUNNING" &&
                "$job_state" != "REQUEUED" && "$job_state" != "RESIZING" &&
                "$job_state" != "SUSPENDED" && "$job_state" != "REVOKED" ]]; then

                job_exit_code="$(echo "$job_info" | cut -d '|' -f 7 | cut -d ':' -f 1)"
                job_signal_number="$(echo "$job_info" | cut -d '|' -f 7 | cut -d ':' -f 2)"

                if [[ $job_signal_number -ne 0 ]]; then
                    job_state="JOB KILLED BY SIGNAL: $job_signal_number"
                elif [[ $job_exit_code -ne 0 ]]; then
                    job_state="JOB EXITED WITH CODE: $job_exit_code"
                elif [[ "$job_state" == "COMPLETED" ]]; then
                    job_energy="$(sacct -p -n -o ConsumedEnergy -j $job_id 2>/dev/null | sed 's/|//' | sed '/^[[:space:]]*$/d' | sed -n '1p')"
                    status="OK"
                fi
                break
            fi
            ;;
        PJM)
            job_info="$(pjstat -H -S $job_id 2>/dev/null)"
            job_state="$(echo "$job_info" | grep "^[ ]*STATE[ ]*:[ ]*" | tr -s ' ' | cut -d ' ' -f 4)"

            if [[ -n "$job_state" ]]; then

                job_exit_code="$(echo "$job_info" | grep "^[ ]*EXIT CODE[ ]*:[ ]*" | tr -s ' ' | cut -d ' ' -f 5)"

                if [[ $job_exit_code == "-" ]]; then
                    job_signal_number="$(echo "$job_info" | grep "^[ ]*SIGNAL NO[ ]*:[ ]*" | tr -s ' ' | cut -d ' ' -f 5)"
                    job_state="JOB KILLED BY SIGNAL: $job_signal_number"
                elif [[ $job_exit_code -ne 0 ]]; then
                    job_state="JOB EXITED WITH CODE: $job_exit_code"
                elif [[ "$job_state" == "EXT" ]]; then
                    job_energy="$(pjstat -H -S $job_id 2>/dev/null | egrep "ENERGY CONSUMPTION OF NODE \(MEASURED\)" | sed -n '1p' | cut -d ' ' -f7)"
                    job_energy="${job_energy}K" # KJ.
                    status="OK"
                fi
                break
            fi
            ;;
        *) # Local
            # No job scheduler
            status="OK"
            break
            ;;
        esac
    done

    after_run_out=""
    if [[ "$status" == "OK" ]]; then
        #
        # Sanity check
        #
        pushd "$job_name" >/dev/null
        after_run_out="$(after_run "$job_name" 2>&1)"
        if [[ $? -eq 0 ]]; then
            jobs_status[$i]='V' # Valid
        else
            status="SANITY CHECK FAILED"

            jobs_status[$i]='F' # Failed
            nfailed_jobs=$((nfailed_jobs + 1))
        fi
        popd >/dev/null
    else
        status="$job_state"
        nfailed_jobs=$((nfailed_jobs + 1))
    fi

    status_string=""
    if [[ "$status" == "OK" ]]; then
        status_string="${GREEN}${status}${COLOR_RESTORE}"
    else
        status_string="${RED}${status}${COLOR_RESTORE}"
    fi

    echo "------------------------------------------------------------------------------"
    echo "$job_name"
    echo "    - Nodes: $nodes"
    echo "    - MPI ranks: $mpi"
    echo "    - OMP threads: $omp"
    echo "    - Energy consumed (Joules): $job_energy"
    echo -e "    - Status: $status_string"
    echo "Message:"
    echo "$after_run_out"
done

echo ""
echo "[==========]"

if [[ $nfailed_jobs -eq 0 ]]; then
    # All jobs passed
    echo -e "[${GREEN}  PASSED  ${COLOR_RESTORE}] $nfailed_jobs/$njobs jobs have failed"
else
    # Some job has failed
    echo -e "[${RED}  FAILED  ${COLOR_RESTORE}] $nfailed_jobs/$njobs jobs have failed"
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
