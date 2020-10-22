CC=gcc
CFLAGS=-Werror -ggdb

default: oss user

oss: oss.c data.h
	$(CC) $(CFLAGS) oss.c -o oss

user: user.c data.h
	$(CC) $(CFLAGS) user.c -o user

clean:
	rm -f oss user
