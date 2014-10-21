/* gcc client.c -o client -lnsl -pthread */

//Get current IP: http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
//Get current IP: http://stackoverflow.com/questions/20237745/sockets-in-clinux-how-do-i-get-client-ip-port

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>       
#include <pthread.h>
#include <signal.h>

#define SERVER_PORT 63122

//we'd read this in from the config file
//Problem is.... how? C will complain ("error: variably modified array") if we dynamically allocate like that.
#define CHUNK_SIZE 512

//socket used to connect to server, and read data from
int sd;
//File pointer, Points to the file we want to read/write to.
FILE *file;
//Buffer used to send/receive. Data is loaded into buffer (see read() and fwrite()).
char buf[CHUNK_SIZE]; 

int main()
{
	//Specifies address: Where we are connecting our socket.
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) }; 
	struct hostent *hp; 

	//Example host name: rc01xcs213.managed.mst.edu
	printf("Please Enter the Server You Would Like to Connect To\n");
	scanf("%s", buf); //read the server address from the keyboard
	
	/* Get host via the server address entered by user into buf*/
	if((hp = gethostbyname(buf)) == NULL)
	{
		printf("Error: Unkown Host");
		exit(1);
	}
	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length ); 
	
	/* create a socket */
	/* internet stream socket, TCP */
	if( ( sd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )  
    { 
		perror( "Error: socket failed" ); 
		exit( 1 ); 
	} 
		
	/*connect to the socket */
	/* now when we need to communicate to the server, we do it over "sd" */
	if (connect( sd, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
	{
		perror( "Error: Connection Issue" ); 
		exit(1);
	}

	//Creates a file to be written to. "Recieve.txt" should be replaced with the actual name of the file
	if((file = fopen(/*"receive.txt"*/ "dog2.jpg", "wb")) == NULL) //w for write, b for binary
	{
		printf("Error writing new file\n");
		exit(1);
	}

	//Clear the buffer
	/* This might not be necessary. Since the test file I was sending with the server was < 512 bytes, I used strlen(buf)
	   to only write the contents of the "send.txt" file. So once it read the first 0, it stopped writing.
		 I'm not sure how strlen() will handle other files though.*/
	memset(buf, '\0', sizeof(buf));

	/*This should permit continuous reading. As long as someone is sending, the client will continue reading*/
	int sent;
	while((sent = read(sd, buf, sizeof(buf))) > 0)
	{
		//Instead of writing the entire buffer, we only write the bits that were sent. This prevents us from writing any garbage data.
		fwrite(buf, sizeof(char), sent, file);
		memset(buf, '\0', sizeof(buf)); //reclear the buffer, so we don't accidently send some tailing data.
	}

	close(sd); //close the socket
	fclose(file); //close the file
	return (0);
}
