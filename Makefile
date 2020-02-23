CC=gcc
CFLAGS=-I.
DEPS = Main.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gbemu: Main.o
	$(CC) -o gbemu Main.c $(CFLAGS)

clean:
	rm *.o
