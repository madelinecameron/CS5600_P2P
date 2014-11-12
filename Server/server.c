/* gcc server.c -o server -lnsl -pthread -lcrypto */
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
#include <dirent.h> //for getting the number of files in a directory
#include <unistd.h> //for checking to see if a file exists or not
#include "compute_md5.h"

//socket for hosting the server
int sock;

struct peer
{
	int m_peer_socket;
	int m_index;
	char m_buf[CHUNK_SIZE]; 
	FILE *m_file;
	pthread_t m_thread;
};

struct peer clients[MAX_CLIENT];

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void * index);
void setUpClientArray();
int findClientArrayOpening();

int main()
{
	struct sockaddr_in server_addr = {AF_INET, htons( SERVER_PORT )};
	struct sockaddr_in client_addr = {AF_INET};
	int length = sizeof(client_addr); //length of socketaddr structure
	
	/* Create a stream socket. Server will listen on this socket for peers. */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Server Error: Socket Failed");
		exit(1);
	}
	
	//variable needed for setsokopt call
	int setsock = 1;
	
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(setsock)) == -1)
	{
		perror("Server Error: Setsockopt failed");
		exit(1);
	}
	
	/* bind the socket to an internet port */
	if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
	{
		perror("Server Error: Bind Failed");
		exit(1);
	}
	
	// listen for clients
	if (listen(sock, MAX_CLIENT) == -1) //MAX_CLIENT = length of queue for pending connections. 
	{
		perror("Server Error: Listen failed");
		exit(1);
	}
	
	setUpClientArray();
	
	while (1)
	{
		int client_array_index;
		if ((client_array_index = findClientArrayOpening()) != -1)
		{
			if ((clients[client_array_index].m_peer_socket = accept(sock, (struct sockaddr*)&client_addr, &length)) == -1)
			{
				perror("Server Error: Accepting issue"); 
				exit(1);
			}
			else
			{
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
	int client_index = *((int *) index);
	
	//clear the buffer
	memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
	
	if (read(clients[client_index].m_peer_socket , clients[client_index].m_buf, CHUNK_SIZE) > 0)
	{
		if (strncmp(clients[client_index].m_buf, "<createtracker", strlen("<createtracker")) == 0)
		{
			//first check for the correct number of arguments
			int num_arg = 0;
			char countArgs[CHUNK_SIZE];
			char *numArgCheck;
			stpcpy(countArgs, clients[client_index].m_buf);
			numArgCheck = strtok(countArgs, " ");
			while (numArgCheck != NULL)
			{
				num_arg = num_arg + 1;
				numArgCheck = strtok(NULL, " \n");
			}
			//If client did not send the correct number of arguments
			if (num_arg != 7)
			{
				write(clients[client_index].m_peer_socket, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
			}
			else
			{
				//<createtracker filename filesize description md5 ip port>
				char *filename, *filesize, *description, *md5, *ip, *port;
				char tokenize[CHUNK_SIZE];
				
				stpcpy(tokenize, clients[client_index].m_buf);
				
				//get the filename
				filename = strtok(tokenize, " ");
				filename = strtok(NULL, " ");
				strcpy(filename, filename);
				
				//get filesize
				filesize= strtok(NULL, " ");
				strcpy(filesize, filesize);
				
				//get description
				description = strtok(NULL, " ");
				strcpy(description, description);
				
				//get md5
				md5 = strtok(NULL, " ");
				strcpy(md5, md5);
				
				//get IP
				ip = strtok(NULL, " ");
				strcpy(ip, ip);
				
				//get port
				port = strtok(NULL, ">");
				strcpy(port, port);
				
				//Check to see if tracker file already exists
				memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
				sprintf(clients[client_index].m_buf, "Tracker Files/%s.track", filename);
				
				pthread_mutex_lock(&file_mutex);
				//If it already exists
				if (access(clients[client_index].m_buf, F_OK) == 0)
				{
					write(clients[client_index].m_peer_socket, "<createtracker ferr>\n", strlen("<createtracker ferr>\n"));
				}
				//If it doesn't already exists, create the tracker file
				else if((clients[client_index].m_file = fopen(clients[client_index].m_buf, "w")) != NULL)
				{
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					
					/* 
					 * We could potentially overflow the buffer here..... should this error checking be handled in client or server?
					 */
					sprintf(clients[client_index].m_buf, "Filename: %s\nFilesize: %s\nDescription: %s\nMD5: %s\n%s:%s:0:%s:%d", filename, filesize, description, md5, ip, port, filesize,  (unsigned)time(NULL));
					fwrite(clients[client_index].m_buf, sizeof(char), strlen(clients[client_index].m_buf), clients[client_index].m_file);
					write(clients[client_index].m_peer_socket, "<createtracker succ>\n", strlen("<createtracker succ>\n"));
					
					fclose(clients[client_index].m_file);
				}
				//if there was a problem creating the file...
				else
				{
					write(clients[client_index].m_peer_socket, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
				}
				pthread_mutex_unlock(&file_mutex);
			}
		}
		else if (strncmp(clients[client_index].m_buf, "<updatetracker", strlen("<updatetracker")) == 0)
		{
			char *filename, *start, *end, *ip, *port;
			char tokenize[CHUNK_SIZE];
			
			strcpy(tokenize, clients[client_index].m_buf);
			
			//get the filename
			filename = strtok(tokenize, " ");
			filename = strtok(NULL, " ");
			strcpy(filename, filename);
			
			//get start
			start = strtok(NULL, " ");
			strcpy(start, start);
			
			//get end
			end = strtok(NULL, " ");
			strcpy(end, end);
			
			//get IP
			ip = strtok(NULL, " ");
			strcpy(ip, ip);
			
			//get port
			port = strtok(NULL, ">");
			strcpy(port, port);
			
			//Check to see if tracker file already exists
			memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
			sprintf(clients[client_index].m_buf, "Tracker Files/%s.track", filename);
			
			pthread_mutex_lock(&file_mutex);
			//If it does not exist
			if (access(clients[client_index].m_buf, F_OK) == -1)
			{
				write(clients[client_index].m_peer_socket, "<updatetracker ferr>\n", strlen("<updatetracke ferr>\n"));
			}
			else
			{
				if((clients[client_index].m_file = fopen(clients[client_index].m_buf, "a")) != NULL)
				{
					sprintf(clients[client_index].m_buf, "\n%s:%s:%s:%s:%d", ip, port, start, end,  (unsigned)time(NULL));
					fwrite(clients[client_index].m_buf, sizeof(char), strlen(clients[client_index].m_buf), clients[client_index].m_file);
					write(clients[client_index].m_peer_socket, "<updatetracker succ>", strlen("<updatetracker succ>"));
					
					fclose(clients[client_index].m_file);
				}
				else
				{
					write(clients[client_index].m_peer_socket, "<updatetracker fail>\n", strlen("<updatetracke fail>\n"));
				}
			}
			pthread_mutex_unlock(&file_mutex);
		}
		else if (strncmp(clients[client_index].m_buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0)
		{
			DIR *tracker_directory;
			struct dirent *individual_file;
			
			pthread_mutex_lock(&file_mutex);
			//Opens the "Tracker Files" folder, and counts the number of files
			if ((tracker_directory = opendir("Tracker Files")) != NULL)
			{
				int num_files = 0;
				while ((individual_file = readdir(tracker_directory)) != NULL)
				{
					//readdir returns root directories "." and ".."
					//We need to ignore them
					if ((strncmp(individual_file->d_name, ".", 1) != 0) && (strncmp(individual_file->d_name, "..", 2) != 0))
					{
						num_files = num_files + 1;
					}
				}
				if (closedir(tracker_directory) == -1)
				{
					//what should we do if it wasn't closed successfully?
				}
				
				//Send the first line of the LIST response.
				sprintf(clients[client_index].m_buf, "<REP LIST %d>\n", num_files);
				write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
			}
			
			if ((tracker_directory = opendir("Tracker Files")) != NULL)
			{
				int num_files = 0;
				while ((individual_file = readdir(tracker_directory)) != NULL)
				{
					//readdir returns root directories "." and ".."
					//We need to ignore them
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
						
							//get the filename
							getline(&line, &len, clients[client_index].m_file);
							filename = strtok(line, ": ");
							filename = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, filename);
							
							//get the filesize
							getline(&line, &len, clients[client_index].m_file);
							filesize = strtok(line, ": ");
							filesize = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, filesize);
							
							//skip over the file description line
							getline(&line, &len, clients[client_index].m_file);
							
							//get the md5
							getline(&line, &len, clients[client_index].m_file);
							md5 = strtok(line, ": ");
							md5 = strtok(NULL, "\n");
							strcat(clients[client_index].m_buf, md5);
							strcat(clients[client_index].m_buf, ">\n");
							
							fclose(clients[client_index].m_file);
							free(line);
							
							write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
						}
					}
				}
				memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
				strcpy(clients[client_index].m_buf, "<REP LIST END>\n");
				write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
				
				if (closedir(tracker_directory) == -1)
				{
					//what should we do if it wasn't closed successfully?
				}
			}
			pthread_mutex_unlock(&file_mutex);
		}
		else if (strncmp(clients[client_index].m_buf, "<GET", strlen("<GET")) == 0)
		{
			char *tracker_filename;
			char *filename;
			//get the filename.track
			char parseFileName[CHUNK_SIZE];
			stpcpy(parseFileName, clients[client_index].m_buf);
			tracker_filename = strtok(parseFileName, " ");
			tracker_filename = strtok(NULL, ">");
			
			//Check to see if tracker file already exists
			sprintf(clients[client_index].m_buf, "Tracker Files/%s", tracker_filename);
			sprintf(tracker_filename, "%s", clients[client_index].m_buf);
			
			pthread_mutex_lock(&file_mutex);
			if (access(clients[client_index].m_buf, F_OK) == 0) //If that file exists.....
			{
				//open the file
				if((clients[client_index].m_file = fopen(tracker_filename, "r")) != NULL)
				{
					write(clients[client_index].m_peer_socket, "<REP GET BEGIN>\n", strlen("<REP GET BEGIN>\n"));
					
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					int read;
					while((read = fread(clients[client_index].m_buf, sizeof(char), CHUNK_SIZE, clients[client_index].m_file)) > 0)
					{
						write(clients[client_index].m_peer_socket, clients[client_index].m_buf, read);
						memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf)); 
					}
					
					char * md5_string;
					md5_string = computeMD5(tracker_filename);
					
					memset(clients[client_index].m_buf, '\0', sizeof(clients[client_index].m_buf));
					sprintf(clients[client_index].m_buf, "\n<REP GET END %s>\n", md5_string);
					free(md5_string);
					
					write(clients[client_index].m_peer_socket, clients[client_index].m_buf, strlen(clients[client_index].m_buf));
				}
				else
				{
					write(clients[client_index].m_peer_socket, "<GET invalid>", strlen("<GET invalid>"));
				}
			}
			else
			{
				write(clients[client_index].m_peer_socket, "<GET invalid>", strlen("<GET invalid>"));
			}
			pthread_mutex_unlock(&file_mutex);
		}
	}
	
	if (close(clients[client_index].m_peer_socket) != 0)
	{
		perror("Closing socket issue");
	}
	clients[client_index].m_peer_socket = -1;
	
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
		clients[index].m_index = index;
		clients[index].m_peer_socket = -1; //default, not yet connected
	}
	return;
}

int findClientArrayOpening()
{
	int index;
	for (index = 0; index < MAX_CLIENT; index++)
	{
		if (clients[index].m_peer_socket == -1)
		{
			return index;
		}
	}
	return -1;
}