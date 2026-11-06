#ifndef NATIVE_STATS_H
#define NATIVE_STATS_H
#include <stddef.h>
#include <stdint.h>
typedef struct { size_t count; uint64_t min,max; long double mean; } NativeStats;
int native_stats_compute(const uint64_t *samples,size_t count,NativeStats *out);
/* Nearest-rank percentile: sorted[ceil(p*n)-1], p in [0,1]. */
int native_stats_percentile_sorted(const uint64_t *sorted,size_t count,long double p,uint64_t *out);
int native_stats_median_sorted(const uint64_t *sorted,size_t count,long double *out);
void native_stats_sort(uint64_t *values,size_t count);
#endif
