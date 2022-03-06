CC=gcc
CFLAGS= -O3 -lpthread

default: run

run:
	$(CC) -o main main.c $(CFLAGS)
	./main
clean:
	rm main