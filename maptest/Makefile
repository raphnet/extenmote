CC=gcc
LD=$(CC)
CFLAGS=-Wall -O2

PROG=maptest

all: $(PROG)


$(PROG): main.o
	$(LD) main.o -o $(PROG) -lrt

%.o: %.c %.h
	$(CC) -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm *.o $(PROG)
