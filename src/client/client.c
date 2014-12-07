/**
 * @file client.c
 * @authors Matthew Lindner, Xiao Deng, Madeline Cameron
 *
 * @brief Multi-threaded client that updates server with tracker file and downloads chunks from other clients
 * @details Multi-threaded client that uses separate threads to seed and download chunks to / from other clients.
 *
 * @section COMPILE
 * gcc client.c -o client.out -lnsl -pthread -lcrypto
 */
 
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
#include <sys/fcntl.h>
#include <errno.h>
#include "config.ini"
#include "compute_md5.h"

//socket used to connect to server
int server_sock;
//socket used to act as "server"
int seed_sock;

int seed_port;

//we aren't really using these
int server_port, max_client, chunk_size;

struct peer
{
	int m_peer_socket; ///< Communication socket for this peer. If value = \b -1, this peer is currently not being used.
	int m_index; ///< Location of peer in the \a peers array.
	char m_buf[CHUNK_SIZE];  ///< Character array used to copy data into/out of \a m_peer_socket
	FILE *m_file; ///< File pointer used to open tracker files.
	pthread_t m_thread; ///< Thread where commands from peer will be processed.
};

struct peer peers[MAX_CLIENT];

void setUpPeerArray();
int findPeerArrayOpening();

void *download(void * index);
void *client_handler(void * index);

/*
void findIP();
*/
void readConfig();

#define SEED 0
#define DOWNLOAD 1
int mode;
int client_i;
char* ip;

int main(int argc, const char* argv[])
{
	if (argc < 4)
	{
		exit(0);
	}
	else
	{
		mode = atoi(argv[1]);
		seed_port = atoi(argv[2]);
		client_i = atoi(argv[3]);
	}

	//Specifies address: Where we are connecting our socket.
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	struct hostent *hp;
	
	char buf[CHUNK_SIZE];

	if((hp = gethostbyname("localhost")) == NULL)
	{
		printf("Error: Unknown Host");
		exit(1);
	}

	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length );

	setUpPeerArray();
	
	if (mode == SEED)
	{
		struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
		struct sockaddr_in client_addr = {AF_INET};
		int length = sizeof(client_addr);
		
		//try to create tracker....
		if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
		{
			perror( "Error: socket failed" );
			exit( 1 );
		}

		/*connect to the server */
		/* now when we need to communicate to the server, we do it over "server_sock" */
		if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
		{
			perror( "Error: Connection Issue" );
			exit(1);
		}
		
		memset(buf, '\0', sizeof(buf));
		char *md5 = computeMD5("picture-wallpaper.jpg");
		sprintf(buf, "<createtracker picture-wallpaper.jpg 35738 img %s localhost %d>", md5, seed_port); 
		write(server_sock, buf , strlen(buf));
		free(md5);
		
		/*if (pthread_create(&(peers[0].m_thread), NULL, &client_handler, &(client_i)) != 0)
		{
			printf("Error Creating Thread\n");
			exit(1);
		}*/
		
		
		clock_t elapsed_time, start = clock();
		int percentage = ((client_i == 1)? (0) : ((client_i * 20) -19));
		int increment, segment_num = 0;
		while (segment_num < 4)
		{
			elapsed_time = clock() - start;
			if (((float)elapsed_time/CLOCKS_PER_SEC) >= 1)// if it's been 10 seconds
			{
				if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
				increment = (percentage == 0)? (5) : (4);			
				
				printf("I am client_%d, and I am advertising the following chunk of the file: %d%% to %d%%.\n", client_i, percentage, percentage + increment);
				
				///////////////////////////////////
				///////////////////////////////////
				///I'm not sure the best way to calc/get the start and end bytes here.
				/////Should I just calculate the start and end bytes myself using (percentage * file size),
				/////Or Should I call findNextChunk() ?
				sprintf(buf, "<updatetracker picture-wallpaper.jpg %d %d localhost %d>", percentage /*start byte*/, percentage + increment /*end byte*/, seed_port);			
				///////////////////////////////////
				///////////////////////////////////
				
				write(server_sock, buf , strlen(buf));
				
				(percentage == 0)? (percentage+=6) : (percentage+=5);
				segment_num++;
				
				start = clock();
			}
		}
	}
	
	int foundPic = 0;
	int sent;
	if (mode == DOWNLOAD)
	{
		while (1)
		{
			if (foundPic == 1)
			{
				fgets(buf, sizeof(buf), stdin);
			}
			else
			{
				// wait 5 seconds
				clock_t elapsed_time, start = clock();
				do
				{
					elapsed_time = clock() - start;
				} while (((float)elapsed_time/CLOCKS_PER_SEC) < 5);
			}
			
			if ((strncmp(buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0) || foundPic == 0)
			{
				/* create a socket */
				/* internet stream socket, TCP */
				if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}

				/*connect to the server */
				/* now when we need to communicate to the server, we do it over "server_sock" */
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
			
				write(server_sock, "<REQ LIST>", strlen("<REQ LIST>"));
				memset(buf, '\0', sizeof(buf));
				while((sent = read(server_sock, buf, sizeof(buf))) > 0)
				{
					if (strstr(buf, "picture-wallpaper.jpg") != NULL)
					{
						foundPic = 1;
					}
					printf("%s", buf);
					memset(buf, '\0', sizeof(buf));
				}
				
				if (foundPic == 1)
				{
					//Since buf now contains a <GET> statement, the <GET> command should be invoked below.
					strcpy(buf, "<GET picture-wallpaper.jpg.track>");
				}
			}
			if (strncmp(buf, "<createtracker", strlen("<createtracker")) == 0)
			{		
				if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
				
				while((sent = read(server_sock, buf, sizeof(buf))) > 0)
				{
					printf("%s", buf);
				}
			}
			if (strncmp(buf, "<updatetracker", strlen("<updatetracker")) == 0)
			{
				if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
			
				write(server_sock, buf, strlen(buf));
				memset(buf, '\0', sizeof(buf));
				while((sent = read(server_sock, buf, sizeof(buf))) > 0)
				{
					printf("%s", buf);
				}
				printf("\n");
			}
			if (strncmp(buf, "<GET", strlen("<GET")) == 0)
			{
				FILE *file;
				char tokenize[CHUNK_SIZE], filename[CHUNK_SIZE];
				char *line;
				if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
			
				write(server_sock, buf, strlen(buf));
				
				strcpy(tokenize, buf);
				/*Get the tracker filename*/
				line = strtok(tokenize, " ");
				line  = strtok(NULL, ">");
				strcpy(filename, line);
				
				if (access(filename, F_OK) != 0)
				{
					file = fopen(filename, "wb");

					memset(buf, '\0', sizeof(buf));
					while((sent = read(server_sock, buf, sizeof(buf))) > 0)
					{
						fwrite(buf, sizeof(char), strlen(buf), file);
						memset(buf, '\0', sizeof(buf));
					}
					
					fclose(file);
					
					//spin off 5 downloads thread.
					int i;
					for (i = 0; i < 5; i++)
					{
						if (pthread_create(&(peers[i].m_thread), NULL, &download, &(peers[i].m_index)) != 0)
						{
							perror("Error creating download thread");
						}
					}
				}
				else
				{
					printf("You already have this tracker file!\n");
				}
			}
			close(server_sock);
		}
	}
	
	
	int i;
	for (i = 0; i < MAX_CLIENT; i++)
	{
		pthread_join(peers[i].m_thread, NULL);
	}

	return 0;
}

void *download(void * index)
{
	//index in array indicates what segment they are responsible for?
	// i = 0 -> 1st sub-chunk (0->20%)
	// i = 1 -> second sub-chunk (21%->40%)
	int client_index = *((int *) index);
	
	//find that chunk
	//connect to client who has that chunk
	//download it
	//repeat
	
	
	printf("%d\n", client_index);
		
	return;
}

void *client_handler(void * index)
{
	/* Dereference the index passed as a parameter by the pthread_create() function */
	int client_index = *((int *) index);
	
	struct sockaddr_in server_addr = { AF_INET, htons( seed_port ) };
	struct sockaddr_in client_addr = {AF_INET};
	/* The length of socketaddr structure */
	int length = sizeof(client_addr);

	char buf[CHUNK_SIZE];
	
	if((seed_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		perror( "Error: socket failed" );
		exit( 1 );
	}
	
	int setsock = 1;
	if(setsockopt(seed_sock, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(setsock)) == -1)
	{
		perror("Server Error: Setsockopt failed");
		exit(1);
	}
	
	/** Bind the socket to an internet port. */
	if (bind(seed_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror("Server Error: Bind Failed");
		exit(1);
	}
	
	if (listen(seed_sock, MAX_CLIENT) == -1)
	{
		perror("Server Error: Listen failed");
		exit(1);
	}
	
	//
	//This will probably need to be in a loop, because they will accept 1 chunk at a time.....
	//
	//
	if ((peers[0].m_peer_socket = accept(seed_sock, (struct sockaddr*)&client_addr, &length)) == -1)
	{
		perror("Server Error: Accepting issue"); 
		exit(1);
	}
	else
	{
		printf("A downloading client has connected.\n");

		/******************************
		 ******************************
		 *      SHARE FILE HERE       *
		 *   PEER SHOULD SND MESSAGE  *
		 *  SAYING WHAT CHUNKS IT WANTS *   
		 ******************************
		 ******************************/
	}
		
	
	/*When we're done sharing*/
	if (close(peers[0].m_peer_socket) != 0)
	{
		perror("Closing socket issue");
	}
	if (close(seed_sock) != 0)
	{
		perror("Closing socket issue");
	}
	if (pthread_cancel(peers[0].m_thread) != 0)
	{
		perror("Ending thread issue");
		exit(1);
	}
	
	return;
}

void setUpPeerArray()
{
	int index;
	for (index = 0; index < MAX_CLIENT; index++)
	{
		/* A m_socket value of -1 indicates that this element of the array is not being used. */
		peers[index].m_peer_socket = -1;
		/* Set each m_index = to it's index in the array.  */
		peers[index].m_index = index;
	}
	return;
}

int findPeerArrayOpening()
{
	int index;
	/* Check each element of the peers array, until we find an opening. */
	for (index = 0; index < MAX_CLIENT; index++)
	{
		/* Once an opening is found, immediately leave the function. */
		if (peers[index].m_peer_socket == -1)
		{
			return index;
		}
	}
	return -1;
}

void readConfig()
{
 	char* line;
	size_t length = 0;
	ssize_t read;
	FILE* configFile;
 
	if ((configFile = fopen("client.conf", "r")) != NULL)
	{
		int lineCount = 0;
		while((read = getline(&line, &length, configFile)) != -1)
		{
			switch(lineCount) 
			{
				/** The first line of the config file contains the server port */
				case 0:
					server_port = atoi(line);
					break;
				/** The second line contains the max number of clients that the server can process at once.*/
				case 1:
					max_client = atoi(line);
					break;
				/** The third line contains the chunk size (in bytes) of the messages passed between the server and clients. */
				case 2:
					chunk_size = atoi(line);
					break;
			}
			lineCount++;
		}
		fclose(configFile);
	}
	/** If a config file could not be opened, default values will be assigned:
	 * server_port = 3456, 
	 * max_client = 5, and 
	 * chunk_size = 1024.
	 */
	else
	{
		server_port = 3456;
 		max_client = 5;
 		chunk_size = 1024;
	}
	
	return;
}

/*
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
*/