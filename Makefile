IDIR=include
ODIR=obj
LDIR=lib
SDIR=src

CC=mips-openwrt-linux-uclibc-gcc
CFLAGS=-I$(IDIR)

LIBS=

_DEPS=serialio.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=regotest.o serialio.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bin/regotest: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f bin/regotest $(ODIR)/*.o

