CC = gcc
MAKE = make
CARPETAS = ESI Instancia Planificador Coordinador

.PHONY = all $(CARPETAS) cleanObj cleanAll

all:
	make -C ESI/src/; \
	make -C Planificador/src/; \
	make -C Coordinador/src/; \
	make -C Instancia/src/

cleanObj:
	for dir in $(CARPETAS); do \
	rm -f $$dir/src/*.o; \
	done

cleanAll: cleanObj
	for dir in $(CARPETAS); do \
	rm -f $$dir/src/$$dir; \
	done