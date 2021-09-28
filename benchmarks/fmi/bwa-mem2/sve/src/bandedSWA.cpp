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

Authors: Vasimuddin Md <vasimuddin.md@intel.com>; Sanchit Misra <sanchit.misra@intel.com>;
*****************************************************************************************/

#include "omp.h"
#include "bandedSWA.h"
#ifdef VTUNE_ANALYSIS
#include <ittnotify.h> 
#endif

#if defined(__clang__) || defined(__GNUC__)
#define __mmask8 uint8_t
#define __mmask16 uint16_t
#define __mmask32 uint32_t
#endif

// ------------------------------------------------------------------------------------
// MACROs for vector code
extern uint64_t prof[10][112];
#define AMBIG 4
#define DUMMY1 99
#define DUMMY2 100

//-----------------------------------------------------------------------------------
// constructor
BandedPairWiseSW::BandedPairWiseSW(const int o_del, const int e_del, const int o_ins,
								   const int e_ins, const int zdrop,
								   const int end_bonus, const int8_t *mat_,
								   const int8_t w_match, const int8_t w_mismatch, int numThreads)
{
	mat = mat_;
	this->m = 5;
	this->end_bonus = end_bonus;
	this->zdrop = zdrop;
	this->o_del = o_del;
	this->o_ins = o_ins;
	this->e_del = e_del;
	this->e_ins = e_ins;
	
	this->w_match	 = w_match;
	this->w_mismatch = w_mismatch*-1;
	this->w_open	 = o_del;  // redundant, used in vector code.
	this->w_extend	 = e_del;  // redundant, used in vector code.
	this->w_ambig	 = DEFAULT_AMBIG;
	this->swTicks = 0;
	this->SW_cells = 0;
	setupTicks = 0;
	sort1Ticks = 0;
	swTicks = 0;
	sort2Ticks = 0;
	this->F8_ = this->H8_  = this->H8__ = NULL;
	this->F16_ = this->H16_  = this->H16__ = NULL;
	
	F8_ = H8_ = H8__ = NULL;
    F8_ = (int8_t *)_mm_malloc(MAX_SEQ_LEN8 * SIMD_WIDTH8 * numThreads * sizeof(int8_t), 64);
    H8_ = (int8_t *)_mm_malloc(MAX_SEQ_LEN8 * SIMD_WIDTH8 * numThreads * sizeof(int8_t), 64);
	H8__ = (int8_t *)_mm_malloc(MAX_SEQ_LEN8 * SIMD_WIDTH8 * numThreads * sizeof(int8_t), 64);

	F16_ = H16_ = H16__ = NULL;
    F16_ = (int16_t *)_mm_malloc(MAX_SEQ_LEN16 * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
    H16_ = (int16_t *)_mm_malloc(MAX_SEQ_LEN16 * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);
	H16__ = (int16_t *)_mm_malloc(MAX_SEQ_LEN16 * SIMD_WIDTH16 * numThreads * sizeof(int16_t), 64);

	if (F8_ == NULL || H8_ == NULL || H8__ == NULL) {
		printf("BSW8 Memory not alloacted!!!\n"); exit(EXIT_FAILURE);
	}   	
	if (F16_ == NULL || H16_ == NULL || H16__ == NULL) {
		printf("BSW16 Memory not alloacted!!!\n"); exit(EXIT_FAILURE);
	}   	
}

// destructor 
BandedPairWiseSW::~BandedPairWiseSW() {
	_mm_free(F8_); _mm_free(H8_); _mm_free(H8__);
	_mm_free(F16_);_mm_free(H16_); _mm_free(H16__);
}

int64_t BandedPairWiseSW::getTicks()
{
    //printf("oneCount = %ld, totalCount = %ld\n", oneCount, totalCount);
    int64_t totalTicks = sort1Ticks + setupTicks + swTicks + sort2Ticks;
    printf("cost breakup: %ld, %ld, %ld, %ld, %ld\n",
            sort1Ticks, setupTicks, swTicks, sort2Ticks,
            totalTicks);

    return totalTicks;
}
// ------------------------------------------------------------------------------------
// Banded SWA - scalar code
// ------------------------------------------------------------------------------------

int BandedPairWiseSW::scalarBandedSWA(int qlen, const uint8_t *query,
									  int tlen, const uint8_t *target,
									  int32_t w, int h0, int *_qle, int *_tle,
									  int *_gtle, int *_gscore,
									  int *_max_off) {
	
	// uint64_t sw_cells = 0;
	eh_t *eh; // score array
	int8_t *qp; // query profile
	int i, j, k, oe_del = o_del + e_del, oe_ins = o_ins + e_ins, beg, end, max, max_i, max_j, max_ins, max_del, max_ie, gscore, max_off;
	
	// assert(h0 > 0); //check !!!
	
	// allocate memory
	qp = (int8_t *) malloc(qlen * m);
    assert(qp != NULL);
	eh = (eh_t *) calloc(qlen + 1, 8);
    assert(eh != NULL);

	// generate the query profile
	for (k = i = 0; k < m; ++k) {
		const int8_t *p = &mat[k * m];
		//for (j = 0; j < qlen; ++j) qp[i++] = p[query[j]-48];  //sub 48
		for (j = 0; j < qlen; ++j) qp[i++] = p[query[j]];
	}

	// fill the first row
	eh[0].h = h0; eh[1].h = h0 > oe_ins? h0 - oe_ins : 0;
	for (j = 2; j <= qlen && eh[j-1].h > e_ins; ++j)
		eh[j].h = eh[j-1].h - e_ins;

	// adjust $w if it is too large
	k = m * m;
	for (i = 0, max = 0; i < k; ++i) // get the max score
		max = max > mat[i]? max : mat[i];
	max_ins = (int)((double)(qlen * max + end_bonus - o_ins) / e_ins + 1.);
	max_ins = max_ins > 1? max_ins : 1;
	w = w < max_ins? w : max_ins;
	max_del = (int)((double)(qlen * max + end_bonus - o_del) / e_del + 1.);
	max_del = max_del > 1? max_del : 1;
	w = w < max_del? w : max_del; // TODO: is this necessary?

	// DP loop
	max = h0, max_i = max_j = -1; max_ie = -1, gscore = -1;
	max_off = 0;
	beg = 0, end = qlen;
	for (i = 0; (i < tlen); ++i) {
		int t, f = 0, h1, m = 0, mj = -1;
		//int8_t *q = &qp[(target[i]-48) * qlen];   // sub 48
		int8_t *q = &qp[(target[i]) * qlen];
		// apply the band and the constraint (if provided)
		if (beg < i - w) beg = i - w;
		if (end > i + w + 1) end = i + w + 1;
		if (end > qlen) end = qlen;
		// compute the first column
		if (beg == 0) {
			h1 = h0 - (o_del + e_del * (i + 1));
			if (h1 < 0) h1 = 0;
		} else h1 = 0;
		for (j = beg; (j < end); ++j) {
			// At the beginning of the loop: eh[j] = { H(i-1,j-1), E(i,j) }, f = F(i,j) and h1 = H(i,j-1)
			// Similar to SSE2-SW, cells are computed in the following order:
			//   H(i,j)   = max{H(i-1,j-1)+S(i,j), E(i,j), F(i,j)}
			//   E(i+1,j) = max{H(i,j)-gapo, E(i,j)} - gape
			//   F(i,j+1) = max{H(i,j)-gapo, F(i,j)} - gape
			eh_t *p = &eh[j];
			int h, M = p->h, e = p->e; // get H(i-1,j-1) and E(i-1,j)
			p->h = h1;          // set H(i,j-1) for the next row
			M = M? M + q[j] : 0;// separating H and M to disallow a cigar like "100M3I3D20M"
			h = M > e? M : e;   // e and f are guaranteed to be non-negative, so h>=0 even if M<0
			h = h > f? h : f;
			h1 = h;             // save H(i,j) to h1 for the next column
			mj = m > h? mj : j; // record the position where max score is achieved
			m = m > h? m : h;   // m is stored at eh[mj+1]
			t = M - oe_del;
			t = t > 0? t : 0;
			e -= e_del;
			e = e > t? e : t;   // computed E(i+1,j)
			p->e = e;           // save E(i+1,j) for the next row
			t = M - oe_ins;
			t = t > 0? t : 0;
			f -= e_ins;
			f = f > t? f : t;   // computed F(i,j+1)
			// SW_cells++;
		}
		eh[end].h = h1; eh[end].e = 0;
		if (j == qlen) {
			max_ie = gscore > h1? max_ie : i;
			gscore = gscore > h1? gscore : h1;
		}
		if (m == 0) break;
		if (m > max) {
			max = m, max_i = i, max_j = mj;
			max_off = max_off > abs(mj - i)? max_off : abs(mj - i);
		} else if (zdrop > 0) {
			if (i - max_i > mj - max_j) {
				if (max - m - ((i - max_i) - (mj - max_j)) * e_del > zdrop) break;
			} else {
				if (max - m - ((mj - max_j) - (i - max_i)) * e_ins > zdrop) break;
			}
		}
		// update beg and end for the next round
		for (j = beg; (j < end) && eh[j].h == 0 && eh[j].e == 0; ++j);
		beg = j;
		for (j = end; (j >= beg) && eh[j].h == 0 && eh[j].e == 0; --j);
		end = j + 2 < qlen? j + 2 : qlen;
		//beg = 0; end = qlen; // uncomment this line for debugging
	}
	free(eh); free(qp);
	if (_qle) *_qle = max_j + 1;
	if (_tle) *_tle = max_i + 1;
	if (_gtle) *_gtle = max_ie + 1;
	if (_gscore) *_gscore = gscore;
	if (_max_off) *_max_off = max_off;
	
#if MAXI
	fprintf(stderr, "%d (%d %d) %d %d %d\n", max, max_i+1, max_j+1, gscore, max_off, max_ie+1);
#endif

	// return sw_cells;
	return max;
}

// -------------------------------------------------------------
// Banded SWA, wrapper function
//-------------------------------------------------------------
void BandedPairWiseSW::scalarBandedSWAWrapper(SeqPair *seqPairArray,
											  uint8_t *seqBufRef,
											  uint8_t *seqBufQer,
											  int numPairs,
											  int nthreads,
											  int32_t w) {

	for (int i=0; i<numPairs; i++)
	{
		SeqPair *p = seqPairArray + i;
		uint8_t *seq1 = seqBufRef + p->idr;
        uint8_t *seq2 = seqBufQer + p->idq;
		
		p->score = scalarBandedSWA(p->len2, seq2, p->len1,
								   seq1, w, p->h0, &p->qle, &p->tle,
								   &p->gtle, &p->gscore, &p->max_off);		
	}

}



inline void sortPairsLen(SeqPair *pairArray, int32_t count, SeqPair *tempArray,
						 int16_t *hist)
{
    int32_t i;

    __m128i zero128 = _mm_setzero_si128();
    for(i = 0; i <= MAX_SEQ_LEN16; i+=8)
    {
        _mm_store_si128((__m128i *)(hist + i), zero128);
    }
    
    for(i = 0; i < count; i++)
    {
        SeqPair sp = pairArray[i];
        hist[sp.len1]++;
    }

    int32_t cumulSum = 0;
    for(i = 0; i <= MAX_SEQ_LEN16; i++)
    {
        int32_t cur = hist[i];
        hist[i] = cumulSum;
        cumulSum += cur;
    }

    for(i = 0; i < count; i++)
    {
        SeqPair sp = pairArray[i];
        int32_t pos = hist[sp.len1];
        tempArray[pos] = sp;
        hist[sp.len1]++;
    }
	
    for(i = 0; i < count; i++) {
        pairArray[i] = tempArray[i];
	}
}

inline void sortPairsId(SeqPair *pairArray, int32_t first,
						int32_t count, SeqPair *tempArray)
{

    int32_t i;
    
    for(i = 0; i < count; i++)
    {
        SeqPair sp = pairArray[i];
        int32_t pos = sp.id - first;
        tempArray[pos] = sp;
    }

    for(i = 0; i < count; i++)
        pairArray[i] = tempArray[i];
}



// SSE2
#define PFD 2
void BandedPairWiseSW::getScores16(SeqPair *pairArray,
								   uint8_t *seqBufRef,
								   uint8_t *seqBufQer,
								   int32_t numPairs,
								   uint16_t numThreads,
								   int32_t w)
{
	smithWatermanBatchWrapper16(pairArray, seqBufRef,
                                seqBufQer, numPairs,
                                numThreads, w);

#if MAXI
	for (int l=0; l<numPairs; l++)
	{
		fprintf(stderr, "%d (%d %d) %d %d %d\n",
				pairArray[l].score, pairArray[l].x, pairArray[l].y,
				pairArray[l].gscore, pairArray[l].max_off, pairArray[l].max_ie);

	}
#endif
	
}

void BandedPairWiseSW::smithWatermanBatchWrapper16(SeqPair *pairArray,
												   uint8_t *seqBufRef,
												   uint8_t *seqBufQer,
												   int32_t numPairs,
												   uint16_t numThreads,
												   int32_t w)
{
#if RDT
    int64_t st1, st2, st3, st4, st5;
	st1 = ___rdtsc();
#endif
	
    uint16_t *seq1SoA = (uint16_t *)_mm_malloc(MAX_SEQ_LEN16 * SIMD_WIDTH16 * numThreads * sizeof(uint16_t), 64);
    uint16_t *seq2SoA = (uint16_t *)_mm_malloc(MAX_SEQ_LEN16 * SIMD_WIDTH16 * numThreads * sizeof(uint16_t), 64);

	assert (seq1SoA != NULL || seq2SoA != NULL);

    int32_t ii;
    int32_t roundNumPairs = ((numPairs + SIMD_WIDTH16 - 1)/SIMD_WIDTH16 ) * SIMD_WIDTH16;
	// assert(roundNumPairs < BATCH_SIZE * SEEDS_PER_READ);
    for(ii = numPairs; ii < roundNumPairs; ii++)
    {
        pairArray[ii].id = ii;
        pairArray[ii].len1 = 0;
        pairArray[ii].len2 = 0;
    }

#if RDT
	st2 = ___rdtsc();
#endif
	
#if SORT_PAIRS       // disbaled in bwa-mem2 (only used in separate benchmark bsw code)
    // Sort the sequences according to decreasing order of lengths
    SeqPair *tempArray = (SeqPair *)_mm_malloc(SORT_BLOCK_SIZE * numThreads *
											   sizeof(SeqPair), 64);
    int16_t *hist = (int16_t *)_mm_malloc((MAX_SEQ_LEN16 + 16) * numThreads *
										  sizeof(int16_t), 64);
#pragma omp parallel num_threads(numThreads)
    {
        int32_t tid = omp_get_thread_num();
        SeqPair *myTempArray = tempArray + tid * SORT_BLOCK_SIZE;
        int16_t *myHist = hist + tid * (MAX_SEQ_LEN16 + 16);

#pragma omp for
        for(ii = 0; ii < roundNumPairs; ii+=SORT_BLOCK_SIZE)
        {
            int32_t first, last;
            first = ii;
            last  = ii + SORT_BLOCK_SIZE;
            if(last > roundNumPairs) last = roundNumPairs;
            sortPairsLen(pairArray + first, last - first, myTempArray, myHist);
        }
    }
    _mm_free(hist);
#endif

#if RDT
	st3 = ___rdtsc();
#endif
	
	int eb = end_bonus;
// #pragma omp parallel num_threads(numThreads)
    {
        int32_t i;
        uint16_t tid = 0; 
        uint16_t *mySeq1SoA = seq1SoA + tid * MAX_SEQ_LEN16 * SIMD_WIDTH16;
        uint16_t *mySeq2SoA = seq2SoA + tid * MAX_SEQ_LEN16 * SIMD_WIDTH16;
		assert(mySeq1SoA != NULL && mySeq2SoA != NULL);
		
        uint8_t *seq1;
        uint8_t *seq2;
		uint16_t h0[SIMD_WIDTH16]  __attribute__((aligned(256)));
		uint16_t qlen[SIMD_WIDTH16] __attribute__((aligned(256)));
		int32_t bsize = 0;
		
		int16_t *H1 = H16_ + tid * SIMD_WIDTH16 * MAX_SEQ_LEN16;
		int16_t *H2 = H16__ + tid * SIMD_WIDTH16 * MAX_SEQ_LEN16;

		__m128i zero128	  = _mm_setzero_si128();
		__m128i e_ins128  = _mm_set1_epi16(e_ins);
		__m128i oe_ins128 = _mm_set1_epi16(o_ins + e_ins);
		__m128i o_del128  = _mm_set1_epi16(o_del);
		__m128i e_del128  = _mm_set1_epi16(e_del);
		__m128i eb_ins128 = _mm_set1_epi16(eb - o_ins);
		__m128i eb_del128 = _mm_set1_epi16(eb - o_del);
		
		int16_t max = 0;
		if (max < w_match) max = w_match;
		if (max < w_mismatch) max = w_mismatch;
		if (max < w_ambig) max = w_ambig;
		
		int nstart = 0, nend = numPairs;

// #pragma omp for schedule(dynamic, 128)
		for(i = nstart; i < nend; i+=SIMD_WIDTH16)
		{
            int32_t j, k;
            uint16_t maxLen1 = 0;
            uint16_t maxLen2 = 0;
			bsize = w;

            for(j = 0; j < SIMD_WIDTH16; j++)
            {
				{ // prefetch block
					SeqPair spf = pairArray[i + j + PFD];
					_mm_prefetch((const char*) seqBufRef + (int64_t)spf.idr, _MM_HINT_NTA);
					_mm_prefetch((const char*) seqBufRef + (int64_t)spf.idr + 64, _MM_HINT_NTA);
				}
                SeqPair sp = pairArray[i + j];
				h0[j] = sp.h0;
				seq1 = seqBufRef + (int64_t)sp.idr;
				
                for(k = 0; k < sp.len1; k++)
                {
                    mySeq1SoA[k * SIMD_WIDTH16 + j] = (seq1[k] == AMBIG?0xFFFF:seq1[k]);
					H2[k * SIMD_WIDTH16 + j] = 0;
                }
				qlen[j] = sp.len2 * max;
                if(maxLen1 < sp.len1) maxLen1 = sp.len1;
            }

            for(j = 0; j < SIMD_WIDTH16; j++)
            {
                SeqPair sp = pairArray[i + j];
                for(k = sp.len1; k <= maxLen1; k++) //removed "="
                {
                    mySeq1SoA[k * SIMD_WIDTH16 + j] = DUMMY1;
					H2[k * SIMD_WIDTH16 + j] = DUMMY1;
                }
            }
//--------------------
			__m128i h0_128 = _mm_load_si128((__m128i*) h0);
			_mm_store_si128((__m128i *) H2, h0_128);
			__m128i tmp128 = _mm_sub_epi16(h0_128, o_del128);
			
			for(k = 1; k < maxLen1; k++)
			{
				tmp128 = _mm_sub_epi16(tmp128, e_del128);
				__m128i tmp128_ = _mm_max_epi16(tmp128, zero128);
				_mm_store_si128((__m128i *)(H2 + k* SIMD_WIDTH16), tmp128_);
			}
//-------------------
            for(j = 0; j < SIMD_WIDTH16; j++)
            {
				{ // prefetch block
					SeqPair spf = pairArray[i + j + PFD];
					_mm_prefetch((const char*) seqBufQer + (int64_t)spf.idq, _MM_HINT_NTA);
					_mm_prefetch((const char*) seqBufQer + (int64_t)spf.idq + 64, _MM_HINT_NTA);
				}
				
                SeqPair sp = pairArray[i + j];
				seq2 = seqBufQer + (int64_t)sp.idq;				
                for(k = 0; k < sp.len2; k++)
                {
                    mySeq2SoA[k * SIMD_WIDTH16 + j] = (seq2[k]==AMBIG?0xFFFF:seq2[k]);
					H1[k * SIMD_WIDTH16 + j] = 0;					
                }
                if(maxLen2 < sp.len2) maxLen2 = sp.len2;
            }
			
            for(j = 0; j < SIMD_WIDTH16; j++)
            {
                SeqPair sp = pairArray[i + j];
                for(k = sp.len2; k <= maxLen2; k++)
                {
                    mySeq2SoA[k * SIMD_WIDTH16 + j] = DUMMY2;
					H1[k * SIMD_WIDTH16 + j] = 0;
                }
            }
//------------------------
			_mm_store_si128((__m128i *) H1, h0_128);
			svbool_t cmp128 = _mm_cmpgt_epi16(h0_128, oe_ins128);
			tmp128 = _mm_sub_epi16(h0_128, oe_ins128);

			tmp128 = _mm_blend_epi16(zero128, tmp128, cmp128);
			_mm_store_si128((__m128i *) (H1 + SIMD_WIDTH16), tmp128);
			for(k = 2; k < maxLen2; k++)
			{
				__m128i h1_128 = tmp128;
				tmp128 = _mm_sub_epi16(h1_128, e_ins128);
				tmp128 = _mm_max_epi16(tmp128, zero128);
				_mm_store_si128((__m128i *)(H1 + k*SIMD_WIDTH16), tmp128);
            }			
//------------------------
			uint16_t myband[SIMD_WIDTH16] __attribute__((aligned(256)));
			uint16_t temp[SIMD_WIDTH16] __attribute__((aligned(256)));
			{
				__m128i qlen128 = _mm_load_si128((__m128i *) qlen);
				__m128i sum128 = _mm_add_epi16(qlen128, eb_ins128);
				_mm_store_si128((__m128i *) temp, sum128);				
				for (int l=0; l<SIMD_WIDTH16; l++) {
					double val = temp[l]/e_ins + 1.0;
					int max_ins = (int) val;
					max_ins = max_ins > 1? max_ins : 1;
					myband[l] = min_(bsize, max_ins);
				}
				sum128 = _mm_add_epi16(qlen128, eb_del128);
				_mm_store_si128((__m128i *) temp, sum128);				
				for (int l=0; l<SIMD_WIDTH16; l++) {
					double val = temp[l]/e_del + 1.0;
					int max_ins = (int) val;
					max_ins = max_ins > 1? max_ins : 1;
					myband[l] = min_(myband[l], max_ins);
					bsize = bsize < myband[l] ? myband[l] : bsize;					
				}
			}

            smithWaterman128_16(mySeq1SoA,
								mySeq2SoA,
								maxLen1,
								maxLen2,
								pairArray + i,
								h0,
								tid,
								numPairs,
								zdrop,
								bsize,
								qlen,
								myband);
        }
    }

#if RDT	
    st4 = ___rdtsc();
#endif
	
#if SORT_PAIRS       // disbaled in bwa-mem2 (only used in separate benchmark bsw code)
	{
    // Sort the sequences according to increasing order of id
#pragma omp parallel num_threads(numThreads)
    {
        int32_t tid = omp_get_thread_num();
        SeqPair *myTempArray = tempArray + tid * SORT_BLOCK_SIZE;

#pragma omp for
        for(ii = 0; ii < roundNumPairs; ii+=SORT_BLOCK_SIZE)
        {
            int32_t first, last;
            first = ii;
            last  = ii + SORT_BLOCK_SIZE;
            if(last > roundNumPairs) last = roundNumPairs;
            sortPairsId(pairArray + first, first, last - first, myTempArray);
        }
    }
    _mm_free(tempArray);
	}
#endif

#if RDT
	st5 = ___rdtsc();
    setupTicks += st2 - st1;
    sort1Ticks += st3 - st2;
    swTicks += st4 - st3;
    sort2Ticks += st5 - st4;
#endif
	// free mem
	_mm_free(seq1SoA);
	_mm_free(seq2SoA);
	
    return;
}

void BandedPairWiseSW::smithWaterman128_16(uint16_t seq1SoA[],
										   uint16_t seq2SoA[],
										   uint16_t nrow,
										   uint16_t ncol,
										   SeqPair *p,
										   uint16_t h0[],
										   uint16_t tid,
										   int32_t numPairs,
										   int zdrop,
										   int32_t w,
										   uint16_t qlen[],
										   uint16_t myband[])
{
	
    __m128i match128	 = _mm_set1_epi16(this->w_match);
    __m128i mismatch128	 = _mm_set1_epi16(this->w_mismatch);
	__m128i w_ambig_128	 = _mm_set1_epi16(this->w_ambig);	// ambig penalty

	__m128i e_del128	= _mm_set1_epi16(this->e_del);
	__m128i oe_del128	= _mm_set1_epi16(this->o_del + this->e_del);
	__m128i e_ins128	= _mm_set1_epi16(this->e_ins);
	__m128i oe_ins128	= _mm_set1_epi16(this->o_ins + this->e_ins);

	int16_t	*F	= F16_ + tid * SIMD_WIDTH16 * MAX_SEQ_LEN16;
    int16_t	*H_h	= H16_ + tid * SIMD_WIDTH16 * MAX_SEQ_LEN16;
	int16_t	*H_v = H16__ + tid * SIMD_WIDTH16 * MAX_SEQ_LEN16;

    int16_t i, j;

	uint16_t tlen[SIMD_WIDTH16];
	uint16_t tail[SIMD_WIDTH16] __attribute((aligned(256)));
	uint16_t head[SIMD_WIDTH16] __attribute((aligned(256)));
	
	int32_t minq = 10000000;
	for (int l=0; l<SIMD_WIDTH16; l++) {
		tlen[l] = p[l].len1;
		qlen[l] = p[l].len2;
		if (p[l].len2 < minq) minq = p[l].len2;
	}
	minq -= 1; // for gscore

	__m128i tlen128 = _mm_load_si128((__m128i *) tlen);
	__m128i qlen128 = _mm_load_si128((__m128i *) qlen);
	__m128i myband128 = _mm_load_si128((__m128i *) myband);
    __m128i zero128 = _mm_setzero_si128();
	__m128i one128	= _mm_set1_epi16(1);
	__m128i two128	= _mm_set1_epi16(2);
	__m128i max_ie128 = zero128;
	__m128i ff128 = _mm_set1_epi16(0xFFFF);
	svbool_t ff128_b = svptrue_b16();
		
   	__m128i tail128 = qlen128, head128 = zero128;
	_mm_store_si128((__m128i *) head, head128);
   	_mm_store_si128((__m128i *) tail, tail128);

	__m128i mlen128 = _mm_add_epi16(qlen128, myband128);
	mlen128 = _mm_min_epu16(mlen128, tlen128);
		
	__m128i hval = _mm_load_si128((__m128i *)(H_v));

	//__mmask16 dmask16 = 0xAAAA;
	
	__m128i maxScore128 = hval;
    for(j = 0; j < ncol; j++)
		_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH16), zero128);
	
	__m128i x128 = zero128;
	__m128i y128 = zero128;
	__m128i gscore = _mm_set1_epi16(-1);
	__m128i max_off128 = zero128;
	svbool_t exit0 = svptrue_b16();
	__m128i zdrop128 = _mm_set1_epi16(zdrop);
	
	int beg = 0, end = ncol;
	int nbeg = beg, nend = end;

#if RDT
	uint64_t tim = __rdtsc();
#endif
	
    for(i = 0; i < nrow; i++)
    {		
        __m128i e11 = zero128;
        __m128i h00, h11, h10;
        __m128i seq1 = _mm_load_si128((__m128i *)(seq1SoA + (i + 0) * SIMD_WIDTH16));

		beg = nbeg; end = nend;
		// Banding
		if (beg < i - w) beg = i - w;
		if (end > i + w + 1) end = i + w + 1;
		if (end > ncol) end = ncol;

		h10 = zero128;
		if (beg == 0)
			h10 = _mm_load_si128((__m128i *)(H_v + (i+1) * SIMD_WIDTH16));

		__m128i j128 = zero128;
		__m128i maxRS1 = zero128;
		
		__m128i i1_128 = _mm_set1_epi16(i+1);
		__m128i y1_128 = zero128;
		
#if RDT	
		uint64_t tim1 = __rdtsc();
#endif
		
		__m128i i128, cache128;
		__m128i phead128 = head128, ptail128 = tail128;
		i128 = _mm_set1_epi16(i);
		cache128 = _mm_sub_epi16(i128, myband128);
		head128 = _mm_max_epi16(head128, cache128);
		cache128 = _mm_add_epi16(i1_128, myband128);
		tail128 = _mm_min_epu16(tail128, cache128);
		tail128 = _mm_min_epu16(tail128, qlen128);
		
		// NEW, trimming.
		svbool_t cmph = _mm_cmpeq_epi16(head128, phead128);
		svbool_t cmpt = _mm_cmpeq_epi16(tail128, ptail128);
		cmph = _mm_and_si128(cmph, cmpt);
		
		//for (int l=beg; l<end && cmp_ht != dmask16; l++) {
        //for (int l=beg; l<end && svptest_any(svptrue_b16(),svnot_z(svptrue_b16(),cmph)); l++) {
		//	__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH16));
		//	__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH16));
		//	
		//	__m128i pj128 = _mm_set1_epi16(l);
		//	__m128i j128 = _mm_set1_epi16(l+1);
		//	svbool_t cmp1 = _mm_cmpgt_epi16(head128, pj128);
		//	//uint16_t cval = _mm_movemask_epi8(cmp1) & dmask16;			
		//	//if (cval == 0x00) break;
        //    if (!svptest_any(svptrue_b16(),cmp1)) break;
		//	svbool_t cmp2 = _mm_cmpgt_epi16(j128, tail128);
		//	cmp1 = _mm_or_si128(cmp1, cmp2);
		//	h128 = _mm_blend_epi16(h128, zero128, cmp1);
		//	f128 = _mm_blend_epi16(f128, zero128, cmp1);
		//	
		//	_mm_store_si128((__m128i *)(F + l * SIMD_WIDTH16), f128);
		//	_mm_store_si128((__m128i *)(H_h + l * SIMD_WIDTH16), h128);
		//}

#if RDT
		prof[DP3][0] += __rdtsc() - tim1;
#endif

		svbool_t cmpim = _mm_cmpgt_epi16(i1_128, mlen128);
		svbool_t cmpht = _mm_cmpeq_epi16(tail128, head128);
		cmpim = _mm_or_si128(cmpim, cmpht);
		// NEW
		cmpht = _mm_cmpgt_epi16(head128, tail128);
		cmpim = _mm_or_si128(cmpim, cmpht);

		//exit0 = _mm_blend_epi16(exit0, zero128, cmpim);
        exit0 = _mm_andnot_si128(cmpim, exit0);

#if RDT
		tim1 = __rdtsc();
#endif
		
		j128 = _mm_set1_epi16(beg);
        #pragma unroll(8)
		for(j = beg; j < end; j++)
		{
            __m128i f11, f21, seq2;
			h00 = _mm_load_si128((__m128i *)(H_h + j * SIMD_WIDTH16));
            f11 = _mm_load_si128((__m128i *)(F + j * SIMD_WIDTH16));

            seq2 = _mm_load_si128((__m128i *)(seq2SoA + (j) * SIMD_WIDTH16));
			
			__m128i pj128 = j128;
			j128 = _mm_add_epi16(j128, one128);

            /* MAIN CODE */
            // Compare both sequences
            svbool_t cmp11 = _mm_cmpeq_epi16(seq1, seq2);
            // match/mismatch score
            __m128i sbt11 = _mm_blend_epi16(mismatch128, match128, cmp11);
            // Compute Ns
            __m128i tmp128 = _mm_or_si128(seq1, seq2);
            svbool_t tmp128_1 = _mm_cmpeq_epi16(tmp128, ff128);
            sbt11 = _mm_blend_epi16(sbt11, w_ambig_128, tmp128_1);

            // H[i-1,j-1] + S[i,j]
            // @PREDICATION
#if 0
            __m128i m11 = _mm_add_epi16(h00, sbt11);
            cmp11 = _mm_cmpeq_epi16(h00, zero128);
            m11 = _mm_blend_epi16(m11, zero128, cmp11);
#else
            __m128i m11;
            {
                svint16_t a_aux = svreinterpret_s16(h00);
                svint16_t b_aux = svreinterpret_s16(sbt11);
                cmp11 = svcmpne_n_s16(svptrue_b16(), a_aux, 0);
                svint16_t r_aux = svadd_z(cmp11,a_aux,b_aux);
                m11 = svreinterpret_s64(r_aux);
            }
#endif

            // Select max(H...,E[i-1,j]
            h11 = _mm_max_epi16(m11, e11);
            // Select max(...,F[i,j-1]
            h11 = _mm_max_epi16(h11, f11);

            // Calculate E[i,j]
            __m128i temp128 = _mm_sub_epi16(m11, oe_ins128);
            __m128i val128  = _mm_max_epi16(temp128, zero128);
            e11 = _mm_sub_epi16(e11, e_ins128);
            e11 = _mm_max_epi16(val128, e11);

            // Calculate F[i,j]
            temp128 = _mm_sub_epi16(m11, oe_del128);
            val128  = _mm_max_epi16(temp128, zero128);
            f21 = _mm_sub_epi16(f11, e_del128);
            f21 = _mm_max_epi16(val128, f21);

			// Masked writing
			svbool_t cmp1 = _mm_cmpgt_epi16(head128, pj128);
			svbool_t cmp2 = _mm_cmpgt_epi16(pj128, tail128);
			//cmp1 = _mm_or_si128(cmp1, cmp2);
			//h10 = _mm_blend_epi16(h10, zero128, cmp1);
			//f21 = _mm_blend_epi16(f21, zero128, cmp1);
			////svbool_t cmp1 = _mm_cmpgt_epi16(j128, tail128);
			cmp1 = _mm_or_si128(cmp1, cmp2);
			
			__m128i bmaxRS = maxRS1;										
			maxRS1 =_mm_max_epi16(maxRS1, h11);							
			svbool_t cmpA = _mm_cmpgt_epi16(maxRS1, bmaxRS);					
			svbool_t cmpB =_mm_cmpeq_epi16(maxRS1, h11);					
			cmpA = _mm_or_si128(cmpA, cmpB);
			__m128i cmpA_1 = _mm_blend_epi16(y1_128, j128, cmpA);
			y1_128 = _mm_blend_epi16(cmpA_1, y1_128, cmp1);
			maxRS1 = _mm_blend_epi16(maxRS1, bmaxRS, cmp1);						

			_mm_store_si128((__m128i *)(H_h + j * SIMD_WIDTH16), h10);
			_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH16), f21);

			h10 = h11;
			
			// gscore calculations
			if (j >= minq)
			{
				svbool_t cmp = _mm_cmpeq_epi16(j128, qlen128);
				__m128i max_gh = _mm_max_epi16(gscore, h11);
				svbool_t cmp_gh = _mm_cmpgt_epi16(gscore, h11);
				__m128i tmp128_1 = _mm_blend_epi16(i1_128, max_ie128, cmp_gh);

				__m128i tmp128_t = _mm_blend_epi16(max_ie128, tmp128_1, cmp);
				tmp128_1 = _mm_blend_epi16(max_ie128, tmp128_t, exit0);				
				
				max_gh = _mm_blend_epi16(gscore, max_gh, exit0);
				max_gh = _mm_blend_epi16(gscore, max_gh, cmp);				

				cmp = _mm_cmpgt_epi16(j128, tail128); 
				max_gh = _mm_blend_epi16(max_gh, gscore, cmp);
				max_ie128 = _mm_blend_epi16(tmp128_1, max_ie128, cmp);
				gscore = max_gh;
			}
        }
		_mm_store_si128((__m128i *)(H_h + j * SIMD_WIDTH16), h10);
		_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH16), zero128);
		
		/* exit due to zero score by a row */
		__m128i bmaxScore128 = maxScore128;
		svbool_t tmp_1 = _mm_cmpeq_epi16(maxRS1, zero128);
		//uint16_t cval = _mm_movemask_epi8(tmp) & dmask16;
        //if (cval == dmask16) break;
        if (!svptest_any(svptrue_b16(),svnot_z(svptrue_b16(),tmp_1))) break;

		//exit0 = _mm_blend_epi16(exit0, zero128,  tmp_1);
        exit0 = _mm_andnot_si128(tmp_1, exit0);

		__m128i score128 = _mm_max_epi16(maxScore128, maxRS1);
		maxScore128 = _mm_blend_epi16(maxScore128, score128, exit0);

		svbool_t cmp = _mm_cmpgt_epi16(maxScore128, bmaxScore128);
		y128 = _mm_blend_epi16(y128, y1_128, cmp);
		x128 = _mm_blend_epi16(x128, i1_128, cmp);		
		// max_off calculations
		__m128i ab = _mm_subs_epu16(y1_128, i1_128);
		__m128i ba = _mm_subs_epu16(i1_128, y1_128);
		__m128i tmp = _mm_or_si128(ab, ba);
		__m128i bmax_off128 = max_off128;
		tmp = _mm_max_epi16(max_off128, tmp);
		max_off128 = _mm_blend_epi16(bmax_off128, tmp, cmp);

        // Z-score
        __m128i tmpi = _mm_sub_epi16(i1_128, x128);
        __m128i tmpj = _mm_sub_epi16(y1_128, y128);
        cmp = _mm_cmpgt_epi16(tmpi, tmpj);
        score128 = _mm_sub_epi16(maxScore128, maxRS1);
        __m128i insdel = _mm_blend_epi16(e_ins128, e_del128, cmp);
        __m128i sub_a128 = _mm_sub_epi16(tmpi, tmpj);
        __m128i sub_b128 = _mm_sub_epi16(tmpj, tmpi);
        tmp = _mm_blend_epi16(sub_b128, sub_a128, cmp);
        tmp = _mm_sub_epi16(score128, tmp);
        cmp = _mm_cmpgt_epi16(tmp, zdrop128);
        exit0 = _mm_andnot_si128(cmp, exit0);


#if RDT
		prof[DP1][0] += __rdtsc() - tim1;
		tim1 = __rdtsc();
#endif
		
		/* Narrowing of the band */
		/* From beg */
		int l;
 		for (l = beg; l < end; l++)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH16));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH16));
			__m128i tmp = _mm_or_si128(f128, h128);
			svbool_t tmp_1 = _mm_cmpeq_epi16(tmp, zero128);
			//uint16_t val = _mm_movemask_epi8(tmp) & dmask16;
			//if (val == dmask16) nbeg = l;
			//else break;
            //if (!svptest_any(svptrue_b16(),svnot_z(svptrue_b16(),tmp_1))) nbeg = l;
            if (svcntp_b16(svptrue_b16(),tmp_1) == SIMD_WIDTH16) nbeg = l;
            else break;
		}
		
		/* From end */
		for (l = end; l >= beg; l--)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH16));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH16));
			__m128i tmp = _mm_or_si128(f128, h128);
			tmp_1 = _mm_cmpeq_epi16(tmp, zero128);
            //uint16_t val = _mm_movemask_epi8(tmp_1) & dmask16;
            //if (val != dmask16) break;
            //if (svptest_any(svptrue_b16(),svnot_z(svptrue_b16(),tmp_1))) break;
            if (svcntp_b16(svptrue_b16(),tmp_1) != SIMD_WIDTH16) break;
		}
		nend = l + 2 < ncol? l + 2: ncol;

		svbool_t tmpb = ff128_b;

		//__m128i exit1 = _mm_xor_si128(exit0, ff128);
        __m128i exit1 = svreinterpret_s64(svdup_s16_z(svnot_z(svptrue_b16(),exit0),0xFFFF));
		__m128i l128 = _mm_set1_epi16(beg);
		for (l = beg; l < end; l++)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH16));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH16));
	
			__m128i tmp = _mm_or_si128(f128, h128);
			tmp = _mm_or_si128(tmp, exit1);			
			//tmp = _mm_cmpeq_epi16(tmp, zero128);
            svbool_t tmp_1 = _mm_cmpeq_epi8(tmp, zero128);
            //uint16_t val = _mm_movemask_epi8(tmp_1) & dmask16;
            //if (val == 0x00) break;
            if (!svptest_any(svptrue_b16(),tmp_1)) break;

            tmp_1 = _mm_and_si128(tmp_1,tmpb);
			l128 = _mm_add_epi16(l128, one128);
			head128 = _mm_blend_epi16(head128, l128, tmp_1);

			tmpb = tmp_1;			
		}
		// _mm_store_si128((__m128i *) head, head128);
		
		__m128i  index128 = tail128;
		tmpb = ff128_b;

		l128 = _mm_set1_epi16(end);
		for (l = end; l >= beg; l--)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH16));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH16));
			
			__m128i tmp = _mm_or_si128(f128, h128);
			tmp = _mm_or_si128(tmp, exit1);
			svbool_t tmp_1 = _mm_cmpeq_epi16(tmp, zero128);			
            //uint16_t val = _mm_movemask_epi8(tmp_1) & dmask16;
            //if (val == 0x00) break;
            if (!svptest_any(svptrue_b16(),tmp_1)) break;
			tmp_1 = _mm_and_si128(tmp_1,tmpb);
			l128 = _mm_sub_epi16(l128, one128);
			// NEW
			index128 = _mm_blend_epi8(index128, l128, tmp_1);

			tmpb = tmp_1;
		}
		index128 = _mm_add_epi16(index128, two128);
		tail128 = _mm_min_epi16(index128, qlen128);

#if RDT
		prof[DP2][0] += __rdtsc() - tim1;
#endif
    }
	
#if RDT
	prof[DP][0] += __rdtsc() - tim;
#endif
	
    int16_t score[SIMD_WIDTH16]  __attribute((aligned(256)));
	_mm_store_si128((__m128i *) score, maxScore128);

	int16_t maxi[SIMD_WIDTH16]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxi, x128);

	int16_t maxj[SIMD_WIDTH16]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxj, y128);

	int16_t max_off_ar[SIMD_WIDTH16]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) max_off_ar, max_off128);

	int16_t gscore_ar[SIMD_WIDTH16]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) gscore_ar, gscore);

	int16_t maxie_ar[SIMD_WIDTH16]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxie_ar, max_ie128);
	
    for(i = 0; i < SIMD_WIDTH16; i++)
    {
		p[i].score = score[i];
		p[i].tle = maxi[i];
		p[i].qle = maxj[i];
		p[i].max_off = max_off_ar[i];
		p[i].gscore = gscore_ar[i];
		p[i].gtle = maxie_ar[i];
    }
	
    return;
}

/********************************************************************************/

// #define PFD 2 // SSE2
void BandedPairWiseSW::getScores8(SeqPair *pairArray,
								  uint8_t *seqBufRef,
								  uint8_t *seqBufQer,
								  int32_t numPairs,
								  uint16_t numThreads,
								  int32_t w)
{
	//assert(SIMD_WIDTH8 == 16 && SIMD_WIDTH16 == 8);
	smithWatermanBatchWrapper8(pairArray, seqBufRef, seqBufQer, numPairs, numThreads, w);

	
#if MAXI
	printf("Vecor code: Writing output..\n");
	for (int l=0; l<numPairs; l++)
	{
		fprintf(stderr, "%d (%d %d) %d %d %d\n",
				pairArray[l].score, pairArray[l].x, pairArray[l].y,
				pairArray[l].gscore, pairArray[l].max_off, pairArray[l].max_ie);

	}
	printf("Vector code: Writing output completed!!!\n\n");
#endif
	
}

void BandedPairWiseSW::smithWatermanBatchWrapper8(SeqPair *pairArray,
												  uint8_t *seqBufRef,
												  uint8_t *seqBufQer,
												  int32_t numPairs,
												  uint16_t numThreads,
												  int32_t w)
{
#if RDT
    int64_t st1, st2, st3, st4, st5;
	st1 = ___rdtsc();
#endif
    uint8_t *seq1SoA = (uint8_t *)_mm_malloc(MAX_SEQ_LEN8 * SIMD_WIDTH8 * numThreads * sizeof(uint8_t), 64);
    uint8_t *seq2SoA = (uint8_t *)_mm_malloc(MAX_SEQ_LEN8 * SIMD_WIDTH8 * numThreads * sizeof(uint8_t), 64);

	if (seq1SoA == NULL || seq2SoA == NULL) {
		fprintf(stderr, "Error! Mem not allocated!!!\n");
		exit(EXIT_FAILURE);
	}
	
    int32_t ii;
    int32_t roundNumPairs = ((numPairs + SIMD_WIDTH8 - 1)/SIMD_WIDTH8 ) * SIMD_WIDTH8;
	// assert(roundNumPairs < BATCH_SIZE * SEEDS_PER_READ);
    for(ii = numPairs; ii < roundNumPairs; ii++)
    {
        pairArray[ii].id = ii;
        pairArray[ii].len1 = 0;
        pairArray[ii].len2 = 0;
    }

#if RDT
	st2 = ___rdtsc();
#endif
	
#if SORT_PAIRS       // disbaled in bwa-mem2 (only used in separate benchmark bsw code)
    // Sort the sequences according to decreasing order of lengths
    SeqPair *tempArray = (SeqPair *)_mm_malloc(SORT_BLOCK_SIZE * numThreads *
											   sizeof(SeqPair), 64);
    int16_t *hist = (int16_t *)_mm_malloc((MAX_SEQ_LEN8 + 32) * numThreads *
										  sizeof(int16_t), 64);
#pragma omp parallel num_threads(numThreads)
    {
        int32_t tid = omp_get_thread_num();
        SeqPair *myTempArray = tempArray + tid * SORT_BLOCK_SIZE;
        int16_t *myHist = hist + tid * (MAX_SEQ_LEN8 + 32);

#pragma omp for
        for(ii = 0; ii < roundNumPairs; ii+=SORT_BLOCK_SIZE)
        {
            int32_t first, last;
            first = ii;
            last  = ii + SORT_BLOCK_SIZE;
            if(last > roundNumPairs) last = roundNumPairs;
            sortPairsLen(pairArray + first, last - first, myTempArray, myHist);
        }
    }
    _mm_free(hist);
#endif

#if RDT
	st3 = ___rdtsc();
#endif

	int eb = end_bonus;
// #pragma omp parallel num_threads(numThreads)
    {
        int32_t i;
        uint16_t tid =  0; 
        uint8_t *mySeq1SoA = seq1SoA + tid * MAX_SEQ_LEN8 * SIMD_WIDTH8;
        uint8_t *mySeq2SoA = seq2SoA + tid * MAX_SEQ_LEN8 * SIMD_WIDTH8;
		assert(mySeq1SoA != NULL && mySeq2SoA != NULL);		
        uint8_t *seq1;
        uint8_t *seq2;
		uint8_t h0[SIMD_WIDTH8]   __attribute__((aligned(256)));
		uint8_t qlen[SIMD_WIDTH8] __attribute__((aligned(256)));
		int32_t bsize = 0;

		int8_t *H1 = H8_ + tid * SIMD_WIDTH8 * MAX_SEQ_LEN8;
		int8_t *H2 = H8__ + tid * SIMD_WIDTH8 * MAX_SEQ_LEN8;

		__m128i zero128	  = _mm_setzero_si128();
		__m128i e_ins128  = _mm_set1_epi8(e_ins);
		__m128i oe_ins128 = _mm_set1_epi8(o_ins + e_ins);
		__m128i o_del128  = _mm_set1_epi8(o_del);
		__m128i e_del128  = _mm_set1_epi8(e_del);
		__m128i eb_ins128 = _mm_set1_epi8(eb - o_ins);
		__m128i eb_del128 = _mm_set1_epi8(eb - o_del);
		
		int8_t max = 0;
		if (max < w_match) max = w_match;
		if (max < w_mismatch) max = w_mismatch;
		if (max < w_ambig) max = w_ambig;
		
		int nstart = 0, nend = numPairs;
		
// #pragma omp for schedule(dynamic, 128)
		for(i = nstart; i < nend; i+=SIMD_WIDTH8)
		{
            int32_t j, k;
            uint8_t maxLen1 = 0;
            uint8_t maxLen2 = 0;
			//bsize = 100;
			bsize = w;
			
            for(j = 0; j < SIMD_WIDTH8; j++)
            {
                SeqPair sp = pairArray[i + j];
				h0[j] = sp.h0;
				seq1 = seqBufRef + (int64_t)sp.idr;
				
                for(k = 0; k < sp.len1; k++)
                {
                    mySeq1SoA[k * SIMD_WIDTH8 + j] = (seq1[k] == AMBIG?0xFF:seq1[k]);
					H2[k * SIMD_WIDTH8 + j] = 0;
                }
				qlen[j] = sp.len2 * max;
                if(maxLen1 < sp.len1) maxLen1 = sp.len1;
            }

            for(j = 0; j < SIMD_WIDTH8; j++)
            {
                SeqPair sp = pairArray[i + j];
                for(k = sp.len1; k <= maxLen1; k++) //removed "="
                {
                    mySeq1SoA[k * SIMD_WIDTH8 + j] = DUMMY1;
					H2[k * SIMD_WIDTH8 + j] = DUMMY1;
                }
            }
//--------------------
			__m128i h0_128 = _mm_load_si128((__m128i*) h0);
			_mm_store_si128((__m128i *) H2, h0_128);
			__m128i tmp128 = _mm_subs_epu8(h0_128, o_del128);

			for(k = 1; k < maxLen1; k++)
			{
				tmp128 = _mm_subs_epu8(tmp128, e_del128);
				//__m128i tmp128_ = _mm_max_epi8(tmp128, zero128);    //epi is not present in SSE2
				_mm_store_si128((__m128i *)(H2 + k* SIMD_WIDTH8), tmp128);
			}
//-------------------

            for(j = 0; j < SIMD_WIDTH8; j++)
            {				
                SeqPair sp = pairArray[i + j];
                // seq2 = seqBuf + (2 * (int64_t)sp.id + 1) * MAX_SEQ_LEN;
				seq2 = seqBufQer + (int64_t)sp.idq;
				
                for(k = 0; k < sp.len2; k++)
                {
                    mySeq2SoA[k * SIMD_WIDTH8 + j] = (seq2[k]==AMBIG?0xFF:seq2[k]);
					H1[k * SIMD_WIDTH8 + j] = 0;					
                }
                if(maxLen2 < sp.len2) maxLen2 = sp.len2;
            }
			
			//maxLen2 = ((maxLen2  + 3) >> 2) * 4;
			
            for(j = 0; j < SIMD_WIDTH8; j++)
            {
                SeqPair sp = pairArray[i + j];
                for(k = sp.len2; k <= maxLen2; k++)
                {
                    mySeq2SoA[k * SIMD_WIDTH8 + j] = DUMMY2;
					H1[k * SIMD_WIDTH8 + j] = 0;
                }
            }
//------------------------
			_mm_store_si128((__m128i *) H1, h0_128);
			svbool_t cmp128 = _mm_cmpgt_epi8(h0_128, oe_ins128);
			tmp128 = _mm_sub_epi8(h0_128, oe_ins128);

			tmp128 = _mm_blend_epi8(zero128, tmp128, cmp128);
			_mm_store_si128((__m128i *) (H1 + SIMD_WIDTH8), tmp128);
			for(k = 2; k < maxLen2; k++)
			{
				// __m128i h1_128 = _mm_load_si128((__m128i *) (H1 + (k-1) * SIMD_WIDTH8));
				__m128i h1_128 = tmp128;
				tmp128 = _mm_subs_epu8(h1_128, e_ins128);   // modif
				// tmp128 = _mm_max_epi8(tmp128, zero128);
				_mm_store_si128((__m128i *)(H1 + k*SIMD_WIDTH8), tmp128);
            }			
//------------------------
			uint8_t myband[SIMD_WIDTH8] __attribute__((aligned(256)));
			uint8_t temp[SIMD_WIDTH8] __attribute__((aligned(256)));
			{
				__m128i qlen128 = _mm_load_si128((__m128i *) qlen);
				__m128i sum128 = _mm_add_epi8(qlen128, eb_ins128);
				_mm_store_si128((__m128i *) temp, sum128);				
				for (int l=0; l<SIMD_WIDTH8; l++) {
					double val = temp[l]/e_ins + 1.0;
					int max_ins = (int) val;
					max_ins = max_ins > 1? max_ins : 1;
					myband[l] = min_(bsize, max_ins);
				}
				sum128 = _mm_add_epi8(qlen128, eb_del128);
				_mm_store_si128((__m128i *) temp, sum128);				
				for (int l=0; l<SIMD_WIDTH8; l++) {
					double val = temp[l]/e_del + 1.0;
					int max_ins = (int) val;
					max_ins = max_ins > 1? max_ins : 1;
					myband[l] = min_(myband[l], max_ins);
					bsize = bsize < myband[l] ? myband[l] : bsize;
				}
			}

            smithWaterman128_8(mySeq1SoA,
							   mySeq2SoA,
							   maxLen1,
							   maxLen2,
							   pairArray + i,
							   h0,
							   tid,
							   numPairs,
							   zdrop,
							   bsize,
							   qlen,
							   myband);			
        }
    }
#if RDT
     st4 = ___rdtsc();
#endif
	 
#if SORT_PAIRS       // disbaled in bwa-mem2 (only used in separate benchmark bsw code)
	{
    // Sort the sequences according to increasing order of id
#pragma omp parallel num_threads(numThreads)
    {
        int32_t tid = omp_get_thread_num();
        SeqPair *myTempArray = tempArray + tid * SORT_BLOCK_SIZE;

#pragma omp for
        for(ii = 0; ii < roundNumPairs; ii+=SORT_BLOCK_SIZE)
        {
            int32_t first, last;
            first = ii;
            last  = ii + SORT_BLOCK_SIZE;
            if(last > roundNumPairs) last = roundNumPairs;
            sortPairsId(pairArray + first, first, last - first, myTempArray);
        }
    }
    _mm_free(tempArray);
	}
#endif

#if RDT
	st5 = ___rdtsc();
    setupTicks = st2 - st1;
    sort1Ticks = st3 - st2;
    swTicks = st4 - st3;
    sort2Ticks = st5 - st4;
#endif
	
	// free mem
	_mm_free(seq1SoA);
	_mm_free(seq2SoA);
	
    return;
}

void BandedPairWiseSW::smithWaterman128_8(uint8_t seq1SoA[],
										  uint8_t seq2SoA[],
										  uint8_t nrow,
										  uint8_t ncol,
										  SeqPair *p,
										  uint8_t h0[],
										  uint16_t tid,
										  int32_t numPairs,
										  int zdrop,
										  int32_t w,
										  uint8_t qlen[],
										  uint8_t myband[])
{

    __m128i match128	 = _mm_set1_epi8(this->w_match);
    __m128i mismatch128	 = _mm_set1_epi8(this->w_mismatch);
	__m128i w_ambig_128	 = _mm_set1_epi8(this->w_ambig);	// ambig penalty

	__m128i e_del128	= _mm_set1_epi8(this->e_del);
	__m128i oe_del128	= _mm_set1_epi8(this->o_del + this->e_del);
	__m128i e_ins128	= _mm_set1_epi8(this->e_ins);
	__m128i oe_ins128	= _mm_set1_epi8(this->o_ins + this->e_ins);

    int8_t	*F	 = F8_ + tid * SIMD_WIDTH8 * MAX_SEQ_LEN8;
    int8_t	*H_h = H8_ + tid * SIMD_WIDTH8 * MAX_SEQ_LEN8;
	int8_t	*H_v = H8__ + tid * SIMD_WIDTH8 * MAX_SEQ_LEN8;

    int8_t i, j;

	uint8_t tlen[SIMD_WIDTH8];
	uint8_t tail[SIMD_WIDTH8] __attribute((aligned(256)));
	uint8_t head[SIMD_WIDTH8] __attribute((aligned(256)));
	
	int32_t minq = 10000000;
	for (int l=0; l<SIMD_WIDTH8; l++) {
		tlen[l] = p[l].len1;
		qlen[l] = p[l].len2;
		if (p[l].len2 < minq) minq = p[l].len2;
	}
	minq -= 1; // for gscore

	__m128i tlen128	  = _mm_load_si128((__m128i *) tlen);
	__m128i qlen128	  = _mm_load_si128((__m128i *) qlen);
	__m128i myband128 = _mm_load_si128((__m128i *) myband);
    __m128i zero128	  = _mm_setzero_si128();
	__m128i one128	  = _mm_set1_epi8(1);
	__m128i two128	  = _mm_set1_epi8(2);
	__m128i max_ie128 = zero128;
	__m128i ff128	  = _mm_set1_epi8(0xFF);
	svbool_t ff128_b = svptrue_b8();
		
   	__m128i tail128 = qlen128, head128 = zero128;
	_mm_store_si128((__m128i *) head, head128);
   	_mm_store_si128((__m128i *) tail, tail128);

	__m128i mlen128 = _mm_add_epi8(qlen128, myband128);
	mlen128 = _mm_min_epu8(mlen128, tlen128);
	
	__m128i hval = _mm_load_si128((__m128i *)(H_v));
	//__mmask16 dmask = 0xFFFF;
	
	__m128i maxScore128 = hval;
    for(j = 0; j < ncol; j++)
		_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH8), zero128);
	
	__m128i x128 = zero128;
	__m128i y128 = zero128;
	__m128i gscore = _mm_set1_epi8(-1);
	__m128i max_off128 = zero128;
	//__m128i exit0 = _mm_set1_epi8(0xFF);
    svbool_t exit0 = svptrue_b8();
	__m128i zdrop128 = _mm_set1_epi8(zdrop);
	
	int beg = 0, end = ncol;
	int nbeg = beg, nend = end;

#if RDT
	uint64_t tim = __rdtsc();
#endif
	
    for(i = 0; i < nrow; i++)
    {		
        __m128i e11 = zero128;
        __m128i h00, h11, h10;
        __m128i seq1 = _mm_load_si128((__m128i *)(seq1SoA + (i + 0) * SIMD_WIDTH8));

		beg = nbeg; end = nend;
		// Banding
		if (beg < i - w) beg = i - w;
		if (end > i + w + 1) end = i + w + 1;
		if (end > ncol) end = ncol;

		h10 = zero128;
		if (beg == 0)
			h10 = _mm_load_si128((__m128i *)(H_v + (i+1) * SIMD_WIDTH8));

		__m128i j128 = zero128;
		__m128i maxRS1 = zero128;
		
		__m128i i1_128 = _mm_set1_epi8(i+1);
		__m128i y1_128 = zero128;
		
#if RDT	
		uint64_t tim1 = __rdtsc();
#endif
		
		// Banding
		__m128i i128, cache128;
		__m128i phead128 = head128, ptail128 = tail128;
		i128 = _mm_set1_epi8(i);
		cache128 = _mm_subs_epu8(i128, myband128);  // modif
		head128 = _mm_max_epu8(head128, cache128);   // epi8 not present
		cache128 = _mm_add_epi8(i1_128, myband128);
		tail128 = _mm_min_epu8(tail128, cache128);
		tail128 = _mm_min_epu8(tail128, qlen128);

		// NEW, trimming.
		svbool_t cmph = _mm_cmpeq_epi8(head128, phead128);
		svbool_t cmpt = _mm_cmpeq_epi8(tail128, ptail128);
		cmph = _mm_and_si128(cmph, cmpt);

        //for (int l=beg; l<end && cmp_ht != dmask; l++) {
        //for (int l=beg; l<end && svptest_any(svptrue_b8(),svnot_z(svptrue_b8(),cmph)); l++) {
		//	//__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH8));
		//	//__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH8));

		//	//__m128i pj128 = _mm_set1_epi8(l);
		//	//svbool_t cmp1 = _mm_cmpgt_epi8(head128, pj128);
        //    ////uint32_t cval = _mm_movemask_epi8(cmp1);
        //    ////if (cval == 0x00) break;
        //    //if (!svptest_any(svptrue_b8(),cmp1)) break;
		//	//svbool_t cmp2 = _mm_cmpgt_epi8(pj128, tail128);
		//	//cmp1 = _mm_or_si128(cmp1, cmp2);
		//	//h128 = _mm_blend_epi8(h128, zero128, cmp1);
		//	//f128 = _mm_blend_epi8(f128, zero128, cmp1);

		//	//_mm_store_si128((__m128i *)(F + l * SIMD_WIDTH8), f128);
		//	//_mm_store_si128((__m128i *)(H_h + l * SIMD_WIDTH8), h128);

        //    svint8_t head_sve = svreinterpret_s8(head128);
        //    svint8_t tail_sve = svreinterpret_s8(tail128);
        //    svint8_t zero_sve = svreinterpret_s8(zero128);

		//	svbool_t cmp1 = svcmple(svptrue_b8(), head_sve, l);
        //    if (svcntp_b8(svptrue_b8(),cmp1) == SIMD_WIDTH8) break;
		//	svbool_t cmp2 = svcmpge(svptrue_b8(),tail_sve, l);
		//	cmp1 = svand_z(svptrue_b8(), cmp1, cmp2);
        //    svint8_t h_sve = svld1(cmp1, H_h + l * SIMD_WIDTH8);
        //    svint8_t f_sve = svld1(cmp1, F + l * SIMD_WIDTH8);
		//	svst1(svptrue_b8(), F + l * SIMD_WIDTH8, f_sve);
		//	svst1(svptrue_b8(), H_h + l * SIMD_WIDTH8, h_sve);
		//}

#if RDT
		prof[DP3][0] += __rdtsc() - tim1;
#endif

		svbool_t cmpim = _mm_cmpgt_epi8(i1_128, mlen128);
		svbool_t cmpht = _mm_cmpeq_epi8(tail128, head128);
		cmpim = _mm_or_si128(cmpim, cmpht);
		// NEW
		cmpht = _mm_cmpgt_epi8(head128, tail128);
		cmpim = _mm_or_si128(cmpim, cmpht);

		//exit0 = _mm_blend_epi8(exit0, zero128, cmpim);
        exit0 = _mm_andnot_si128(cmpim, exit0);

#if RDT
		tim1 = __rdtsc();
#endif
		
		j128 = _mm_set1_epi8(beg);
		for(j = beg; j < end; j++)
		{
            __m128i f11, f21, seq2;

			h00 = _mm_load_si128((__m128i *)(H_h + j * SIMD_WIDTH8));
            f11 = _mm_load_si128((__m128i *)(F + j * SIMD_WIDTH8));

            seq2 = _mm_load_si128((__m128i *)(seq2SoA + (j) * SIMD_WIDTH8));
			
			__m128i pj128 = j128;
			j128 = _mm_add_epi8(j128, one128);

            // MAIN CODE
            svbool_t cmp11 = _mm_cmpeq_epi8(seq1, seq2);
            __m128i sbt11 = _mm_blend_epi8(mismatch128, match128, cmp11);
            __m128i tmp128 = _mm_or_si128(seq1, seq2);
            svbool_t tmp128_1 = _mm_cmpeq_epi8(tmp128, ff128);
            sbt11 = _mm_blend_epi8(sbt11, w_ambig_128, tmp128_1);

            // @PREDICATION
#if 0
            __m128i m11 = _mm_add_epi8(h00, sbt11);
            cmp11 = _mm_cmpeq_epi8(h00, zero128);
            m11 = _mm_blend_epi8(m11, zero128, cmp11);
            m11 = _mm_and_si128(m11, svreinterpret_s64(svdup_u8_z(_mm_cmpgt_epi8(m11, zero128),0xFF)));
#else
            __m128i m11;
            {
                svint8_t a_aux = svreinterpret_s8(h00);
                svint8_t b_aux = svreinterpret_s8(sbt11);
                cmp11 = svcmpne_n_s8(svptrue_b8(), a_aux, 0);
                svint8_t r_aux = svadd_z(cmp11,a_aux,b_aux);
                cmp11 = svcmpgt_n_s8(svptrue_b8(), r_aux, 0);
                r_aux = svorr_n_s8_z(cmp11,r_aux,0);
                m11 = svreinterpret_s64(r_aux);
            }
#endif

            h11 = _mm_max_epu8(m11, e11);
            h11 = _mm_max_epu8(h11, f11);
            __m128i temp128 = _mm_subs_epu8(m11, oe_ins128);
            e11 = _mm_subs_epu8(e11, e_ins128);
            e11 = _mm_max_epu8(temp128, e11);
            temp128 = _mm_subs_epu8(m11, oe_del128);
            f21 = _mm_subs_epu8(f11, e_del128);
            f21 = _mm_max_epu8(temp128, f21);

			// Masked writing
			svbool_t cmp1 = _mm_cmpgt_epi8(head128, pj128);
			svbool_t cmp2 = _mm_cmpgt_epi8(pj128, tail128);
			cmp1 = _mm_or_si128(cmp1, cmp2);
			//h10 = _mm_blend_epi8(h10, zero128, cmp1);
			//f21 = _mm_blend_epi8(f21, zero128, cmp1);
			
			// got this block out of MAIN_CODE
			__m128i bmaxRS = maxRS1;										
			maxRS1 =_mm_max_epu8(maxRS1, h11);   // modif
			svbool_t cmpA = _mm_cmpgt_epi8(maxRS1, bmaxRS);					
			svbool_t cmpB = _mm_cmpeq_epi8(maxRS1, h11);					
			cmpA = _mm_or_si128(cmpA, cmpB);								
			__m128i cmpA_1 = _mm_blend_epi8(y1_128, j128, cmpA);
			y1_128 = _mm_blend_epi8(cmpA_1, y1_128, cmp1);
			maxRS1 = _mm_blend_epi8(maxRS1, bmaxRS, cmp1);						

			_mm_store_si128((__m128i *)(H_h + j * SIMD_WIDTH8), h10);
			_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH8), f21);

			h10 = h11;
						
			// gscore calculations
			if (j >= minq)
			{
				svbool_t cmp = _mm_cmpeq_epi8(j128, qlen128);
				svbool_t cmp_gh = _mm_cmpgt_epi8(gscore, h11);
				__m128i tmp128_1 = _mm_blend_epi8(i1_128, max_ie128, cmp_gh);
				__m128i max_gh = _mm_blend_epi8(h11, gscore, cmp_gh);
				
				tmp128_1 = _mm_blend_epi8(max_ie128, tmp128_1, cmp);
				tmp128_1 = _mm_blend_epi8(max_ie128, tmp128_1, exit0);
				
				max_gh = _mm_blend_epi8(gscore, max_gh, exit0);
				max_gh = _mm_blend_epi8(gscore, max_gh, cmp);				
			
				cmp = _mm_cmpgt_epi8(j128, tail128); 
				max_gh = _mm_blend_epi8(max_gh, gscore, cmp);
				max_ie128 = _mm_blend_epi8(tmp128_1, max_ie128, cmp);
				gscore = max_gh;
			}
        }
		_mm_store_si128((__m128i *)(H_h + j * SIMD_WIDTH8), h10);
		_mm_store_si128((__m128i *)(F + j * SIMD_WIDTH8), zero128);
		
		
		/* exit due to zero score by a row */
		//uint16_t cval = 0;
		__m128i bmaxScore128 = maxScore128;
		svbool_t tmp_1 = _mm_cmpeq_epi8(maxRS1, zero128);
        //cval = _mm_movemask_epi8(tmp_1);
        //if (cval == 0xFFFF) break;
        if (!svptest_any(svptrue_b8(),svnot_z(svptrue_b8(),tmp_1))) break;

		//exit0 = _mm_blend_epi8(exit0, zero128,  tmp_1);
        exit0 = _mm_andnot_si128(tmp_1, exit0);

		__m128i score128 = _mm_max_epu8(maxScore128, maxRS1);   // epi8 not present, modif
		maxScore128 = _mm_blend_epi8(maxScore128, score128, exit0);

		svbool_t cmp = _mm_cmpgt_epi8(maxScore128, bmaxScore128);
		y128 = _mm_blend_epi8(y128, y1_128, cmp);
		x128 = _mm_blend_epi8(x128, i1_128, cmp);
		
		// max_off calculations
		__m128i ab = _mm_subs_epu8(y1_128, i1_128);
		__m128i ba = _mm_subs_epu8(i1_128, y1_128);
		__m128i tmp = _mm_or_si128(ab, ba);

		__m128i bmax_off128 = max_off128;
		tmp = _mm_max_epu8(max_off128, tmp);  // modif
		max_off128 = _mm_blend_epi8(bmax_off128, tmp, cmp);

		// Z-score
        __m128i tmpi = _mm_sub_epi8(i1_128, x128);
        __m128i tmpj = _mm_sub_epi8(y1_128, y128);
        cmp = _mm_cmpgt_epi8(tmpi, tmpj);
        score128 = _mm_sub_epi8(maxScore128, maxRS1);
        __m128i insdel = _mm_blend_epi8(e_ins128, e_del128, cmp);
        __m128i sub_a128 = _mm_sub_epi8(tmpi, tmpj);
        __m128i sub_b128 = _mm_sub_epi8(tmpj, tmpi);
        tmp = _mm_blend_epi8(sub_b128, sub_a128, cmp);
        tmp = _mm_sub_epi8(score128, tmp);
        cmp = _mm_cmpgt_epi8(tmp, zdrop128);
        exit0 = _mm_andnot_si128(cmp, exit0);


#if RDT
		prof[DP1][0] += __rdtsc() - tim1;
		tim1 = __rdtsc();
#endif
		
		/* Narrowing of the band */
		/* From beg */
		int l;
 		for (l = beg; l < end; l++)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH8));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH8));
			__m128i tmp = _mm_or_si128(f128, h128);
			svbool_t tmp_1 = _mm_cmpeq_epi8(tmp, zero128);
            //uint16_t val = _mm_movemask_epi8(tmp_1);
            //if (val == 0xFFFF) nbeg = l;
            //else break;
            //if (!svptest_any(svptrue_b8(),svnot_z(svptrue_b8(),tmp_1))) nbeg = l;
            if (svcntp_b8(svptrue_b8(),tmp_1) == SIMD_WIDTH8) nbeg = l;
            else break;
		}
		
		/* From end */
		for (l = end; l >= beg; l--)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH8));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH8));
			__m128i tmp = _mm_or_si128(f128, h128);
			svbool_t tmp_1 = _mm_cmpeq_epi8(tmp, zero128);
            //uint16_t val = _mm_movemask_epi8(tmp_1);
            //if (val != 0xFFFF) break;
            //if (svptest_any(svptrue_b8(),svnot_z(svptrue_b8(),tmp_1))) break;
            if (svcntp_b8(svptrue_b8(),tmp_1) != SIMD_WIDTH8) break;
		}
		// int pnend =nend;
		nend = l + 2 < ncol? l + 2: ncol;
        svbool_t tmpb = ff128_b;

		//__m128i exit1 = _mm_xor_si128(exit0, ff128);
        __m128i exit1 = svreinterpret_s64(svdup_u8_z(svnot_z(svptrue_b8(),exit0),0xFF));
		__m128i l128 = _mm_set1_epi8(beg);
		
		for (l = beg; l < end; l++)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH8));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH8));
	
			__m128i tmp = _mm_or_si128(f128, h128);
			//tmp = _mm_or_si128(tmp, _mm_xor_si128(exit0, ff128));
			tmp = _mm_or_si128(tmp, exit1);			
			svbool_t tmp_1 = _mm_cmpeq_epi8(tmp, zero128);
            //uint32_t val = _mm_movemask_epi8(tmp_1);
            //if (val == 0x00) break;
            if (!svptest_any(svptrue_b8(),tmp_1)) break;

			tmp_1 = _mm_and_si128(tmp_1,tmpb);

			l128 = _mm_add_epi8(l128, one128);
			head128 = _mm_blend_epi8(head128, l128, tmp_1);

			tmpb = tmp_1;			
		}
		
		__m128i  index128 = tail128;
		tmpb = ff128_b;

		l128 = _mm_set1_epi8(end);
		for (l = end; l >= beg; l--)
		{
			__m128i f128 = _mm_load_si128((__m128i *)(F + l * SIMD_WIDTH8));
			__m128i h128 = _mm_load_si128((__m128i *)(H_h + l * SIMD_WIDTH8));
			
			__m128i tmp = _mm_or_si128(f128, h128);
			tmp = _mm_or_si128(tmp, exit1);
			svbool_t tmp_1 = _mm_cmpeq_epi8(tmp, zero128);			
            //uint32_t val = _mm_movemask_epi8(tmp_1);
            //if (val == 0x00) break;
            if (!svptest_any(svptrue_b8(),tmp_1)) break;

			tmp_1 = _mm_and_si128(tmp_1,tmpb);
			l128 = _mm_sub_epi8(l128, one128);
			// NEW
			index128 = _mm_blend_epi8(index128, l128, tmp_1);

			tmpb = tmp_1;
		}
		index128 = _mm_add_epi8(index128, two128);
		tail128 = _mm_min_epu8(index128, qlen128);   // epi8 not present, modif
		
#if RDT
		prof[DP2][0] += __rdtsc() - tim1;
#endif
    }
   
#if RDT
	prof[DP][0] += __rdtsc() - tim;
#endif
	
    int8_t score[SIMD_WIDTH8]  __attribute((aligned(256)));
	_mm_store_si128((__m128i *) score, maxScore128);

	int8_t maxi[SIMD_WIDTH8]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxi, x128);

	int8_t maxj[SIMD_WIDTH8]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxj, y128);

	int8_t max_off_ar[SIMD_WIDTH8]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) max_off_ar, max_off128);

	int8_t gscore_ar[SIMD_WIDTH8]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) gscore_ar, gscore);

	int8_t maxie_ar[SIMD_WIDTH8]  __attribute((aligned(256)));
    _mm_store_si128((__m128i *) maxie_ar, max_ie128);
	
    for(i = 0; i < SIMD_WIDTH8; i++)
    {
		p[i].score = score[i];
		p[i].tle = maxi[i];
		p[i].qle = maxj[i];
		p[i].max_off = max_off_ar[i];
		p[i].gscore = gscore_ar[i];
		p[i].gtle = maxie_ar[i];
    }
	
    return;
}

