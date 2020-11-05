CC=gcc
CFLAGS=-Werror -ggdb

default: master user

master: master.c data.h
	$(CC) $(CFLAGS) master.c -o master

user: user.c data.h
	$(CC) $(CFLAGS) user.c -o user

clean:
	rm -f master user
