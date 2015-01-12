CC=gcc
CFLAGS:= -std=c11 -pedantic -Wall -g $(CFLAGS)
LDFLAGS:= $(LDFLAGS)
SRC = ski_ast.c utils.c

all: a.out

a.out: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC)

run : all test.c
	cpp -P -lang-c test.c test.lzk
	./a.out < test.lzk
