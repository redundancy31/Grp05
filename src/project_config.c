#include "project_config.h"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif
#if defined(__linux__)
#define POSIXFLAG "-D_POSIX_C_SOURCE=200809L "
#else
#define POSIXFLAG ""
#endif
#define BASEFLAGS "-std=c11 -Wall -Wextra -Wpedantic -Iinclude " POSIXFLAG
#define VARFLAGS "-std=c11 -Wall -Wextra -Wpedantic -Werror -DNDEBUG -Iinclude -Iprovided " POSIXFLAG
static const TTVersion versions[]={
 {"v0_baseline_strtok_running_double_heap_O0","src/market_baseline.c","-O0",BASEFLAGS "-g3 -O0"},
 {"v1_baseline_strtok_running_double_heap_O2","src/market_baseline.c","-O2",BASEFLAGS "-O2 -DNDEBUG"},
 {"v2_helper_sscanf_naive_double_stream_O2","src/market_variant.c + provided/trade_io.c","-O2",VARFLAGS "-O2 -DREADER_KIND=0 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0"},
 {"v3_own_pointer_reader_naive_double_stream_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=1 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0"},
 {"v4_manual_digit_parser_naive_double_stream_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0"},
 {"v5_manual_digit_parser_running_double_stream_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 -DFIXED_TRADE_CAPACITY=12000000U"},
 {"v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=1 -DSTORAGE_KIND=0"},
 {"v7_manual_digit_parser_running_double_stream_O3","src/market_variant.c","-O3",VARFLAGS "-O3 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 -DFIXED_TRADE_CAPACITY=12000000U"},
 {"v8_manual_running_double_static_trade_array_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=1 -DFIXED_TRADE_CAPACITY=12000000U"},
 {"v9_manual_running_double_exact_malloc_trade_array_O2","src/market_variant.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=2 -DFIXED_TRADE_CAPACITY=12000000U"},
 {"v10_manual_running_double_soa_O2","src/market_soa.c","-O2",VARFLAGS "-O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=3 -DFIXED_TRADE_CAPACITY=12000000U"}};
size_t tt_version_count(void){return sizeof versions/sizeof versions[0];}
const TTVersion*tt_version_at(size_t i){return i<tt_version_count()?&versions[i]:NULL;}
const TTVersion*tt_version_find(const char*id){if(!id)return NULL;for(size_t i=0;i<tt_version_count();i++)if(!strcmp(id,versions[i].id))return &versions[i];return NULL;}
static int exists(const char*p){return access(p,F_OK)==0;}
static void err(char*out,size_t n,const char*m){if(n)snprintf(out,n,"%s",m);}
int tt_resolve_dataset(const char*name,char*out,size_t cap,int*sw,int*lw,char*error,size_t ec){
 if(!name||!out||!cap||!sw||!lw){err(error,ec,"invalid dataset resolver arguments");return 0;}
 const char*files[3]={0};int ds=30,dl=60;
 if(!strcmp(name,"test20")){files[0]="test20 - Copy (6).csv";ds=3;dl=5;}
 else if(!strcmp(name,"dev")||!strcmp(name,"development"))files[0]="dev_1M - Copy (6).csv";
 else if(!strcmp(name,"stress"))files[0]="stress_full.csv";
 else {if(exists(name)){if(strlen(name)>=cap){err(error,ec,"dataset path exceeds output buffer");return 0;}strcpy(out,name);if(strstr(name,"test20")){ds=3;dl=5;}*sw=ds;*lw=dl;return 1;}err(error,ec,"dataset path or documented alias not found");return 0;}
 const char*dirs[]={".","data","datasets"};char found[3][1024];size_t count=0;
 for(size_t d=0;d<3;d++){int n=snprintf(found[count],sizeof found[count],"%s%s%s",strcmp(dirs[d],".")?dirs[d]:"",strcmp(dirs[d],".")?"/":"",files[0]);if(n<0||(size_t)n>=sizeof found[count])continue;if(exists(found[count]))count++;}
 if(count==0){err(error,ec,"dataset alias file missing; pass an explicit path or add the supplied file to project root/data/datasets");return 0;}
 if(count>1){err(error,ec,"dataset alias is ambiguous across project root/data/datasets; pass an explicit path");return 0;}
 if(strlen(found[0])>=cap){err(error,ec,"resolved dataset path exceeds output buffer");return 0;}strcpy(out,found[0]);*sw=ds;*lw=dl;return 1;
}
