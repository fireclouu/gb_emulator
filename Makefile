CC = gcc

IDIR = ./src/includes
GBDIR = ./src/gameboy
HSDIR = ./src/host

CFLAGS = -I$(IDIR)

ODIR = ./obj
SDIR = ./src

LIBS=-lm -lSDL2

_DEPS = shared.h cpu.h cycle.h display.h host.h mmu.h utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o cycle.o disassembler.o display.o gpu.o host.o cpu.o mmu.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_SRC = %.c
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))
GBSRC = $(patsubst %,$(GBDIR)/%,$(_SRC))
HSSRC = $(patsubst %,$(HSDIR)/%,$(_SRC))

$(ODIR)/%.o: $(SRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(GBSRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(HSSRC) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gbemu: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o gbemu
