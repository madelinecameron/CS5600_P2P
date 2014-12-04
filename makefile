CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lm

client:	src/client/client.c
	gcc src/client/client.c -o client.out -lnsl -pthread
	sh test_clients/setup_subfolders.sh

server: src/server/server.c
	gcc src/server/server.c -o server.out -lnsl -pthread -lcrypto

#test : src/client/test.o src/client/client_support.o
#	${CC} ${CFLAGS} src/client/client_support.o src/client/test.o ${LDFLAGS} -o src/client/test.out
#	
#client_support.o : src/client/client_support.c src/client/client_support.h
#	${CC} ${CFLAGS} -c src/client/client_support.c
#
#test.o : src/client/test.c
#	${CC} ${CFLAGS} -c src/client/test.c
	
clean:
	rm *.o *.out
