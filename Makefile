CC = gcc

IDIR = ./src/includes
CFLAGS = -I$(IDIR)

ODIR = ./obj
SDIR = ./src

LIBS=-lm -lSDL2

_DEPS = main.h display.h mmu.h utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o disassembler.o display.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_SRC = %.c
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

$(ODIR)/%.o: $(SRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gbemu: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o gbemu
