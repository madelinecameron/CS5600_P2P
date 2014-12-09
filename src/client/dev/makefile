CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lm

test : test.o client_support.o
	${CC} ${CFLAGS} client_support.o test.o ${LDFLAGS} -o test.out
	
client_support.o : client_support.c client_support.h
	${CC} ${CFLAGS} -c client_support.c

test.o : test.c
	${CC} ${CFLAGS} -c test.c
	
clean:
	rm *.o *.out
