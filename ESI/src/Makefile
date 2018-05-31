CFLAGS = -lcommons -lparsi

%: %.o
	$(CC) -o $(*F) $(*F).o $(CFLAGS)

ESI: ESI.o

ESI.o: ESI.h