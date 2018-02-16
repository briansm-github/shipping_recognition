OBJS  = chop.o features.o compare.o


CC=gcc 
CFLAGS= -O2
LDFLAGS=  -O -lm

all:    $(OBJS)
	$(CC) -o chop chop.o 
	$(CC) -o features features.o -lm -lfftw3f
	$(CC) -o compare compare.o 




clean: 
	rm -f *.o *~

.c.o:	$<
	$(CC) $(CFLAGS) -c $<

