#include "native_stats.h"
#include <math.h>
#include <stdlib.h>
int native_stats_compute(const uint64_t *x,size_t n,NativeStats *o){if(!x||!o||!n)return 0;o->count=n;o->min=o->max=x[0];o->mean=0;for(size_t i=0;i<n;i++){if(x[i]<o->min)o->min=x[i];if(x[i]>o->max)o->max=x[i];/* incremental mean avoids sum overflow */o->mean+=(long double)x[i]/(long double)n;}return isfinite((double)o->mean);}
int native_stats_percentile_sorted(const uint64_t*x,size_t n,long double p,uint64_t*out){if(!x||!n||!out||p<0||p>1||!isfinite((double)p))return 0;size_t rank=(size_t)ceill(p*(long double)n);if(rank==0)rank=1;*out=x[rank-1];return 1;}
int native_stats_median_sorted(const uint64_t*x,size_t n,long double*out){if(!x||!n||!out)return 0;if(n&1U)*out=(long double)x[n/2];else *out=((long double)x[n/2-1]+(long double)x[n/2])/2.0L;return isfinite((double)*out);}
void native_stats_sort(uint64_t*x,size_t n){if(!x)return;for(size_t i=1;i<n;i++){uint64_t value=x[i];size_t j=i;while(j>0&&x[j-1]>value){x[j]=x[j-1];j--;}x[j]=value;}}
