#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;alarm(10);av_log_set_level(AV_LOG_QUIET);
 AVFormatContext *format=NULL;AVCodecContext *decoder=NULL;AVPacket *packet=NULL;AVFrame *frame=NULL;int result=1;
 if(avformat_open_input(&format,argv[1],NULL,NULL)<0 || avformat_find_stream_info(format,NULL)<0){puts("Could not read the MP4 movie.");goto done;}
 int index=av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
 if(index<0 || format->streams[index]->codecpar->codec_id!=AV_CODEC_ID_H264){puts("Enhanced movies must contain H.264 video. HEVC and other codecs are not supported by this build.");goto done;}
 for(unsigned i=0;i<format->nb_streams;i++)if(format->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO && format->streams[i]->codecpar->codec_id!=AV_CODEC_ID_AAC){puts("Enhanced movie sound must use AAC audio.");goto done;}
 const AVCodec *codec=avcodec_find_decoder(AV_CODEC_ID_H264);decoder=avcodec_alloc_context3(codec);
 if(!decoder || avcodec_parameters_to_context(decoder,format->streams[index]->codecpar)<0 || decoder->width>8192 || decoder->height>8192 || avcodec_open2(decoder,codec,NULL)<0){puts("Could not open the enhanced video decoder.");goto done;}
 packet=av_packet_alloc();frame=av_frame_alloc();if(!packet||!frame)goto done;
 for(int i=0;i<300 && av_read_frame(format,packet)>=0;i++){
  if(packet->stream_index==index && avcodec_send_packet(decoder,packet)>=0 && avcodec_receive_frame(decoder,frame)>=0){puts("Compatible H.264/AAC enhanced movie.");result=0;break;}
  av_packet_unref(packet);
 }
 if(result)puts("The enhanced movie did not produce a playable video frame.");
done:
 av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&decoder);avformat_close_input(&format);return result;
}
