CC=gcc
AR=ar

OPTS=-Iinclude -g -Wall

.SUFFIXES:

.SUFFIXES: .c .o

.c.o:
	$(CC) $(OPTS) -c -o $@ $<

all: libbasilisk.a bskcompile bskrun bsktreasure data-compile

OBJS=\
  src/bskarray.o \
  src/bskatdef.o \
  src/bskbitset.o \
  src/bskctgry.o \
  src/bskdb.o \
  src/bskdebug.o \
  src/bskexec.o \
  src/bskidtbl.o \
  src/bsklexer.o \
  src/bskparse.o \
  src/bskrule.o \
  src/bskstream.o \
  src/bsksymtb.o \
  src/bskthing.o \
  src/bsktokens.o \
  src/bskutdef.o \
  src/bskutil.o \
  src/bskvalue.o \
  src/bskvar.o

libbasilisk.a: $(OBJS)
	ar r libbasilisk.a $(OBJS)
 
bskcompile: libbasilisk.a bskcompile.c bskcallbacks.c
	$(CC) $(OPTS) -o bskcompile bskcompile.c bskcallbacks.c -lm -L. -lbasilisk

bskrun: libbasilisk.a bskrun.c bskcallbacks.c
	$(CC) $(OPTS) -o bskrun bskrun.c bskcallbacks.c -lm -L. -lbasilisk

bsktreasure: libbasilisk.a bsktreasure.c bskcallbacks.c
	$(CC) $(OPTS) -o bsktreasure bsktreasure.c bskcallbacks.c -lm -L. -lbasilisk

data-compile: dat/standard/*.bsk dat/snfist/*.bsk dat/scitadel/*.bsk libbasilisk.a
	bskcompile dat/standard/index.bsk "dat/standard|dat/snfist|dat/scitadel"

clean:
	rm -f libbasilisk.a
	rm -f bskrun
	rm -f bskcompile
	rm -f bsktreasure
	rm -f src/*.o
	rm -f dat/standard/index.bdb

