#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include <math.h>

/*
 * Fills the first supplied row with the correct pixel data to contain the circle.
 */
void get_circle_row_data(uint8_t *tempRow, uint8_t *row, int y, int width, int height, int centerX, int centerY, int radius) {
	int x;
	int yBrightness = (y - centerY) / 10;

	for (x = 0; x < width; x++) {
		if (pow((x - centerX), 2) + pow((y - centerY), 2) < pow(radius, 2)) {
			tempRow[(x*3)+0] = 255;
			tempRow[(x*3)+1] = 76;
			tempRow[(x*3)+2] = 7;
		}
		else {
			tempRow[(x*3)+0] = row[(x*3)+0];
			tempRow[(x*3)+1] = row[(x*3)+1];
			tempRow[(x*3)+2] = row[(x*3)+2];
		}
	}
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y, i;
	int radius, centerX, centerY;
	uint8_t tempRow[width*3];
	uint8_t *row;

	/* Get the radius of the ball */
	if (height > width)
		radius = width / 10;
	else
		radius = height / 10;

	/* Get the center of the ball */
	centerX = width / 2;
	centerY = height / 2;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
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
								radius);
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

				// Save the frame to disk
				if(++i<=5)
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// Read pixel data
	/*
	pic = pFrameRGB->data[0];
	printf("linesize=%d\n", pFrameRGB->linesize[0]);
	printf("width=%d\n", pCodecCtx->width);
	printf("height=%d\n", pCodecCtx->height);

	for (y = 0; y < pCodecCtx->height; y++) {
		for (x = 0; x < pFrameRGB->linesize[0]; x+=3) {
			r = pic[y * pFrameRGB->linesize[0] + x + 0];
			g = pic[y * pFrameRGB->linesize[0] + x + 1];
			b = pic[y * pFrameRGB->linesize[0] + x + 2];
			if (y == 7) {
				r = 0;
				g = 255;
				b = 0;
			}
			printf("pixel:(%d,%d,%d)\n", r, g, b);
		}
	}
	*/

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
