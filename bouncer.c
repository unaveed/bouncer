#include <stdio.h>
#include <string.h>

#include "bouncer.h"

int main(int argc, char **argv) {
	char correctExt[4] = {"jpg"};
	char *input_file;
	const char *ext;

	/* Check to make sure correct number of arguments are supplied. */
	if (argc != 2) {
		printf("Incorrect number of arguments.\n");
		usage();
		return 1;
	}

	input_file = argv[1];	/* Set input file */

	/* Check filetype */
	ext = strrchr(input_file, '.');
	if(!ext || ext == input_file || strncmp(ext+1, correctExt, 4)) {
		printf("Invalid filetype.\n");
		usage();
		return 1;
	}
	AVFormatContext *pictureFormatCxt = NULL;
	AVCodec 		*codec = NULL;
	AVCodecContext 	*avct= NULL;
	AVFrame 		*picture = NULL;
	AVPacket 		avpkt;
	int				i, picStream, width, height;
	
	av_register_all();
	
	if(avformat_open_input(&pictureFormatCxt, input_file, NULL, NULL)!=0)
		return -1;

	/* Find the JPEG decoder */
	codec = avcodec_find_decoder(CODEC_ID_JPEGLS);
	if(!codec){
		printf("Codec could not be found\n");
		exit(1);
	}

	if(avformat_find_stream_info(pictureFormatCxt, NULL)<0)
		return -1;

	/* May not need */
	av_dump_format(pictureFormatCxt, 0, input_file, 0);

	picStream = -1;
	for(i = 0; i<pictureFormatCxt->nb_streams; i++){
		if(pictureFormatCxt->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			picStream=i;
			break;
		}
		if(picStream == -1)
			return -1;
	}
	avct = pictureFormatCxt->streams[picStream]->codec;
	
	/* See if width or height can be retrieved for the leopard.jpg, delete later  */	
	width = avct->width;
	printf("Width should equal 600, actual =  %d\n", width);

	height = avct->height;
	printf("Height should equal 450, actual =  %d\n", height);

	return 0;
}

/* Prints the usage message. */
void usage() {
	printf("Usage: bouncer <filename.jpg>\n");
}
