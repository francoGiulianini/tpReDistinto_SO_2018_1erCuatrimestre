CC = gcc
MAKE = make
SUBDIRS = Coordinador/src ESI/src Instancia/src Planificador/src

.PHONY = subdirs $(SUBDIRS) clean

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	-rm -f Coordinador/src/*.o ESI/src/*.o Instancia/src/*.o Planificador/src/*.o