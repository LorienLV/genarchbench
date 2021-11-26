#!/bin/bash

command="$@"

scriptfolder="$(dirname $(realpath $0))"

bash "$scriptfolder/mn4_vtune_profiling.sh" $command
bash "$scriptfolder/mn4_advisor_profiling.sh" $command
