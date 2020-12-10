CC=gcc
CFLAGS=-Werror -ggdb -Wall

default: oss user

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

oss: oss.c data.h queue.o
	$(CC) $(CFLAGS) oss.c queue.o -o oss -lm

user: user.c data.h
	$(CC) $(CFLAGS) user.c -o user -lm

clean:
	rm -f oss user *.o
