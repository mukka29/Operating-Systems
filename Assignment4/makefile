CC=gcc
CFLAGS=-Werror -ggdb -Wall

default: master user

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

master: master.c data.h queue.o
	$(CC) $(CFLAGS) master.c queue.o -o master

user: user.c data.h
	$(CC) $(CFLAGS) user.c -o user

clean:
	rm -f master user queue.o
