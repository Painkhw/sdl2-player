#include <sys/time.h>
#include <stdio.h>

#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"

long timestamp;
long getCurrentTime()  
{  
   struct timeval tv;  
   gettimeofday(&tv,NULL);  
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;  
}

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

//int main(int argc, char *argv[]) {
int main() {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtxOrig = NULL;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL;
  AVFrame         *pFrameYUV = NULL;
  AVPacket        packet;
  int             frameFinished;
  float           aspect_ratio;
  struct SwsContext *sws_ctx = NULL;

  SDL_Surface     *screen;
  SDL_Rect        rect;
  SDL_Event       event;

  printf("software start time %ld, \n ",getCurrentTime());
  // Register all formats and codecs
  av_register_all();

  avformat_network_init();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  char url_udp[] = "udp://@:2330";
  char url_rtsp[] = "rtsp://192.168.31.144:554/livestream/0/1";
  // Open video file
  if(avformat_open_input(&pFormatCtx, url_udp, NULL, NULL)!=0)
    return -1; // Couldn't open file

  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, url_udp, 0);
  
  // Find the first video stream
  videoStream=-1;
  
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }

  // Copy context
  pCodecCtx = avcodec_alloc_context3(pCodec);
  if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
    fprintf(stderr, "Couldn't copy codec context");
    return -1; // Error copying codec context
  }

  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    return -1; // Could not open codec
  
  // Allocate video frame
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  
  if(pFrameYUV == NULL) {
    return -1;
  }

  // initialize SWS context for software scaling

  sws_ctx = sws_getContext(
         pCodecCtx->width,
			   pCodecCtx->height,
			   pCodecCtx->pix_fmt,
			   pCodecCtx->width,
			   pCodecCtx->height,
			   AV_PIX_FMT_YUV420P,
			   SWS_BILINEAR,
			   NULL,
			   NULL,
			   NULL);

  int numBytes = avpicture_get_size(
        AV_PIX_FMT_YUV420P,
        pCodecCtx->width,
        pCodecCtx->height);
  uint8_t* buffer = (uint8_t *)av_malloc( numBytes*sizeof(uint8_t) );

  avpicture_fill((AVPicture *)pFrameYUV, buffer, AV_PIX_FMT_YUV420P,
      pCodecCtx->width, pCodecCtx->height);

  // Read frames and save first five frames to disk
  SDL_Rect sdlRect;
  sdlRect.x = 0;
  sdlRect.y = 0;
  sdlRect.w = pCodecCtx->width;
  sdlRect.h = pCodecCtx->height;

  // SDL
  if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER )) {
      fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
      exit(1);
  }

  SDL_Window* sdlWindow = SDL_CreateWindow("FFmpeg Player",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      pCodecCtx->width,  pCodecCtx->height,
      0);
  if( !sdlWindow ) {
      fprintf(stderr, "SDL: could not set video mode - exiting\n");
      exit(1);
  }

  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
  SDL_Texture* sdlTexture = SDL_CreateTexture(
      sdlRenderer,
      SDL_PIXELFORMAT_IYUV,
      SDL_TEXTUREACCESS_STREAMING,
      pCodecCtx->width,
      pCodecCtx->height);
    
  //raw_video_time_base = av_inv_q(pCodecCtxOrig->framerate);

  // Read frames and save first five frames to disk
  i=0;

  AVRational time_base;
	time_base.den = 90000;
	time_base.num = 1;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
     // printf("=================pFrame->pkt_pts: %ld, pFrame->pkt_dts: %ld \n pFrameYUV->pkt_pts: %ld \n pFrameYUV->pkt_dts \n",
     //                 pFrame->pkt_pts, pFrame->pkt_dts, pFrameYUV->pkt_pts, pFrameYUV->pkt_dts);

      printf("=================packet->pkt_pts: %ld, packet->pkt_dts: %ld \n",packet.pts, packet.dts);
      timestamp = packet.pts * av_q2d(time_base);
      printf("timestamp = %ld \n",timestamp);
      printf("Current time %ld, \n ",getCurrentTime());
      
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

    //  printf("***********************pFrame->pkt_pts: %ld, pFrame->pkt_dts: %ld \n pFrameYUV->pkt_pts: %ld \n pFrameYUV->pkt_dts \n",
    //                  pFrame->pkt_pts, pFrame->pkt_dts, pFrameYUV->pkt_pts, pFrameYUV->pkt_dts);
      // Did we get a video frame?
      if(frameFinished) {
        sws_scale(
          sws_ctx,
          (uint8_t const * const *)pFrame->data,
          pFrame->linesize,
          0,
          pCodecCtx->height,
          pFrameYUV->data,
          pFrameYUV->linesize   
        );
      //  printf("##################pFrame->pkt_pts: %ld, pFrame->pkt_dts: %ld \n pFrameYUV->pkt_pts: %ld \n pFrameYUV->pkt_dts \n",
      //                pFrame->pkt_pts, pFrame->pkt_dts, pFrameYUV->pkt_pts, pFrameYUV->pkt_dts);

        SDL_UpdateTexture( sdlTexture, &sdlRect, pFrameYUV->data[0], pFrameYUV->linesize[0] );
        SDL_RenderClear( sdlRenderer );
        SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );
        SDL_RenderPresent( sdlRenderer );
      }

    }
    
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
    SDL_PollEvent(&event);
    switch(event.type) {
      case SDL_QUIT:
        SDL_Quit();
        exit(0);
        break;
      default:
        break;
    }
  }

  SDL_DestroyTexture(sdlTexture);
  // Free the YUV frame
  av_frame_free(&pFrame);
  av_frame_free(&pFrameYUV);

  // Close the codec
  avcodec_close(pCodecCtx);
  avcodec_close(pCodecCtxOrig);
  
  // Close the video file
  avformat_close_input(&pFormatCtx);
  
  return 0;
}