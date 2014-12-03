client:	src/client/client.c
	gcc src/client/client.c -o bin/client.out -lnsl -pthread

server: src/server/server.c
	gcc src/server/server.c -o bin/server.out -lnsl -pthread -lcrypto
