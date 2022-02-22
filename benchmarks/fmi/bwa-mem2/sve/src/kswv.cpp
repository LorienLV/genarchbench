/*************************************************************************************
                           The MIT License

   BWA-MEM2  (Sequence alignment using Burrows-Wheeler Transform),
   Copyright (C) 2019  Intel Corporation, Heng Li.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

Authors: Vasimuddin Md <vasimuddin.md@intel.com>; Sanchit Misra <sanchit.misra@intel.com>
*****************************************************************************************/

#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "kswv.h"
#include "limits.h"


// ------------------------------------------------------------------------------------
// MACROs for vector code
#if MAINY
uint64_t prof[10][112];
#else
extern uint64_t prof[10][112];
#endif

#define AMBIG_ 4  // ambiguous base
// for 16 bit
#define DUMMY1_ 4
#define DUMMY2_ 5
#define DUMMY3 26
#define AMBR16 15
#define AMBQ16 16
// for 8-bit
#define DUMMY8 8
#define DUMMY5 5
#define AMBRQ 0xFF
#define AMBR 4
#define AMBQ 8


// -----------------------------------------------------------------------------------

// constructor
kswv::kswv(const int o_del, const int e_del, const int o_ins,
           const int e_ins, const int8_t w_match, const int8_t w_mismatch,
           int numThreads, int32_t maxRefLen = MAX_SEQ_LEN_REF_SAM,
           int32_t maxQerLen = MAX_SEQ_LEN_QER_SAM)
{

    this->m = 5;
    this->o_del = o_del;
    this->o_ins = o_ins;
    this->e_del = e_del;
    this->e_ins = e_ins;
    
    this->w_match    = w_match;
    this->w_mismatch = w_mismatch;
    this->w_open     = o_del;  // redundant, used in vector code.
    this->w_extend   = e_del;  // redundant, used in vector code.
    this->w_ambig    = DEFAULT_AMBIG;
    this->g_qmax = max_(w_match, w_mismatch);
    this->g_qmax = max_(this->g_qmax, w_ambig);

    this->maxRefLen = maxRefLen + 16;
    this->maxQerLen = maxQerLen + 16;
    
    this->swTicks = 0;
    setupTicks = 0;
    sort1Ticks = 0;
    swTicks = 0;
    sort2Ticks = 0;

    F16     = (int16_t *)_mm_malloc(this->maxQerLen * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
    H16_0   = (int16_t *)_mm_malloc(this->maxQerLen * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
    H16_1   = (int16_t *)_mm_malloc(this->maxQerLen * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
    H16_max = (int16_t *)_mm_malloc(this->maxQerLen * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
    rowMax16 = (int16_t *)_mm_malloc(this->maxRefLen * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);

    F8 = (uint8_t*) F16;
    H8_0 = (uint8_t*) H16_0;
    H8_1 = (uint8_t*) H16_1;
    H8_max = (uint8_t*) H16_max;
    rowMax8 = (uint8_t*) rowMax16;
}

// destructor 
kswv::~kswv() {
    _mm_free(F16); _mm_free(H16_0); _mm_free(H16_max); _mm_free(H16_1);
    _mm_free(rowMax16);
}

/**************************************Scalar code***************************************/
/* This is the original SW code from bwa-mem. We are keeping both, 8-bit and 16-bit 
   implementations, here for benchmarking purpose. 
   The interface call to the code is very simple and similar to the one we used above.
   By default the code is disabled.
 */

#if ORIGINAL_SW_SAM
void kswv::bwa_fill_scmat(int8_t mat[25]) {
    int a = this->w_match;
    int b = this->w_mismatch;
    int ambig = this->w_ambig;
    
    int i, j, k;
    for (i = k = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j)
            mat[k++] = i == j? a : b;
        mat[k++] = ambig; // ambiguous base
    }
    for (j = 0; j < 5; ++j) mat[k++] = ambig;
}

/**
 * Initialize the query data structure
 *
 * @param size   Number of bytes used to store a score; valid valures are 1 or 2
 * @param qlen   Length of the query sequence
 * @param query  Query sequence
 * @param m      Size of the alphabet
 * @param mat    Scoring matrix in a one-dimension array
 *
 * @return       Query data structure
 */
kswq_t* kswv::ksw_qinit(int size, int qlen, uint8_t *query, int m, const int8_t *mat)
{
    kswq_t *q;
    int slen, a, tmp, p;

    size = size > 1? 2 : 1;
    p = 8 * (3 - size); // # values per __m128i
    slen = (qlen + p - 1) / p; // segmented length
    q = (kswq_t*)malloc(sizeof(kswq_t) + 256 + 16 * slen * (m + 4)); // a single block of memory
    q->qp = (__m128i*)(((size_t)q + sizeof(kswq_t) + 15) >> 4 << 4); // align memory
    q->H0 = q->qp + slen * m;
    q->H1 = q->H0 + slen;
    q->E  = q->H1 + slen;
    q->Hmax = q->E + slen;
    q->slen = slen; q->qlen = qlen; q->size = size;
    // compute shift
    tmp = m * m;
    for (a = 0, q->shift = 127, q->mdiff = 0; a < tmp; ++a) { // find the minimum and maximum score
        if (mat[a] < (int8_t)q->shift) q->shift = mat[a];
        if (mat[a] > (int8_t)q->mdiff) q->mdiff = mat[a];
    }
    
    q->max = q->mdiff;
    q->shift = 256 - q->shift; // NB: q->shift is uint8_t
    q->mdiff += q->shift; // this is the difference between the min and max scores

    // An example: p=8, qlen=19, slen=3 and segmentation:
    //  {{0,3,6,9,12,15,18,-1},{1,4,7,10,13,16,-1,-1},{2,5,8,11,14,17,-1,-1}}
    if (size == 1) {
        int8_t *t = (int8_t*)q->qp;
        for (a = 0; a < m; ++a) {
            int i, k, nlen = slen * p;
            const int8_t *ma = mat + a * m;
            for (i = 0; i < slen; ++i)
                for (k = i; k < nlen; k += slen) // p iterations
                    *t++ = (k >= qlen? 0 : ma[query[k]]) + q->shift;
        }
    } else {
        int16_t *t = (int16_t*)q->qp;
        for (a = 0; a < m; ++a) {
            int i, k, nlen = slen * p;
            const int8_t *ma = mat + a * m;
            for (i = 0; i < slen; ++i)
                for (k = i; k < nlen; k += slen) // p iterations
                    *t++ = (k >= qlen? 0 : ma[query[k]]);
        }
    }
    return q;
}

kswr_t kswv::kswvScalar_u8(kswq_t *q, int tlen, const uint8_t *target,
                          int _o_del, int _e_del, int _o_ins, int _e_ins,
                          int xtra) // the first gap costs -(_o+_e)
{
    int slen, i, m_b, n_b, te = -1, gmax = 0, minsc, endsc;
    uint64_t *b;
    __m128i zero, oe_del, e_del, oe_ins, e_ins, shift, *H0, *H1, *E, *Hmax;
    kswr_t r;

#define __max_16(ret, xx) do { \
        (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 8)); \
        (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 4)); \
        (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 2)); \
        (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 1)); \
        (ret) = _mm_extract_epi16((xx), 0) & 0x00ff;        \
    } while (0)
    
    // initialization
    r = g_defr;
    minsc = (xtra & KSW_XSUBO)? xtra & 0xffff : 0x10000;
    endsc = (xtra & KSW_XSTOP)? xtra & 0xffff : 0x10000;
    m_b = n_b = 0; b = 0;   zero = _mm_set1_epi32(0);
    oe_del = _mm_set1_epi8(_o_del + _e_del);
    e_del = _mm_set1_epi8(_e_del);
    oe_ins = _mm_set1_epi8(_o_ins + _e_ins);
    e_ins = _mm_set1_epi8(_e_ins);
    shift = _mm_set1_epi8(q->shift);
    H0 = q->H0; H1 = q->H1; E = q->E; Hmax = q->Hmax;
    slen = q->slen;
    for (i = 0; i < slen; ++i) {
        _mm_store_si128(E + i, zero);
        _mm_store_si128(H0 + i, zero);
        _mm_store_si128(Hmax + i, zero);
    }
    // the core loop
    for (i = 0; i < tlen; ++i) {
        int j, k, cmp, imax;
        __m128i e, h, t, f = zero, max = zero, *S = q->qp + target[i] * slen; // s is the 1st score vector
        h = _mm_load_si128(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
        h = _mm_slli_si128(h, 1); // h=H(i-1,-1); << instead of >> because x64 is little-endian
        for (j = 0; LIKELY(j < slen); ++j) {
            /* SW cells are computed in the following order:
             *   H(i,j)   = max{H(i-1,j-1)+S(i,j), E(i,j), F(i,j)}
             *   E(i+1,j) = max{H(i,j)-q, E(i,j)-r}
             *   F(i,j+1) = max{H(i,j)-q, F(i,j)-r}
             */
            // compute H'(i,j); note that at the beginning, h=H'(i-1,j-1)
            h = _mm_adds_epu8(h, _mm_load_si128(S + j));
            h = _mm_subs_epu8(h, shift); // h=H'(i-1,j-1)+S(i,j)
            e = _mm_load_si128(E + j); // e=E'(i,j)
            h = _mm_max_epu8(h, e);
            h = _mm_max_epu8(h, f); // h=H'(i,j)
            max = _mm_max_epu8(max, h); // set max
            _mm_store_si128(H1 + j, h); // save to H'(i,j)
            // now compute E'(i+1,j)
            e = _mm_subs_epu8(e, e_del); // e=E'(i,j) - e_del
            t = _mm_subs_epu8(h, oe_del); // h=H'(i,j) - o_del - e_del
            e = _mm_max_epu8(e, t); // e=E'(i+1,j)
            _mm_store_si128(E + j, e); // save to E'(i+1,j)
            // now compute F'(i,j+1)
            f = _mm_subs_epu8(f, e_ins);
            t = _mm_subs_epu8(h, oe_ins); // h=H'(i,j) - o_ins - e_ins
            f = _mm_max_epu8(f, t);
            // get H'(i-1,j) and prepare for the next j
            h = _mm_load_si128(H0 + j); // h=H'(i-1,j)
        }
        // NB: we do not need to set E(i,j) as we disallow adjecent insertion and then deletion
        for (k = 0; LIKELY(k < 16); ++k) { // this block mimics SWPS3; NB: H(i,j) updated in the lazy-F loop cannot exceed max
            f = _mm_slli_si128(f, 1);
            for (j = 0; LIKELY(j < slen); ++j) {
                h = _mm_load_si128(H1 + j);
                h = _mm_max_epu8(h, f); // h=H'(i,j)
                _mm_store_si128(H1 + j, h);
                h = _mm_subs_epu8(h, oe_ins);
                f = _mm_subs_epu8(f, e_ins);
                cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_subs_epu8(f, h), zero));
                if (UNLIKELY(cmp == 0xffff)) goto end_loop16;
            }
        }
end_loop16:
        //int k;for (k=0;k<16;++k)printf("%d ", ((uint8_t*)&max)[k]);printf("\n");
        __max_16(imax, max); // imax is the maximum number in max
        if (imax >= minsc) { // write the b array; this condition adds branching unfornately
            if (n_b == 0 || (int32_t)b[n_b-1] + 1 != i) { // then append
                if (n_b == m_b) {
                    m_b = m_b? m_b<<1 : 8;
                    b = (uint64_t*) realloc (b, 8 * m_b);
                }
                b[n_b++] = (uint64_t)imax<<32 | i;
            } else if ((int)(b[n_b-1]>>32) < imax) b[n_b-1] = (uint64_t)imax<<32 | i; // modify the last
        }
        if (imax > gmax) {
            gmax = imax; te = i; // te is the end position on the target
            for (j = 0; LIKELY(j < slen); ++j) // keep the H1 vector
                _mm_store_si128(Hmax + j, _mm_load_si128(H1 + j));
            if (gmax + q->shift >= 255 || gmax >= endsc) break;
        }
        S = H1; H1 = H0; H0 = S; // swap H0 and H1
    }
    r.score = gmax + q->shift < 255? gmax : 255;
    r.te = te;
    if (r.score != 255) { // get a->qe, the end of query match; find the 2nd best score
        int max = -1, tmp, low, high, qlen = slen * 16;
        uint8_t *t = (uint8_t*) Hmax;
        for (i = 0; i < qlen; ++i, ++t)
            if ((int)*t > max) max = *t, r.qe = i / 16 + i % 16 * slen;
            else if ((int)*t == max && (tmp = i / 16 + i % 16 * slen) < r.qe) r.qe = tmp; 
        if (b) {
            assert(q->max != 0);
            i = (r.score + q->max - 1) / q->max;
            low = te - i; high = te + i;
            for (i = 0; i < n_b; ++i) {
                int e = (int32_t)b[i];
                if ((e < low || e > high) && (int)(b[i]>>32) > r.score2)
                    r.score2 = b[i]>>32, r.te2 = e;
            }
        }
    }
    
#if MAXI
    fprintf(stderr, "score: %d, te: %d, qe: %d, score2: %d, te2: %d\n",
            r.score, r.te, r.qe, r.score2, r.te2);
#endif
    
    free(b);
    return r;
}

kswr_t kswv::kswvScalar_i16(kswq_t *q, int tlen, const uint8_t *target,
                            int _o_del, int _e_del, int _o_ins, int _e_ins,
                            int xtra) // the first gap costs -(_o+_e)
{
    int slen, i, m_b, n_b, te = -1, gmax = 0, minsc, endsc;
    uint64_t *b;
    __m128i zero, oe_del, e_del, oe_ins, e_ins, *H0, *H1, *E, *Hmax;
    kswr_t r;
#define SIMD16 8

#define __max_8(ret, xx) do { \
        (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 8)); \
        (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 4)); \
        (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 2)); \
        (ret) = _mm_extract_epi16((xx), 0); \
    } while (0)

    // initialization
    r = g_defr;
    minsc = (xtra&KSW_XSUBO)? xtra&0xffff : 0x10000;
    endsc = (xtra&KSW_XSTOP)? xtra&0xffff : 0x10000;
    m_b = n_b = 0; b = 0;
    zero = _mm_set1_epi32(0);
    oe_del = _mm_set1_epi16(_o_del + _e_del);
    e_del = _mm_set1_epi16(_e_del);
    oe_ins = _mm_set1_epi16(_o_ins + _e_ins);
    e_ins = _mm_set1_epi16(_e_ins);
    H0 = q->H0; H1 = q->H1; E = q->E; Hmax = q->Hmax;
    slen = q->slen;
    for (i = 0; i < slen; ++i) {
        _mm_store_si128(E + i, zero);
        _mm_store_si128(H0 + i, zero);
        _mm_store_si128(Hmax + i, zero);
    }
    // the core loop
    for (i = 0; i < tlen; ++i) {
        int j, k, imax;
        __m128i e, t, h, f = zero, max = zero, *S = q->qp + target[i] * slen; // s is the 1st score vector
        h = _mm_load_si128(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
        h = _mm_slli_si128(h, 2);
        for (j = 0; LIKELY(j < slen); ++j) {

            h = _mm_adds_epi16(h, *S++);
            e = _mm_load_si128(E + j);
            h = _mm_max_epi16(h, e);
            h = _mm_max_epi16(h, f);
            max = _mm_max_epi16(max, h);
            _mm_store_si128(H1 + j, h);
            e = _mm_subs_epu16(e, e_del);
            t = _mm_subs_epu16(h, oe_del);
            e = _mm_max_epi16(e, t);
            _mm_store_si128(E + j, e);
            f = _mm_subs_epu16(f, e_ins);
            t = _mm_subs_epu16(h, oe_ins);
            f = _mm_max_epi16(f, t);
            h = _mm_load_si128(H0 + j);
        }
        
        for (k = 0; LIKELY(k < 16); ++k) {
            f = _mm_slli_si128(f, 2);
            for (j = 0; LIKELY(j < slen); ++j) {
                h = _mm_load_si128(H1 + j);
                h = _mm_max_epi16(h, f);
                _mm_store_si128(H1 + j, h);
                h = _mm_subs_epu16(h, oe_ins);
                f = _mm_subs_epu16(f, e_ins);
                if(UNLIKELY(!_mm_movemask_epi8(_mm_cmpgt_epi16(f, h)))) goto end_loop8;
            }
        }
        
end_loop8:
        __max_8(imax, max);
        if (imax >= minsc) {
            if (n_b == 0 || (int32_t)b[n_b-1] + 1 != i) {
                if (n_b == m_b) {
                    m_b = m_b? m_b<<1 : 8;
                    b = (uint64_t*)realloc(b, 8 * m_b);
                }
                b[n_b++] = (uint64_t)imax<<32 | i;
            } else if ((int)(b[n_b-1]>>32) < imax) b[n_b-1] = (uint64_t)imax<<32 | i; // modify the last
        }
        if (imax > gmax) {
            gmax = imax; te = i;
            for (j = 0; LIKELY(j < slen); ++j)
                _mm_store_si128(Hmax + j, _mm_load_si128(H1 + j));
            if (gmax >= endsc) break;
        }
        S = H1; H1 = H0; H0 = S;
    }
    
    r.score = gmax; r.te = te;
    {
        int max = -1, tmp, low, high, qlen = slen * 8;
        uint16_t *t = (uint16_t*)Hmax;
        for (i = 0, r.qe = -1; i < qlen; ++i, ++t)
            if ((int)*t > max) max = *t, r.qe = i / 8 + i % 8 * slen;
            else if ((int)*t == max && (tmp = i / 8 + i % 8 * slen) < r.qe) r.qe = tmp; 
        if (b) {
            assert(q->max != 0);
            i = (r.score + q->max - 1) / q->max;
            low = te - i; high = te + i;
            for (i = 0; i < n_b; ++i) {
                int e = (int32_t)b[i];
                if ((e < low || e > high) && (int)(b[i]>>32) > r.score2)
                    r.score2 = b[i]>>32, r.te2 = e;
            }
        }
    }

    free(b);
    return r;
}

// -------------------------------------------------------------
// kswc scalar, wrapper function, the interface.
//-------------------------------------------------------------
void kswv::kswvScalarWrapper(SeqPair *seqPairArray,
                             uint8_t *seqBufRef,
                             uint8_t *seqBufQer,
                             kswr_t* aln,
                             int numPairs,
                             int nthreads,
                             bool sw, int tid) {
    
    int8_t mat[25];
    bwa_fill_scmat(mat);

    int st = 0, ed = numPairs;    
    for (int i = st; i < ed; i++)
    {
        SeqPair *p = seqPairArray + i;
        kswr_t *myaln = aln + p->regid;
            
        uint8_t *target = seqBufRef + p->idr;
        uint8_t *query = seqBufQer + p->idq;

        int tlen = p->len1;
        int qlen = p->len2;
        int xtra = p->h0;

        kswq_t *q;
        kswr_t ks;
            
        int vw = (xtra & KSW_XBYTE)? 1 : 2;
        if (sw == 0) {
            assert(vw == 1);
            q = ksw_qinit((xtra & KSW_XBYTE)? 1 : 2, qlen, query, this->m, mat);
            ks = kswvScalar_u8(q, tlen, target,
                               o_del, e_del,
                               o_ins, e_ins,
                               xtra, tid, i);
            free(q);
        } else {
            assert(vw == 2);
            q = ksw_qinit(2, qlen, query, this->m, mat);
            ks = kswvScalar_i16(q, tlen, target,
                                o_del, e_del,
                                o_ins, e_ins,
                                xtra);
            free(q);
        }

        myaln->score = ks.score;
        myaln->tb = ks.tb;
        myaln->te = ks.te;
        myaln->qb = ks.qb;
        myaln->qe = ks.qe;
        myaln->score2 = ks.score2;
        myaln->te2 = ks.te2;
    }

}
#endif

/************************** standalone benchmark code *************************************/
/** This is a standalone code for the benchmarking purpose 
    By default it is disabled.
**/

#if MAINY
#define DEFAULT_MATCH 1
#define DEFAULT_MISMATCH -4
#define DEFAULT_OPEN 6
#define DEFAULT_EXTEND 1
#define DEFAULT_AMBIG -1

// #define MAX_NUM_PAIRS 1000
// #define MATRIX_MIN_CUTOFF -100000000
// #define LOW_INIT_VALUE (INT32_MIN/2)
// #define AMBIG 52

int spot = 42419;
double freq = 2.3*1e9;
int32_t w_match, w_mismatch, w_open, w_extend, w_ambig;
uint64_t SW_cells;
char *pairFileName;
FILE *pairFile;
int8_t h0 = 0;
double clock_freq;
//uint64_t prof[10][112], data, SW_cells2;


void parseCmdLine(int argc, char *argv[])
{
    int i;
    w_match = DEFAULT_MATCH;
    w_mismatch = DEFAULT_MISMATCH;
    w_open = DEFAULT_OPEN;
    w_extend = DEFAULT_EXTEND;
    w_ambig = DEFAULT_AMBIG;
    
    int pairFlag = 0;
    for(i = 1; i < argc; i+=2)
    {
        if(strcmp(argv[i], "-match") == 0)
        {
            w_match = atoi(argv[i + 1]);
        }
        if(strcmp(argv[i], "-mismatch") == 0) //penalty, +ve number
        {
            w_mismatch = atoi(argv[i + 1]);
        }
        if(strcmp(argv[i], "-ambig") == 0)
        {
            w_ambig = atoi(argv[i + 1]);
        }

        if(strcmp(argv[i], "-gapo") == 0)
        {
            w_open = atoi(argv[i + 1]);
        }
        if(strcmp(argv[i], "-gape") == 0)
        {
            w_extend = atoi(argv[i + 1]);
        }
        if(strcmp(argv[i], "-pairs") == 0)
        {
            pairFileName = argv[i + 1];
            pairFlag = 1;
        }
        if(strcmp(argv[i], "-h0") == 0)
        {
            h0 = atoi(argv[i + 1]);
        }
    }
    if(pairFlag == 0)
    {
        printf("ERROR! pairFileName not specified.\n");
        exit(EXIT_FAILURE);
    }
}

int loadPairs(SeqPair *seqPairArray, uint8_t *seqBufRef, uint8_t* seqBufQer, FILE *pairFile)
{
    static int32_t cnt = 0;
    int32_t numPairs = 0;
    while(numPairs < MAX_NUM_PAIRS_ALLOC)
    {
        int32_t xtra = 0;
        char temp[10];
        fgets(temp, 10, pairFile);
        sscanf(temp, "%x", &xtra);
        //printf("xtra: %d, %x, %s\n", xtra, xtra, temp);

        //if(!fgets((char *)(seqBuf + numPairs * 2 * MAX_SEQ_LEN), MAX_SEQ_LEN, pairFile))
        if(!fgets((char *)(seqBufRef + numPairs * this->maxRefLen), this->maxRefLen, pairFile))
        {
            break;
        }
        //if(!fgets((char *)(seqBuf + (numPairs * 2 + 1) * MAX_SEQ_LEN), MAX_SEQ_LEN, pairFile))
        if(!fgets((char *)(seqBufQer + numPairs * this->maxQerLen), this->maxQerLen, pairFile)) 
        {
            printf("ERROR! Odd number of sequences in %s\n", pairFileName);
            break;
        }

        SeqPair sp;
        sp.id = numPairs;
        // sp.seq1 = seqBuf + numPairs * 2 * MAX_SEQ_LEN;
        // sp.seq2 = seqBuf + (numPairs * 2 + 1) * MAX_SEQ_LEN;
        sp.len1 = strnlen((char *)(seqBufRef + numPairs * this->maxRefLen), this->maxRefLen) - 1;
        sp.len2 = strnlen((char *)(seqBufQer + numPairs * this->maxQerLen), this->maxQerLen) - 1;
        sp.h0 = xtra;
        // sp.score = 0;

        //uint8_t *seq1 = seqBuf + numPairs * 2 * MAX_SEQ_LEN;
        //uint8_t *seq2 = seqBuf + (numPairs * 2 + 1) * MAX_SEQ_LEN;
        uint8_t *seq1 = seqBufRef + numPairs * this->maxRefLen;
        uint8_t *seq2 = seqBufQer + numPairs * this->maxQerLen;

        for (int l=0; l<sp.len1; l++)
            seq1[l] -= 48;
        for (int l=0; l<sp.len2; l++)
            seq2[l] -= 48;

        seqPairArray[numPairs] = sp;
        numPairs++;
        // SW_cells += (sp.len1 * sp.len2);
    }
    // fclose(pairFile);
    return numPairs;
}

// profiling stats
uint64_t find_stats(uint64_t *val, int nt, double &min, double &max, double &avg)
{
    min = 1e10;
    max = 0;
    avg = 0;
    for (int i=0; i<nt; i++) {
        avg += val[i];
        if (max < val[i]) max = val[i];
        if (min > val[i]) min = val[i];     
    }
    avg /= nt;
}

int main(int argc, char *argv[])
{

#ifdef VTUNE_ANALYSIS
    printf("Vtune analysis enabled....\n");
    __itt_pause();
#endif
    FILE *fsam = fopen("fsam.txt", "w");    
    kswv *mate;

    parseCmdLine(argc, argv);

    SeqPair *seqPairArray = (SeqPair *)_mm_malloc((MAX_NUM_PAIRS + SIMD_WIDTH8) * sizeof(SeqPair), 64);
    uint8_t *seqBufRef = NULL, *seqBufQer = NULL;
    seqBufRef = (uint8_t *)_mm_malloc((this->maxRefLen * MAX_NUM_PAIRS + MAX_LINE_LEN)
                                      * sizeof(int8_t), 64);
    seqBufQer = (uint8_t *)_mm_malloc((this->maxQerLen * MAX_NUM_PAIRS + MAX_LINE_LEN)
                                      * sizeof(int8_t), 64);

    kswr_t *aln = NULL;
    aln = (kswr_t *) _mm_malloc ((MAX_NUM_PAIRS + SIMD_WIDTH8) * sizeof(kswr_t), 64);
    
    if (seqBufRef == NULL || seqBufQer == NULL || aln == NULL)
    {
        printf("Memory not allocated\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Memory allocated: %0.2lf MB\n",
               ((int64_t)(this->maxRefLen + this->maxQerLen ) * MAX_NUM_PAIRS + MAX_LINE_LEN)/1e6);
    }
    uint64_t tim = __rdtsc(), readTim = 0;
    
    int32_t numThreads = 1;
#pragma omp parallel
    {
        int32_t tid = omp_get_thread_num();
        int32_t nt = omp_get_num_threads();
        if(tid == (nt - 1))
        {
            numThreads = nt;
        }
    }
    numThreads =1 ;
    //printf("Done reading input file!!, numPairs: %d, nt: %d\n",
    //     numPairs, numThreads);

#if SORT_PAIRS     // disbaled in bwa-mem2 (only used in separate benchmark sw code)
    printf("\tSorting is enabled !!\n");
#endif

#if MAXI
    printf("\tResults printing (on console) is enabled.....\n");
#endif

    tim = __rdtsc();
    sleep(1);
    freq = __rdtsc() - tim;

    int numPairs = 0, totNumPairs = 0;
    FILE *pairFile = fopen(pairFileName, "r");
    if(pairFile == NULL)
    {
        fprintf(stderr, "Could not open file: %s\n", pairFileName);
        exit(EXIT_FAILURE);
    }

    // BandedPairWiseSW *pwsw = new BandedPairWiseSW(w_match, w_mismatch, w_open,
    //                                            w_extend, w_ambig, end_bonus);
    kswv *pwsw = new kswv(w_open, w_extend, w_open, w_extend, w_match, w_mismatch, numThreads);
    

    int64_t myTicks = 0;
    printf("Processor freq: %0.2lf MHz\n", freq/1e6);
    printf("Executing int8 code....\n");
#if VEC
    printf("Executing AVX512 vectorized code!!\n");
    
    while (1) {
        uint64_t tim = __rdtsc();

        //printf("Loading current batch of pairs..\n");
        numPairs = loadPairs(seqPairArray, seqBufRef, seqBufQer, pairFile);
        totNumPairs += numPairs;
#if STAT
        printf("Loading done, numPairs: %d\n", numPairs);      
        if (totNumPairs > spot) spot -= totNumPairs - numPairs;
        else continue;
#endif
        readTim += __rdtsc() - tim;
        if (numPairs == 0) break;

        tim = __rdtsc();
        int phase = 0;
        pwsw->getScores8(seqPairArray, seqBufRef, seqBufQer, aln, numPairs, numThreads, phase);
        // pwsw->getScores16(seqPairArray, seqBufRef, seqBufQer, aln, numPairs, numThreads, phase);
        myTicks += __rdtsc() - tim;

#if STAT
        for (int l=0; l<10; l++)
        {
            // SeqPair r = seqPairArray[l];
            kswr_t r = aln[l];
            fprintf(stderr, "%d %d %d %d %d %d %d\n", r.score, r.tb, r.te, r.qb, r.qe, r.score2, r.te2);
        }       
        break;
#endif
        
#if MAXI
        // printf("Execution complete!!, writing output!\n");
        //for (int l=0; l<numPairs; l++)
        //{
        //  // SeqPair r = seqPairArray[l];
        //  kswr_t r = aln[l];
        //  fprintf(stderr, "%d %d %d %d %d %d %d\n", r.score, r.tb, r.te, r.qb, r.qe, r.score2, r.te2);
        //}
        // printf("Vector code: Writing output completed!!!\n\n");
        // printf("Vector code -- Wrote output to the file\n");
#endif
    }
    
    // int64_t myTicks = 0;//pwsw->getTicks();
    printf("Read time  = %0.2lf\n", readTim/freq);
    printf("Overall SW cycles = %ld, %0.2lf\n", myTicks, myTicks*1.0/freq);
    printf("SW cells(T)  = %ld\n", SW_cells);
    printf("SW cells(||)  = %ld\n", SW_cells2);
    printf("SW GCUPS  = %lf\n", SW_cells * freq / myTicks);
    
    {
        printf("More stats:\n");
        // double freq = 2.3*1e9;
        double min, max, avg;
        find_stats(prof[1], numThreads, min, max, avg);
        printf("Time in pre-processing: %0.2lf (%0.2lf, %0.2lf)\n",
               avg*1.0/freq, min*1.0/freq, max*1.0/freq);
        find_stats(prof[0], numThreads, min, max, avg);
        printf("Time spent in smithWaterman(): %0.2lf (%0.2lf, %0.2lf)\n",
               avg*1.0/freq, min*1.0/freq, max*1.0/freq);
        printf("\nTotal cells computed: %ld, %0.2lf\n",
               prof[2][0], prof[2][0]*1.0/(totNumPairs));
        printf("\tTotal useful cells computed: %ld, %0.2lf\n",
               prof[3][0], prof[3][0]*1.0/(totNumPairs));
        // printf("Total bp read from memory: %ld\n", data);
        printf("Computations after exit: %ld\n", prof[4][0]);
        printf("Cumulative seq1/2 len: %ld, %ld\n", prof[4][1], prof[4][2]);
    
#if 1
        printf("\nDebugging info:\n");
        printf("Time taken for DP loop: %0.2lf\n", prof[DP][0]*1.0/freq);
        printf("Time taken for DP loop upper part: %0.2lf\n", prof[DP3][0]*1.0/freq);   
        printf("Time taken for DP inner loop: %0.2lf\n", prof[DP1][0]*1.0/freq);
        printf("Time taken for DP loop lower part: %ld\n", prof[DP2][0]);
#endif
    }

    
#else
    printf("Executing scalar code!!\n");
    while (1) {
        uint64_t tim = __rdtsc();
        numPairs = loadPairs(seqPairArray, seqBufRef, seqBufQer, pairFile);
        totNumPairs += numPairs;
#if STAT
        printf("Loading done, numPairs: %d\n", numPairs);
        if (totNumPairs > spot) spot -= totNumPairs - numPairs;
        else continue;
#endif
        readTim += __rdtsc() - tim;
        if (numPairs == 0) break;

        tim = __rdtsc();
        pwsw->kswvScalarWrapper(seqPairArray,
                               seqBufRef,
                               seqBufQer,
                               aln,
                               numPairs,
                               numThreads);
        myTicks += __rdtsc() - tim;
#if STAT
        for (int l=0; l<10; l++)
        {
            // SeqPair r = seqPairArray[l];
            kswr_t r = aln[l];
            fprintf(stderr, "%d %d %d %d %d %d %d\n", r.score, r.tb, r.te, r.qb, r.qe, r.score2, r.te2);
        }       
        break;
#endif
        
    } // while
    
    printf("Read time  = %0.2lf\n", readTim/freq);
    printf("Overall SW cycles = %ld, %0.2lf\n", myTicks, myTicks*1.0/freq);
    printf("Time taken for DP loop lower part: %ld \n", prof[DP2][0]);

#if 1
        printf("\nDebugging info:\n");
        printf("Time taken for DP loop: %0.2lf\n", prof[DP][0]*1.0/freq);
        printf("Time taken for DP loop upper part: %0.2lf\n", prof[DP3][0]*1.0/freq);   
        printf("Time taken for DP inner loop: %0.2lf\n", prof[DP1][0]*1.0/freq);
        printf("Time taken for DP loop lower part: %ld\n", prof[DP2][0]);
#endif

#endif


#ifdef VTUNE_ANALYSIS
    printf("Vtune analysis enabled....\n");
#endif
        
    // free memory
    _mm_free(seqPairArray);
    _mm_free(seqBufRef);
    _mm_free(seqBufQer);
    _mm_free(aln);
    
    fclose(pairFile);
    fclose(fsam);
    return 1;
}
#endif  // MAINY
