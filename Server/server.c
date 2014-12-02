/**
 * @file server.c
 * @authors Matthew Lindner, Xiao Deng
 *
 * @brief Multi-threaded server that maintains and distributes tracker files.
 * @details Centralized, multi-threaded server whose purpose is to distribute tracker files to clients who what to share files on a P2P network.
 *
 * The server can process four types of requests:
 * 	-# createtracker
 * 	-# updatetracker
 * 	-# LIST
 * 	-# GET
 *
 * Once the server processes a single request from a client, it closes the connection to that client.
 * Each client is handled in its own thread. This allows the server to handle multiple clients at a single time.
 *
 * @section COMPILE
 * gcc server.c -o server.out -lnsl -pthread -lcrypto
 */

#include "config.ini"
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
#include <dirent.h>
#include <unistd.h>
#include "compute_md5.h"

/**
 * Socket variable for hosting the server. Server listens for connections.
 */
int sock;

/**
 * Represents a peer (client) application.
 * Each peer is handled with its own thread.
 */
struct peer
{
	int m_peer_socket; ///< Communication socket for this peer. If value = \b -1, this peer is currently not being used.
	int m_index; ///< Location of peer in the \a clients array.
	char m_buf[CHUNK_SIZE];  ///< Character array used to copy data into/out of \a m_peer_socket
	FILE *m_file; ///< File pointer used to open tracker files.
	pthread_t m_thread; ///< Thread where commands from peer will be processed.
};

/**
 * Array of clients.
 * \a MAX_CLIENT is the maximum number of clients that server can serve at a single time, and the maximum number of threads that can be executed at once.
 */
struct peer clients[MAX_CLIENT];

/**
 * Mutex used to prevent a file from being operated on by multiple threads at the same time.
 * Prevents the possibility of race-condition and dead-lock.
 */
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * When a client connects to the server, it's thread of execution is handled by this function.
 * A command (createtracker, updatetracker, LIST, GET) will be read from the client's socket, and processed.
 * @param index Index in the \a clients array where the new client is stored.
 */
void *client_handler(void * index);
/**
 * Initializes each peer in the \a clients array.
 * 
 * Each \a m_index is set to \b i, the index of the array (0 - \a MAX_CLIENT).
 * Each \a m_peer_socket is set to \b -1 to indicate that this peer is currently not being used.
 */
void setUpClientArray();
/**
 * Returns an index indicating where a new client can be stored in the \a clients array. Checks each element of the array sequentially until an unused slot is located.
 * @return Index (0 - \a MAX_CLIENT -1) of the \a clients array where new client will be stored, -1 if array is full (server is currently serving the maximum amount of clients).
 */
int findClientArrayOpening();

/**
 * When server.out is executed, the server will listen for peers on a stream socket.
 * When a peer connects, if the server is not already at capacity (\a MAX_CLIENT), a thread will be spun off to handle the peer.
 * Multiple peers can be handled at once.
 * The server will then read a command from that peer's socket, and process that command.
 * Once that single command has been processed, the server disconnect from the peer.
 */
int main()
{
	struct sockaddr_in server_addr = {AF_INET, htons( SERVER_PORT )};
	struct sockaddr_in client_addr = {AF_INET};
	/* The length of socketaddr structure */
	int length = sizeof(client_addr);
	
	/**
	 * Create a stream socket. Server will listen on this socket for peers.
	 */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Server Error: Socket Failed");
		exit(1);
	}
	
	/* Variable needed for setsokopt call */
	int setsock = 1;
	
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(setsock)) == -1)
	{
		perror("Server Error: Setsockopt failed");
		exit(1);
	}
	
	/** Bind the socket to an internet port. */
	if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror("Server Error: Bind Failed");
		exit(1);
	}
	
	/** Listen for clients. \a MAX_CLIENT = length of queue for pending connections.  */
	if (listen(sock, MAX_CLIENT) == -1)
	{
		perror("Server Error: Listen failed");
		exit(1);
	}
	
	/** Initialize the client array so each element is marked as unused. */
	setUpClientArray();
	
	/** While running, the server will allow up to a predefined number of peers connect at once. 
	 * When a peer tries to connect, if the server is not at capacity, the peer will be placed in the \a clients array, and a thread will be spun off
	 * to process that peer. If the server is already at full capacity, the peer will have to wait for an opening.
	 */
	while (1)
	{
		int client_array_index;
		/* The server searches for an opening in the client array. If the array is full, -1 is returned */
		if ((client_array_index = findClientArrayOpening()) != -1)
		{
			/* Accept the client, and store it's socket ID in the clients array. */
			if ((clients[client_array_index].m_peer_socket = accept(sock, (struct sockaddr*)&client_addr, &length)) == -1)
			{
				perror("Server Error: Accepting issue"); 
				exit(1);
			}
			else
			{
				/* Spin off a thread to process the peer. The peer's index in the clients array is passed as a 
				 * parameter to indicate which peer is being processed. */
				if (pthread_create(&(clients[client_array_index].m_thread), NULL, &client_handler, &(clients[client_array_index].m_index)) != 0)
				{
					printf("Error Creating Thread\n");
					exit(1);
				}
			}
		}
	}
	
	return 0;
}
	
void *client_handler(void * index)
{
	/* Dereference the index passed as a parameter by the pthread_create() function */
	int client_index = *((int *) index);
	
	/* Clear the buffer of any data used by a previous peer in the same index of the clients array. */
	memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
	
	/**
	 * Read data from the peer's socket, store it in m_buf.
	 * Compare the string stored in m_buf to see if it is any of the four commands.
	 * And then serve the peer.
	 */
	if (read(clients[client_index].m_peer_socket , clients[client_index].m_buf, CHUNK_SIZE) > 0)
	{
		/** <b>CREATETRACKER Command</b> */
		if (strncmp(clients[client_index].m_buf, "<createtracker", strlen("<createtracker")) == 0)
		{
			/* First check if the client send the correct number of arguments */
			int num_arg = 0;
			char countArgs[CHUNK_SIZE];
			char *numArgCheck;
			
			stpcpy(countArgs, clients[client_index].m_buf);
			/* Each argument/param is seperated with a space. */
			numArgCheck = strtok(countArgs, " ");
			while (numArgCheck != NULL)
			{
				/* For each string of text separated by a space, or delimited with a new line, 
				 * increment num_arg */
				num_arg = num_arg + 1;
				numArgCheck = strtok(NULL, " \n");
			}
			/** If client did not send the correct number of arguments, send a "createtracker fail" protocol message. */
			if (num_arg != 7)
			{
				write(clients[client_index].m_peer_socket, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
			}
			else
			{
				/** The create tracker command is broken up into 7 words:
				 * createtracker, filename, filesize, description, md5, ip, and port.
				 * We are interested in the last 6.
				 */
				char *filename, *filesize, *description, *md5, *ip, *port;
				char tokenize[CHUNK_SIZE];
				
				/* We will copy the buffer into a new array, and parse. */
				stpcpy(tokenize, clients[client_index].m_buf);
				
				/* Get the filename */
				filename = strtok(tokenize, " "); /* Skip over the "createtracker" string. */
				filename = strtok(NULL, " ");
				strcpy(filename, filename);
				
				/* Get filesize */
				filesize= strtok(NULL, " ");
				strcpy(filesize, filesize);
				
				/* Get description */
				description = strtok(NULL, " ");
				strcpy(description, description);
				
				/* Get md5 */
				md5 = strtok(NULL, " ");
				strcpy(md5, md5);
				
				/* Get IP */
				ip = strtok(NULL, " ");
				strcpy(ip, ip);
				
				/* Get port */
				port = strtok(NULL, ">");
				strcpy(port, port);
				
				
				/* We now need to check to see if this tracker file already exists. */
				memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
				sprintf(clients[client_index].m_buf, "Tracker Files/%s.track", filename);
				
				/* Ensure that only 1 thread is checking the directory at a time */
				pthread_mutex_lock(&file_mutex);
				
				/** If this tracker file already exists, send a "createtracker ferr" protocol message. */
				if (access(clients[client_index].m_buf, F_OK) == 0)
				{
					write(clients[client_index].m_peer_socket, "<createtracker ferr>\n", strlen("<createtracker ferr>\n"));
				}
				/** Otherwise, since this tracker file does not exist, create the tracker file. 
				 * Start by creating an empty file.
				 */
				else if((clients[client_index].m_file = fopen(clients[client_index].m_buf, "w")) != NULL)
				{
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					
					/* Copy the tracker file contents into the buffer. */
					sprintf(clients[client_index].m_buf, "Filename: %s\nFilesize: %s\nDescription: %s\nMD5: %s\n%s:%s:0:%s:%d", filename, filesize, description, md5, ip, port, filesize,  (unsigned)time(NULL));
					/* Write the buffer contents to the new tracker file. */
					fwrite(clients[client_index].m_buf, sizeof(char), strlen(clients[client_index].m_buf), clients[client_index].m_file);
					/** Let the client know that the creation was successful with a "createtracker succ" protocol message. */
					write(clients[client_index].m_peer_socket, "<createtracker succ>\n", strlen("<createtracker succ>\n"));
					
					/* Close the tracker file. */
					fclose(clients[client_index].m_file);
				}
				/** If there was a problem creating the file, send the user a "createtracker fail" protocol message. */
				else
				{
					write(clients[client_index].m_peer_socket, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
				}
				/* Unlock the mutex. */
				pthread_mutex_unlock(&file_mutex);
			}
		}
		/** <b>UPDATETRACKER Command</b> */
		else if (strncmp(clients[client_index].m_buf, "<updatetracker", strlen("<updatetracker")) == 0)
		{
			/** The update tracker command is broken up into 6 words:
			 * updatetracker, filename, start byte, end byte, ip, and port.
			 * We are interested in the last 5.
			 */
			char *filename, *start, *end, *ip, *port;
			char tokenize[CHUNK_SIZE];
			
			strcpy(tokenize, clients[client_index].m_buf);
			
			/* Get the filename */
			filename = strtok(tokenize, " ");
			filename = strtok(NULL, " ");
			strcpy(filename, filename);
			
			/* Get the start byte */
			start = strtok(NULL, " ");
			strcpy(start, start);
			
			/* Get the end byte */
			end = strtok(NULL, " ");
			strcpy(end, end);
			
			/* Get IP */
			ip = strtok(NULL, " ");
			strcpy(ip, ip);
			
			/* Get port */
			port = strtok(NULL, ">");
			strcpy(port, port);
			
			/* We now need to check to see if this tracker file already exists. */
			memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
			sprintf(clients[client_index].m_buf, "Tracker Files/%s.track", filename);
			
			pthread_mutex_lock(&file_mutex);
			
			/** If this tracker file does not exist, send a "createtracker ferr" protocol message. */
			if (access(clients[client_index].m_buf, F_OK) == -1)
			{
				write(clients[client_index].m_peer_socket, "<updatetracker ferr>\n", strlen("<updatetracke ferr>\n"));
			}
			/** Otherwise, append the new chunk data to the end of the tracker file. */
			else
			{
				if((clients[client_index].m_file = fopen(clients[client_index].m_buf, "a")) != NULL)
				{
					/* Copy contents into buffer. */
					sprintf(clients[client_index].m_buf, "\n%s:%s:%s:%s:%d", ip, port, start, end,  (unsigned)time(NULL));
					/* Append the buffer contents to the end of the tracker file. */
					fwrite(clients[client_index].m_buf, sizeof(char), strlen(clients[client_index].m_buf), clients[client_index].m_file);
					/** Let the client know that the update was successful with a "updatetracker succ" protocol message. */
					write(clients[client_index].m_peer_socket, "<updatetracker succ>", strlen("<updatetracker succ>"));
					
					/* Close the tracker file. */
					fclose(clients[client_index].m_file);
				}
				/** If there was a problem updating the file, send the user a "updatetracker fail" protocol message. */
				else
				{
					write(clients[client_index].m_peer_socket, "<updatetracker fail>\n", strlen("<updatetracke fail>\n"));
				}
			}
			/* Unlock the mutex. */
			pthread_mutex_unlock(&file_mutex);
		}
		/** <b>REQ LIST Command</b> */
		else if (strncmp(clients[client_index].m_buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0)
		{
			DIR *tracker_directory;
			struct dirent *individual_file;
			
			pthread_mutex_lock(&file_mutex);
			/** First, opens the "Tracker Files" folder, and counts the number of files. */
			if ((tracker_directory = opendir("Tracker Files")) != NULL)
			{
				int num_files = 0;
				while ((individual_file = readdir(tracker_directory)) != NULL)
				{
					/*readdir returns root directories "." and ".."*/
					/*We need to ignore them*/
					if ((strncmp(individual_file->d_name, ".", 1) != 0) && (strncmp(individual_file->d_name, "..", 2) != 0))
					{
						num_files = num_files + 1;
					}
				}
				if (closedir(tracker_directory) == -1)
				{
					perror("Closing dir error.\n");
				}
				
				/* Send the first line of the LIST response. */
				sprintf(clients[client_index].m_buf, "<REP LIST %d>\n", num_files);
				write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
			}
			
			/** For each file in the "Tracker Files" folder, stores the tracker file's: Filename, filesize, and md5. */
			if ((tracker_directory = opendir("Tracker Files")) != NULL)
			{
				int num_files = 0;
				while ((individual_file = readdir(tracker_directory)) != NULL)
				{
					/*readdir returns root directories "." and ".."*/
					/*We need to ignore them*/
					if ((strncmp(individual_file->d_name, ".", 1) != 0) && (strncmp(individual_file->d_name, "..", 2) != 0))
					{
						num_files = num_files + 1;
					
						memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
						char *line = NULL;
						char *filename, *filesize, *md5;
						size_t len = 0;
						
						sprintf(clients[client_index].m_buf, "Tracker Files/%s", individual_file->d_name);
						
						if((clients[client_index].m_file = fopen(clients[client_index].m_buf, "r")) != NULL)
						{
							memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
							
							sprintf(clients[client_index].m_buf, "<%d",num_files);
						
							/* Get the filename */
							getline(&line, &len, clients[client_index].m_file);
							filename = strtok(line, ": ");
							filename = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, filename);
							
							/* Get the filesize */
							getline(&line, &len, clients[client_index].m_file);
							filesize = strtok(line, ": ");
							filesize = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, filesize);
							
							/* Skip over the file description line */
							getline(&line, &len, clients[client_index].m_file);
							
							/* Get the md5 */
							getline(&line, &len, clients[client_index].m_file);
							md5 = strtok(line, ": ");
							md5 = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, md5);
							strcat(clients[client_index].m_buf, ">\n");
							
							fclose(clients[client_index].m_file);
							free(line);
							
							/** Sends each tracker file info, indexed by a number. */
							write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
						}
					}
				}
				memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
				strcpy(clients[client_index].m_buf, "<REP LIST END>\n");
				/* Send the footer of the "REQ" protocol message */
				write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
				
				if (closedir(tracker_directory) == -1)
				{
					perror("Closing dir error.\n");
				}
			}
			pthread_mutex_unlock(&file_mutex);
		}
		/** <b>GET Command</b> */
		else if (strncmp(clients[client_index].m_buf, "<GET", strlen("<GET")) == 0)
		{
			char *tracker_filename, *filename;
			char parseFileName[CHUNK_SIZE];
			
			/* Get the filename.track */
			stpcpy(parseFileName, clients[client_index].m_buf);
			tracker_filename = strtok(parseFileName, " ");
			tracker_filename = strtok(NULL, ">");
			
			sprintf(clients[client_index].m_buf, "Tracker Files/%s", tracker_filename);
			sprintf(tracker_filename, "%s", clients[client_index].m_buf);
			
			pthread_mutex_lock(&file_mutex);
			/** First, the server checks to make sure that the requested tracker file already exists. */
			if (access(clients[client_index].m_buf, F_OK) == 0) /* If that file exists..... */
			{
				/** Next, it opens the file, and copies it's contents. */
				if((clients[client_index].m_file = fopen(tracker_filename, "r")) != NULL)
				{
					write(clients[client_index].m_peer_socket, "<REP GET BEGIN>\n", strlen("<REP GET BEGIN>\n"));
					
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					int read;
					/** It then sends the peer the tracker file, copied into a buffer. */
					while((read = fread(clients[client_index].m_buf, sizeof(char), CHUNK_SIZE, clients[client_index].m_file)) > 0)
					{
						write(clients[client_index].m_peer_socket, clients[client_index].m_buf, read);
						memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf)); 
					}
					
					/**Finally, it includes the md5 sum of the tracker file itself, and appends it to the end of the "GET" protocol footer. */
					char * md5_string;
					md5_string = computeMD5(tracker_filename);
					
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					sprintf(clients[client_index].m_buf, "\n<REP GET END %s>\n", md5_string);
					free(md5_string);
					
					write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
				}
				/** If the tracker file could not be opened, the server sends the peer a "GET invalid" protocol error message. */
				else
				{
					write(clients[client_index].m_peer_socket, "<GET invalid>", strlen("<GET invalid>"));
				}
			}
			/** If the file does not exist, the server sends the peer a "GET invalid" protocol error message. */
			else
			{
				write(clients[client_index].m_peer_socket, "<GET invalid>", strlen("<GET invalid>"));
			}
			pthread_mutex_unlock(&file_mutex);
		}
	}
	
	/** <b> Closing the connection to the peer.</b> */
	/** 
	 * Once the peer's request has been handled, the server closes that socket, 
	 * indicates that there is a new opening in the \a client array, 
	 * and ends the thread. */
	if (close(clients[client_index].m_peer_socket) != 0)
	{
		perror("Closing socket issue");
	}
	/* Indicate that this element of the clients array is free to use. */
	clients[client_index].m_peer_socket = -1;
	
	/* End the thread.
	 * In this context, thread clean-up behaviour using pthread_cancel() and pthread_exit() are the same. */
	if (pthread_cancel(clients[client_index].m_thread) != 0)
	{
		perror("Ending thread issue");
		exit(1);
	}
	
	return;
}

void setUpClientArray()
{
	int index;
	for (index = 0; index < MAX_CLIENT; index++)
	{
		/* Set each m_index = to it's index in the array.  */
		clients[index].m_index = index;
		/* A m_socet value of 01 indicates that this element of the array is not being used. */
		clients[index].m_peer_socket = -1;
	}
	return;
}

int findClientArrayOpening()
{
	int index;
	/* Check each element of the clients array, until we find an opening. */
	for (index = 0; index < MAX_CLIENT; index++)
	{
		/* Once an opening is found, immediately leave the function. */
		if (clients[index].m_peer_socket == -1)
		{
			return index;
		}
	}
	return -1;
}