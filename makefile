all: bouncer

bouncer:
	gcc -o bouncer -I../ffmpeg/include -L../ffmpeg/lib bouncer.c `pkg-config --cflags --libs libavutil libavformat libavcodec`

clean:
	rm -rf *.o bouncer *.mp4 *.xkcd *.out

movie:
	ffmpeg -f image2 -i frame%03d.xkcd movie.mp4
	#ffmpeg -f img2 -i frame%03d.xkcd movie.mp4
