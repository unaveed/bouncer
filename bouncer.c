#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>


/*
 * Fills the supplied row with the correct pixel data to contain the circle.
 */
void get_circle_row_data(uint8_t *row, int y, int width, int height, int centerX, int centerY, int radius, int colors[]) {
	/*
	 * x = column
	 * left = left edge of ball
	 * right = right edge of ball
	 * bottom = bottom edge of ball
	 * brightness = value to use for color (with the gradient)
	 * yPos = y value of pixel we are looking at with respect to the center
	 * xPos = x value of pixel we are looking at with respect to the center
	 */
	int x, left, right, top, bottom, brightness, yPos, xPos;
	left   = centerX - radius;
	right  = centerX + radius;
	top    = centerY - radius;
	bottom = centerY + radius;

	for (x = 0; x < width; x++) {
		/* Inside the circle */
		if (pow((x - centerX), 2) + pow((y - centerY), 2) < pow(radius, 2)) {

			// Get y value
			if (y < centerY)
				yPos = centerY - y;
			else
				yPos = y - centerY;

			// Get x value
			if (x < centerX)
				xPos = centerX - x;
			else
				xPos = x - centerX;

			// Figure out brightness
			if (yPos > xPos)
				brightness = yPos;
			else if (yPos < xPos)
				brightness = xPos;
			else if (yPos == 0 && xPos == 0);
			else
				brightness = xPos;

			// Assign colors
			row[(x*3)+0] = 255;
			row[(x*3)+1] = colors[radius - brightness];
			row[(x*3)+2] = 255;
		}
	}
}

/*
 * Figures out where the circle should be drawn and takes care of drawing it.
 */
void draw_circle(AVFrame *pFrame, int width, int height, int iFrame, int centerX, int centerY, int radius) {
	int  y, i;
	uint8_t row[width*3];
	int colors[radius];	/* Represents the level of colors for the gradient */

	/* Set up color levels */
	for (i = 0; i < radius; i++)
		colors[i] = i * (255 / radius);

	// Write pixel data
	for(y = 0; y < height; y++) {
		/* If circle data is contained in the row, write it */
		if (y >= (centerY - radius) && y <= (centerY + radius)) {
			get_circle_row_data(pFrame->data[0]+y*pFrame->linesize[0],
								y,
								width, 
								height, 
								centerX, 
								centerY, 
								radius,
								colors);
		}
	}

}

int main(int argc, char *argv[]) {
	AVFormatContext *pFormatCtx = NULL;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx = NULL;
	AVCodec         *pCodec = NULL;
	AVFrame         *pFrame = NULL; 
	AVFrame         *pFrameRGB = NULL;
	AVPacket        packet;
	int             frameFinished;
	int             numBytes, size, ret;
	int				x, y;
	uint8_t			r, g, b;
	uint8_t         *buffer = NULL;
	AVFrame         *xkcd_frame = NULL;
	AVPacket		pkt;
	AVCodec			*xkcd_codec = NULL;
	int				got_output = 0;
	char			filename[32];
	FILE			*f;
	AVCodecContext	*xkcd_ctx;
	const char *ext;
	char correctExt[4] = {"jpg"};

	/*
	 * centerX, centerY = center of the ball
	 * radius = radius of the ball
	 * bottom = bottom y value of the ball
	 * parts = how many parts to break the bouncing into (for example, 12 parts would mean 12 frames for falling down, 12 frames to bounce back up)
	 * ballBounceInc = the actual value to increment by for the bouncing to occur
	 * distance = distance between the bottom part of the ball (bottom) and the bottom of the image
	 * down = true if ball if falling down, false if it is bouncing up
	 */
	int				centerX, centerY, radius, bottom, parts, ballBounceInc, ballPos, distance, down;

	AVDictionary    *optionsDict = NULL;
	struct SwsContext      *sws_ctx = NULL;

	if(argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}

	/* Check file extension */
	ext = strrchr(argv[1], '.');
	if(!ext || ext == argv[1] || strncmp(ext+1, correctExt, 4)){
		printf("Invalid file type, please select a .jpg file.\n");
		return -1;
	}

	/* Code example from http://dranger.com/ffmpeg/tutorial01.html */
	// Register all formats and codecs
	av_register_all();

	// Open video file
	if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

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
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}

	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame=av_frame_alloc();

	// Allocate an AVFrame structure
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL)
		return -1;

	// Determine required buffer size and allocate buffer
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	sws_ctx = sws_getContext (
				pCodecCtx->width,
				pCodecCtx->height,
				pCodecCtx->pix_fmt,
				pCodecCtx->width,
				pCodecCtx->height,
				PIX_FMT_RGB24,
				SWS_BILINEAR,
				NULL,
				NULL,
				NULL);

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	// Read frames
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(
					sws_ctx,
					(uint8_t const * const *)pFrame->data,
					pFrame->linesize,
					0,
					pCodecCtx->height,
					pFrameRGB->data,
					pFrameRGB->linesize);

			}
			sws_freeContext(sws_ctx);
			/* End example */
				
			/* Get center of ball */
			centerX = pCodecCtx->width/2;
			centerY = pCodecCtx->height/2;

			/* Get the radius of the ball */
			if (pCodecCtx->height > pCodecCtx->width)
				radius = pCodecCtx->width / 10;
			else
				radius = pCodecCtx->height / 10;

			bottom = centerY + radius; /* Bottom edge of ball */

			parts = 15;	/* Number of frames till the ball hits the bottom (or top, for that matter) */

			distance = pCodecCtx->height - bottom;
			ballBounceInc = distance / parts;

			ballPos = 0;
			down = 0;
				
			// Save the frames to disk
			for (i = 0; i < 300; i++) {
				/* Code example from ffmpeg/doc/examples/decoding_encoding.c */
				xkcd_codec = avcodec_find_encoder(AV_CODEC_ID_XKCD);
				xkcd_ctx = avcodec_alloc_context3(xkcd_codec);

				xkcd_ctx->width = pCodecCtx->width;
				xkcd_ctx->height = pCodecCtx->height;
				xkcd_ctx->pix_fmt = xkcd_codec->pix_fmts[0];

				if(avcodec_open2(xkcd_ctx, xkcd_codec, NULL) < 0){
					fprintf(stderr, "Could not open codec\n");
					exit(1);
				}

				// Open file
				sprintf(filename, "frame%.03d.xkcd", i); 
				f = fopen(filename, "wb");

				xkcd_frame = av_frame_alloc(); 
				xkcd_frame->format = xkcd_ctx->pix_fmt;
				xkcd_frame->width = xkcd_ctx->width;
				xkcd_frame->height = xkcd_ctx->height;

				ret = av_image_alloc(xkcd_frame->data, xkcd_frame->linesize, xkcd_ctx->width, xkcd_ctx->height, xkcd_ctx->pix_fmt,32);
				if(ret <  0){
					fprintf(stderr, "Could not allocate raw picture buffer\n");
					exit(1);
				}

				/* Initialize packet */
				av_init_packet(&pkt);
				pkt.data = NULL;
				pkt.size = 0;

				fflush(stdout);
				/* End example */

				/* Prepare dummy image */
				for (y = 0; y < pCodecCtx->height; y++){
					for(x = 0; x < pCodecCtx->width * 3; x++){
						xkcd_frame->data[0][y * xkcd_frame->linesize[0] + x] = pFrameRGB->data[0][y * pFrameRGB->linesize[0] + x];
					}
				}

				/* Figure out where to move the ball and move it */
				if (down) {
					if (ballPos < distance)
						ballPos += ballBounceInc;
					else {
						down = 0;
						ballPos -= ballBounceInc;
					}
				}
				else {
					if (ballPos > 0)
						ballPos -= ballBounceInc;
					else {
						down = 1;
						ballPos += ballBounceInc;
					}
				}

				draw_circle(xkcd_frame, xkcd_ctx->width, xkcd_ctx->height, i, centerX, centerY+ballPos, radius); 
				
				/* Code example from ffmpeg/doc/examples/decoding_encoding.c */
				/* Encode the image */
				ret = avcodec_encode_video2(xkcd_ctx, &pkt, xkcd_frame, &got_output);
				
				if(ret < 0){
					fprintf(stderr, "Error encoding the iamge\n");
					exit(1);
				}
				if(got_output){
					fwrite(pkt.data, 1, pkt.size, f);
					av_free_packet(&pkt);
				}

				avcodec_close(xkcd_ctx);
				av_free(xkcd_ctx);
				av_freep(&xkcd_frame->data[0]);
				av_frame_free(&xkcd_frame);

				// Close file
				fclose(f);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		av_free_packet(&pkt);
	}

	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);
	/* End example */

	return 0;
}
