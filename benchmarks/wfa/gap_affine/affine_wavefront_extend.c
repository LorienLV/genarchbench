/*
 *                             The MIT License
 *
 * Wavefront Alignments Algorithms
 * Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 * This file is part of Wavefront Alignments Algorithms.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * PROJECT: Wavefront Alignments Algorithms
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: WFA extend exact-matches component
 */

#include "gap_affine/affine_wavefront_extend.h"
#include "gap_affine/affine_wavefront_reduction.h"
#include "gap_affine/affine_wavefront_utils.h"
#include "utils/string_padded.h"

#ifdef __ARM_FEATURE_SVE
  #include <arm_sve.h>
#endif

#if 0
void print_mask (svbool_t mask) {
    // Create a "1" vector with inactive lanes to zero
    svuint32_t res = svdup_u32_z(mask, 1U);

    uint32_t v[16] = {0};
    svst1_u32(svptrue_b32(), v, res);
    for (int i=0; i<16; i++) {
        fprintf(stderr, "%u ", v[i]);
    }
    fprintf(stderr, "\n");
}

void print_svint32 (svint32_t _v) {
    int32_t v[16] = {0};
    svst1_s32(svptrue_b32(), v, _v);
    for (int i=0; i<16; i++) {
        fprintf(stderr, "%d ", v[i]);
    }
    fprintf(stderr, "\n");
}

void print_svuint32 (svuint32_t _v) {
    uint32_t v[16] = {0};
    svst1_u32(svptrue_b32(), v, _v);
    for (int i=0; i<16; i++) {
        fprintf(stderr, "%u ", v[i]);
    }
    fprintf(stderr, "\n");
}

void print_svuint32_hex (svuint32_t _v) {
    uint32_t v[16] = {0};
    svst1_u32(svptrue_b32(), v, _v);
    for (int i=0; i<16; i++) {
        fprintf(stderr, "0x%x ", v[i]);
    }
    fprintf(stderr, "\n");
}
#endif

/*
 * Reduce wavefront
 */
void affine_wavefronts_reduce_wavefront_offsets(
    affine_wavefronts_t* const affine_wavefronts,
    affine_wavefront_t* const wavefront,
    const int pattern_length,
    const int text_length,
    const int min_distance,
    const int max_distance_threshold,
    const int alignment_k) {
  // Parameters
  const awf_offset_t* const offsets = wavefront->offsets;
  int k;
  // Reduce from bottom
  const int top_limit = MIN(alignment_k-1,wavefront->hi);
  for (k=wavefront->lo;k<top_limit;++k) {
    const int distance = affine_wavefronts_compute_distance(pattern_length,text_length,offsets[k],k);
    if (distance - min_distance  <= max_distance_threshold) break;
    ++(wavefront->lo);
  }
  // Reduce from top
  const int botton_limit = MAX(alignment_k+1,wavefront->lo);
  for (k=wavefront->hi;k>botton_limit;--k) {
    const int distance = affine_wavefronts_compute_distance(pattern_length,text_length,offsets[k],k);
    if (distance - min_distance <= max_distance_threshold) break;
    --(wavefront->hi);
  }
  // Check hi/lo range
  if (wavefront->lo > wavefront->hi) {
    wavefront->null = true;
  }
}
void affine_wavefronts_reduce_wavefronts(
    affine_wavefronts_t* const affine_wavefronts,
    const int pattern_length,
    const int text_length,
    const int score) {
  // Parameters
  const int min_wavefront_length = affine_wavefronts->reduction.min_wavefront_length;
  const int max_distance_threshold = affine_wavefronts->reduction.max_distance_threshold;
  const int alignment_k = AFFINE_WAVEFRONT_DIAGONAL(text_length,pattern_length);
  // Fetch m-wavefront
  affine_wavefront_t* const mwavefront = affine_wavefronts->mwavefronts[score];
  if (mwavefront==NULL) return;
  if ((mwavefront->hi - mwavefront->lo + 1) < min_wavefront_length) return;
  // Compute min-distance
  const awf_offset_t* const offsets = mwavefront->offsets;
  int min_distance = MAX(pattern_length,text_length);
  int k;
  for (k=mwavefront->lo;k<=mwavefront->hi;++k) {
    const int distance = affine_wavefronts_compute_distance(pattern_length,text_length,offsets[k],k);
    min_distance = MIN(min_distance,distance);
  }
  // Reduce m-wavefront
  affine_wavefronts_reduce_wavefront_offsets(
      affine_wavefronts,mwavefront,pattern_length,text_length,
      min_distance,max_distance_threshold,alignment_k);
  // Reduce i-wavefront
  affine_wavefront_t* const iwavefront = affine_wavefronts->iwavefronts[score];
  if (iwavefront!=NULL) {
    if (mwavefront->lo > iwavefront->lo) iwavefront->lo = mwavefront->lo;
    if (mwavefront->hi < iwavefront->hi) iwavefront->hi = mwavefront->hi;
    if (iwavefront->lo > iwavefront->hi) iwavefront->null = true;
  }
  // Reduce d-wavefront
  affine_wavefront_t* const dwavefront = affine_wavefronts->dwavefronts[score];
  if (dwavefront!=NULL) {
    if (mwavefront->lo > dwavefront->lo) dwavefront->lo = mwavefront->lo;
    if (mwavefront->hi < dwavefront->hi) dwavefront->hi = mwavefront->hi;
    if (dwavefront->lo > dwavefront->hi) dwavefront->null = true;
  }
}

/*
 * Wavefront offset extension comparing characters
 */
void affine_wavefronts_extend(
    affine_wavefronts_t* const affine_wavefronts,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    const int score) {

  // Fetch m-wavefront
  affine_wavefront_t* const mwavefront = affine_wavefronts->mwavefronts[score];
  if (mwavefront==NULL) return;
  // Extend diagonally each wavefront point
  awf_offset_t* const offsets = mwavefront->offsets;

#if defined(__ARM_FEATURE_SVE)
  #pragma message("affine_wavefront_extend: ARM-SVE version")

  const int k_min = mwavefront->lo;
  const int k_max = mwavefront->hi;
  // Extend diagonally each wavefront point
  int k;

  uint64_t num_elems = svcntw();
  // fprintf(stderr, "num elements: %zu\n", num_elems);
  // fprintf(stderr, "text_len: %d, pattern_len=%d\n", text_length, pattern_length);

  // init mask
  //size_t wf_length = k_max - k_min;
  svbool_t mask;

  for (k=k_min;k<=k_max;k+=num_elems) {
    // Get number of elements that will be computed in this iteration
    //int active_elements = wf_length - k;
    svint32_t ks = svindex_s32(k, 1);
    mask = svcmple_s32(svptrue_b32(), ks, svdup_s32(k_max));
    svbool_t original_mask = mask;
    // fprintf(stderr, "Ks: "); print_svint32(ks);
    // fprintf(stderr, "FIRST MASK!! active_elements=%d\n", active_elements);

    svint32_t sv_offsets = svld1(mask, &offsets[k]);
    // fprintf(stderr, "Offsets: "); print_svint32(sv_offsets);

    bool svtest = svptest_any(svptrue_b32(), mask);

    while (svtest) {
        // fprintf(stderr, "Initial mask: "); print_mask(mask);
        // h = o
        svint32_t h_signed = sv_offsets;
        // v = o - k
        svint32_t v_signed = svsub_z(mask, sv_offsets, ks);

        // fprintf(stderr, "vs: "); print_svint32(v_signed);
        // fprintf(stderr, "hs: "); print_svint32(h_signed);
        // Cast to unsigned to do the shift
        svuint32_t v = svreinterpret_u32_s32(v_signed);
        svuint32_t h = svreinterpret_u32_s32(h_signed);
        // fprintf(stderr, "v: "); print_svuint32(v);
        // fprintf(stderr, "h: "); print_svuint32(h);

        svuint32_t bases_pattern = svld1_gather_s32offset_u32(mask, (uint32_t*)pattern, v_signed);
        bases_pattern = svrevb_u32_z(mask, bases_pattern);
        svuint32_t bases_text = svld1_gather_s32offset_u32(mask, (uint32_t*)text, h_signed);
        bases_text = svrevb_u32_z(mask, bases_text);
        // fprintf(stderr, "Pattern: "); print_svuint32_hex(bases_pattern);
        // fprintf(stderr, "Text: "); print_svuint32_hex(bases_text);
        // END TODO


        svuint32_t xor_result = sveor_u32_z(mask, bases_pattern, bases_text);
        // fprintf(stderr, "XOR: "); print_svuint32(xor_result);
        svuint32_t clz_res = svclz_u32_z(mask, xor_result);
        // fprintf(stderr, "CLZ: "); print_svuint32(clz_res);
        svuint32_t Eq = svlsr_u32_z(mask, clz_res, svdup_u32(3U));
        // fprintf(stderr, "Eq: "); print_svuint32(Eq);

        // Make sure we don't count beyond the sequence
        svuint32_t remaining_v = svsub_u32_z(mask, svdup_u32(pattern_length), v);
        svuint32_t remaining_h = svsub_u32_z(mask, svdup_u32(text_length), h);
        Eq = svmin_u32_z(mask, Eq, remaining_v);
        Eq = svmin_u32_z(mask, Eq, remaining_h);

        sv_offsets = svadd_s32_m(mask, sv_offsets, svreinterpret_s32(Eq));
        // fprintf(stderr, "Offsets: "); print_svint32(sv_offsets);

        // Only diagonals that have 4 elements equal (so they have not finished) will continue
        // fprintf(stderr, "Mask v0: "); print_mask(mask);
        mask = svcmpgt_n_u32(mask, Eq, 3U);

        // fprintf(stderr, "Mask v1: "); print_mask(mask);

        // v < pattern_length
        svbool_t mask_v = svcmplt_n_u32(mask, v, (uint32_t)pattern_length);
        // fprintf(stderr, "Mask v_len: "); print_mask(mask_v);
        mask = svand_b_z(mask, mask, mask_v);
        // h < text_length
        svbool_t mask_h = svcmplt_n_u32(mask, h, (uint32_t)text_length);
        // fprintf(stderr, "Mask v_len: "); print_mask(mask_h);
        mask = svand_b_z(mask, mask, mask_h);

        // v++ and h++
        v = svadd_n_u32_z(mask, v, 4);
        h = svadd_n_u32_z(mask, h, 4);

        svtest = svptest_any(svptrue_b32(), mask);
        // // fprintf(stderr, "Mask: "); print_mask(mask); fprintf(stderr, "svtest=%d\n", svtest);
    }
    svst1_s32(original_mask, &offsets[k], sv_offsets);
    // fprintf(stderr, "outside while!\n");
    //exit(1);
  }
  // fprintf(stderr, "\n");

#else
  #pragma message("affine_wavefront_extend: SCALAR version")
  // // Fetch m-wavefront
  // affine_wavefront_t* const mwavefront = affine_wavefronts->mwavefronts[score];
  // if (mwavefront==NULL) return;
  // // Extend diagonally each wavefront point
  // awf_offset_t* const offsets = mwavefront->offsets;
  int k;
  for (k=mwavefront->lo;k<=mwavefront->hi;++k) {
    // Exact extend
    const awf_offset_t offset = offsets[k];
    int v = AFFINE_WAVEFRONT_V(k,offset);
    int h = AFFINE_WAVEFRONT_H(k,offset);
    while (pattern[v++]==text[h++]) {
      ++(offsets[k]);
    }
  }
#endif
}

/*
 * Gap-Affine Wavefront exact extension
 */
void affine_wavefronts_extend_wavefront_packed(
    affine_wavefronts_t* const affine_wavefronts,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    const int score) {
  // Extend wavefront
  affine_wavefronts_extend(
      affine_wavefronts,pattern,pattern_length,
      text,text_length,score);
  // Reduce wavefront dynamically
  if (affine_wavefronts->reduction.reduction_strategy == wavefronts_reduction_dynamic) {
    affine_wavefronts_reduce_wavefronts(
        affine_wavefronts,pattern_length,
        text_length,score);
  }
}
