#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>       
#include <pthread.h>
#include <signal.h>

/* gcc client.c -o client -lnsl -pthread */

#define SERVER_PORT 63122


int main()
{
	int sd; //socket used to connect to server
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) }; 
	char buf[512]; 
	struct hostent *hp; 

	FILE *file;
	
	printf("Please Enter the Server You Would Like to Connect To\n");
	scanf("%s", buf); //read the server address from the keyboard
	
	/* get the host */
	if((hp = gethostbyname(buf)) == NULL)
	{
		printf("Error: Unkown Host");
		exit(1);
	}
	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length ); 
	
	/* create a socket */
	if( ( sd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )  //internet stream socket, TCP
    { 
		perror( "Error: socket failed" ); 
		exit( 1 ); 
	} 
		
	/*connect to the socket */
	if (connect( sd, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
	{
		perror( "Error: Connection Issue" ); 
		exit(1);
	}

	if((file = fopen("receive.txt", "wb")) == NULL) //w for write, b for binary
	{
		printf("Error writing new file\n");
		exit(1);
	}

	//clear the buffer
	memset(buf, 0, sizeof(buf));

	while(read(sd, buf, sizeof(buf)) > 0)
	{
		fwrite(buf, sizeof(char), strlen(buf), file);
	}

	close(sd); //close the socket
	return (0);
}