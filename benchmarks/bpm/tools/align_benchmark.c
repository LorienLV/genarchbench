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

#include <omp.h>

#include "utils/commons.h"
#include "system/profiler_timer.h"

#include "edit/edit_bpm.h"

#include "benchmark/benchmark_edit.h"
#include "benchmark/benchmark_bitpal.h"

#if VTUNE_ANALYSIS
    #include <ittnotify.h>
#endif
#if FAPP_ANALYSIS
    #include "fj_tool/fapp.h"
#endif
#if DYNAMORIO_ANALYSIS
    // Two definitions of bool fix.
    #undef bool
    #include <dr_api.h>
#endif

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
  int threads;
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
  .threads=1,
  .progress = 10000,
  .verbose = false
};

typedef struct sequence_t {
  char *data;
  int length;
  size_t allocated;
} sequence_t;

typedef struct seq_pair_t {
  int id;
  sequence_t seq1;
  sequence_t seq2;

  int score;

  struct seq_pair_t *next;
} seq_pair_t;

/*
 * Benchmark
 */
void align_benchmark(const alg_algorithm_type alg_algorithm) {
  // Init
  timer_reset(&(parameters.timer_global));

  FILE *input_file = fopen(parameters.input, "r");
  if (input_file == NULL) {
    fprintf(stderr,"Input file '%s' couldn't be opened\n", parameters.input);
    exit(1);
  }

  FILE *output_file = NULL;
  if (parameters.output != NULL) {
    output_file = fopen(parameters.output,"w");
  }

  char more_seqs = 1;
  int seqs_read = 0;
  int seqs_processed = 0;
  #pragma omp parallel num_threads(parameters.threads)
  {
    align_input_t align_input;

    align_input.verbose = parameters.verbose;
    align_input.mm_allocator = mm_allocator_new(BUFFER_SIZE_8M);

    // timer_reset(&align_input.timer);

    // Linked list of read sequences.
    seq_pair_t *head = malloc(sizeof(*head));

    head->seq1.length = -1;
    head->seq2.length = -1;
    head->seq1.data = NULL;
    head->seq2.data = NULL;
    head->next = NULL;

    // Read and assign the same number of sequences to each thread.
    seq_pair_t *it = head;
    while(more_seqs) {
      #pragma omp for schedule(static, 1)
      for (size_t i = 0; i < omp_get_num_threads(); ++i) {
        #pragma omp critical
        {
          it->seq1.length = getline(&it->seq1.data, &it->seq1.allocated, input_file);
          it->seq2.length = getline(&it->seq2.data, &it->seq2.allocated, input_file);

          if (it->seq1.length == -1 || it->seq2.length == -1) {
            // OMP: Implicit flush of more_seqs at the end of the for loop.
            more_seqs = 0;
          }
          else {
            // OMP: Implicit flush of seqs_reads on entry and exit of the critical section.
            it->id = seqs_read;
            ++seqs_read;

            it->next = malloc(sizeof(*it));
            it = it->next;

            it->seq1.length = -1;
            it->seq2.length = -1;
            it->seq1.data = NULL;
            it->seq2.data = NULL;
            it->next = NULL;
          }
        }
      }
    }

    #pragma omp barrier
    #pragma omp master
    {
      timer_start(&(parameters.timer_global));
#if VTUNE_ANALYSIS
      __itt_resume();
#endif
#if FAPP_ANALYSIS
      fapp_start("benchmark_edit_bpm", 1, 0);
#endif
#if DYNAMORIO_ANALYSIS
      dr_app_setup_and_start();
#endif
    }

    // Process the sequences.

    it = head;
    while(it != NULL) {
      if (it->seq1.length > 0 && it->seq2.length > 0) {

        align_input.pattern = it->seq1.data + 1;
        align_input.pattern_length = it->seq1.length - 2;
        align_input.pattern[align_input.pattern_length] = '\0';
        align_input.text = it->seq2.data + 1;
        align_input.text_length = it->seq2.length - 2;
        align_input.text[align_input.text_length] = '\0';

        // Align queries using DP
        switch (alg_algorithm) {
          case alignment_edit_bpm:
            it->score = benchmark_edit_bpm(&align_input);
            break;
          case alignment_bitpal_edit:
            it->score = benchmark_bitpal_m0_x1_g1(&align_input);
            break;
          case alignment_bitpal_scored:
            it->score = benchmark_bitpal_m1_x4_g2(&align_input);
            break;
          default:
            fprintf(stderr,"Algorithm unknown or not implemented\n");
            exit(1);
            break;
        }

        // Update counters and print debug info if verbose.
        if (parameters.verbose) {
          #pragma omp critical 
          {
            ++seqs_processed;
            if (seqs_processed % parameters.progress == 0) {
              // Compute speed
              const uint64_t time_elapsed_global = timer_get_current_total_ns(&(parameters.timer_global));
              const float rate_global = (float)seqs_processed/(float)TIMER_CONVERT_NS_TO_S(time_elapsed_global);
              // const uint64_t time_elapsed_alg = timer_get_current_total_ns(&align_timer);
              // const float rate_alg = (float)seqs_processed/(float)TIMER_CONVERT_NS_TO_S(time_elapsed_alg);
              fprintf(stderr,"...processed %d reads "
                  "(benchmark=%2.3f reads/s)\n",
                  seqs_processed, rate_global);
              
              // fprintf(stderr,"...processed %d reads "
              //     "(benchmark=%2.3f reads/s;alignment=%2.3f reads/s)\n",
              //     seqs_processed, rate_global, rate_alg);
            }
          }
        }
      }

      it = it->next;
    }

    #pragma omp barrier
    #pragma omp master
    {
#if DYNAMORIO_ANALYSIS
      dr_app_stop_and_cleanup();
#endif
#if VTUNE_ANALYSIS
      __itt_pause();
#endif
#if FAPP_ANALYSIS
      fapp_stop("benchmark_edit_bpm", 1, 0);
#endif
      timer_stop(&(parameters.timer_global));
    }

    // Print scores and free private data.
    it = head;
    while(it != NULL) {
      if (it->seq1.length > 0 && it->seq2.length > 0 && output_file != NULL) {
        fprintf(output_file,"[%d] score=%d\n", it->id, it->score);
      }

      free(it->seq1.data);
      free(it->seq2.data);

      seq_pair_t *temp = it;
      it = it->next;
      free(temp);
    }

    mm_allocator_delete(align_input.mm_allocator);
  }

  // Print benchmark results
  fprintf(stderr,"[Benchmark]\n");
  fprintf(stderr,"=> Total.reads            %d\n", seqs_read);
  fprintf(stderr,"=> Time.Benchmark      ");
  timer_print(stderr,&parameters.timer_global,NULL);
  // fprintf(stderr,"  => Time.Alignment    ");
  // timer_print(stderr,&align_timer, &parameters.timer_global);
  // Free
  fclose(input_file);
  if (output_file != NULL) fclose(output_file);
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
      "          --threads|t <integer>                                      \n"
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
    { "threads", required_argument, 0, 't' },
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
    c=getopt_long(argc,argv,"a:i:o:t:p:g:P:c:vh",long_options,&option_index);
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
    case 't':
      parameters.threads = atoi(optarg);
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
