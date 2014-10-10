/* gcc server.c -o server -lnsl -pthread */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>      
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define SERVER_PORT 63122
#define MAX_CLIENT 10
#define BUFFER_SIZE 512

//socket for hosting the server (sd), socket for the client (ns)
//We should have a variable for each peer. Use this variable in the peer thread.
//Maybe construct a peer struct/class? Maybe an array of them? Contains socket variable, thread, etc.
int sd, ns;

//length of socketaddr structure
int length;

//File to be sent
FILE *file;

//Send contents of this buffer
char buf[512];

int main()
{
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	struct sockaddr_in client_addr = { AF_INET };
	length = sizeof( client_addr );
	
	/* Create a stream socket. Server will listen on this socket for peers. */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Server Error: Socket Failed");
		exit(1);
	}
	
	//variable needed for setsokopt call
	int setsock = 1;
	
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(setsock)) == -1)
	{
		perror("Server Error: Setsockopt failed");
		exit(1);
	}
	
	/* bind the socket to an internet port */
	if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror("Server Error: Bind Failed");
		exit(1);
	}
	
	// listen for clients
	printf(">>Server is now listening for up to 1 client\n");
	if (listen(sd, 1) == -1) //1 = length of queue for pending connections. We'll have to make this longer.
	{
		perror("Server Error: Listen failed");
		exit(1);
	}
	
	//In order to continously listen for clients, I think we'll need a while(!quit); loop, or something.
	//For an example, see the "server.c" file in the "Chat Room - Reference" folder.

	//If we decide to use a Peer struct, it's socket variable would replace "ns".
	//Once a peer connects, create a thread.
	if ((ns = accept(sd, (struct sockaddr*)&client_addr, &length)) == -1)
	{
		perror("Server Error: Accept Failed");
	}

	//open the file to be sent
	if((file = fopen("send.txt", "rb")) == NULL) //r for read, b for binary
	{
		printf("Error opening file\n");
		exit(1);
	}

	//clear the buffer
	memset(buf, 0, sizeof(buf));

	//copy the file to be sent
	if(fread(buf, sizeof(char), sizeof(buf), file) < 0)
	{
		printf("Error loading file\n");
	}
	/* Perhaps for fread() and write(), we'll use a loop.
	   Read a chunk, send a chunk. Repeat until entire file is read*/
	write(ns, buf, strlen(buf));
	
	
	fclose(file); // close the file stream
	close(sd); //close the socket

	return (0);
}