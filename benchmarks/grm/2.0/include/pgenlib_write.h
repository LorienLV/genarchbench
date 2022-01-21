#ifndef __PGENLIB_WRITE_H__
#define __PGENLIB_WRITE_H__

// This library is part of PLINK 2.00, copyright (C) 2005-2022 Shaun Purcell,
// Christopher Chang.
//
// This library is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 3 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.


// pgenlib_write contains writer-specific code.

#include "pgenlib_misc.h"

#ifdef __cplusplus
namespace plink2 {
#endif

typedef struct PgenWriterCommonStruct {
  // was marked noncopyable, but, well, gcc 9 caught me cheating (memcpying the
  // whole struct) in the multithreaded writer implementation.  So, copyable
  // now.
  uint32_t variant_ct;
  uint32_t sample_ct;
  PgenGlobalFlags phase_dosage_gflags;  // subset of gflags

  // there should be a single copy of these arrays shared by all threads.
  // allele_idx_offsets is read-only.
  uint64_t* vblock_fpos;
  unsigned char* vrec_len_buf;
  uintptr_t* vrtype_buf;
  const uintptr_t* allele_idx_offsets;
  uintptr_t* explicit_nonref_flags;  // usually nullptr

  // needed for multiallelic-phased case
  uintptr_t* genovec_hets_buf;

  STD_ARRAY_DECL(uint32_t, 4, ldbase_genocounts);

  // should match ftello() return value in singlethreaded case, but be set to
  // zero in multithreaded case
  uint64_t vblock_fpos_offset;

  // these must hold sample_ct entries
  // genovec_invert_buf also used as phaseinfo and dphase_present temporary
  // storage
  uintptr_t* genovec_invert_buf;
  uintptr_t* ldbase_genovec;

  // these must hold 2 * (sample_ct / kPglMaxDifflistLenDivisor) entries
  uintptr_t* ldbase_raregeno;
  uint32_t* ldbase_difflist_sample_ids;  // 1 extra entry, == sample_ct

  // this must fit 64k variants in multithreaded case
  unsigned char* fwrite_buf;
  unsigned char* fwrite_bufp;

  uint32_t ldbase_common_geno;  // UINT32_MAX if ldbase_genovec present
  uint32_t ldbase_difflist_len;

  // I'll cache this for now
  uintptr_t vrec_len_byte_ct;

  uint32_t vidx;
} PgenWriterCommon;

// Given packed arrays of unphased biallelic genotypes in uncompressed plink2
// binary format (00 = hom ref, 01 = het ref/alt1, 10 = hom alt1, 11 =
// missing), {Single,Multi}threaded_pgen_writer performs difflist (sparse
// variant), one bit (mostly-two-value), and LD compression before writing to
// disk, and backfills the header at the end.  CPRA -> CPR merging is under
// development.
// The major difference between the two interfaces is that
// Multithreaded_pgen_writer forces you to process large blocks of variants at
// a time (64k per thread).  So Singlethreaded_pgen_writer is still worth using
// in some cases (memory is very limited, I/O is slow, no programmer time to
// spare for the additional complexity).

typedef struct STPgenWriterStruct {
  NONCOPYABLE(STPgenWriterStruct);
#ifdef __cplusplus
  PgenWriterCommon& GET_PRIVATE_pwc() { return pwc; }
  PgenWriterCommon const& GET_PRIVATE_pwc() const { return pwc; }
  FILE*& GET_PRIVATE_pgen_outfile() { return pgen_outfile; }
  FILE* const& GET_PRIVATE_pgen_outfile() const { return pgen_outfile; }
 private:
#endif
  PgenWriterCommon pwc;
  FILE* pgen_outfile;
} STPgenWriter;

typedef struct MTPgenWriterStruct {
  NONCOPYABLE(MTPgenWriterStruct);
  FILE* pgen_outfile;
  uint32_t thread_ct;
  PgenWriterCommon* pwcs[];
} MTPgenWriter;

HEADER_INLINE const uintptr_t* SpgwGetAlleleIdxOffsets(STPgenWriter* spgwp) {
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  return pwcp->allele_idx_offsets;
}

HEADER_INLINE uint32_t SpgwGetVariantCt(STPgenWriter* spgwp) {
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  return pwcp->variant_ct;
}

HEADER_INLINE uint32_t SpgwGetSampleCt(STPgenWriter* spgwp) {
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  return pwcp->sample_ct;
}

HEADER_INLINE uint32_t SpgwGetVidx(STPgenWriter* spgwp) {
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  return pwcp->vidx;
}

void PreinitSpgw(STPgenWriter* spgwp);

// phase_dosage_gflags zero vs. nonzero is most important: this determines size
// of header.  Otherwise, setting more flags than necessary just increases
// memory requirements.
//
// nonref_flags_storage values:
//   0 = no info stored
//   1 = always trusted
//   2 = always untrusted
//   3 = use explicit_nonref_flags
//
// Caller is responsible for printing open-fail error message.
// In the multiallelic case, it is not necessary for the body of
// allele_idx_offsets to already be filled if known_max_allele_ct is provided;
// allele_idx_offsets[x] and [x+1] only have to be finalized before writing
// (0-based) variant #x.
// Conversely, if optional_max_allele_ct is set to 0, it will be computed from
// allele_idx_offsets.
// The body of explicit_nonref_flags also doesn't need to be filled until
// flush.
PglErr SpgwInitPhase1(const char* __restrict fname, const uintptr_t* __restrict allele_idx_offsets, uintptr_t* __restrict explicit_nonref_flags, uint32_t variant_ct, uint32_t sample_ct, uint32_t optional_max_allele_ct, PgenGlobalFlags phase_dosage_gflags, uint32_t nonref_flags_storage, STPgenWriter* spgwp, uintptr_t* alloc_cacheline_ct_ptr, uint32_t* max_vrec_len_ptr);

void SpgwInitPhase2(uint32_t max_vrec_len, STPgenWriter* spgwp, unsigned char* spgw_alloc);

// moderately likely that there isn't enough memory to use the maximum number
// of threads, so this returns per-thread memory requirements before forcing
// the caller to specify thread count
// (eventually should write code which falls back on STPgenWriter
// when there isn't enough memory for even a single 64k variant block, at least
// for the most commonly used plink 2.0 functions)
void MpgwInitPhase1(const uintptr_t* __restrict allele_idx_offsets, uint32_t variant_ct, uint32_t sample_ct, PgenGlobalFlags phase_dosage_gflags, uintptr_t* alloc_base_cacheline_ct_ptr, uint64_t* alloc_per_thread_cacheline_ct_ptr, uint32_t* vrec_len_byte_ct_ptr, uint64_t* vblock_cacheline_ct_ptr);

// Caller is responsible for printing open-fail error message.
PglErr MpgwInitPhase2(const char* __restrict fname, const uintptr_t* __restrict allele_idx_offsets, uintptr_t* __restrict explicit_nonref_flags, uint32_t variant_ct, uint32_t sample_ct, PgenGlobalFlags phase_dosage_gflags, uint32_t nonref_flags_storage, uint32_t vrec_len_byte_ct, uintptr_t vblock_cacheline_ct, uint32_t thread_ct, unsigned char* mpgw_alloc, MTPgenWriter* mpgwp);


// trailing bits of genovec must be zeroed out
void PwcAppendBiallelicGenovec(const uintptr_t* __restrict genovec, PgenWriterCommon* pwcp);

BoolErr SpgwFlush(STPgenWriter* spgwp);

HEADER_INLINE PglErr SpgwAppendBiallelicGenovec(const uintptr_t* __restrict genovec, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  PwcAppendBiallelicGenovec(genovec, pwcp);
  return kPglRetSuccess;
}

// trailing bits of raregeno must be zeroed out
// all raregeno entries assumed to be unequal to difflist_common_geno; the
// difflist should be compacted first if this isn't true
// difflist_len must be <= 2 * (sample_ct / kPglMaxDifflistLenDivisor);
// there's an assert checking this
void PwcAppendBiallelicDifflistLimited(const uintptr_t* __restrict raregeno, const uint32_t* __restrict difflist_sample_ids, uint32_t difflist_common_geno, uint32_t difflist_len, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendBiallelicDifflistLimited(const uintptr_t* __restrict raregeno, const uint32_t* __restrict difflist_sample_ids, uint32_t difflist_common_geno, uint32_t difflist_len, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  PwcAppendBiallelicDifflistLimited(raregeno, difflist_sample_ids, difflist_common_geno, difflist_len, pwcp);
  return kPglRetSuccess;
}

// Two interfaces for appending multiallelic hardcalls:
// 1. sparse: genovec, bitarray+values describing ref/altx hardcalls which
//    aren't ref/alt1, bitarray+values describing altx/alty hardcalls which
//    aren't alt1/alt1.
//    patch_01_vals[] contains one entry per relevant sample (only need altx
//    index), patch_10_vals[] contains two.
//    Ok if patch_01_ct == patch_10_ct == 0; in this case no aux1 track is
//    saved and bit 3 of vrtype is not set.  (Note that multiallelic dosage may
//    still be present when vrtype bit 3 is unset.)
// 2. generic dense: takes a length-2n array of AlleleCode allele codes.
//    Assumes [2k] <= [2k+1] for each k.  Instead of providing direct API
//    functions for this, we just provide a dense -> sparse helper function.
//
// All arrays should be vector-aligned.
BoolErr PwcAppendMultiallelicSparse(const uintptr_t* __restrict genovec, const uintptr_t* __restrict patch_01_set, const AlleleCode* __restrict patch_01_vals, const uintptr_t* __restrict patch_10_set, const AlleleCode* __restrict patch_10_vals, uint32_t patch_01_ct, uint32_t patch_10_ct, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendMultiallelicSparse(const uintptr_t* __restrict genovec, const uintptr_t* __restrict patch_01_set, const AlleleCode* __restrict patch_01_vals, const uintptr_t* __restrict patch_10_set, const AlleleCode* __restrict patch_10_vals, uint32_t patch_01_ct, uint32_t patch_10_ct, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  if (unlikely(PwcAppendMultiallelicSparse(genovec, patch_01_set, patch_01_vals, patch_10_set, patch_10_vals, patch_01_ct, patch_10_ct, pwcp))) {
    return kPglRetVarRecordTooLarge;
  }
  return kPglRetSuccess;
}

// This may not zero out trailing halfword of patch_{01,10}_set.
void PglMultiallelicDenseToSparse(const AlleleCode* __restrict wide_codes, uint32_t sample_ct, uintptr_t* __restrict genoarr, uintptr_t* __restrict patch_01_set, AlleleCode* __restrict patch_01_vals, uintptr_t* __restrict patch_10_set, AlleleCode* __restrict patch_10_vals, uint32_t* __restrict patch_01_ct_ptr, uint32_t* __restrict patch_10_ct_ptr);

// If remap is not nullptr, this simultaneously performs a rotation operation:
// wide_codes[2n] and [2n+1] are set to remap[geno[n]] rather than geno[n], and
// bit n of flipped (if flipped non-null) is set iff phase orientation is
// flipped (i.e. wide_codes[2n] was larger than wide_codes[2n+1] before the
// final reordering pass; caller needs to know this to properly update
// phaseinfo, dphase_delta, multidphase_delta).
// It currently assumes no present alleles are being mapped to 'missing'.
void PglMultiallelicSparseToDense(const uintptr_t* __restrict genovec, const uintptr_t* __restrict patch_01_set, const AlleleCode* __restrict patch_01_vals, const uintptr_t* __restrict patch_10_set, const AlleleCode* __restrict patch_10_vals, const AlleleCode* __restrict remap, uint32_t sample_ct, uint32_t patch_01_ct, uint32_t patch_10_ct, uintptr_t* __restrict flipped, AlleleCode* __restrict wide_codes);

// phasepresent == nullptr ok, that indicates that ALL heterozygous calls are
// phased.  Caller should use e.g. PwcAppendBiallelicGenovec() if it's known
// in advance that no calls are phased.
// Ok for phaseinfo to have bits set at non-het calls, NOT currently okay for
//   phasepresent
void PwcAppendBiallelicGenovecHphase(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, PgenWriterCommon* pwcp);

// phasepresent == nullptr ok
// ok for trailing bits of phaseinfo to not be zeroed out, NOT currently ok for
//   phasepresent
HEADER_INLINE PglErr SpgwAppendBiallelicGenovecHphase(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  PwcAppendBiallelicGenovecHphase(genovec, phasepresent, phaseinfo, pwcp);
  return kPglRetSuccess;
}

BoolErr PwcAppendMultiallelicGenovecHphase(const uintptr_t* __restrict genovec, const uintptr_t* __restrict patch_01_set, const AlleleCode* __restrict patch_01_vals, const uintptr_t* __restrict patch_10_set, const AlleleCode* __restrict patch_10_vals, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, uint32_t patch_01_ct, uint32_t patch_10_ct, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendMultiallelicGenovecHphase(const uintptr_t* __restrict genovec, const uintptr_t* __restrict patch_01_set, const AlleleCode* __restrict patch_01_vals, const uintptr_t* __restrict patch_10_set, const AlleleCode* __restrict patch_10_vals, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, uint32_t patch_01_ct, uint32_t patch_10_ct, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  if (unlikely(PwcAppendMultiallelicGenovecHphase(genovec, patch_01_set, patch_01_vals, patch_10_set, patch_10_vals, phasepresent, phaseinfo, patch_01_ct, patch_10_ct, pwcp))) {
    return kPglRetVarRecordTooLarge;
  }
  return kPglRetSuccess;
}


// dosage_main[] has length dosage_ct, not sample_ct
// ok for traling bits of dosage_present to not be zeroed out
BoolErr PwcAppendBiallelicGenovecDosage16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict dosage_present, const uint16_t* dosage_main, uint32_t dosage_ct, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendBiallelicGenovecDosage16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict dosage_present, const uint16_t* dosage_main, uint32_t dosage_ct, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  if (unlikely(PwcAppendBiallelicGenovecDosage16(genovec, dosage_present, dosage_main, dosage_ct, pwcp))) {
    return kPglRetVarRecordTooLarge;
  }
  return kPglRetSuccess;
}

BoolErr PwcAppendBiallelicGenovecHphaseDosage16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, const uintptr_t* __restrict dosage_present, const uint16_t* dosage_main, uint32_t dosage_ct, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendBiallelicGenovecHphaseDosage16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, const uintptr_t* __restrict dosage_present, const uint16_t* dosage_main, uint32_t dosage_ct, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  if (unlikely(PwcAppendBiallelicGenovecHphaseDosage16(genovec, phasepresent, phaseinfo, dosage_present, dosage_main, dosage_ct, pwcp))) {
    return kPglRetVarRecordTooLarge;
  }
  return kPglRetSuccess;
}

// dosage_present cannot be null for nonzero dosage_ct
// could make dosage_main[] has length dosage_ct + dphase_ct instead of having
// separate dphase_delta[]?
BoolErr PwcAppendBiallelicGenovecDphase16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, const uintptr_t* __restrict dosage_present, const uintptr_t* __restrict dphase_present, const uint16_t* dosage_main, const int16_t* dphase_delta, uint32_t dosage_ct, uint32_t dphase_ct, PgenWriterCommon* pwcp);

HEADER_INLINE PglErr SpgwAppendBiallelicGenovecDphase16(const uintptr_t* __restrict genovec, const uintptr_t* __restrict phasepresent, const uintptr_t* __restrict phaseinfo, const uintptr_t* __restrict dosage_present, const uintptr_t* dphase_present, const uint16_t* dosage_main, const int16_t* dphase_delta, uint32_t dosage_ct, uint32_t dphase_ct, STPgenWriter* spgwp) {
  if (unlikely(SpgwFlush(spgwp))) {
    return kPglRetWriteFail;
  }
  PgenWriterCommon* pwcp = &GET_PRIVATE(*spgwp, pwc);
  if (unlikely(PwcAppendBiallelicGenovecDphase16(genovec, phasepresent, phaseinfo, dosage_present, dphase_present, dosage_main, dphase_delta, dosage_ct, dphase_ct, pwcp))) {
    return kPglRetVarRecordTooLarge;
  }
  return kPglRetSuccess;
}

// Backfills header info, then closes the file.
PglErr SpgwFinish(STPgenWriter* spgwp);

// Last flush automatically backfills header info and closes the file.
// (caller should set mpgwp = nullptr after that)
PglErr MpgwFlush(MTPgenWriter* mpgwp);


// these close the file if open, but do not free any memory
// MpgwCleanup() handles mpgwp == nullptr, since it shouldn't be allocated on
// the stack
// error-return iff reterr was success and was changed to kPglRetWriteFail
// (i.e. an error message should be printed), though this is not relevant for
// plink2
BoolErr CleanupSpgw(STPgenWriter* spgwp, PglErr* reterrp);

BoolErr CleanupMpgw(MTPgenWriter* mpgwp, PglErr* reterrp);

#ifdef __cplusplus
}  // namespace plink2
#endif

#endif  // __PGENLIB_WRITE_H__
