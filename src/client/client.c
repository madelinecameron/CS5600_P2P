/**
 * @file client.c
 * @authors Matthew Lindner, Xiao Deng, Madeline Cameron
 *
 * @brief Multi-threaded client that updates server with tracker file and downloads chunks from other clients
 * @details Multi-threaded client that uses separate threads to seed and download chunks to / from other clients.
 *
 * @section COMPILE
 * gcc client.c -o client -lnsl -pthread
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

//socket used to connect to server
int server_sock;
//socket used to act as "server"
int seed_sock;
//pthread_t main_seed_thread;
int SERVER_PORT, SEED_PORT, MAX_NUM_CLIENT, CHUNK_SIZE;

#include "compute_md5.h" //This relies on CHUNK_SIZE being defined above it. So that's why it's all the way down here.

typedef struct
{
	int m_peer_socket; ///< Communication socket for this peer. If value = \b -1, this peer is currently not being used.
	int m_index; ///< Location of peer in the \a peers array.
	char m_buf[1024];  ///< Character array used to copy data into/out of \a m_peer_socket.
                        //Cannot be variably declared
	FILE *m_file; ///< File pointer used to open tracker files.
	pthread_t m_thread; ///< Thread where commands from peer will be processed.
} peer;

peer* peers;

void setUpPeerArray();
int findPeerArrayOpening();

void *download(void * index);
void *client_handler(void * index);
void *seed(void * param);

int findCommand();
void findIP();
void readConfig();

#define SEED 0
#define DOWNLOAD 1
int mode;
int client_i;

//Buffer used to send/receive. Data is loaded into buffer (see read() and fwrite()).
char buf[1024]; //Cannot be variably declared
char* ip;

int main(int argc, const char* argv[])
{
	printf("Reading config file...\n");
	readConfig();
    
    peers = malloc(sizeof(peer) * MAX_NUM_CLIENT);
    if(!peers) //Mem allocation failed!
    {
        perror("Memory allocation for peers array failed!");
    }
    memset(peers, 0, sizeof(peer) * MAX_NUM_CLIENT);

    //Specifies address: Where we are connecting our socket.
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	struct hostent *hp;

	if((hp = gethostbyname("localhost")) == NULL)
	{
		printf("Error: Unknown Host");
		exit(1);
	}

	bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length );

	if (argc < 4)
    {
        exit(0);
    }
    else
    {
        mode = atoi(argv[1]);
        port = atoi(argv[2]);
        client_i = atoi(argv[3]);
    }
    
    setUpPeerArray();
    
    if (mode == SEED)
    {
        /* We should pass a parameter in when executeing ./client.out to signify which port we'll be using */
        struct sockaddr_in server_addr = {AF_INET, htons( SERVER_PORT +1 )}; //Here
        struct sockaddr_in client_addr = {AF_INET};
        int length = sizeof(client_addr);
        
        if ((seed_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("Server Error: Socket Failed");
            exit(1);
        }
        
        /* Variable needed for setsokopt call */
        int setsock = 1;
        if(setsockopt(seed_sock, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(setsock)) == -1)
        {
            perror("Server Error: Setsockopt failed");
            exit(1);
        }
        
        fcntl(seed_sock, F_SETFL, O_NONBLOCK);
        
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
                
        clock_t cur, start = clock();
        int cur_chunk = ((client_i - 1)*20) + 1;
        int count_to_4 = 0;
        
        if (client_i == 1)
        {
            cur_chunk--;
        }
        while (1)
        {
            int peer_array_index;
            
            /* The server searches for an opening in the client array. If the array is full, -1 is returned */
            if ((peer_array_index = findPeerArrayOpening()) != -1)
            {
                /* Accept the client, and store it's socket ID in the clients array. */
                if ((peers[peer_array_index].m_peer_socket = accept(seed_sock, (struct sockaddr*)&client_addr, &length)) == -1)
                {
                    //perror("Server Error: Accepting issue"); 
                    //exit(1);
                }
                else
                {
                    printf("Peer has connected.\n");
                    /* Spin off a thread to process the peer. The peer's index in the clients array is passed as a 
                     * parameter to indicate which peer is being processed. */
                    if (pthread_create(&(peers[peer_array_index].m_thread), NULL, &client_handler, &(peers[peer_array_index].m_index)) != 0)
                    {
                        printf("Error Creating Thread\n");
                        exit(1);
                    }
                }
            }
            cur = clock() - start;
            if ((((float)cur/CLOCKS_PER_SEC) >= 10) && count_to_4 < 4)// if it's been 10 seconds
            {
                int increment;
                if (cur_chunk == 0)
                {
                    increment = 5;
                }
                else
                {
                    increment = 4;
                }
                
                printf("I am client_%d, and I am advertising the following chunk of the file: %d%% to %d%%.\n", client_i, cur_chunk, cur_chunk + increment);
                
                if(cur_chunk == 0)
                {
                    cur_chunk++;
                }
                cur_chunk += 5;
                count_to_4++;
                
                start = clock();
            }
        }
    
        if (close(seed_sock) != 0)
        {
            perror("Closing socket issue");
        }
    }
 
    char buf[CHUNK_SIZE];
    int sent;
    if (mode == DOWNLOAD)
    {
        while (1)
        {
            fgets(buf, sizeof(buf), stdin);

            if (strncmp(buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0)
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
                    printf("%s", buf);
                    memset(buf, '\0', sizeof(buf));
                }
            }
            else if (strncmp(buf, "<createtracker", strlen("<createtracker")) == 0)
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
            else if (strncmp(buf, "<updatetracker", strlen("<updatetracker")) == 0)
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
            else if (strncmp(buf, "<GET", strlen("<GET")) == 0)
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
        }
    }

	close(server_sock); //close the socket

	return (0);
}

void *download(void * index)
{
    //index in array indicates what sub-chunk they want?
    // i = 0 -> 1st sub-chunk.
    // i = 1 -> second sub-chunk
    int client_index = *((int *) index);
    
    printf("%d\n", client_index);
    
    //mutex lock
    //get first part of your sub-chunk
    //get second part
    //get third part
    //get fourth part
    //get fifth part
    //mutex unlock
    
    return;
}

void *client_handler(void * index)
{
    struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
    struct hostent *hp;
 
    char buf[CHUNK_SIZE];
    /* Dereference the index passed as a parameter by the pthread_create() function */
    int client_index = *((int *) index);
    
    //create tracker
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
    
    //create tracker
    //get md5
    memset(buf, '\0', sizeof(buf));
    char *md5 = computeMD5("picture-wallpaper.jpg");
    sprintf(buf, "<createtracker picture 35738 img %s localhost %d", md5, port); 
    write(server_sock, buf , strlen(buf));
    free(md5);

	/******************************
	 ******************************
	 *             SHARE FILE HERE           *
	 *   PEER SHOULD SND MESSAGE    *
	 *   SAYING WHAT FILE IT WANTS   *  //Is this how we should do this?
	 ******************************
	 ******************************/

	 //UPDATE TRACKER EVERY 10 SECONDS


	/*When we're done sharing*/
	if (close(peers[client_index].m_peer_socket) != 0)
	{
		perror("Closing socket issue");
	}
	if (pthread_cancel(peers[client_index].m_thread) != 0)
	{
		perror("Ending thread issue");
		exit(1);
	}

	return;
}

void setUpPeerArray()
{
	int index;
	for (index = 0; index < MAX_NUM_CLIENT; index++)
	{
		/* Set each m_index = to it's index in the array.  */
		peers[index].m_index = index;
		/* A m_socket value of -1 indicates that this element of the array is not being used. */
		peers[index].m_peer_socket = -1;
	}
	return;
}

int findPeerArrayOpening()
{
	int index;
	/* Check each element of the peers array, until we find an opening. */
	for (index = 0; index < MAX_NUM_CLIENT; index++)
	{
		/* Once an opening is found, immediately leave the function. */
		if (peers[index].m_peer_socket == -1)
		{
			return index;
		}
	}
	return -1;
}


int findCommand()
{
    int i;
	char* isThisYourCmd[15];
	printf("1\n");
	char* createTracker = "<createtracker";
	printf("2\n");
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

void readConfig()
{
	char* line;
	size_t length = 0;
	ssize_t read;
	FILE* configFile = fopen("client.conf", "r");

	if(configFile == NULL)
	{
		printf("Client config file doesn't exist! Reverting to default options!\n");
		SERVER_PORT = 63122;
		SEED_PORT = 63123;
		MAX_NUM_CLIENT = 10;
		CHUNK_SIZE = 512;
		return;
	}

    int lineCount = 0;
	while((read = getline(&line, &length, configFile)) != -1)
	{
        
        switch(lineCount) 
        {
            case 0:
                SERVER_PORT = atoi(line);
                break;
            case 1:
                SEED_PORT = atoi(line);
                break;
            case 2:
                MAX_NUM_CLIENT = atoi(line);
                break;
            case 3:
                CHUNK_SIZE = atoi(line);
                break;
        }
        lineCount++;
	}

	fclose(configFile);
}
