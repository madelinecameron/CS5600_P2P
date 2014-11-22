client:	Client/client.c
	gcc Client/client.c -o bin/client -lnsl -pthread

server: Server/server.c
	gcc Server/server.c -o bin/server -lnsl -pthread -lcrypto
