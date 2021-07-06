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

#include "utils/commons.h"
#include "system/profiler_timer.h"

#include "edit/edit_bpm.h"

#include "benchmark/benchmark_edit.h"
#include "benchmark/benchmark_bitpal.h"

/*
 * Algorithms
 */
typedef enum {
  alignment_edit_bpm,
  alignment_bitpal_edit,
  alignment_bitpal_scored,
} alg_algorithm_type;

/*
 * Generic parameters
 */
typedef struct {
  // Input
  char *algorithm;
  char *input;
  char *output;
  // Profile
  profiler_timer_t timer_global;
  int progress;
  // Check
  bool verbose;
} benchmark_args;
benchmark_args parameters = {
  // Input
  .algorithm=NULL,
  .input=NULL,
  .progress = 10000,
  .verbose = false
};

/*
 * Benchmark
 */
void align_benchmark(const alg_algorithm_type alg_algorithm) {
  // Parameters
  FILE *input_file = NULL;
  char *line1 = NULL, *line2 = NULL;
  int line1_length=0, line2_length=0;
  size_t line1_allocated=0, line2_allocated=0;
  align_input_t align_input;
  // Init
  timer_reset(&(parameters.timer_global));
  timer_start(&(parameters.timer_global));
  input_file = fopen(parameters.input, "r");
  if (input_file==NULL) {
    fprintf(stderr,"Input file '%s' couldn't be opened\n",parameters.input);
    exit(1);
  }
  if (parameters.output != NULL) {
    align_input.output_file = fopen(parameters.output,"w");
  } else {
    align_input.output_file = NULL;
  }
  align_input.verbose = parameters.verbose;
  align_input.mm_allocator = mm_allocator_new(BUFFER_SIZE_8M);
  timer_reset(&align_input.timer);
  // Read-align loop
  int reads_processed = 0, progress = 0;
  while (true) {
    // Read queries
    line1_length = getline(&line1,&line1_allocated,input_file);
    if (line1_length==-1) break;
    line2_length = getline(&line2,&line2_allocated,input_file);
    if (line1_length==-1) break;
    // Configure input
    align_input.sequence_id = reads_processed;
    align_input.pattern = line1+1;
    align_input.pattern_length = line1_length-2;
    align_input.pattern[align_input.pattern_length] = '\0';
    align_input.text = line2+1;
    align_input.text_length = line2_length-2;
    align_input.text[align_input.text_length] = '\0';
    // Align queries using DP
    switch (alg_algorithm) {
      case alignment_edit_bpm:
        benchmark_edit_bpm(&align_input);
        break;
      case alignment_bitpal_edit:
        benchmark_bitpal_m0_x1_g1(&align_input);
        break;
      case alignment_bitpal_scored:
        benchmark_bitpal_m1_x4_g2(&align_input);
        break;
      default:
        fprintf(stderr,"Algorithm unknown or not implemented\n");
        exit(1);
        break;
    }
    // Update progress
    ++reads_processed;
    if (++progress == parameters.progress) {
      progress = 0;
      // Compute speed
      const uint64_t time_elapsed_global = timer_get_current_total_ns(&(parameters.timer_global));
      const float rate_global = (float)reads_processed/(float)TIMER_CONVERT_NS_TO_S(time_elapsed_global);
      const uint64_t time_elapsed_alg = timer_get_current_total_ns(&(align_input.timer));
      const float rate_alg = (float)reads_processed/(float)TIMER_CONVERT_NS_TO_S(time_elapsed_alg);
      fprintf(stderr,"...processed %d reads "
          "(benchmark=%2.3f reads/s;alignment=%2.3f reads/s)\n",
          reads_processed,rate_global,rate_alg);
    }
  }
  timer_stop(&(parameters.timer_global));
  // Print benchmark results
  fprintf(stderr,"[Benchmark]\n");
  fprintf(stderr,"=> Total.reads            %d\n",reads_processed);
  fprintf(stderr,"=> Time.Benchmark      ");
  timer_print(stderr,&parameters.timer_global,NULL);
  fprintf(stderr,"  => Time.Alignment    ");
  timer_print(stderr,&align_input.timer,&parameters.timer_global);
  // Free
  fclose(input_file);
  if (align_input.output_file != NULL) fclose(align_input.output_file);
  mm_allocator_delete(align_input.mm_allocator);
  free(line1);
  free(line2);
}
/*
 * Generic Menu
 */
void usage() {
  fprintf(stderr,
      "USE: ./align_benchmark -i input -o output                            \n"
      "      Options::                                                      \n"
      "        [Input]                                                      \n"
      "          --algorithm|a <algorithm>                                  \n"
      "              bpm-edit                                               \n"
      "              bitpal-edit                                            \n"
      "              bitpal-scored                                          \n"
      "          --input|i <File>                                           \n"
      "        [Misc]                                                       \n"
      "          --progress|P <integer>                                     \n"
      "          --help|h                                                   \n");
}
void parse_arguments(int argc,char** argv) {
  struct option long_options[] = {
    /* Input */
    { "algorithm", required_argument, 0, 'a' },
    { "input", required_argument, 0, 'i' },
    { "output", required_argument, 0, 'o' },
    /* Misc */
    { "progress", required_argument, 0, 'P' },
    { "verbose", no_argument, 0, 'v' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 } };
  int c,option_index;
  if (argc <= 1) {
    usage();
    exit(0);
  }
  while (1) {
    c=getopt_long(argc,argv,"a:i:o:p:g:P:c:vh",long_options,&option_index);
    if (c==-1) break;
    switch (c) {
    /*
     * Input
     */
    case 'a':
      parameters.algorithm = optarg;
      break;
    case 'i':
      parameters.input = optarg;
      break;
    case 'o':
      parameters.output = optarg;
      break;
    /*
     * Misc
     */
    case 'P':
      parameters.progress = atoi(optarg);
      break;
    case 'v':
      parameters.verbose = true;
      break;
    case 'h':
      usage();
      exit(1);
    // Other
    case '?': default:
      fprintf(stderr,"Option not recognized \n");
      exit(1);
    }
  }
}
int main(int argc,char* argv[]) {
  // Parsing command-line options
  parse_arguments(argc,argv);
  // Select option
  if (strcmp(parameters.algorithm,"bpm-edit")==0) {
    align_benchmark(alignment_edit_bpm);
  } else if (strcmp(parameters.algorithm,"bitpal-edit")==0) {
    align_benchmark(alignment_bitpal_edit);
  } else if (strcmp(parameters.algorithm,"bitpal-scored")==0) {
    align_benchmark(alignment_bitpal_scored);
  } else {
    fprintf(stderr,"Algorithm '%s' not recognized\n",parameters.algorithm);
    exit(1);
  }
}
