CFLAGS = -lcommons -pthread

%: %.o
	cc -o $(*F) $(*F).o $(CFLAGS)

Coordinador: Coordinador.o

Coordinador.o: Coordinador.h