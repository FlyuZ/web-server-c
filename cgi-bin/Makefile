CC = gcc
CFLAGS = -O2 -Wall -I .

all: add

serve_dynamic: add.c
	$(CC) $(CFLAGS) -o addc add.c

clean:
	rm -f adder *~
