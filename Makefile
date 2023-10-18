ifeq ($(DEBUG), y)
	CFLAGS = -Wall -O -g -DDEBUG
else
	CFLAGS = -Wall -O
endif

ifndef CC
	CC = gcc
endif

PROG=nc2

all: $(PROG)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c 

$(PROG): main.o
	$(CC) -o $(PROG) main.o

clean:
	rm -f *.o $(PROG)

