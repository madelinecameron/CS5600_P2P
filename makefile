CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lm
TEST_DIR = src/client/dev/
CLIENT_DIR = src/client/
SERVER_DIR = src/server/

all: client server
	sh test_clients/setup_subfolders.sh

client: client.o client_support.o
	${CC} ${CFLAGS} ${TEST_DIR}client_support.o client.o ${LDFLAGS} -o client.out
	
client.o: ${CLIENT_DIR}client.o
	${CC} ${CFLAGS} -c ${CLIENT_DIR}client.c -lnsl -pthread -lcrypto

server: ${SERVER_DIR}server.c
	${CC} ${CFLAGS} ${SERVER_DIR}server.c -o server.out -lnsl -pthread -lcrypto

test: ${TEST_DIR}test.o ${TEST_DIR}client_support.o
	${CC} ${CFLAGS} ${TEST_DIR}client_support.o ${TEST_DIR}test.o ${LDFLAGS} -o ${TEST_DIR}test.out
	
client_support.o: ${TEST_DIR}client_support.c ${TEST_DIR}client_support.h
	${CC} ${CFLAGS} -c ${TEST_DIR}client_support.c

test.o: ${TEST_DIR}test.c
	${CC} ${CFLAGS} -c ${TEST_DIR}test.c
	
clean:
	rm ${TEST_DIR}*.o ${TEST_DIR}*.out ${CLIENT_DIR}*.out ${SERVER_DIR}*.out
