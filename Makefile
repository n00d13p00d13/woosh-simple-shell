CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -O1 -fsanitize=address

all:
	mkdir -p build
	$(CC) $(CFLAGS) -c woosh.c -o build/woosh.o
	$(CC) $(CFLAGS) -c vector.c -o build/vector.o
	$(CC) $(CFLAGS) build/woosh.o build/vector.o -o build/woosh

clean:
	rm -rf build
