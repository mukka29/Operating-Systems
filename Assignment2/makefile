CC=gcc
CFLAGS=-Werror -ggdb

default: master palin

master: master.c data.h
	$(CC) $(CFLAGS) master.c -o master

palin: palin.c data.h
	$(CC) $(CFLAGS) palin.c -o palin

clean:
	rm -f master palin palin.out nopalin.out
