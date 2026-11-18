#include <curl/curl.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
int main(void){
 curl_global_init(CURL_GLOBAL_DEFAULT);CURL *c=curl_easy_init();if(!c){puts("Could not start the connection test.");return 1;}
 curl_easy_setopt(c,CURLOPT_URL,"wss://matchmaking.keeperfx.workers.dev/ws");
 curl_easy_setopt(c,CURLOPT_CONNECT_ONLY,2L);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT_MS,5000L);curl_easy_setopt(c,CURLOPT_TIMEOUT_MS,8000L);curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);
 CURLM *multi=curl_multi_init();if(!multi){curl_easy_cleanup(c);return 1;}
 curl_multi_add_handle(multi,c);int running=1;CURLcode result=CURLE_OPERATION_TIMEDOUT;
 struct timespec start,now;clock_gettime(CLOCK_MONOTONIC,&start);
 while(running){
  if(curl_multi_perform(multi,&running)!=CURLM_OK){result=CURLE_RECV_ERROR;break;}
  if(!running){int left;CURLMsg *message;while((message=curl_multi_info_read(multi,&left)))if(message->msg==CURLMSG_DONE)result=message->data.result;break;}
  clock_gettime(CLOCK_MONOTONIC,&now);if(now.tv_sec-start.tv_sec>=8)break;
  curl_multi_poll(multi,NULL,0,100,NULL);
 }
 long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);
 clock_gettime(CLOCK_MONOTONIC,&now);
 long elapsed=(now.tv_sec-start.tv_sec)*1000+(now.tv_nsec-start.tv_nsec)/1000000;
 if(result==CURLE_OK&&status==101)printf("Lobby service: connected securely (HTTP 101, %ld ms).\n",elapsed);
 else printf("Online connection failed: %s. Check your connection and choose Test Connection to retry. LAN play remains available.\n",curl_easy_strerror(result));
 int udp=socket(AF_INET,SOCK_DGRAM,0);
 if(udp>=0){struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(5556);
  if(bind(udp,(struct sockaddr *)&address,sizeof(address))==0)puts("Local UDP port 5556: available.");
  else if(errno==EADDRINUSE)puts("Local UDP port 5556: already in use. Close another game instance or choose a different port.");
  else puts("Local UDP port 5556: could not be checked.");close(udp);
 }
 puts("Incoming peer reachability/NAT: not verified by this test.\nFor hosting problems, check macOS firewall permission, VPN settings and router UDP forwarding/hole-punch support.\nA full host/join test needs a second compatible player.");
 curl_multi_remove_handle(multi,c);curl_easy_cleanup(c);curl_multi_cleanup(multi);curl_global_cleanup();return result==CURLE_OK&&status==101?0:1;
}
