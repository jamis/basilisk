include Makefile.opts

OPTS = $(BASEOPTS)

all:
	@echo "Please specify one of the following options to build Basilisk:"
	@echo "  dynamic_lib: builds ONLY the dynamically linked Basilisk"
	@echo "     library."
	@echo "  dynamic: builds a dynamically linked library, all utilities"
	@echo "     and compiles the data files."
	@echo "  static_lib: builds ONLY the statically linked Basilisk"
	@echo "     library."
	@echo "  static: builds a statically linked library, all utilities"
	@echo "     and compiles the data files."
	@echo "  package: builds .tar.gz files for the Basilisk sources,"
	@echo "     documentation, and data files."
	@echo
	@echo "Once the library has been built, you may specify one of the"
	@echo "following options:"
	@echo "  utils: builds ONLY the basilisk utilities"
	@echo "  compile: compiles the basilisk data"
	@echo "  clean: removes all executables and object files from the"
	@echo "     Basilisk directories."

static: static_lib utils compile
	@echo "Done making static basilisk library"

static_lib: prepare
	@echo "Making static basilisk library"
	@make -f Makefile.static all

dynamic: dynamic_lib utils compile
	@echo "Done making dynamic basilisk library"

dynamic_lib: prepare
	@echo "Making dynamic basilisk library"
	@make -f Makefile.dynamic all

package:
	@echo "Building basilisk packages"
	@make -f Makefile.packages all

prepare:
	@mkdir -p $(OUTDIR)

utils: bskcompile bskrun bsktreasure
	@echo "Utility programs finished"

compile: dat/standard/*.bsk dat/snfist/*.bsk dat/scitadel/*.bsk bskcompile
	@echo "Compiling data files"
	@bskcompile dat/standard/index.bsk "dat/standard|dat/snfist|dat/scitadel|dat/dragon|dat/nbomt|dat/defenders|dat/tnblood|dat/boem|dat/diablo|dat/dungeon|dat/motp|dat/polyhedron|dat/mof|dat/frcs"
	
bskcompile: bskcompile.c bskcallbacks.c
	$(CC) $(OPTS) -o bskcompile bskcompile.c bskcallbacks.c $(RUNLIBS)

bskrun: bskrun.c bskcallbacks.c
	$(CC) $(OPTS) -o bskrun bskrun.c bskcallbacks.c $(RUNLIBS)

bsktreasure: bsktreasure.c bskcallbacks.c
	$(CC) $(OPTS) -o bsktreasure bsktreasure.c bskcallbacks.c $(RUNLIBS)

update:
	# doesn't work on Windows (but ok Cygwin).  How to fix?
	CVS_RSH=ssh; export CVS_RSH; cvs update

clean:
	rm -f libbasilisk.a
	rm -f libbasilisk.so
	rm -f bskrun
	rm -f bskcompile
	rm -f bsktreasure
	rm -f dat/standard/index.bdb
	rm -rf $(OUTDIR)
