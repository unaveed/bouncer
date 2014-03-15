all: bouncer

LIBS=-llibavcodec.a -llibavdevice.a -llibavfilter.a -llibavformat.a -llibavutil.a -llibswresample.a -llibswscale.a
bouncer: bouncer.o
	gcc bouncer.o -o bouncer

bouncer.o: bouncer.c
	gcc -c bouncer.c -L../ffmpeg/lib $(LIBS) -I../ffmpeg/include

clean:
	rm -rf *.o bouncer *.mp4 *.xkcd

movie:
