all: bouncer

CINC=-I../ffmpeg/include
HEADERS=$(CINC)/libavcodec

bouncer: bouncer.o
	gcc bouncer.o -o bouncer

bouncer.o: bouncer.c
	gcc -c bouncer.c $(HEADERS)

clean:
	rm -rf *.o bouncer *.mp4 *.xkcd

movie:
