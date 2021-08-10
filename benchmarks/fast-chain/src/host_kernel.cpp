#include <vector>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <cmath>
#include "omp.h"
#include "host_kernel.h"
#include "common.h"
// #include "minimap.h"
// #include "mmpriv.h"
// #include "kalloc.h"
#ifdef __ARM_FEATURE_SVE
    #include <arm_sve.h>
#endif /* __ARM_FEATURE_SVE */

static inline int32_t ilog2_32(uint32_t v) {
    constexpr uint32_t base = 31;
    const uint32_t leading_zeros = __builtin_clz(v);

    return base - leading_zeros;
}

const int BACKSEARCH = 65;
#define MM_SEED_SEG_SHIFT  48
#define MM_SEED_SEG_MASK   (0xffULL<<(MM_SEED_SEG_SHIFT))

// void chain_dp_sve(call_t *a, return_t *ret, int32_t *anchor_r, int32_t *anchor_q, int32_t *anchor_l) {
static void chain_dp(call_t *a, return_t *ret) {
    // constexpr float gap_scale = 1.0f;
    // constexpr int max_skip = 25;
    // constexpr int is_cdna = 0;
    constexpr int max_iter = 5000;

    const auto max_dist_x = a->max_dist_x;
    const auto max_dist_y = a->max_dist_y;
    const auto bw = a->bw;

    const auto avg_qspan = a->avg_qspan;

    // const auto n_segs = a->n_segs;
    const auto n = a->n;

    ret->n = n;
    ret->scores.resize(n);
    ret->parents.resize(n);
    ret->targets.resize(n);
    ret->peak_scores.resize(n);

    // TODO: GET THIS OUT OF HERE.
    // Array of structs to structs of arrays.
    std::vector<uint64_t> anchors_x; // ERROR: THIS CAN NOT BE uint32, what can we do?
    anchors_x.reserve(n);
    std::vector<uint64_t> anchors_y;
    anchors_y.reserve(n);

    for (int64_t i = 0; i < n; ++i) {
        anchors_x.push_back(a->anchors[i].x);
        anchors_y.push_back(a->anchors[i].y);
    }

    int64_t st = 0;

    // fill the score and backtrack arrays
    for (int64_t i = 0; i < n; ++i) {
        const auto ri = anchors_x[i];
        const int32_t qi = static_cast<int32_t>(anchors_y[i]);
        const int32_t q_spani = anchors_y[i] >> 32 & 0xff;

        int64_t max_j = -1;
        int32_t max_f = q_spani;

        while (st < i && ri > anchors_x[st] + max_dist_x) {
            ++st; //predecessor's position is too far
        }
        if (i - st > max_iter) {
            st = i - max_iter; //predecessor's index is too far
        }

        for (int64_t j = i - 1; j >= st; --j) {
            const auto rj = anchors_x[j];
            const int32_t qj = static_cast<int32_t>(anchors_y[j]);

            const int64_t dr = ri - rj;
            const int32_t dq = qi - qj;

            const int32_t dd = std::abs(dr - dq);

            if ((dr == 0 || dq <= 0) ||
               (dq > max_dist_y || dq > max_dist_x) ||
               (dd > bw)) {
                continue;
            }
            // Can not vectorize "continue". Use a mask instead.
            // const bool skip = ((dr == 0 || dq <= 0) ||
            //                    (dq > max_dist_y || dq > max_dist_x) ||
            //                    (dd > bw));

            const int64_t dr_dq_min = (dr < dq) ? dr : dq;
            const int32_t oc = (dr_dq_min < q_spani) ? dr_dq_min : q_spani;

            // TODO: CAN'T VECTORIZE __builtin_clz 
            const int32_t log_dd = (dd) ? ilog2_32(dd) : 0;
            const int32_t gap_cost = static_cast<int>(dd * 0.01f * avg_qspan) + (log_dd >> 1);

            const int32_t score = ret->scores[j] + oc - gap_cost;

            // TODO: CAN'T VECTORIZE THIS. THE COMPILER IS ONLY ABLE TO PERFORM
            // ONE REDUCTION.
            // Ideas:
            //     1. j to int32_t and: uint64_t max = (j << 32) & score.  
            //        max = ((int32_t)(max & 0xffffffff) > score) ? max = (j << 32) & score : max;
            // Reduction
            if (score > max_f) {
                max_f = score;
                max_j = j;
            }
        }
        ret->scores[i] = max_f;
        ret->parents[i] = max_j;
        ret->peak_scores[i] = max_j >= 0 && ret->peak_scores[max_j] > max_f ? ret->peak_scores[max_j] : max_f;
    }
}

void host_chain_kernel(std::vector<call_t> &args, std::vector<return_t> &rets, int numThreads) {
    #pragma omp parallel num_threads(numThreads)
    {
        #pragma omp for schedule(dynamic)
        for (size_t batch = 0; batch < args.size(); batch++) {
            call_t *arg = &args[batch];
            return_t *ret = &rets[batch];
            // fprintf(stderr, "%lld\t%f\t%d\t%d\t%d\t%d\n", arg->n, arg->avg_qspan, arg->max_dist_x, arg->max_dist_y, arg->bw, arg->n_segs);
            chain_dp(arg, ret);
        }
    }
}