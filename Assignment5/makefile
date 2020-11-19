CC=gcc
CFLAGS=-Werror -ggdb -Wall

default: oss user

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

res.o: res.c res.h
	$(CC) $(CFLAGS) -c res.c

oss: oss.c data.h queue.o res.o
	$(CC) $(CFLAGS) oss.c queue.o res.o -o oss

user: user.c data.h res.o
	$(CC) $(CFLAGS) user.c res.o -o user

clean:
	rm -f oss user *.o
