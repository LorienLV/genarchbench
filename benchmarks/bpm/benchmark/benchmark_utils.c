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

#include "benchmark/benchmark_utils.h"

/*
 * Display
 */
void benchmark_print_alignment(
    FILE* const stream,
    align_input_t* const align_input,
    const int score_computed,
    edit_cigar_t* const cigar_computed,
    const int score_correct,
    edit_cigar_t* const cigar_correct) {
  // Print Sequence
  fprintf(stream,"ALIGNMENT (#%d)\n",align_input->sequence_id);
  fprintf(stream,"  PATTERN  %s\n",align_input->pattern);
  fprintf(stream,"  TEXT     %s\n",align_input->text);
  // Print CIGARS
  if (cigar_computed != NULL && score_computed != -1) {
    fprintf(stream,"    COMPUTED\tscore=%d\t",score_computed);
    edit_cigar_print(stream,cigar_computed);
    fprintf(stream,"\n");
  }
  if (cigar_computed != NULL) {
    edit_cigar_print_pretty(stream,
        align_input->pattern,align_input->pattern_length,
        align_input->text,align_input->text_length,
        cigar_computed,align_input->mm_allocator);
  }
  if (cigar_correct != NULL && score_correct != -1) {
    fprintf(stream,"    CORRECT \tscore=%d\t",score_correct);
    edit_cigar_print(stream,cigar_correct);
    fprintf(stream,"\n");
  }
  if (cigar_correct != NULL) {
    edit_cigar_print_pretty(stream,
        align_input->pattern,align_input->pattern_length,
        align_input->text,align_input->text_length,
        cigar_correct,align_input->mm_allocator);
  }
}






