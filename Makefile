CC = gcc
CFLAGS = -I. -lSDL2
DEPS = Main.h
OBJ = Main.o Display.o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< -I.

gbemu: $(OBJ)
	$(CC) -o gbemu $^ $(CFLAGS)

clean:
	rm *.o
