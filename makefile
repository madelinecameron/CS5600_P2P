client:	Client/client.c
	gcc Client/client.c -o bin/client.out -lnsl -pthread

server: Server/server.c
	gcc Server/server.c -o bin/server.out -lnsl -pthread -lcrypto
