#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>






/*
 * Video encoding example
 */
static void video_encode_example(const char *filename, int codec_id)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    printf("Encode video file %s\n", filename);

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    c->time_base = (AVRational){1,25};
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

    /* encode 1 second of video */
    for (i = 0; i < 25; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < c->height/2; y++) {
            for (x = 0; x < c->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    printf("\n");
}














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
	int  y, i, got_output;
	uint8_t tempRow[width*3];
	uint8_t *row;
    AVCodec *codec;
    AVCodecContext *c= NULL;
    AVPacket pkt;





/* *********** NEW WORK *************** */

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_XKCD);

    c = avcodec_alloc_context3(codec);

    /* resolution must be a multiple of two */
    c->width = width;
    c->height = height;
    c->pix_fmt = codec->pix_fmts[0];

    /* open it */
    avcodec_open2(c, codec, NULL);

	// Open file
	sprintf(szFilename, "frame%.03d.xkcd", iFrame);
	pFile = fopen(szFilename, "wb");
	if(pFile == NULL)
		return;

    pFrame->format = c->pix_fmt;
    pFrame->width  = c->width;
    pFrame->height = c->height;

    av_image_alloc(pFrame->data, pFrame->linesize, c->width, c->height, c->pix_fmt, 32);

	av_init_packet(&pkt);
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;



/* *********** END WORK *************** */




	int colors[radius];	/* Represents the level of colors for the gradient */

	for (i = 0; i < radius; i++)
		colors[i] = i * (255 / radius);



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
			for (i = 0; i < width*3; i++) {
				//(pFrame->data[0]+y*pFrame->linesize[0])[i] = tempRow[i];
			}
			//fwrite(tempRow, 1, width*3, pFile);
		}
		/* Normal picture */
		else {
			//fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
		}
	}

	/* encode the image */
	avcodec_encode_video2(c, &pkt, pFrame, &got_output);

	fwrite(pkt.data, 1, pkt.size, pFile);

	// Close file
	fclose(pFile);

	av_free_packet(&pkt);
    avcodec_close(c);
    av_free(c);
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
				down = 0;
				// Save the frames to disk
				for (i = 0; i < 300; i++) {
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
