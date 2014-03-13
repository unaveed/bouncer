all: bouncer

bouncer: bouncer.o
	gcc bouncer.o -o bouncer

bouncer.o: bouncer.c
	gcc -c bouncer.c

