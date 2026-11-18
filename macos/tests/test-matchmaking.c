#include <SDL3/SDL.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
extern char matchmaking_ws_url[];
extern char matchmaking_ip_url[];
extern void matchmaking_connect_async(void);
extern int matchmaking_take_error(void);
extern const char *matchmaking_last_error(void);
extern int matchmaking_request_list(void);
int LbNetLog(const char *format,...){va_list args;va_start(args,format);int r=vfprintf(stderr,format,args);va_end(args);return r;}
int main(int argc,char **argv){
 if(matchmaking_last_error()[0])return 3;
 if(argc==1){matchmaking_connect_async();SDL_Delay(1500);if(matchmaking_take_error()||matchmaking_request_list()!=0)return 1;puts("PASS: live lobby WebSocket connected and accepted list request");SDL_Delay(100);return 0;}
 if(argc!=2)return 2;
 snprintf(matchmaking_ws_url,144,"ws://127.0.0.1:%s/ws",argv[1]);
 snprintf(matchmaking_ip_url,144,"http://127.0.0.1:%s/ip",argv[1]);
 for(int attempt=0;attempt<2;attempt++){
 Uint64 start=SDL_GetTicks();matchmaking_connect_async();int error=0;
 while(SDL_GetTicks()-start<6500 && !(error=matchmaking_take_error()))SDL_Delay(10);
 if(!matchmaking_last_error()[0]){fputs("Missing diagnostic error history\n",stderr);return 1;}
 if(error!=1){fprintf(stderr,"Expected bounded connection failure\n");return 1;}
 fflush(stderr);printf("PASS: attempt %d returned connection error in %llu ms\n",attempt+1,(unsigned long long)(SDL_GetTicks()-start));
 SDL_Delay(200);
 }
 SDL_Delay(500);return 0;
}
