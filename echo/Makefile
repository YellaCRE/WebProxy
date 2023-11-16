# echoserver, echoclient make
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread -lrt

all : echoclient echoserver

csapp.o : csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

echoserver.o : echoserver.c csapp.h
	$(CC) $(CFLAGS) -c echoserver.c

echoclient.o : echoclient.c csapp.h
	$(CC) $(CFLAGS) -c echoclient.c

echoserver : echoserver.o csapp.o
	$(CC) $(CFLAGS) -o echoserver echoserver.o csapp.o $(LDFLAGS)

echoclient : echoclient.o csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.o csapp.o $(LDFLAGS)

clean:
	rm -f *.o
	rm -f echoserver echoclient