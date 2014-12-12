/**
 * @file client.c
 * @authors Matthew Lindner, Xiao Deng, Madeline Cameron
 *
 * @brief Multi-threaded client that updates server with tracker file and downloads chunks from other clients
 * @details Multi-threaded client that uses separate threads to seed and download chunks to / from other clients.
 *
 * The client has two modes of execution: seed and download.
 *
 * @section COMPILE
 * g++ client.c -o client.out -lnsl -pthread -lcrypto
 */
 
#ifndef _GNU_SOURCE
#define _GNU_SOURCE //Used for findIP()
#endif
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
#include "constants.ini"
#include "compute_md5.h"

/**
 * Socket variable for connecting the tracker server.
 */
int server_sock;
/**
 * Socket variable accepting connections from other clients. Client listens for connections on this socket.
 */
int seed_sock;
/**
 * Port that other clients can connect to this client on.
 */
int seed_port;
/**
 * Time interval (in seconds) that the client contacts the server with an <updatetracker> command.
 */
int server_update_frequency;
/**
 * The port the tracker server is on.
 */
int server_port;
/**
 * Which mode the client is operating in: seed (mode = 0) or download (mode = 1).
 */
int mode;
/**
 * Constant.
 */
#define SEED 0
/**
 * Constant.
 */
#define DOWNLOAD 1
/**
 * Client number (1-5). Passed as a command line argument. Used in the final demonstration to show which client was sharing what file segment (i.e. 1%-5%). 
 */
int client_i;
/**
 * Stores the IP address of the tracker.
 * Deprecated.
 */
char* ip;
/**
 * The maximum number of connections the client can have at a single time, and the maximum number of threads that can be executed at once.
 */
int max_client;
/**
 * The maximum size (in bytes) of a message sent between two hosts.
 */
int chunk_size;

/**
 * Represents a peer (client) application.
 * Each peer is handled with its own thread.
 * Each peer has it's own: socket, index (in the peers array), buffer (for temporarily storing data), a file pointer, and a thread variable.
 */
struct peer
{
	int m_peer_socket; ///< Communication socket for this peer. If value = \b -1, this peer is currently not being used.
	int m_index; ///< Location of peer in the \a peers array.
	char m_buf[CHUNK_SIZE];  ///< Character array used to copy data into/out of \a m_peer_socket
	FILE *m_file; ///< File pointer used to open tracker files.
	pthread_t m_thread; ///< Thread where commands from peer will be processed.
};

/**
 * Array of clients.
 * \a MAX_CLIENT is the maximum number of connections the client can have at a single time, and the maximum number of threads that can be executed at once.
 */
struct peer peers[MAX_CLIENT];
/**
 * Initializes each peer in the \a peers array.
 * 
 * Each \a m_index is set to \b i, the index of the array (0 - \a MAX_CLIENT).
 * Each \a m_peer_socket is set to \b -1 to indicate that this peer is currently not being used.
 */
void setUpPeerArray();
/**
 * Returns an index indicating where a new peer can be stored in the \a peers array. Checks each element of the array sequentially until an unused slot is located.
 * @return Index (0 - \a MAX_CLIENT -1) of the \a clients array where new client will be stored, -1 if array is full (client is currently serving the maximum amount of peers).
 */
int findPeerArrayOpening();
/**
 * Opens the "picture-wallpaper.jpg.track" tracker file. Parses out the port of a peer currently sharing a file chunk, and connects to that peer.
 * Used when the server is in DOWNLOAD mode.
 */
void *download(void * index);
/**
 * First enables the peer to accept TCP connections from other peers. Listens for up to \a MAX_CLIENT peers, and prints a message whenever another peer has made a connection.
 */
void *client_handler(void * index);

/**
 * Deprecated. Stores the IP address of this client in the \a IP variable.
 *
 * void findIP();
 */
 
/**
 * Reads in \a server_port, \a max_client, \a chunk_size, and \a server_update_frequency (in that order) from a config file.
 * If the config file cannot be opened, or is not found, these variables are given default values: 3456, 10, 1024, and 900 respectfully.
 */
void readConfig();

/**
 * When client.out is executed, the client will operate in one of two modes.
 * In SEED mode, the client first connects to the tracker server and creates a tracker file. It spins of a thread to accept and serve connections (client_handler()), and then updates the tracker server every \a server_update_frequency seconds with new chunks that it is sharing.
 * In download mode, the client contacts the server every 5 seconds, looking to see if anyone is currently sharing "picture-wallpaper.jpg". Once the server responds with this information, the client downloads the "picture-wallpaper.jpg.track " tracker file from the server, and spins off threads to download the image.
 *
 */
int main(int argc, const char* argv[])
{
	/**
	 * First checks to see if mode, seed_port, client_i, and server_update_frequency were passed in as parameters.
	 * If no parameters were passed, readConfig() is called, and a default values are assigned.
	 */
	if (argc < 5)
	{
		readConfig();
	}
	else
	{
		server_port = 3457;
		mode = atoi(argv[1]);
		seed_port = atoi(argv[2]);
		client_i = atoi(argv[3]);
		server_update_frequency = atoi(argv[4]);
	}

	/* Specifies address: Where we are connecting our socket. */
	struct sockaddr_in server_addr = { AF_INET, htons( server_port ) };
	struct hostent *hp;
	
	/* A buffer for temporarily storing text used in main. */
	char buf[CHUNK_SIZE];

	/** Get the IP address of the tracker server.*/
	if((hp = gethostbyname("localhost")) == NULL)
	{
		printf("Error: Unknown Host");
		exit(1);
	}
	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length );

	/** Initialize the client array so each element is marked as unused. */
	setUpPeerArray();
	
	if (mode == SEED)
	{
		/** Initialize a TCP connection to the tracker server. */
		struct sockaddr_in server_addr = { AF_INET, htons( server_port ) };
		
		if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
		{
			perror( "Error: socket failed" );
			exit( 1 );
		}

		/** Connect to the server.
		 * Now when we need to communicate to the server, we do it over "server_sock".
		*/
		if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
		{
			perror( "Error: Connection Issue" );
			exit(1);
		}
		
		memset(buf, '\0', sizeof(buf));
		
		/** Calculate the MD5 of the picture file we will be sharing. */
		sprintf(buf, "test_clients/client_%d/picture-wallpaper.jpg", client_i);
		char *md5 = computeMD5(buf);
		/** Contact the tracker server, and try to create a tracker file. */
		sprintf(buf, "<createtracker picture-wallpaper.jpg 35738 img %s localhost %d>", md5, seed_port); 
		write(server_sock, buf , strlen(buf));
		free(md5);
		
		/** Spin off a single thread that will accept connections, and share chunks. */
		/* We use the 0th element of the peers array since we only need 1 thread to upload (as per Final Demo requirement. */
		if (pthread_create(&(peers[0].m_thread), NULL, &client_handler, &(client_i)) != 0)
		{
			printf("Error Creating Thread\n");
			exit(1);
		}

		/* Clocks used to wait a predetermined amount of time between contacting the server for a list of tracker files.*/
		clock_t elapsed_time, start = clock();
		/* The initial lower bound of the segment percentage we are sharing. (ie 21% for client_i = 1) */
		int percentage = ((client_i == 1)? (0) : ((client_i * 20) - 19));
		int increment, segment_num = 0;
		/* The client shares in 5% increments. 20 / 5 = 4. So we will increment our percentage 4 times. */
		while (segment_num < 4)
		{
			elapsed_time = clock() - start;
			/* If it has been so many seconds... */
			if (((float)elapsed_time/CLOCKS_PER_SEC) >= server_update_frequency)
			{
				if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}
				/* Reconnect the the tracker server. */
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
				increment = (percentage == 0)? (5) : (4);			
				
				printf("I am client_%d, and I am advertising the following chunk of the file: %d%% to %d%%.\n", client_i, percentage, percentage + increment);

				/* Update the server, letting it know that we are now sharing an additional 5% of the file. If we had more time, we would have calculated the actual start and end bytes. */
				sprintf(buf, "<updatetracker picture-wallpaper.jpg %d %d localhost %d>", percentage /*start byte*/, percentage + increment /*end byte*/, seed_port);
				write(server_sock, buf , strlen(buf));
				
				/**Increment the percentage of the file we are sharing. */ 
				(percentage == 0)? (percentage+=6) : (percentage+=5);
				segment_num++;
				
				start = clock();
			}
		}
	}
	
	/* When foundPic = 1, this means that the server has responded to the <REQ LIST> command indicating that someone is sharing "picture-wallpaper.jpg"
	   Every 5 seconds, the client will send the <REQ LIST> command and check the servers response. Once somone is sharing the file we want, foundPic = 1.*/
	int foundPic = 0;
	int sent;
	/**
	 * When presenting in DOWNLOAD mode, the client will automatically contact the tracker server every 5 seconds until someone is sharing the picture-wallpaper.jpg.
	 * The client will then call the <GET> command, download the tracker file from the server, and spin off a download thread.
	 * The client will then allow the user to input commands from the keyboard until the client is closed.
	 */
	if (mode == DOWNLOAD)
	{
		while (1)
		{
			/* Once we begin downloading the picture, we will allow the user to input commands. */
			if (foundPic == 1 /* && donwload_finish == false*/)
			{
				fgets(buf, sizeof(buf), stdin);
			}
			else
			{
				/* Wait 5 seconds. */
				clock_t elapsed_time, start = clock();
				do
				{
					elapsed_time = clock() - start;
				} while (((float)elapsed_time/CLOCKS_PER_SEC) < 5);
			}
			/* For presenting mode, the <REQ LIST> command will automatically be called (the foundPic == 0 will always be evaluated to true). */
			if ((strncmp(buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0) || foundPic == 0)
			{
				/* create a socket */
				/* internet stream socket, TCP */
				if( ( server_sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
				{
					perror( "Error: socket failed" );
					exit( 1 );
				}

				/* connect to the server */
				if (connect( server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
				{
					perror( "Error: Connection Issue" );
					exit(1);
				}
			
				/* Send the server the <REQ LIST> command. */
				write(server_sock, "<REQ LIST>", strlen("<REQ LIST>"));
				memset(buf, '\0', sizeof(buf));
				while((sent = read(server_sock, buf, sizeof(buf))) > 0)
				{
					/* Search the server's message for the picture-wallpaper tracker file. */
					if (strstr(buf, "picture-wallpaper.jpg") != NULL)
					{
						/*Once we find it, set foundPic to 1 to allow keyboard input after the download has begun. */
						foundPic = 1;
					}
					printf("%s", buf);
					memset(buf, '\0', sizeof(buf));
				}
				
				/* Since buf now contains a <GET> statement, the <GET> command should be invoked below. */
				if (foundPic == 1)
				{
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
				
				/* Send the server the <createtracker> command. */
				write(server_sock, buf, strlen(buf));
				memset(buf, '\0', sizeof(buf));
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
			/* When presenting, this code will automatically be executed once someone is sharing the picture-wallpaper.jpg file. */
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
			
				/* Send the tracker server the command. */
				write(server_sock, buf, strlen(buf));
				
				strcpy(tokenize, buf);
				/*Get the tracker filename*/
				line = strtok(tokenize, " ");
				line  = strtok(NULL, ">");
				strcpy(filename, line);
				
				/* Create an empty file to save the tracker file in. */
				file = fopen(filename, "wb");

				memset(buf, '\0', sizeof(buf));
				while((sent = read(server_sock, buf, sizeof(buf))) > 0)
				{
					/* Save the tracker file (sent from server). */
					fwrite(buf, sizeof(char), strlen(buf), file);
					memset(buf, '\0', sizeof(buf));
				}
				
				fclose(file);
				
				/* Spin off 5 downloads thread. */
				int i;
				for (i = 0; i < 1; i++) /* For debugging purposes, we only spin off 1 thread. */
				{
					if (pthread_create(&(peers[i].m_thread), NULL, &download, &(peers[i].m_index)) != 0)
					{
						perror("Error creating download thread");
					}
				}
			}
			
			/*
			if (download_finish == true)
			{
				//call xiao's appendSegment() function too piece together everything
			}
			*/
			close(server_sock);
		}
	}
	
	
	int i;
	/** Close the program once all threads have completed their work (seeding or downloading). */
	for (i = 0; i < MAX_CLIENT; i++)
	{
		pthread_join(peers[i].m_thread, NULL);
	}

	return 0;
}

void *download(void * index)
{
	//index indicates what segment they are responsible for
	// i = 0 -> 1st sub-chunk (0->20%)
	// i = 1 -> second sub-chunk (21%->40%)
	
	//int client_index = *((int *) index);
	
	//find that chunk
	//connect to client who has that chunk
	//download it
	//update tracker <updatetracker picture-wallpaper.jpg start end localhost seed_port>
	//repeat
	
	size_t len = 0;
	int port;
	char *line, *temp;
	/* Open tracker file. */
	FILE *tracker;
	if ((tracker = fopen("picture-wallpaper.jpg.track", "r")) != NULL)
	{
		/* Read past of the tracker file information. We are currently interested in chunk info. */
		do
		{
			getline(&line, &len, tracker);
		} while (strncmp(line, "MD5:", 4) != 0);
		
		/* Get the port number of a peer sharing a chunk. */
		getline(&line, &len, tracker);
		strtok(line, ":");
		temp = strtok(NULL, ":");
		port = atoi(temp);
		
		/* Connect to the peer at that port. */
		struct sockaddr_in server_addr = {AF_INET, htons(port)}; 
		struct hostent *hp; 
		hp = gethostbyname("localhost");
		bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length );
		
		int new_sock = socket( AF_INET, SOCK_STREAM, 0 );
		connect(new_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)); /*Connect. */

		/* Send the serving peer our download request. */
		char buf[CHUNK_SIZE];
		/* If we had more time, we would have calculated the actual start and end byte of the chunk we want using the index argument. */
		strcpy(buf, "<download picture-wallpaper.jpg.track 0 35738>");
		
		/* For demo purposes, we are showing that we have successfully sent another peer some data. */
		/* We wanted to sure that we successfully implemented peer-peer communication. */
		int sent = write(new_sock, buf, sizeof(buf));
		printf("%d\n", sent);
		
		free(line);
		fclose(tracker);
	}
	else
	{
		printf("Could not open tracker file.");
	}
}

void *client_handler(void * index)
{
	/* Dereference the index passed as a parameter by the pthread_create() function */
	//int client_index = *((int *) index);
	
	struct sockaddr_in server_addr = { AF_INET, htons( seed_port ) };
	struct sockaddr *client_addr = NULL;
	socklen_t *length = NULL;

	/* Enable this client to accept connections from other peers. */
	
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
	
	/* Bind the socket to an internet port. */
	if (bind(seed_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror("Server Error: Bind Failed");
		exit(1);
	}
	
	/* We will listen for multiple peers. */
	if (listen(seed_sock, MAX_CLIENT) == -1)
	{
		perror("Server Error: Listen failed");
		exit(1);
	}
	
	/* The plan was to continuously listen for connections.
	 * Peers would send a <download> command indicating which chunk they desired.
	 * We would then send the chunk, disconnect from that peer, and then wait for annother connection. */
	if ((peers[0].m_peer_socket = accept(seed_sock, client_addr, length)) == -1)
	{
		perror("Server Error: Accepting issue"); 
		exit(1);
	}
	else
	{
		/* Used to show that we had successfully implemented peer-peer communication. */
		printf("A downloading client has connected.\n");

		/* read download command
		sent = read(seed_sock, buf, sizeof(buf));
		*/
		
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
				/** The fourth line contains the default amount of time (in seconds) the client should wait in between updating the server with new tracker info. */
				case 3:
					server_update_frequency = atoi(line);
					break;
			}
			lineCount++;
		}
		fclose(configFile);
	}
	/** If a config file could not be opened, default values will be assigned:
	 * server_port = 3456, 
	 * max_client = 5,
	 * chunk_size = 1024 bytes, and 
	 * server_update_frequency = 900 seconds (15 minutes)
	 */
	else
	{
		server_port = 3456;
 		max_client = 5;
 		chunk_size = 1024;
		server_update_frequency = 900;
	}
	
	return;
}

/*
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
