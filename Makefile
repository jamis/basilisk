CC=gcc
AR=ar

DBGFLAGS ?= -g
OPTS=-Iinclude $(DBGFLAGS) -Wall

OUTDIR=output
SRCDIR=src

RUNLIBS = -L. -lbasilisk -lm

all: LIBOPTS =
all: LIBNAME = libbasilisk.a
all: prepare libbasilisk_static utils data

library: LIBOPTS = -Wl,-rpath .
library: LIBNAME = libbasilisk.so
library: OPTS += -fPIC -DPIC
library: prepare libbasilisk_dynamic utils data

utils: bskcompile bskrun bsktreasure
data: data-compile

BASES=\
  bskarray \
  bskatdef \
  bskbitset \
  bskctgry \
  bskdb \
  bskdebug \
  bskexec \
  bskidtbl \
  bsklexer \
  bskparse \
  bskrule \
  bskstream \
  bsksymtb \
  bskthing \
  bsktokens \
  bskutdef \
  bskutil \
  bskvalue \
  bskvar

OBJS=$(BASES:%=$(OUTDIR)/%.o)
SRCS=$(BASES:%=$(SRCDIR)/%.c)

prepare:
	mkdir -p $(OUTDIR)

libbasilisk_static: $(OBJS)
	ar r $(LIBNAME) $(OBJS)

libbasilisk_dynamic: $(OBJS)
	ld -shared -o $(LIBNAME) $(OBJS)
 
bskcompile: $(LIBNAME) bskcompile.c bskcallbacks.c
	$(CC) $(OPTS) $(LIBOPTS) -o bskcompile bskcompile.c bskcallbacks.c $(RUNLIBS)

bskrun: $(LIBNAME) bskrun.c bskcallbacks.c
	$(CC) $(OPTS) $(LIBOPTS) -o bskrun bskrun.c bskcallbacks.c $(RUNLIBS)

bsktreasure: $(LIBNAME) bsktreasure.c bskcallbacks.c
	$(CC) $(OPTS) $(LIBOPTS) -o bsktreasure bsktreasure.c bskcallbacks.c $(RUNLIBS)

data-compile: dat/standard/*.bsk dat/snfist/*.bsk dat/scitadel/*.bsk bskcompile $(LIBNAME)
	bskcompile dat/standard/index.bsk "dat/standard|dat/snfist|dat/scitadel"

$(OUTDIR)/bskarray.o: $(SRCDIR)/bskarray.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskatdef.o: $(SRCDIR)/bskatdef.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskbitset.o: $(SRCDIR)/bskbitset.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskctgry.o: $(SRCDIR)/bskctgry.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskdb.o: $(SRCDIR)/bskdb.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskdebug.o: $(SRCDIR)/bskdebug.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskexec.o: $(SRCDIR)/bskexec.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskidtbl.o: $(SRCDIR)/bskidtbl.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bsklexer.o: $(SRCDIR)/bsklexer.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskparse.o: $(SRCDIR)/bskparse.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskrule.o: $(SRCDIR)/bskrule.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskstream.o: $(SRCDIR)/bskstream.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bsksymtb.o: $(SRCDIR)/bsksymtb.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskthing.o: $(SRCDIR)/bskthing.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bsktokens.o: $(SRCDIR)/bsktokens.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskutdef.o: $(SRCDIR)/bskutdef.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskutil.o: $(SRCDIR)/bskutil.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskvalue.o: $(SRCDIR)/bskvalue.c
	$(CC) $(OPTS) -c -o $@ $<

$(OUTDIR)/bskvar.o: $(SRCDIR)/bskvar.c
	$(CC) $(OPTS) -c -o $@ $<

clean:
	rm -f libbasilisk.a
	rm -f libbasilisk.so
	rm -f bskrun
	rm -f bskcompile
	rm -f bsktreasure
	rm -f dat/standard/index.bdb
	rm -rf $(OUTDIR)
