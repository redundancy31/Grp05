#include "native_stats.h"
#include "project_config.h"
#include "../provided/timing.h"
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define mkdir_one(p) _mkdir(p)
#define PATH_SEP '\\'
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define mkdir_one(p) mkdir(p,0700)
#define PATH_SEP '/'
#endif
#define MAX_SAMPLES 1000
#define CAPTURE_SIZE (1024U*1024U)
typedef struct {uint64_t wall,counting,loading,compute,rss,logical_allocated,logical_used;unsigned long long trades;long long buys,sells,bars;double vwap;int has_vwap,has_buys,has_sells,has_bars;char metric[48];uint64_t checksum;} Sample;
typedef struct {long double median,mean,tps;uint64_t p50,p99,min,max,peak_rss,logical_allocated,logical_used;unsigned long long trades;size_t n;int selected;char metric[48];} Summary;
static uint64_t hash_bytes(uint64_t h,const unsigned char*p,size_t n){for(size_t i=0;i<n;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;}
static int dataset_identity(const char*path,uint64_t*size,uint64_t*hash){FILE*f=fopen(path,"rb");if(!f)return 0;unsigned char b[65536];size_t n;*size=0;*hash=UINT64_C(14695981039346656037);while((n=fread(b,1,sizeof b,f))){if(UINT64_MAX-*size<n){fclose(f);return 0;}*size+=(uint64_t)n;*hash=hash_bytes(*hash,b,n);}int ok=!ferror(f);return fclose(f)==0&&ok;}
static int key_u64(const char*line,const char*key,uint64_t*out){const char*p=strstr(line,key);if(!p)return 0;p+=strlen(key);char*e;errno=0;unsigned long long v=strtoull(p,&e,10);if(errno||e==p)return 0;*out=(uint64_t)v;return 1;}
static int pathcat(char*out,size_t n,const char*a,const char*b){int z=snprintf(out,n,"%s%c%s",a,PATH_SEP,b);return z>=0&&(size_t)z<n;}
#if defined(_WIN32)
static int quote_arg(char*cmd,size_t cap,size_t*at,const char*s){if(*at+2>=cap)return 0;cmd[(*at)++]='"';while(*s){size_t bs=0;while(s[bs]=='\\')bs++;s+=bs;if(*s=='"'){if(*at+2*bs+2>=cap)return 0;while(bs--)cmd[(*at)++]='\\';cmd[(*at)++]='\\';cmd[(*at)++]='"';s++;}else if(!*s){if(*at+2*bs+1>=cap)return 0;while(bs--)cmd[(*at)++]='\\';break;}else{if(*at+bs+1>=cap)return 0;while(bs--)cmd[(*at)++]='\\';cmd[(*at)++]=*s++;}}if(*at+2>=cap)return 0;cmd[(*at)++]='"';cmd[(*at)++]=' ';cmd[*at]=0;return 1;}
#endif
static int run_child(const char*exe,char*const av[],const char*outpath){
#if defined(_WIN32)
 SECURITY_ATTRIBUTES sa={sizeof sa,NULL,TRUE};HANDLE file=CreateFileA(outpath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return -1;char cmd[8192]={0};size_t at=0;for(size_t i=0;av[i];i++)if(!quote_arg(cmd,sizeof cmd,&at,av[i])){CloseHandle(file);return -1;}if(at)cmd[at-1]=0;STARTUPINFOA si={0};PROCESS_INFORMATION pi={0};si.cb=sizeof si;si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);si.hStdOutput=file;si.hStdError=file;BOOL ok=CreateProcessA(exe,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);CloseHandle(file);if(!ok)return -1;WaitForSingleObject(pi.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return (int)code;
#else
 pid_t p=fork();if(p<0)return -1;if(p==0){int fd=open(outpath,O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0)_exit(126);dup2(fd,1);dup2(fd,2);close(fd);execv(exe,av);_exit(127);}int status;if(waitpid(p,&status,0)<0)return -1;return WIFEXITED(status)?WEXITSTATUS(status):128;
#endif
}
static int read_capture(const char*path,Sample*s,int phases){FILE*f=fopen(path,"rb");if(!f)return 0;char*buf=malloc(CAPTURE_SIZE+1);if(!buf){fclose(f);return 0;}uint64_t h=UINT64_C(14695981039346656037);size_t n;int found=0;while((n=fread(buf,1,CAPTURE_SIZE,f))>0){buf[n]=0;h=hash_bytes(h,(unsigned char*)buf,n);char*save=NULL;for(char*line=strtok_r(buf,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){unsigned long long tr;long long count;double v;if(sscanf(line,"trades=%llu",&tr)==1){s->trades=tr;found|=1;}if(sscanf(line,"final_vwap=%lf",&v)==1){s->vwap=v;s->has_vwap=1;}if(sscanf(line,"buy_signals=%lld",&count)==1){s->buys=count;s->has_buys=1;}if(sscanf(line,"sell_signals=%lld",&count)==1){s->sells=count;s->has_sells=1;}if(sscanf(line,"ohlc_bars=%lld",&count)==1){s->bars=count;s->has_bars=1;}if(phases&&strncmp(line,"PHASES|",7)==0){char m[48]="unavailable";if(key_u64(line,"counting_ns=",&s->counting)&&key_u64(line,"loading_ns=",&s->loading)&&key_u64(line,"compute_ns=",&s->compute)&&key_u64(line,"process_peak_memory_bytes=",&s->rss)){key_u64(line,"logical_allocated_bytes=",&s->logical_allocated);key_u64(line,"logical_used_bytes=",&s->logical_used);const char*q=strstr(line,"process_memory_metric=");if(q){q+=22;size_t z=strcspn(q,"|\r\n");if(z>=sizeof m)z=sizeof m-1;memcpy(m,q,z);m[z]=0;snprintf(s->metric,sizeof s->metric,"%s",m);}found|=2;}}}if(ferror(f)){free(buf);fclose(f);return 0;}}
 free(buf);if(fclose(f))return 0;s->checksum=h;return (found&1)&&s->has_vwap&&s->has_buys&&s->has_sells&&s->has_bars&&(!phases||found==3);}
static int mkdir_checked(const char*p){if(mkdir_one(p)==0)return 1;if(errno==EEXIST)return 0;return -1;}
static int write_run(const char*exe,const char*dataset,int sw,int lw,const char*capture,int with_phases,Sample*s){char swb[24],lwb[24];snprintf(swb,sizeof swb,"%d",sw);snprintf(lwb,sizeof lwb,"%d",lw);char*args[6]={(char*)exe,(char*)dataset,swb,lwb,NULL,NULL};int n=4;if(with_phases)args[n++]="--bench-phases";args[n]=NULL;uint64_t start=now_ns();int rc=run_child(exe,args,capture);s->wall=now_ns()-start;if(rc!=0){fprintf(stderr,"benchmark: executable failed (%d): %s\n",rc,exe);return 0;}return read_capture(capture,s,with_phases);}
static int same_result(const Sample*a,const Sample*b){return a->trades==b->trades&&a->buys==b->buys&&a->sells==b->sells&&a->bars==b->bars&&a->has_vwap&&b->has_vwap&&fabs(a->vwap-b->vwap)<=1e-5;}
static int select_version(const char*id,size_t i){const TTVersion*v=tt_version_at(i);return !strcmp(id,"all")||!strcmp(id,v->id)||!strcmp(v->id,"v0_baseline_strtok_running_double_heap_O0");}
int main(int argc,char**argv){
 if(argc!=5&&argc!=7){fprintf(stderr,"Usage: %s DATASET BUILD_DIR REPEATS VERSION_ID|all [SHORT_WINDOW LONG_WINDOW]\n",argv[0]);return 2;}char*end;long repeats=strtol(argv[3],&end,10);if(*end||repeats<5||repeats>MAX_SAMPLES){fputs("benchmark: repeats must be 5..1000\n",stderr);return 2;}
 int sw=30,lw=60;char dataset[2048],error[256];if(!tt_resolve_dataset(argv[1],dataset,sizeof dataset,&sw,&lw,error,sizeof error)){fprintf(stderr,"benchmark: %s\n",error);return 2;}
 if(argc==7){char*e1,*e2;long a=strtol(argv[5],&e1,10),b=strtol(argv[6],&e2,10);if(*e1||*e2||a<1||b<=a||b>100000){fputs("benchmark: windows must satisfy 1 <= short < long <= 100000\n",stderr);return 2;}sw=(int)a;lw=(int)b;}
 if(strcmp(argv[4],"all")&&!tt_version_find(argv[4])){fprintf(stderr,"benchmark: unknown version ID %s\n",argv[4]);return 2;}
 char results[2048],run[2048],name[80];if(!pathcat(results,sizeof results,argv[2],"benchmark-results")){fputs("benchmark: build path too long\n",stderr);return 2;}int mr=mkdir_checked(results);if(mr<0){perror(results);return 2;}uint64_t stamp=now_ns();snprintf(name,sizeof name,"run-%" PRIu64,stamp);if(!pathcat(run,sizeof run,results,name)){fputs("benchmark: output path too long\n",stderr);return 2;}if(mkdir_checked(run)!=1){fprintf(stderr,"benchmark: output directory collision or creation failure: %s\n",run);return 2;}
 char rawpath[2200],summarypath[2200],capture[2200],manifestpath[2200];if(!pathcat(rawpath,sizeof rawpath,run,"raw_samples.csv")||!pathcat(summarypath,sizeof summarypath,run,"summary.csv")||!pathcat(capture,sizeof capture,run,"capture.tmp")||!pathcat(manifestpath,sizeof manifestpath,run,"manifest.txt"))return 2;
 FILE*raw=fopen(rawpath,"wb");if(!raw){perror(rawpath);return 2;}fprintf(raw,"version_id,source_identity,compiler_flags,repeat,process_wall_ns,counting_ns,loading_ns,compute_ns,logical_reserved_bytes,logical_used_bytes,peak_process_memory_bytes,memory_metric,trades,throughput_trades_per_second,timing_scope,output_checksum_fnv64\n");
 size_t selected=0;Summary summary[11]={0};
 for(size_t i=0;i<tt_version_count();i++){if(select_version(argv[4],i))selected++;}
 if(!selected){fputs("benchmark: no versions selected\n",stderr);return 2;}
 Sample expected={0};int baseline_set=0;long double baseline_wall_median=0;
 for(size_t i=0;i<tt_version_count();i++){const TTVersion*v=tt_version_at(i);if(!select_version(argv[4],i))continue;char binname[256],exe[2200];
#if defined(_WIN32)
 snprintf(binname,sizeof binname,"%s.exe",v->id);
#else
 snprintf(binname,sizeof binname,"%s",v->id);
#endif
 char ladder[2048];if(!pathcat(ladder,sizeof ladder,argv[2],"ladder")||!pathcat(exe,sizeof exe,ladder,binname)){fputs("benchmark: executable path too long\n",stderr);fclose(raw);return 2;}if(!tt_version_find(v->id)){fputs("benchmark: registry error\n",stderr);return 2;}
 Sample gate={0};if(!write_run(exe,dataset,sw,lw,capture,0,&gate)){fprintf(stderr,"benchmark: correctness precheck failed for %s\n",v->id);fclose(raw);return 3;}if(!baseline_set){expected=gate;baseline_set=1;}else if(!same_result(&gate,&expected)){fprintf(stderr,"benchmark: correctness output mismatch for %s (trades/signals/OHLC/VWAP)\n",v->id);fclose(raw);return 3;}
 uint64_t walls[MAX_SAMPLES];Sample samples[MAX_SAMPLES];for(long r=0;r<repeats;r++){Sample s={0};if(!write_run(exe,dataset,sw,lw,capture,1,&s)||!same_result(&s,&gate)){fprintf(stderr,"benchmark: incomplete or changed result for %s repeat %ld\n",v->id,r+1);fclose(raw);return 3;}walls[r]=s.wall;samples[r]=s;double tps=s.wall?(double)((long double)s.trades*1000000000.0L/(long double)s.wall):0.0;fprintf(raw,"%s,%s,\"%s\",%ld,%" PRIu64 ",",v->id,v->source,v->flags,r+1,s.wall);if(s.counting)fprintf(raw,"%" PRIu64,s.counting);else fputs("NA",raw);fprintf(raw,",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",",s.loading,s.compute,s.logical_allocated,s.logical_used);if(s.rss&&s.metric[0])fprintf(raw,"%" PRIu64,s.rss);else fputs("NA",raw);fprintf(raw,",%s,%llu,%.9g,process_wall,%" PRIu64 "\n",s.metric[0]?s.metric:"unavailable",s.trades,tps,s.checksum);if(ferror(raw)){perror(rawpath);fclose(raw);return 2;}}
 native_stats_sort(walls,(size_t)repeats);NativeStats st;if(!native_stats_compute(walls,(size_t)repeats,&st)){fputs("benchmark: stats failure\n",stderr);fclose(raw);return 2;}uint64_t p50,p99;long double median;if(!native_stats_median_sorted(walls,(size_t)repeats,&median)||!native_stats_percentile_sorted(walls,(size_t)repeats,.50L,&p50)||!native_stats_percentile_sorted(walls,(size_t)repeats,.99L,&p99)){fputs("benchmark: percentile failure\n",stderr);fclose(raw);return 2;}if(!strcmp(v->id,"v0_baseline_strtok_running_double_heap_O0"))baseline_wall_median=median;
 summary[i].median=median;summary[i].p50=p50;summary[i].p99=p99;summary[i].min=st.min;summary[i].max=st.max;summary[i].mean=st.mean;summary[i].n=st.count;summary[i].trades=samples[0].trades;summary[i].tps=median?((long double)summary[i].trades*1000000000.0L/median):0;summary[i].selected=1;for(long r=0;r<repeats;r++){if(samples[r].rss>summary[i].peak_rss)summary[i].peak_rss=samples[r].rss;if(samples[r].logical_allocated>summary[i].logical_allocated)summary[i].logical_allocated=samples[r].logical_allocated;if(samples[r].logical_used>summary[i].logical_used)summary[i].logical_used=samples[r].logical_used;snprintf(summary[i].metric,sizeof summary[i].metric,"%s",samples[r].metric);}
 }
 if(fclose(raw)){perror(rawpath);return 2;}
 FILE*sf=fopen(summarypath,"wb");if(!sf){perror(summarypath);return 2;}fprintf(sf,"version_id,source_identity,mean_process_wall_ns,median_process_wall_ns,p50_ns,p99_ns,min_ns,max_ns,throughput_trades_per_second,speedup_vs_v0,peak_process_memory_bytes,memory_metric,logical_reserved_bytes,logical_used_bytes,repeat_count,compiler_optimization,compiler_flags\n");
 for(size_t i=0;i<tt_version_count();i++)if(summary[i].selected){const TTVersion*v=tt_version_at(i);double tps=(double)summary[i].tps,mean=(double)summary[i].mean,median=(double)summary[i].median,speedup=(double)(baseline_wall_median/summary[i].median);fprintf(sf,"%s,%s,%.9g,%.9g,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.9g,",v->id,v->source,mean,median,summary[i].p50,summary[i].p99,summary[i].min,summary[i].max,tps);if(baseline_wall_median)fprintf(sf,"%.9g,",speedup);else fputs("NA,",sf);if(summary[i].peak_rss&&summary[i].metric[0])fprintf(sf,"%" PRIu64,summary[i].peak_rss);else fputs("NA",sf);fprintf(sf,",%s,",summary[i].metric[0]?summary[i].metric:"unavailable");if(summary[i].logical_allocated)fprintf(sf,"%" PRIu64,summary[i].logical_allocated);else fputs("NA",sf);fputc(',',sf);if(summary[i].logical_used)fprintf(sf,"%" PRIu64,summary[i].logical_used);else fputs("NA",sf);fprintf(sf,",%zu,%s,\"%s\"\n",summary[i].n,v->optimization,v->flags);}
 if(fclose(sf)){perror(summarypath);return 2;}FILE*mf=fopen(manifestpath,"wb");if(!mf){perror(manifestpath);return 2;}uint64_t dataset_bytes=0,dataset_hash=0;if(!dataset_identity(dataset,&dataset_bytes,&dataset_hash)){perror("benchmark dataset identity");fclose(mf);return 2;}fprintf(mf,"status=complete\ndataset=%s\ndataset_bytes=%" PRIu64 "\ndataset_fnv1a64_noncryptographic=%" PRIu64 "\nwindows=%d/%d\nrepeats=%ld\nselected=%s\nplatform=%s\ncompiler=%s\ntiming_scope=process_wall; replay excludes build and plotting\nwarmup=one correctness-gate process per version; not included in samples\noutput_mode=summary only; PHASES metrics captured separately\npercentile=nearest-rank\n",dataset,dataset_bytes,dataset_hash,sw,lw,repeats,argv[4],
#if defined(_WIN32)
 "Windows",
#else
 "POSIX",
#endif
 __VERSION__);fclose(mf);remove(capture);fprintf(stderr,"benchmark complete: %s\nraw=%s\nsummary=%s\n",run,rawpath,summarypath);return 0;
}
