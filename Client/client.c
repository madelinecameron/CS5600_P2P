/* gcc client.c -o client -lnsl -pthread */
#define _GNU_SOURCE //Used for findIP()
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <ifaddrs.h>
#include <unistd.h>

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
char* ip;

int findCommand()
{
    int i;
	char* isThisYourCmd[15];
	printf("1\n");
	char* createTracker = "<createtracker";
	printf("2\n")
	isThisYourCmd[0] = createTracker;

    printf("Bleh");
	strcpy(isThisYourCmd[1], "<REQ LIST>");

	for(i = 0; strcmp(buf, isThisYourCmd[i]) != 0 && i < (sizeof(isThisYourCmd)/sizeof(char*)); i++)
	{
		printf("I'm totally doing stuff\nCmd: %s", isThisYourCmd[i]);
	}
}
//Credit to: http://man7.org/linux/man-pages/man3/getifaddrs.3.html
void findIP()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,"eth0") == 0) && (ifa->ifa_addr->sa_family == AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            strcpy(ip, ifa->ifa_name);
        }
    }
}

int main()
{
	//Specifies address: Where we are connecting our socket.
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	struct hostent *hp;

    printf("Please input server address: ");
	//Example host name: rc01xcs213.managed.mst.edu
	//printf("Please Enter the Server You Would Like to Connect To\n");
	//scanf("%s", buf); //read the server address from the keyboard
	fgets(buf, CHUNK_SIZE, stdin);

	//strcpy(buf, "rc01xcs213");

	/* Get host via the server address entered by user into buf*/
	if((hp = gethostbyname("rc01xcs213")) == NULL)
	{
		printf("Error: Unknown Host");
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

    findCommand();
	findIP(); //Sets 'ip' with char* representation of address

	int sent;
	//memset(buf, '\0', sizeof(buf));
	if (strncmp(buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0)
	{
		write(sd, "<REQ LIST>", strlen("<REQ LIST>"));
		memset(buf, '\0', sizeof(buf));
		while((sent = read(sd, buf, sizeof(buf))) > 0)
		{
			printf("%s", buf);
			memset(buf, '\0', sizeof(buf));
		}
	}
	else if (strncmp(buf, "<createtracker", strlen("<createtracker")) == 0)
	{
		write(sd, buf, strlen(buf));
		memset(buf, '\0', sizeof(buf));
		while((sent = read(sd, buf, sizeof(buf))) > 0)
		{
			printf("%s", buf);
		}
	}
	else if (strncmp(buf, "<updatetracker", strlen("<updatetracker")) == 0)
	{
		write(sd, buf, strlen(buf));
		memset(buf, '\0', sizeof(buf));
		while((sent = read(sd, buf, sizeof(buf))) > 0)
		{
			printf("%s", buf);
		}
		printf("\n");
	}
	else if (strncmp(buf, "<GET", strlen("<GET")) == 0)
	{
		write(sd, buf, strlen(buf));

		file = fopen("dog.jpg", "wb");
		/*while(read(sd, buf, sizeof(buf)) > 0)
		{
			printf("%s", buf);
		}*/

		memset(buf, '\0', sizeof(buf));
		while((sent = read(sd, buf, sizeof(buf))) > 0)
		{
			fwrite(buf, sizeof(char), sent, file);
			memset(buf, '\0', sizeof(buf));
		}
		/*
			printf("%s\n", buf);

			save = strstr(buf, "<REP GET END");
			if(save!= NULL);
			{
				printf("123");
				printf("%s\n", save);
			}

			memset(buf, '\0', sizeof(buf));
		}
		memset(buf, '\0', sizeof(buf));

		rewind(temp);
		track = fopen("temp.track", "w");

		size_t len = 0;
		char *line = NULL;
		getline(&line, &len, temp); //ignore the first line
		int counter = 0;
		int read_size;
		while(counter < 20)
		{
			counter = counter + 1;
			getline(&line, &len, temp);
			if(strncmp(line, "<REP GET END", strlen("<REP GET END")) == 0)
			{
				//TODO md5
				printf("stopped at line %d\n", counter);
				break;
			}
			//printf("%s\n", line);
			fwrite(line, sizeof(char), strlen(line), track);
		}
		binary = fopen("temp.bin", "w");
		while( read_size = fread( buf, sizeof(char), sizeof(buf), temp ) )
		{
			fwrite(buf, sizeof(char), read_size, binary);
		}

		fclose(temp);
		fclose(binary);
		fclose(track);

		*/
		fclose(file);

	}

	//write(sd, "<createtracker test2 10 test_description md5 101.101 100>", strlen("<createtracker test2 10 test_description md5 101.101 100>"));
	//write(sd, "<GET dog.track>", strlen("<GET dog.track>"));
	//write(sd, "<updatetracker test 0 100 202.202 200>\n", strlen("<updatetracker test 0 100 202.202 200>\n"));

	///Delete me
	/*
	if((file = fopen("dog.jpg", "wb")) == NULL) //w for write, b for binary
	{
		printf("Error writing new file\n");
		exit(1);
	}
	memset(buf, '\0', sizeof(buf));

	while((sent = read(sd, buf, sizeof(buf))) > 0)
	{
		fwrite(buf, sizeof(char), sent, file);
		memset(buf, '\0', sizeof(buf));
	}
	*/
	/*

	//Creates a file to be written to. "Recieve.txt" should be replaced with the actual name of the file
	if((file = fopen("dog2.jpg", "wb")) == NULL) //w for write, b for binary
	{
		printf("Error writing new file\n");
		exit(1);
	}

	//Clear the buffer
	memset(buf, '\0', sizeof(buf));

	//This should permit continuous reading. As long as someone is sending, the client will continue reading
	int sent;
	while((sent = read(sd, buf, sizeof(buf))) > 0)
	{
		//Instead of writing the entire buffer, we only write the bits that were sent. This prevents us from writing any garbage data.
		fwrite(buf, sizeof(char), sent, file);
		memset(buf, '\0', sizeof(buf)); //reclear the buffer, so we don't accidently send some tailing data.
	}


	fclose(file); //close the file
	*/
	close(sd); //close the socket

	return (0);
}
