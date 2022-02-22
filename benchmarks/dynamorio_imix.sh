#!/bin/bash

#
# The command to execute should not use more than 1 thread.
#

# exit when any command fails
set -e

if [[ -z $DYNAMORIO_INSTALL_PATH ]]; then
    echo "ERROR: 'DYNAMORIO_INSTALL_PATH' not set"
    exit 1
fi

full_command="$@"
command_folder="$(dirname "$1")"
command_executable="$(basename "$1")"

pushd "$command_folder"

"${DYNAMORIO_INSTALL_PATH}/bin64/drconfig" -reg ${command_executable} -c  "${DYNAMORIO_INSTALL_PATH}/api/bin/libopcodes.so"

command_exit_code=0
${full_command} || command_exit_code=$?

"${DYNAMORIO_INSTALL_PATH}/bin64/drconfig" -unreg ${command_executable} || true # Ignore errors in this command

popd

exit $command_exit_code
