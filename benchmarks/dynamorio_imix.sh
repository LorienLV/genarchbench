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

"${DYNAMORIO_INSTALL_PATH}/bin64/drrun" -c "${DYNAMORIO_INSTALL_PATH}/api/bin/libopcodes.so" -- $@
