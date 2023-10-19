ifndef CC
	CC = gcc
endif

CFLAGS = -Wall -O
PROG=nc2

all: $(PROG)

debug: CFLAGS = -Wall -O -g -DDEBUG
debug: ${PROG}

main.o: main.c
	$(CC) $(CFLAGS) -c main.c 

$(PROG): main.o
	$(CC) -o $(PROG) main.o

clean:
	rm -f *.o $(PROG)

