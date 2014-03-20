#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>



/*
 * Fills the first supplied row with the correct pixel data to contain the circle.
 */
void get_circle_row_data(uint8_t *tempRow, uint8_t *row, int y, int width, int height, int centerX, int centerY, int radius, int colors[]) {
	int x;
	int left, right, top, bottom, inc, brightness, level;
	int yPos, xPos;
	left   = centerX - radius;
	right  = centerX + radius;
	top    = centerY - radius;
	bottom = centerY + radius;
	inc = (right - centerX) / radius;

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
				// TODO: Figure out what to do here.  Maybe nothing still?
			else
				brightness = xPos;

			// Assign colors
			tempRow[(x*3)+0] = 255;
			tempRow[(x*3)+1] = colors[radius - brightness];
			tempRow[(x*3)+2] = 255;
		}
		/* Outside the circle */
		else {
			tempRow[(x*3)+0] = row[(x*3)+0];
			tempRow[(x*3)+1] = row[(x*3)+1];
			tempRow[(x*3)+2] = row[(x*3)+2];
		}
	}
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame, int centerX, int centerY, int radius) {
	FILE *pFile;
	char szFilename[32];
	int  y, i;
	uint8_t tempRow[width*3];
	uint8_t *row;


	int colors[radius];	/* Represents the level of colors for the gradient */

	for (i = 0; i < radius; i++)
		colors[i] = i * (255 / radius);


	// Open file
	sprintf(szFilename, "frame%.03d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if(pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for(y = 0; y < height; y++) {
		/* If circle data is contained in the row, write it */
		if (y >= (centerY - radius) && y <= (centerY + radius)) {
			get_circle_row_data(tempRow,
								pFrame->data[0]+y*pFrame->linesize[0],
								y,
								width, 
								height, 
								centerX, 
								centerY, 
								radius,
								colors);
			fwrite(tempRow, 1, width*3, pFile);
		}
		/* Normal picture */
		else
			fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

		/*
		if (y % 2 == 0)
			fwrite(green, 1, width*3, pFile);
		else
			fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
		*/
	}

	// Close file
	fclose(pFile);
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
	int             numBytes;
	int				y, x;
	uint8_t			r, g, b;
	uint8_t         *buffer = NULL;
	uint8_t         *pic = NULL;
	int				centerX, centerY, radius, bottom, parts, ballBounceInc, ballPos, distance, updown;

	AVDictionary    *optionsDict = NULL;
	struct SwsContext      *sws_ctx = NULL;

	if(argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}
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

	// Read frames and save first five frames to disk
	i=0;
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

				
				centerX = pCodecCtx->width/2;
				centerY = pCodecCtx->height/2;

				/* Get the radius of the ball */
				if (pCodecCtx->height > pCodecCtx->width)
					radius = pCodecCtx->width / 10;
				else
					radius = pCodecCtx->height / 10;

				bottom = centerY + radius;

				parts = 15;

				distance = pCodecCtx->height - bottom;
				ballBounceInc = distance / parts;

				ballPos = 0;
				updown = 0;
				// Save the frames to disk
				for (i = 0; i < 300; i++) {
					if (!updown) {
						if (ballPos < distance)
							ballPos += ballBounceInc;
						else {
							updown = 1;
							ballPos -= ballBounceInc;
						}
					}
					else {
						if (ballPos > 0)
							ballPos -= ballBounceInc;
						else {
							updown = 0;
							ballPos += ballBounceInc;
						}
					}
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, centerX, centerY+ballPos, radius);

				}
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
}
