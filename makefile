CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lm
TEST_DIR = src/client/dev/
CLIENT_DIR = src/client/
SERVER_DIR = src/server/

all: client server

client:	${CLIENT_DIR}client.c
	g++ ${CLIENT_DIR}client.c -o client.out -lnsl -pthread -lcrypto
#sh test_clients/setup_subfolders.sh

server: ${SERVER_DIR}server.c
	g++ ${SERVER_DIR}server.c -o server.out -lnsl -pthread -lcrypto

test: ${TEST_DIR}test.o ${TEST_DIR}client_support.o
	${CC} ${CFLAGS} ${TEST_DIR}client_support.o ${TEST_DIR}test.o ${LDFLAGS} -o ${TEST_DIR}test.out
	
client_support.o: ${TEST_DIR}client_support.c ${TEST_DIR}client_support.h
	${CC} ${CFLAGS} -c ${TEST_DIR}client_support.c

test.o: ${TEST_DIR}test.c
	${CC} ${CFLAGS} -c ${TEST_DIR}test.c
	
clean:
	rm ${TEST_DIR}*.o ${TEST_DIR}*.out ${CLIENT_DIR}*.out ${SERVER_DIR}*.out
