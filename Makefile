all: bouncer

bouncer: bouncer.o
	gcc bouncer.o -o bouncer

bouncer.o: bouncer.c
	gcc -c bouncer.c

clean:
	rm -rf *.o bouncer *.mp4 *.xkcd
