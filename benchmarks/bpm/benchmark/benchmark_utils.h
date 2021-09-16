/*
 *  Wavefront Alignments Algorithms
 *  Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 *  This file is part of Wavefront Alignments Algorithms.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * PROJECT: Wavefront Alignments Algorithms
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Wavefront Alignments Algorithms Benchmark
 */

#ifndef BENCHMARK_UTILS_H_
#define BENCHMARK_UTILS_H_

#include "utils/commons.h"
#include "system/mm_allocator.h"
#include "system/profiler_timer.h"
#include "edit/edit_cigar.h"

/*
 * Constants
 */
#define ALIGN_DEBUG_CHECK_CORRECT   0x00000001
#define ALIGN_DEBUG_CHECK_SCORE     0x00000002
#define ALIGN_DEBUG_CHECK_ALIGNMENT 0x00000004
#define ALIGN_DEBUG_DISPLAY_INFO    0x00000008

#define ALIGN_DEBUG_CHECK_DISTANCE_METRIC_EDIT       0x00000010
#define ALIGN_DEBUG_CHECK_DISTANCE_METRIC_INDEL      0x00000020
#define ALIGN_DEBUG_CHECK_DISTANCE_METRIC_GAP_LINEAL 0x00000040
#define ALIGN_DEBUG_CHECK_DISTANCE_METRIC_GAP_AFFINE 0x00000080

/*
 * Alignment Input
 */
typedef struct {
  // Sequence Info
  int sequence_id;
  char* pattern;
  int pattern_length;
  char* text;
  int text_length;
  // Output
  // FILE* output_file;
  // MM
  mm_allocator_t* mm_allocator;
  // DEBUG
  // profiler_timer_t timer;
  bool verbose;
} align_input_t;

/*
 * Display
 */
void benchmark_print_alignment(
    FILE* const stream,
    align_input_t* const align_input,
    const int score_computed,
    edit_cigar_t* const cigar_computed,
    const int score_correct,
    edit_cigar_t* const cigar_correct);

#endif /* BENCHMARK_UTILS_H_ */
