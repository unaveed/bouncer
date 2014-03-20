all: bouncer

bouncer:
	gcc -o bouncer -I../ffmpeg/include -L../ffmpeg/lib bouncer.c `pkg-config --cflags --libs libavutil libavformat libavcodec libswscale`

clean:
	rm -rf *.o bouncer *.mp4 *.ppm *.out

movie:
	ffmpeg -f image2 -i frame%03d.ppm movie.mp4
