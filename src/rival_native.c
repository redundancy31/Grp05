#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

static const double reaction_us[10]={.1,.2,.5,1,2,5,10,20,50,100};
typedef struct { long double pnl,opened; unsigned long long buys,sells,trigger,next,unfilled,ignored; int holding; } Account;
static int line(FILE*f,char*b,size_t n){if(!fgets(b,(int)n,f))return 0;size_t z=strlen(b);if(z&&b[z-1]=='\n')b[--z]=0;if(z&&b[z-1]=='\r')b[z-1]=0;if(z+1==n)return -1;return 1;}
static int col(char*s,int c,char**v){static char value[256];char*p=s;while(c--){p=strchr(p,',');if(!p)return 0;p++;}char*e=strchr(p,',');size_t n=e?(size_t)(e-p):strlen(p);if(n>=sizeof value)return 0;memcpy(value,p,n);value[n]=0;*v=value;return 1;}
static int real(char*s,long double*v){char*e;errno=0;*v=strtold(s,&e);return !errno&&e!=s&&!*e&&isfinite((double)*v);}
static int integer(char*s,long long*v){char*e;errno=0;*v=strtoll(s,&e,10);return !errno&&e!=s&&!*e;}
static int val(char*row,int c,long double*v){char*s;return col(row,c,&s)&&real(s,v);}
static int companion(const char*base,const char*ext,char*out,size_t cap){size_t n=strlen(base),stem=n;const char*slash1=strrchr(base,'/'),*slash2=strrchr(base,'\\');const char*slash=slash1;if(!slash||(slash2&&slash2>slash))slash=slash2;const char*dot=strrchr(base,'.');if(dot&&(!slash||dot>slash))stem=(size_t)(dot-base);if(stem+strlen(ext)+1>cap)return 0;memcpy(out,base,stem);strcpy(out+stem,ext);return 1;}
static int output_exists(const char*p){FILE*f=fopen(p,"rb");if(!f)return 0;fclose(f);return 1;}
static int gnuplot_quote(FILE*f,const char*p){fputc('\'',f);for(;*p;p++){if(*p=='\\')fputc('/',f);else{if(*p=='\'')fputc('\\',f);fputc(*p,f);}}fputc('\'',f);return !ferror(f);}
static void usage(char*n){fprintf(stderr,"Usage: %s DATASET BASE_DECISIONS BASE_LATENCY FAST_DECISIONS FAST_LATENCY [SUMMARY.csv [--details]]\n",n);}
int main(int argc,char**argv){
 if(argc!=6&&argc!=7&&argc!=8){usage(argv[0]);return 2;}if(argc==8&&strcmp(argv[7],"--details")){usage(argv[0]);return 2;}int write_details=argc==8;
 FILE*d=fopen(argv[1],"rb"),*f[4]={fopen(argv[2],"rb"),fopen(argv[3],"rb"),fopen(argv[4],"rb"),fopen(argv[5],"rb")};
 if(!d||!f[0]||!f[1]||!f[2]||!f[3]){perror("rival input");return 2;}
 char datpath[4096]={0},scriptpath[4096]={0},pngpath[4096]={0},decpath[4096]={0};FILE*dat=NULL,*script=NULL,*decisions=NULL;
 if(argc>=7){if(!companion(argv[6],".dat",datpath,sizeof datpath)||!companion(argv[6],".gnuplot",scriptpath,sizeof scriptpath)||!companion(argv[6],".png",pngpath,sizeof pngpath)||(write_details&&!companion(argv[6],".decisions.csv",decpath,sizeof decpath))){fputs("rival: output path too long\n",stderr);return 2;}if(output_exists(argv[6])||output_exists(datpath)||output_exists(scriptpath)||output_exists(pngpath)||(write_details&&output_exists(decpath))){fputs("rival: refusing to overwrite existing summary/graph/decision evidence\n",stderr);return 2;}}
 FILE*out=argc>=7?fopen(argv[6],"wb"):stdout;if(!out){perror(argv[6]);return 2;}
 if(write_details){decisions=fopen(decpath,"wb");if(!decisions){perror(decpath);return 2;}fprintf(decisions,"version,trade,signal,processing_duration_us,trigger_price,next_trade_price,execution_price,decision,holding_before,holding_after,cash_pnl_after\n");}
 char b[5][4096];int rc=line(d,b[0],sizeof b[0]);if(rc!=1){fputs("rival: empty dataset\n",stderr);return 2;}
 if(strstr(b[0],"price")||strstr(b[0],"Price")){if(line(d,b[0],sizeof b[0])!=1){fputs("rival: dataset has no trades\n",stderr);return 2;}}
 for(int i=0;i<4;i++)if(line(f[i],b[i+1],sizeof b[i+1])!=1){fputs("rival: missing replay CSV header\n",stderr);return 2;}
 Account a[2][10]={0};unsigned long long ordinal=0;long double price,next_price;char*s;
 for(;;){if(!col(b[0],1,&s)||!real(s,&price)){fprintf(stderr,"rival: invalid price at trade %llu\n",ordinal+1);return 2;}
  int rr[4];for(int i=0;i<4;i++)rr[i]=line(f[i],b[i+1],sizeof b[i+1]);int all=1,any=0;for(int i=0;i<4;i++){if(rr[i]<0){fputs("rival: overlong replay row\n",stderr);return 2;}if(!rr[i])all=0;else any=1;}
  if(!all){if(any){fputs("rival: replay row counts differ\n",stderr);return 2;}fputs("rival: replay ended before dataset\n",stderr);return 2;}
  ++ordinal;long long ord[4];for(int i=0;i<4;i++){if(!col(b[i+1],0,&s)||!integer(s,&ord[i])||ord[i]!=(long long)ordinal){fprintf(stderr,"rival: trade order mismatch at %llu\n",ordinal);return 2;}}
  long double sd[2],ns[2];if(!val(b[1],8,&sd[0])||!val(b[3],8,&sd[1])||!val(b[2],4,&ns[0])||!val(b[4],4,&ns[1])||sd[0]!=sd[1]||sd[0]<-1||sd[0]>1||ns[0]<0||ns[1]<0){fprintf(stderr,"rival: invalid/mismatched decision or latency at %llu\n",ordinal);return 2;}
  int have_next=line(d,b[0],sizeof b[0]);if(have_next<0){fputs("rival: overlong dataset row\n",stderr);return 2;}if(have_next){if(!col(b[0],1,&s)||!real(s,&next_price)){fprintf(stderr,"rival: invalid next price after trade %llu\n",ordinal);return 2;}}
  int side=(int)sd[0];if(side){for(int v=0;v<2;v++)for(int k=0;k<10;k++){Account*q=&a[v][k];int before=q->holding,actionable=(side>0&&!q->holding)||(side<0&&q->holding);int is_detail=(k==3&&decisions);const char*ver=v?"fast":"baseline";const char*signal=side>0?"BUY":"SELL";double duration_us=(double)ns[v]/1000.0;if(!actionable){q->ignored++;if(is_detail)fprintf(decisions,"%s,%llu,%s,%.9g,%.12g,,,ignored_position_rule,%d,%d,%.12g\n",ver,ordinal,signal,duration_us,(double)price,before,before,(double)q->pnl);continue;}if(!have_next){q->unfilled++;if(is_detail)fprintf(decisions,"%s,%llu,%s,%.9g,%.12g,,,unfilled_no_next_trade,%d,%d,%.12g\n",ver,ordinal,signal,duration_us,(double)price,before,before,(double)q->pnl);continue;}int trigger=ns[v]<(long double)reaction_us[k]*1000.0L;long double fill=trigger?price:next_price;if(trigger)q->trigger++;else q->next++;if(side>0){q->pnl-=fill;q->buys++;q->holding=1;q->opened=fill;}else{q->pnl+=fill;q->sells++;q->holding=0;q->opened=0;}if(is_detail)fprintf(decisions,"%s,%llu,%s,%.9g,%.12g,%.12g,%.12g,%s,%d,%d,%.12g\n",ver,ordinal,signal,duration_us,(double)price,(double)next_price,(double)fill,trigger?"filled_at_trigger_price":"filled_at_next_trade_price",before,q->holding,(double)q->pnl);}}
  if(!have_next){for(int i=0;i<4;i++){int tail=line(f[i],b[i+1],sizeof b[i+1]);if(tail!=0){fputs("rival: replay contains rows beyond dataset EOF\n",stderr);return 2;}}break;}
 }
 fprintf(out,"reaction_us,baseline_pnl,fast_pnl,money_lost_fast_minus_baseline,baseline_holding,baseline_open_price,fast_holding,fast_open_price,baseline_buys,baseline_sells,baseline_trigger,baseline_next,baseline_unfilled,baseline_ignored,fast_buys,fast_sells,fast_trigger,fast_next,fast_unfilled,fast_ignored\n");
 if(argc>=7){dat=fopen(datpath,"wb");script=fopen(scriptpath,"wb");if(!dat||!script){perror("rival graph output");return 2;}}
 for(int k=0;k<10;k++){Account*x=&a[0][k],*y=&a[1][k];double bp=(double)x->pnl,fp=(double)y->pnl,ml=fp-bp;fprintf(out,"%.1f,%.12g,%.12g,%.12g,%d,%.12g,%d,%.12g,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",reaction_us[k],bp,fp,ml,x->holding,(double)x->opened,y->holding,(double)y->opened,x->buys,x->sells,x->trigger,x->next,x->unfilled,x->ignored,y->buys,y->sells,y->trigger,y->next,y->unfilled,y->ignored);if(dat)fprintf(dat,"%.1f,%.12g,%.12g,%.12g\n",reaction_us[k],bp,fp,ml);}
 if(ferror(out)|| (dat&&ferror(dat))||(decisions&&ferror(decisions))){perror("rival output");return 2;}if(argc>=7){fprintf(script,"set terminal pngcairo size 900,540 enhanced\nset output ");gnuplot_quote(script,pngpath);fprintf(script,"\nset logscale x\nset datafile separator comma\nset xlabel 'Rival reaction time (microseconds)'\nset ylabel 'Cash P&L (price units)'\nset key outside\nplot ");gnuplot_quote(script,datpath);fprintf(script," using 1:2 with linespoints title 'baseline P&L', ");gnuplot_quote(script,datpath);fprintf(script," using 1:3 with linespoints title 'fast P&L', ");gnuplot_quote(script,datpath);fprintf(script," using 1:4 with linespoints title 'fast minus baseline'\n");if(ferror(script)||fclose(script)||fclose(dat)|| (decisions&&fclose(decisions)) || fclose(out)){perror("rival graph close");return 2;}}
 else if(ferror(out)){perror("rival output");return 2;}
 fprintf(stderr,"rival complete: validated %llu aligned trades; sweep includes 1.0 us; memory is O(1)\n",ordinal);if(argc>=7)fprintf(stderr,"sweep=%s\ngraph_data=%s\ngnuplot_script=%s\npng_target=%s\n",argv[6],datpath,scriptpath,pngpath);if(write_details)fprintf(stderr,"decisions=%s\n",decpath);return 0;
}
