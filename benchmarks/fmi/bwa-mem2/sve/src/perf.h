#ifndef PERF_STOPWATCH_H
#define PERF_STOPWATCH_H

#include <stdint.h>

/**
 * @author Lorién López Villellas (738488) (738488@unizar.es)
 *
 * A module for counting hardware events using perf.
 * A perf_stopwatch works the same way a regular stopwatch does,
 * you can restart the stopwatch, start counting events by playing it,
 * or stop counting events by stopping it.
 *
 * Warning: This module does not support concurrency, perf events are counted for
 * every thread of the first process that call perf_stopwatch_play().
 * Any call to any function must be done sequentially.
 */

typedef enum perf_stopwatch_events {
    CPU_CYCLES,
    INSTRUCTIONS,
    CACHE_REFERENCES,
    CACHE_MISSES,
    BRANCH_INSTRUCTIONS,
    BRANCH_MISSES,
    BUS_CYCLES,
    STALLED_CYCLES_FRONTEND,
    STALLED_CYCLES_BACKEND,
    REF_CPU_CYCLES,

    L1D_READ_ACCESS,
    L1I_READ_ACCESS,
    LL_READ_ACCESS,
    DTLB_READ_ACCESS,
    ITLB_READ_ACCESS,
    BPU_READ_ACCESS,
    NODE_READ_ACCESS,

    L1D_READ_MISSES,
    L1I_READ_MISSES,
    LL_READ_MISSES,
    DTLB_READ_MISSES,
    ITLB_READ_MISSES,
    BPU_READ_MISSES,
    NODE_READ_MISSES,

    L1D_WRITE_ACCESS,
    L1I_WRITE_ACCESS,
    LL_WRITE_ACCESS,
    DTLB_WRITE_ACCESS,
    ITLB_WRITE_ACCESS,
    BPU_WRITE_ACCESS,
    NODE_WRITE_ACCESS,

    L1D_WRITE_MISSES,
    L1I_WRITE_MISSES,
    LL_WRITE_MISSES,
    DTLB_WRITE_MISSES,
    ITLB_WRITE_MISSES,
    BPU_WRITE_MISSES,
    NODE_WRITE_MISSES,

    L1D_PREFETCH_ACCESS,
    L1I_PREFETCH_ACCESS,
    LL_PREFETCH_ACCESS,
    DTLB_PREFETCH_ACCESS,
    ITLB_PREFETCH_ACCESS,
    BPU_PREFETCH_ACCESS,
    NODE_PREFETCH_ACCESS,

    L1D_PREFETCH_MISSES,
    L1I_PREFETCH_MISSES,
    LL_PREFETCH_MISSES,
    DTLB_PREFETCH_MISSES,
    ITLB_PREFETCH_MISSES,
    BPU_PREFETCH_MISSES,
    NODE_PREFETCH_MISSES,

    CPU_CLOCK,
    TASK_CLOCK,
    PAGE_FAULTS,
    CONTEXT_SWITCHES,
    CPU_MIGRATIONS,
    PAGE_FAULTS_MIN,
    PAGE_FAULTS_MAJ,
    ALIGNMENT_FAULTS,
    EMULATION_FAULTS,
    DUMMY,
    BPF_OUTPUT,
    RAW_EVENT1,
    RAW_EVENT2,
    RAW_EVENT3,
    RAW_EVENT4,

    NUM_EVENTS
} perf_stopwatch_events_t;

typedef struct perf_stopwatch {
    uint64_t start_count[NUM_EVENTS]; // HW counters on play time.
    uint64_t total_count[NUM_EVENTS]; // Total count between plays and stops.
} perf_stopwatch_t;

static perf_stopwatch _psw[48*3];

#define PERF_COUNTERS 4
void perf_start();
void perf_start_1(const int index[PERF_COUNTERS], const int tid);
void perf_start_2(const int tid);
void perf_stop_1(const int tid);
void perf_stop_2(const int tid);

/**
 * Restarts psw's total_count.
 */
void perf_stopwatch_restart(const int index);

/**
 * Start counting HW events in psw.
 */
void perf_stopwatch_play(const int index, const int tid);

/**
 * Stop counting HW events in psw.
 */
void perf_stopwatch_stop(const int index, const int tid);

void perf_accum_max();

/**
 * Print every total events counter of psw.
 */
void perf_stopwatch_print_all_counters(const int index);
void perf_stopwatch_print_all_counters(const int start, const int end);

/**
 * Get the PSW event counter referred by EVENT.
 *
 * @param event event reference.
 * @param count will be the total counter of the event.
 * @return 1 if the event EVENT is readable, 0 otherwise.
 */
char perf_stopwatch_get_counter(const int index, const perf_stopwatch_events_t event,
                                uint64_t *const count);

/**
 * Get the event descriptor referred by EVENT.
 *
 * @param event event reference.
 * @return const char* (event descriptor).
 */
const char *perf_stopwatch_get_descriptor(const perf_stopwatch_events_t event);

#endif
