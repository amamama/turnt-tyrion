CC=gcc
CFLAGS:= -std=c11 -pedantic -Wall -g $(CFLAGS)
LDFLAGS:= $(LDFLAGS)
SRC = ski_ast.c utils.c

all: a.out

a.out: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC)

run : all
	./a.out $(ARGS)
