#include "project_config.h"
#include <stdio.h>
#include <string.h>
int main(void){char p[1024],e[256];int s=0,l=0;if(tt_version_count()!=11||!tt_version_find("v5_manual_digit_parser_running_double_stream_O2")||tt_version_find("v99"))return 1;if(!tt_resolve_dataset("test20",p,sizeof p,&s,&l,e,sizeof e)||s!=3||l!=5||strstr(p,"test20 - Copy (6).csv")==NULL){fprintf(stderr,"test20 alias: %s\n",e);return 2;}if(!tt_resolve_dataset("test20 - Copy (6).csv",p,sizeof p,&s,&l,e,sizeof e))return 3;if(tt_resolve_dataset("missing-dataset.csv",p,sizeof p,&s,&l,e,sizeof e))return 4;puts("project config tests PASS");return 0;}
