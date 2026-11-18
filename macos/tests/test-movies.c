#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 AVFormatContext *f=NULL;if(avformat_open_input(&f,argv[1],NULL,NULL)<0)return 1;
 if(avformat_find_stream_info(f,NULL)<0)return 1;
 AVCodecContext *contexts[16]={0};int counts[16]={0};
 for(unsigned i=0;i<f->nb_streams&&i<16;i++){
 const AVCodec *codec=avcodec_find_decoder(f->streams[i]->codecpar->codec_id);
 if(!codec){fprintf(stderr,"Missing codec %s\n",avcodec_get_name(f->streams[i]->codecpar->codec_id));return 1;}
 contexts[i]=avcodec_alloc_context3(codec);avcodec_parameters_to_context(contexts[i],f->streams[i]->codecpar);if(avcodec_open2(contexts[i],codec,NULL)<0)return 1;
 }
 AVPacket *p=av_packet_alloc();AVFrame *frame=av_frame_alloc();int n=0;
 while(n<500 && av_read_frame(f,p)>=0){int i=p->stream_index;if(i<16&&contexts[i]){int r=avcodec_send_packet(contexts[i],p);if(r<0)return 1;while(avcodec_receive_frame(contexts[i],frame)>=0){counts[i]++;av_frame_unref(frame);}}av_packet_unref(p);n++;}
 for(unsigned i=0;i<f->nb_streams&&i<16;i++){printf("%s: %s %d decoded frames\n",argv[1],avcodec_get_name(f->streams[i]->codecpar->codec_id),counts[i]);if(!counts[i])return 1;avcodec_free_context(&contexts[i]);}
 av_frame_free(&frame);av_packet_free(&p);avformat_close_input(&f);return 0;
}
