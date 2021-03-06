CC = gcc
CFLAGS = -Wall -Wextra -O2 -I$(IDIR) -lSDL2

ODIR = ./obj
IDIR = ./includes
SDIR = ./src
GBDIR = ./src/gameboy

_DEPS = display.h main.h cpu.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o cpu.o cycle.o disassembler.o display.o gpu.o mmu.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_SRC = %.c
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))
GBSRC = $(patsubst %,$(GBDIR)/%,$(_SRC))

$(ODIR)/%.o: $(SRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(GBSRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gbemu2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -rf $(ODIR)/*.o gbemu2
