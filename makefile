CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lm
DEV_DIR = src/client/dev/
CLIENT_DIR = src/client/
SERVER_DIR = src/server/

all: client server
	@echo "\n ======== CS5600 P2P project Makefile ========\n"
	@echo "\n ======== [MAKE] Directory management in progress... ========\n"
	sh test_clients/setup_subfolders.sh
	@echo "\n ======== [MAKE] DONE! ========\n"

client: client.o ${DEV_DIR}client_support.o
	@echo "\n ======== [MAKE] Linking client ... ========\n"
	${CC} ${CFLAGS} ${DEV_DIR}client_support.o client.o ${LDFLAGS} -o client.out -lnsl -pthread -lcrypto
	
server: ${SERVER_DIR}server.c
	@echo "\n ======== [MAKE] Linking server ... ========\n"
	${CC} ${CFLAGS} ${SERVER_DIR}server.c -o server.out -lnsl -pthread -lcrypto

test: ${DEV_DIR}test.o ${DEV_DIR}client_support.o
	@echo "\n ======== [MAKE] Compiling test ... ========\n"
	${CC} ${CFLAGS} ${DEV_DIR}client_support.o ${DEV_DIR}test.o ${LDFLAGS} -o ${DEV_DIR}test.out
	
client.o: ${CLIENT_DIR}client.o
	@echo "\n ======== [MAKE] Compiling client.o ... ========\n"
	${CC} ${CFLAGS} -c ${CLIENT_DIR}client.c -lnsl -pthread -lcrypto
	
client_support.o: ${DEV_DIR}client_support.c ${DEV_DIR}client_support.h
	@echo "\n ======== [MAKE] Compiling client_support.o ... ========\n"
	${CC} ${CFLAGS} -c ${DEV_DIR}client_support.c

test.o: ${DEV_DIR}test.c
	@echo "\n ======== [MAKE] Compiling test.o ... ========\n"
	${CC} ${CFLAGS} -c ${DEV_DIR}test.c
	
clean:
	@echo "\n ======== [MAKE] Cleaning up ... ========\n"
	rm ${DEV_DIR}*.o ${DEV_DIR}*.o ${CLIENT_DIR}*.o *.out *.o -rf test_clients/client_*
	@echo "\n ======== [MAKE] DONE!  ========\n"