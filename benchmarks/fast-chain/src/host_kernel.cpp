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

static const char LogTable256[256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
    };

static inline int ilog2_32(uint32_t v) {
    uint32_t t, tt;
    if ((tt = v >> 16)) {
        return (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
    }
    return (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
}

const int BACKSEARCH = 65;
#define MM_SEED_SEG_SHIFT  48
#define MM_SEED_SEG_MASK   (0xffULL<<(MM_SEED_SEG_SHIFT))

// void chain_dp_sve(call_t *a, return_t *ret, int32_t *anchor_r, int32_t *anchor_q, int32_t *anchor_l) {
static void chain_dp(call_t *a, return_t *ret) {
    constexpr float gap_scale = 1.0f;
    constexpr int max_iter = 5000;
    constexpr int max_skip = 25;
    constexpr int is_cdna = 0;

    const auto max_dist_x = a->max_dist_x;
    const auto max_dist_y = a->max_dist_y;
    const auto bw = a->bw;

    const auto avg_qspan = a->avg_qspan;

    const auto n_segs = a->n_segs;
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
    std::vector<int32_t> anchors_q_span;
    anchors_q_span.reserve(n);
    // std::vector<int32_t> anchors_sid;
    // anchors_sid.reserve(n);

    for (int64_t i = 0; i < n; ++i) {
        anchors_x.push_back(a->anchors[i].x);
        anchors_y.push_back(a->anchors[i].y);
        anchors_q_span.push_back(a->anchors[i].y >> 32 & 0xff);
        // anchors_sid.push_back((a->anchors[i].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT);
    }

    int64_t st = 0;

    // fill the score and backtrack arrays
    for (int32_t i = 0; i < n; ++i) {
        const auto ri = anchors_x[i];
        const auto qi = anchors_y[i];
        const auto q_spani = anchors_q_span[i];

        int32_t max_j = -1;
        int32_t max_f = q_spani;

        std::pair<int32_t, int32_t> max = std::make_pair(-1, q_spani);

        int32_t n_skip = 0;

        while (st < i && ri > anchors_x[st] + max_dist_x) {
            ++st; //predecessor's position is too far
        }
        if (i - st > max_iter) {
            st = i - max_iter; //predecessor's index is too far
        }

        #pragma omp declare reduction \
            (maxpair:std::pair<int32_t,int32_t>:omp_out=omp_in.second > omp_out.second ? omp_in : omp_out) \
            initializer(omp_priv=std::make_pair(-1, std::numeric_limits<std::int32_t>::min()))

        #pragma omp simd reduction(maxpair:max)
        //#pragma omp simd reduction(max:max_f)
        for (int32_t j = i - 1; j >= st; --j) {
            const auto rj = anchors_x[j];
            const auto qj = anchors_y[j];

            int32_t dr = ri - rj;
            int32_t dq = qi - qj;

            int32_t dd = std::abs(dr - dq);

            if ((dr == 0 || dq <= 0) ||
               (dq > max_dist_y || dq > max_dist_x) ||
               (dd > bw)) {
                continue;
            }

            int32_t oc = (dq < dr) ? dq : dr;
            oc = (oc > q_spani) ? q_spani : oc;

            int32_t score = ret->scores[j] + oc;

            int32_t log_dd = (dd) ? ilog2_32(dd) : 0;

            int32_t gap_cost = (int)(dd * .01 * avg_qspan) + (log_dd >> 1);

            score -= gap_cost;

            // max_j = (score > max_f) ? j : max_j;
            max = (score > max.second) ? std::make_pair(j, score) : max;

            // if (score > max_f) {
            //     max_f = score;
            //     max_j = j;
            // }
        }
        ret->scores[i] = max.second;
        ret->parents[i] = max.first;
        ret->peak_scores[i] = max.first >= 0 && ret->peak_scores[max.first] > max.second ? ret->peak_scores[max.first] : max.second;
        //ret->scores[i] = max_f;
        //ret->parents[i] = max_j;
        //ret->peak_scores[i] = max_j >= 0 && ret->peak_scores[max_j] > max_f ? ret->peak_scores[max_j] : max_f;
    }
    // // fill the score and backtrack arrays
    // for (int32_t i = 0; i < n; ++i) {
    //     const auto ri = anchors_x[i];

    //     const auto qi = (int32_t)anchors_y[i];
    //     const auto q_span = anchors_q_span[i];

    //     const auto sidi = (anchors_y[i] & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;

    //     int32_t max_j = -1;
    //     int32_t max_f = q_span;
    //     int32_t n_skip = 0;
    //     int32_t min_d;

    //     while (st < i && ri > anchors_x[st] + max_dist_x) {
    //         ++st;
    //     }
    //     if (i - st > max_iter) {
    //         st = i - max_iter;
    //     }

    //     svbool_t all = svptrue_b32();
    //     svbool_t pg;
        
    //     for (int64_t j = i - 1; svptest_first(all, pg = svwhilege_b32(j, st)); j -= svcntd()) {
    //         svuint32_t anchor_x_j = svld1_u32(pg, &anchors_x[j]); // anchor_x_j = anchors_x[j]
    //         svuint32_t anchor_y_j = svld1_u32(pg, &anchors_y[j]); // anchor_y_j = anchors_y[j]

    //         // int32_t dr = ri - anchors_x[j];
    //         svint32_t dr = svsubr_s32_x(pg, anchor_x_j, ri);

    //         // int32_t dq = qi - (int32_t)anchors_y[j];
    //         svint32_t dq = svsubr_s32_x(pg, anchor_y_j, qi);

    //         // int32_t sidj = (anchors_y[j] & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;
    //         svint32_t sidj = svand_(pg, &anchors_y[j]);
            
    //         // if ((sidi == sidj && dr == 0) || dq <= 0) {
    //         //     continue;    // don't skip if an anchor is used by multiple segments; see below
    //         // }
    //         // if ((sidi == sidj && dq > max_dist_y) || dq > max_dist_x) {
    //         //     continue;
    //         // }
    //         int32_t dd = dr > dq ? dr - dq : dq - dr;
    //         // if (sidi == sidj && dd > bw) {
    //         //     continue;
    //         // }
    //         // if (n_segs > 1 && !is_cdna && sidi == sidj && dr > max_dist_y) {
    //         //     continue;
    //         // }
    //         min_d = dq < dr ? dq : dr;
    //         int32_t sc = min_d > q_span ? q_span : dq < dr ? dq : dr;
    //         int32_t log_dd = dd ? ilog2_32(dd) : 0;
    //         int32_t gap_cost = 0;
    //         if (is_cdna || sidi != sidj) {
    //             int c_log, c_lin;
    //             c_lin = (int)(dd * .01 * avg_qspan);
    //             c_log = log_dd;
    //             if (sidi != sidj && dr == 0) {
    //                 ++sc;    // possibly due to overlapping paired ends; give a minor bonus
    //             }
    //             else if (dr > dq || sidi != sidj) {
    //                 gap_cost = c_lin < c_log ? c_lin : c_log;
    //             }
    //             else {
    //                 gap_cost = c_lin + (c_log >> 1);
    //             }
    //         }
    //         else {
    //             gap_cost = (int)(dd * .01 * avg_qspan) + (log_dd >> 1);
    //         }
    //         sc -= (int)((double)gap_cost * gap_scale + .499);
    //         sc += ret->scores[j];
    //         // if (sc > max_f) {
    //         //     max_f = sc, max_j = j;
    //         //     if (n_skip > 0) {
    //         //         --n_skip;
    //         //     }
    //         // }
    //         // else if (ret->targets[j] == i) {
    //         //     if (++n_skip > max_skip) {
    //         //         break;
    //         //     }
    //         // }
    //         if (ret->parents[j] >= 0) {
    //             ret->targets[ret->parents[j]] = i;
    //         }
    //     }
    //     ret->scores[i] = max_f;
    //     ret->parents[i] = max_j;
    //     ret->peak_scores[i] = max_j >= 0 && ret->peak_scores[max_j] > max_f ? ret->peak_scores[max_j] : max_f;
    // }
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