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
 * DESCRIPTION: Dynamic-programming alignment table
 */

#include "edit/edit_cigar.h"

/*
 * Setup
 */
void edit_cigar_allocate(
    edit_cigar_t* const edit_cigar,
    const int pattern_length,
    const int text_length,
    mm_allocator_t* const mm_allocator) {
  edit_cigar->max_operations = pattern_length+text_length;
  edit_cigar->operations = mm_allocator_malloc(mm_allocator,edit_cigar->max_operations);
  edit_cigar->begin_offset = edit_cigar->max_operations - 1;
  edit_cigar->end_offset = edit_cigar->max_operations;
  edit_cigar->score = INT32_MIN;
}
void edit_cigar_clear(
    edit_cigar_t* const edit_cigar) {
  edit_cigar->begin_offset = edit_cigar->max_operations - 1;
  edit_cigar->end_offset = edit_cigar->max_operations;
  edit_cigar->score = INT32_MIN;
}
void edit_cigar_free(
    edit_cigar_t* const edit_cigar,
    mm_allocator_t* const mm_allocator) {
  mm_allocator_free(mm_allocator,edit_cigar->operations);
}
/*
 * Accessors
 */
int edit_cigar_get_matches(
    edit_cigar_t* const edit_cigar) {
  int i, num_matches=0;
  for (i=edit_cigar->begin_offset;i<edit_cigar->end_offset;++i) {
    num_matches += (edit_cigar->operations[i]=='M');
  }
  return num_matches;
}
void edit_cigar_add_mismatches(
    char* const pattern,
    const int pattern_length,
    char* const text,
    const int text_length,
    edit_cigar_t* const edit_cigar) {
  // Refine adding mismatches
  int i, p=0, t=0;
  for (i=edit_cigar->begin_offset;i<edit_cigar->end_offset;++i) {
    // Check limits
    if (p >= pattern_length || t >= text_length) break;
    switch (edit_cigar->operations[i]) {
      case 'M':
        edit_cigar->operations[i] = (pattern[p]==text[t]) ? 'M' : 'X';
        ++p; ++t;
        break;
      case 'I':
        ++t;
        break;
      case 'D':
        ++p;
        break;
      default:
        fprintf(stderr,"Aband-gaba. Wrong edit operation\n");
        exit(1);
        break;
    }
  }
  while (p < pattern_length) { edit_cigar->operations[i++] = 'D'; ++p; };
  while (t < text_length) { edit_cigar->operations[i++] = 'I'; ++t; };
  edit_cigar->end_offset = i;
  edit_cigar->operations[edit_cigar->end_offset] = '\0';
  //  // DEBUG
  //  printf("Score=%ld\nPath-length=%lu\nCIGAR=%s\n",
  //      gaba_alignment->score,gaba_alignment->plen,
  //      edit_cigar->operations);
}
/*
 * Score
 */
int edit_cigar_score_edit(
    edit_cigar_t* const edit_cigar) {
  int score = 0, i;
  for (i=edit_cigar->begin_offset;i<edit_cigar->end_offset;++i) {
    switch (edit_cigar->operations[i]) {
      case 'M': break;
      case 'X':
      case 'D':
      case 'I': ++score; break;
      default: return INT_MIN;
    }
  }
  return score;
}
/*
 * Utils
 */
int edit_cigar_cmp(
    edit_cigar_t* const edit_cigar_a,
    edit_cigar_t* const edit_cigar_b) {
  // Compare lengths
  const int length_cigar_a = edit_cigar_a->end_offset - edit_cigar_a->begin_offset;
  const int length_cigar_b = edit_cigar_b->end_offset - edit_cigar_b->begin_offset;
  if (length_cigar_a != length_cigar_b) return length_cigar_a - length_cigar_b;
  // Compare operations
  char* const operations_a = edit_cigar_a->operations + edit_cigar_a->begin_offset;
  char* const operations_b = edit_cigar_b->operations + edit_cigar_b->begin_offset;
  int i;
  for (i=0;i<length_cigar_a;++i) {
    if (operations_a[i] != operations_b[i]) {
      return operations_a[i] - operations_b[i];
    }
  }
  // Equal
  return 0;
}
void edit_cigar_copy(
    edit_cigar_t* const edit_cigar_dst,
    edit_cigar_t* const edit_cigar_src) {
  edit_cigar_dst->max_operations = edit_cigar_src->max_operations;
  edit_cigar_dst->begin_offset = edit_cigar_src->begin_offset;
  edit_cigar_dst->end_offset = edit_cigar_src->end_offset;
  edit_cigar_dst->score = edit_cigar_src->score;
  memcpy(edit_cigar_dst->operations+edit_cigar_src->begin_offset,
         edit_cigar_src->operations+edit_cigar_src->begin_offset,
         edit_cigar_src->end_offset-edit_cigar_src->begin_offset);
}
bool edit_cigar_check_alignment(
    FILE* const stream,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    edit_cigar_t* const edit_cigar,
    const bool verbose) {
  // Parameters
  char* const operations = edit_cigar->operations;
  // Traverse CIGAR
  int pattern_pos=0, text_pos=0, i;
  for (i=edit_cigar->begin_offset;i<edit_cigar->end_offset;++i) {
    switch (operations[i]) {
      case 'M':
        // Check match
        if (pattern[pattern_pos] != text[text_pos]) {
          if (verbose) {
            fprintf(stream,
                "Align Check. Alignment not matching (pattern[%d]=%c != text[%d]=%c)\n",
                pattern_pos,pattern[pattern_pos],text_pos,text[text_pos]);
          }
          return false;
        }
        ++pattern_pos;
        ++text_pos;
        break;
      case 'X':
        // Check mismatch
        if (pattern[pattern_pos] == text[text_pos]) {
          if (verbose) {
            fprintf(stream,
                "Align Check. Alignment not mismatching (pattern[%d]=%c == text[%d]=%c)\n",
                pattern_pos,pattern[pattern_pos],text_pos,text[text_pos]);
          }
          return false;
        }
        ++pattern_pos;
        ++text_pos;
        break;
      case 'I':
        ++text_pos;
        break;
      case 'D':
        ++pattern_pos;
        break;
      default:
        fprintf(stderr,"CIGAR check. Unknown edit operation '%c'\n",operations[i]);
        exit(1);
        break;
    }
  }
  // Check alignment length
  if (pattern_pos != pattern_length) {
    if (verbose) {
      fprintf(stream,
          "Align Check. Alignment incorrect length (pattern-aligned=%d,pattern-length=%d)\n",
          pattern_pos,pattern_length);
    }
    return false;
  }
  if (text_pos != text_length) {
    if (verbose) {
      fprintf(stream,
          "Align Check. Alignment incorrect length (text-aligned=%d,text-length=%d)\n",
          text_pos,text_length);
    }
    return false;
  }
  // OK
  return true;
}
/*
 * Display
 */
void edit_cigar_print(
    FILE* const stream,
    edit_cigar_t* const edit_cigar) {
  char last_op = edit_cigar->operations[edit_cigar->begin_offset];
  int last_op_length = 1;
  int i;
  for (i=edit_cigar->begin_offset+1;i<edit_cigar->end_offset;++i) {
    if (edit_cigar->operations[i]==last_op) {
      ++last_op_length;
    } else {
      fprintf(stream,"%d%c",last_op_length,last_op);
      last_op = edit_cigar->operations[i];
      last_op_length = 1;
    }
  }
  fprintf(stream,"%d%c",last_op_length,last_op);
}
void edit_cigar_print_pretty(
    FILE* const stream,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    edit_cigar_t* const edit_cigar,
    mm_allocator_t* const mm_allocator) {
  // Parameters
  char* const operations = edit_cigar->operations;
  // Allocate alignment buffers
  const int max_buffer_length = text_length+pattern_length+1;
  char* const pattern_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  char* const ops_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  char* const text_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  // Compute alignment buffers
  int i, alg_pos = 0, pattern_pos = 0, text_pos = 0;
  for (i=edit_cigar->begin_offset;i<edit_cigar->end_offset;++i) {
    switch (operations[i]) {
      case 'M':
        if (pattern[pattern_pos] != text[text_pos]) {
          pattern_alg[alg_pos] = pattern[pattern_pos];
          ops_alg[alg_pos] = 'X';
          text_alg[alg_pos++] = text[text_pos];
        } else {
          pattern_alg[alg_pos] = pattern[pattern_pos];
          ops_alg[alg_pos] = '|';
          text_alg[alg_pos++] = text[text_pos];
        }
        pattern_pos++; text_pos++;
        break;
      case 'X':
        if (pattern[pattern_pos] != text[text_pos]) {
          pattern_alg[alg_pos] = pattern[pattern_pos++];
          ops_alg[alg_pos] = ' ';
          text_alg[alg_pos++] = text[text_pos++];
        } else {
          pattern_alg[alg_pos] = pattern[pattern_pos++];
          ops_alg[alg_pos] = 'X';
          text_alg[alg_pos++] = text[text_pos++];
        }
        break;
      case 'I':
        pattern_alg[alg_pos] = '-';
        ops_alg[alg_pos] = ' ';
        text_alg[alg_pos++] = text[text_pos++];
        break;
      case 'D':
        pattern_alg[alg_pos] = pattern[pattern_pos++];
        ops_alg[alg_pos] = ' ';
        text_alg[alg_pos++] = '-';
        break;
      default:
        break;
    }
  }
  i=0;
  while (pattern_pos < pattern_length) {
    pattern_alg[alg_pos+i] = pattern[pattern_pos++];
    ops_alg[alg_pos+i] = '?';
    ++i;
  }
  i=0;
  while (text_pos < text_length) {
    text_alg[alg_pos+i] = text[text_pos++];
    ops_alg[alg_pos+i] = '?';
    ++i;
  }
  // Print alignment pretty
  fprintf(stream,"      PRETTY.ALIGNMENT\t");
  edit_cigar_print(stderr,edit_cigar);
  fprintf(stream,"\n");
  fprintf(stream,"      PATTERN    %s\n",pattern_alg);
  fprintf(stream,"                 %s\n",ops_alg);
  fprintf(stream,"      TEXT       %s\n",text_alg);
  // Free
  mm_allocator_free(mm_allocator,pattern_alg);
  mm_allocator_free(mm_allocator,ops_alg);
  mm_allocator_free(mm_allocator,text_alg);
}


