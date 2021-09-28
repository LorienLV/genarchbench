#define _GNU_SOURCE 1

#include "perf.h"

#include <asm/unistd.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// File descriptor used by perf.
static int fd[NUM_EVENTS];

// Event description.
static const char *descriptors[NUM_EVENTS] = {
    "cpu cycles",
    "instructions",
    "cache references",
    "cache misses",
    "branch instructions",
    "branch misses",
    "bus cycles",
    "stalled cycles frontend",
    "stalled cycles backend",
    "ref cpu cycles",

    "L1D read access",
    "L1I read access",
    "LL read access",
    "DTLB read access",
    "ITLB read access",
    "BPU read access",
    "NODE read access",

    "L1D read misses",
    "L1I read misses",
    "LL read misses",
    "DTLB read misses",
    "ITLB read misses",
    "BPU read misses",
    "NODE read misses",

    "L1D write access",
    "L1I write access",
    "LL write access",
    "DTLB write access",
    "ITLB write access",
    "BPU write access",
    "NODE write access",

    "L1D write misses",
    "L1I write misses",
    "LL write misses",
    "DTLB write misses",
    "ITLB write misses",
    "BPU write misses",
    "NODE write misses",

    "L1D prefetch access",
    "L1I prefetch access",
    "LL prefetch access",
    "DTLB prefetch access",
    "ITLB prefetch access",
    "BPU prefetch access",
    "NODE prefetch access",

    "L1D prefetch misses",
    "L1I prefetch misses",
    "LL prefetch misses",
    "DTLB prefetch misses",
    "ITLB prefetch misses",
    "BPU prefetch misses",
    "NODE prefetch misses",

    "cpu clock",
    "task clock",
    "page faults",
    "context switches",
    "cpu migrations",
    "page faults min",
    "page faults maj",
    "alignment faults",
    "emulation faults",
    "dummy",
    "bpf output",

    "RAW SIMD_INST_RETIRED",
    "RAW SVE_INST_RETIRED",
    "RAW SVE_MATH_SPEC",
    "RAW ASE_SVE_LD_SPEC",
};

// Type of events. This must match the event structure!!
static const int type[NUM_EVENTS] = {
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,
    PERF_TYPE_HARDWARE,

    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,
    PERF_TYPE_HW_CACHE,

    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,
    PERF_TYPE_SOFTWARE,

    PERF_TYPE_RAW,
    PERF_TYPE_RAW,
    PERF_TYPE_RAW,
    PERF_TYPE_RAW,
};

// Name of the events to measure
static const int event[NUM_EVENTS] = {
    /* Hardware events */
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_CACHE_REFERENCES,
    PERF_COUNT_HW_CACHE_MISSES,
    PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
    PERF_COUNT_HW_BRANCH_MISSES,
    PERF_COUNT_HW_BUS_CYCLES,
    PERF_COUNT_HW_STALLED_CYCLES_FRONTEND,
    PERF_COUNT_HW_STALLED_CYCLES_BACKEND,
    PERF_COUNT_HW_REF_CPU_CYCLES,

    /* Cache events */

    // L1D Read access
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // L1I Read access
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // LL Read access
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // DTLB Read access
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // ITLB Read access
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // BPU Read access
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // NODE Read access
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    //

    // L1D Read misses
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // L1I Read misses
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // LL Read misses
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // DTLB Read misses
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // ITLB Read misses
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // BPU Read misses
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // NODE Read misses
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_READ << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    //

    // L1D Write access
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // L1I Write access
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // LL Write access
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // DTLB Write access
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // ITLB Write access
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // BPU Write access
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // NODE Write access
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    //

    // L1D Write misses
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // L1I Write misses
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // LL Write misses
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // DTLB Write misses
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // ITLB Write misses
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // BPU Write misses
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // NODE Write misses
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    //

    // L1D Prefetch access
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // L1I Prefetch access
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // LL Prefetch access
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // DTLB Prefetch access
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // ITLB Prefetch access
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // BPU Prefetch access
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    // NODE Prefetch access
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),

    //

    // L1D Prefetch misses
    PERF_COUNT_HW_CACHE_L1D |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // L1I Prefetch misses
    PERF_COUNT_HW_CACHE_L1I |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // LL Prefetch misses
    PERF_COUNT_HW_CACHE_LL |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // DTLB Prefetch misses
    PERF_COUNT_HW_CACHE_DTLB |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // ITLB Prefetch misses
    PERF_COUNT_HW_CACHE_ITLB |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // BPU Prefetch misses
    PERF_COUNT_HW_CACHE_BPU |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    // NODE Prefetch misses
    PERF_COUNT_HW_CACHE_NODE |
    (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),

    /* Software events */
    PERF_COUNT_SW_CPU_CLOCK,
    PERF_COUNT_SW_TASK_CLOCK,
    PERF_COUNT_SW_PAGE_FAULTS,
    PERF_COUNT_SW_CONTEXT_SWITCHES,
    PERF_COUNT_SW_CPU_MIGRATIONS,
    PERF_COUNT_SW_PAGE_FAULTS_MIN,
    PERF_COUNT_SW_PAGE_FAULTS_MAJ,
    PERF_COUNT_SW_ALIGNMENT_FAULTS,
    PERF_COUNT_SW_EMULATION_FAULTS,
    PERF_COUNT_SW_DUMMY,
    PERF_COUNT_SW_BPF_OUTPUT,

    0x8000,
    0x8002,
    0x800e,
    0x8085,
};

void perf_stopwatch_restart(const int index) {
    for (int i = 0; i < NUM_EVENTS; ++i) {
        _psw[index].total_count[i] = 0;
    }
}

/**
 * Creates a file descriptor that allows measuring performance information.
 * Each file descriptor corresponds to one event that is measured; these can
 * be grouped together to measure multiple events simultaneously.
 *
 * @param hw_event structure that provides detailed configuration information
 *                 for the event being created.
 * @param pid PID to monitor.
 * @param cpu CPU to monitor.
 * @param group_fd The group_fd argument allows event groups to be created.
 *                 An event group has one event which is the group leader.
 *                 The leader is created first, with group_fd = -1. The rest of
 *                 the group members are created
 *                 with subsequent perf_event_open() calls with group_fd being
 *                 set to the file descriptor of the group leader.  (A single
 *                 event on its own is created with group_fd = -1 and is
 *                 considered to be a group with only 1 member.)  An event group
 *                 is scheduled onto the CPU as a unit: it will be put onto the
 *                 CPU only if all of the events in the group can be put onto
 *                 the CPU.  This means that the values of the member events can
 *                 be meaningfully compared—added, divided (to get ratios), and
 *                 so on—with each other, since they have counted events for the
 *                 same set of executed instructions.
 * @param flags The flags argument is formed by ORing together zero or more of
 *              the following values:
 *
 *              PERF_FLAG_FD_CLOEXEC (since Linux 3.14)
 *                       This flag enables the close-on-exec flag for the
 *                       created event file descriptor, so that the file
 *                       descriptor is automatically closed on execve(2).
 *                       Setting the close-on-exec flags at creation time,
 *                       rather than later with fcntl(2), avoids potential
 *                       race conditions where the calling thread invokes
 *                       perf_event_open() and fcntl(2) at the same time as
 *                       another thread calls fork(2) then execve(2).
 *
 *               PERF_FLAG_FD_NO_GROUP
 *                       This flag tells the event to ignore the group_fd
 *                       parameter except for the purpose of setting up output
 *                       redirection using the PERF_FLAG_FD_OUTPUT flag.
 *
 *               PERF_FLAG_FD_OUTPUT (broken since Linux 2.6.35)
 *                       This flag re-routes the event's sampled output to
 *                       instead be included in the mmap buffer of the event
 *                       specified by group_fd.
 *
 *               PERF_FLAG_PID_CGROUP (since Linux 2.6.39)
 *                       This flag activates per-container system-wide
 *                       monitoring. A container is an abstraction that
 *                       isolates a set of resources for finer-grained control
 *                       (CPUs, memory, etc.). In this mode, the event is
 *                       measured only if the thread running on the monitored
 *                       CPU belongs to the designated container (cgroup). The
 *                       cgroup is identified by passing a file descriptor
 *                       opened on its directory in the cgroupfs filesystem.
 *                       For instance, if the cgroup to monitor is called test,
 *                       then a file descriptor opened on /dev/cgroup/test
 *                       (assuming cgroupfs is mounted on /dev/cgroup) must be
 *                       passed as the pid parameter. cgroup monitoring is
 *                       available only for system-wide events and may therefore
 *                       require
 *                       extra permissions.
 * @return file descriptor, for use in subsequent system calls (read(2),
 *         mmap(2), prctl(2), fcntl(2), etc.).
 */
static int perf_event_open(const struct perf_event_attr *const hw_event, const pid_t pid,
                           const int cpu, const int group_fd, const unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

/**
 * Creates a perf struct for tracking the HW events.
 *
 * @param pe perf structure.
 * @param type overall event type (see perf_event_attr man).
 * @param config This specifies which event you want, in conjunction with the
                 type field. The config1 and config2 fields are also taken
                 into account in cases where 64 bits is not enough to fully
                 specify the event. The encoding of these fields are event
                 dependent (see perf_event_attr man).
 */
static void perf_struct(struct perf_event_attr *const pe, const int type, const int config) {

    memset(pe, 0, sizeof(struct perf_event_attr));
    pe->size = sizeof(struct perf_event_attr);
    pe->disabled = 1;

    // Exclude kernel and hypervisor from being measured.
    pe->exclude_kernel = 1;
    pe->exclude_hv = 1;

    // Children inherit it
    pe->inherit = 1;

    // Type of event to measure.
    pe->type = type;
    pe->config = config;
}

void perf_start() {
    static struct perf_event_attr pe[NUM_EVENTS];
    // Get perf_struct.
    for (int i = 0; i < NUM_EVENTS; ++i) {
        perf_struct(&pe[i], type[i], event[i]);
    }
    // Open perf leader.
    for (int i = 0; i < NUM_EVENTS; ++i) {
        fd[i] = perf_event_open(&pe[i], 0, -1, -1, 0);

        if (fd[i] == -1) {
            // fprintf(stderr, "Error opening event %llx (%s) %s\n",
            //         pe[i].config, descriptors[i], strerror(errno));
        }
    }
    // Enable descriptor to read hw counters.
    for (int i = 0; i < NUM_EVENTS; ++i) {
        if (fd[i] != -1) {
            ioctl(fd[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(fd[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

/**
 * Initialize and activate HW event counters.
 *
 */
void perf_start_1(const int index[PERF_COUNTERS], const int tid) {
    static struct perf_event_attr pe[PERF_COUNTERS];
    // Initialize to -1
    for (int i = 0; i < NUM_EVENTS; ++i) {
        fd[i] = -1;
    }
    // Get perf_struct.
    for (int i = 0; i < PERF_COUNTERS && index[i] != -1; ++i) {
        perf_struct(&pe[i], type[index[i]], event[index[i]]);
    }
    // Open perf leader.
    for (int i = 0; i < PERF_COUNTERS && index[i] != -1; ++i) {
        fd[index[i]] = perf_event_open(&pe[i], 0, -1, -1, 0);
    }
}
void perf_start_2(const int tid) {
    // Enable descriptor to read hw counters.
    for (int i = 0; i < NUM_EVENTS; ++i) {
        if (fd[i] != -1) {
            //ioctl(fd[i+NUM_EVENTS*tid], PERF_EVENT_IOC_RESET, 0);
            ioctl(fd[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

void perf_stop_1(const int tid) {
    // "Turn off" hw counters.
    for (int i = 0; i < NUM_EVENTS; ++i) {
        if (fd[i] != -1) {
            ioctl(fd[i], PERF_EVENT_IOC_DISABLE, 0);
        }
    }
}

void perf_stop_2(const int tid) {
    for (int i = 0; i < NUM_EVENTS; ++i) {
        if (fd[i] != -1) {
            close(fd[i]);
        }
    }
}

void perf_stopwatch_play(const int index, const int tid) {
    // Reading HW counter.
    for (int i = 0; i < NUM_EVENTS; i++) {
        if (fd[i] == -1) {
            continue;
        }

        if (sizeof(uint64_t) != read(fd[i], &_psw[index].start_count[i], sizeof(uint64_t))) {
            printf("%16s: ERROR reading perf event.\n", descriptors[i]);
        }
    }
}

void perf_stopwatch_stop(const int index, const int tid) {
    // Reading HW counter.
    for (int i = 0; i < NUM_EVENTS; i++) {
        uint64_t stop_count;

        if (fd[i] == -1) {
            continue;
        }

        if (sizeof(uint64_t) != read(fd[i], &stop_count, sizeof(uint64_t))) {
            printf("%16s: ERROR reading perf event.\n", descriptors[i]);
        }
        else {
            _psw[index].total_count[i] += (stop_count - _psw[index].start_count[i]);
        }
    }
}

void perf_stopwatch_print_all_counters(const int index) {
    for (int i = 0; i < NUM_EVENTS; i++) {
        if (fd[i] != -1) {
            printf("%16s: %14lu\n", descriptors[i], _psw[index].total_count[i]);
        }
    }
}

void perf_stopwatch_print_all_counters(const int start, const int end) {
    for (int i = 0; i < NUM_EVENTS; i++) {
        if (fd[i] != -1) {
            uint64_t total = 0;
            for (int j = start; j < end; j++) {
                total += _psw[j].total_count[i];
            }
            printf("%16s: %14lu\n", descriptors[i], total);
        }
    }
}

char perf_stopwatch_get_counter(const int index, const perf_stopwatch_events_t event,
                                uint64_t *const count) {

    *count = _psw[index].total_count[event];

    return fd[event] != -1;
}

const char *perf_stopwatch_get_descriptor(perf_stopwatch_events_t event) {
    return descriptors[event];
}
