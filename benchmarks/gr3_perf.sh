#!/bin/bash

command="$@"

perf_fifo="perf_ctl.fifo"
test -p ${perf_fifo} && unlink ${perf_fifo}
mkfifo ${perf_fifo}

# Important to use -D -1 to start paused.
perf stat -D -1 -e cpu_cycles,stall,stall_backend,stall_frontend,stall_backend_mem \
          -o perf_stat.txt --control fifo:${perf_fifo} -- $command
