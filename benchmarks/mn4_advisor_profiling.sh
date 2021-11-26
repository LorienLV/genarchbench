#!/bin/bash

command="$@"

module load intel/2019.4
module load parallel_studio/2019.4

# roofline = survey + tripcounts = rooftop + instruction mix results.
advixe-cl --collect=roofline --flop --project-dir=./advisor_results -- $command
advixe-cl --report=survey --mix --project-dir=./advisor_results
