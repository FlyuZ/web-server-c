CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: webServer cgi

webServer: webServer.c functionLib.o
	$(CC) $(CFLAGS) -o webServer webServer.c functionLib.o $(LIB)

functionLib.o: functionLib.c
	$(CC) $(CFLAGS) -c functionLib.c

cgi:
	(cd cgi-bin; make)

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)
