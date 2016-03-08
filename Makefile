BDIR=bin
IDIR=include
ODIR=obj
LDIR=lib
SDIR=src

CC=mips-openwrt-linux-uclibc-gcc
CFLAGS=-I$(IDIR)

LIBS=

_DEPS=regoComm.h regoSerialIO.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=regoClient.o regoComm.o regoSerialIO.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/regoClient: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

install:
	scp $(BDIR)/regoClient root@heat:

clean:
	rm -f $(BDIR)/regoClient $(ODIR)/*.o

