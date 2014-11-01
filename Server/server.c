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
#include <dirent.h> //for getting the number of files in a directory
#include <unistd.h> //for checking to see if a file exists or not
#include <openssl/md5.h> //for md5

#define SERVER_PORT 63122
#define MAX_CLIENT 10

//we'd read this in from the config file
//Problem is.... how? C will complain ("error: variably modified array") if we dynamically allocate like that.
#define CHUNK_SIZE 512

//socket for hosting the server (sd), socket for the client (ns)
//We should have a variable for each peer. Use this variable in the peer thread.
//Maybe construct a peer struct/class? Maybe an array of them? Contains socket variable, thread, etc.
int sd, ns;

//File to be sent
FILE *file;

//Send contents of this buffer
char buf[CHUNK_SIZE];

int main()
{
	struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) };
	struct sockaddr_in client_addr = { AF_INET };
	int length = sizeof( client_addr ); //length of socketaddr structure
	
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
	
	//In order to continuously listen for clients, I think we'll need a while(!quit); loop, or something.
	//For an example, see the "server.c" file in the "Chat Room - Reference" folder.

	//If we decide to use a Peer struct, it's socket variable would replace "ns".
	//Once a peer connects, create a thread.
	if ((ns = accept(sd, (struct sockaddr*)&client_addr, &length)) == -1)
	{
		perror("Server Error: Accept Failed");
	}

	//clear the buffer
	memset(buf, '\0', sizeof(buf));
	
	//bool goAgain = true;
	while(1)
	{
		if (read(ns, buf, CHUNK_SIZE) > 0)
		{
			if (strncmp(buf, "<createtracker", strlen("<createtracker")) == 0)
			{
				//first check for the correct number of arguments
				int num_arg = 0;
				char tempBufCopy[CHUNK_SIZE];
				char *numArgCheck;
				stpcpy(tempBufCopy, buf);
				numArgCheck = strtok(tempBufCopy, " ");
				while (numArgCheck != NULL)
				{
					num_arg = num_arg + 1;
					numArgCheck = strtok(NULL, " \n");
				}
				//If client did not send the correct number of arguments
				if (num_arg != 7)
				{
					write(ns, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
					break;
				}
			
				//<createtracker filename filesize description md5 ip port>
				char *filename, *filesize, *description, *md5, *ip, *port;
				
				char temp[CHUNK_SIZE];
				stpcpy(temp, buf);
				
				//get the filename
				filename = strtok(temp, " ");
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
				memset(buf, '\0', sizeof(buf));
				strcpy(buf, "Tracker Files/");
				strcat(buf, filename);
				strcat(buf, ".track");
				if (access(buf, F_OK) == 0)
				{
					write(ns, "<createtracker ferr>\n", strlen("<createtracker ferr>\n"));
				}
				//If it doesn't already exists, create the tracker file
				else if((file = fopen(buf, "w")) != NULL)
				{
					//memset(buf, '\0', sizeof(buf));
					strcpy(buf, "Filename: ");
					strcat(buf, filename);
					strcat(buf, "\n");
					fwrite(buf, sizeof(char), strlen(buf), file);
					
					strcpy(buf, "Filesize: ");
					strcat(buf, filesize);
					strcat(buf, "\n");
					fwrite(buf, sizeof(char), strlen(buf), file);
					
					strcpy(buf, "Description: ");
					strcat(buf, description);
					strcat(buf, "\n");
					fwrite(buf, sizeof(char), strlen(buf), file);
					
					strcpy(buf, "MD5: ");
					strcat(buf, md5);
					strcat(buf, "\n");
					fwrite(buf, sizeof(char), strlen(buf), file);
					
					strcpy(buf, ip);
					strcat(buf, ":");
					strcat(buf, port);
					strcat(buf, ":0:");
					strcat(buf, filesize);
					strcat(buf, ":");
					char temp[100];
					sprintf(temp, "%d", (unsigned)time(NULL));
					strcat(buf, temp);
					fwrite(buf, sizeof(char), strlen(buf), file);
					
					write(ns, "<createtracker succ>\n", strlen("<createtracker succ>\n"));
					fclose(file);
				}
				//if there was a problem creating the file...
				else
				{
					write(ns, "<createtracker fail>\n", strlen("<createtracker fail>\n"));
				}
				
				break;

			}
			else if (strncmp(buf, "<updatetracker", strlen("<updatetracker")) == 0)
			{
				char *filename, *start, *end, *ip, *port;
				char temp[CHUNK_SIZE];
				strcpy(temp, buf);
				
				//get the filename
				filename = strtok(temp, " ");
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
				
				//"<updatetracker test 0 100 202.202 200>\n"
				
				//Check to see if tracker file already exists
				memset(buf, '\0', sizeof(buf));
				strcpy(buf, "Tracker Files/");
				strcat(buf, filename);
				strcat(buf, ".track");
				//If it does not exist
				if (access(buf, F_OK) == -1)
				{
					write(ns, "<updatetracker ferr>\n", strlen("<updatetracke ferr>\n"));
				}
				else
				{
					if((file = fopen(buf, "a")) != NULL)
					{
						//ip port start end time
						char temp[CHUNK_SIZE];
						strcpy(temp, "\n");
						strcat(temp, ip);
						strcat(temp, ":");
						strcat(temp, port);
						strcat(temp, ":");
						strcat(temp, start);
						strcat(temp, ":");
						strcat(temp, end);
						strcat(temp, ":");
						char tempTime[100];
						sprintf(tempTime, "%d", (unsigned)time(NULL));
						strcat(temp, tempTime);
						
						fwrite(temp, sizeof(char), strlen(temp), file);
						
						write(ns, "<updatetracker succ>", strlen("<updatetracker succ>"));
					}
					else
					{
						write(ns, "<updatetracker fail>\n", strlen("<updatetracke fail>\n"));
					}
				}
				
				break;
				
			}
			else if (strncmp(buf, "<REQ LIST>", strlen("<REQ LIST>")) == 0)
			{
				DIR *tracker_directory;
				struct dirent *individual_file;
				
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
					closedir(tracker_directory);
					
					//Send the first line of the LIST response.
					stpcpy(buf, "<Rep List ");
					char cNumFiles[100];
					sprintf(cNumFiles, "%d", num_files);
					strcat(buf, cNumFiles);
					strcat(buf, ">\n");
					write(ns, buf, strlen(buf));
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
						
							memset(buf, '\0', sizeof(buf));
							char *line = NULL;
							char *filename, *filesize, *md5;
							size_t len = 0;
							
							strcpy(buf, "Tracker Files/");
							strcat(buf, individual_file->d_name);
							
							if((file = fopen(buf, "r")) != NULL)
							{
								memset(buf, '\0', sizeof(buf));
							
								strcpy(buf, "<");
								char temp[100];
								sprintf(temp, "%d",num_files);
								strcat(buf, temp);
							
								//get the filename
								getline(&line, &len, file);
								filename = strtok(line, ": ");
								filename = strtok(NULL, "\n");
								strcat(buf, filename);
								//strcat(buf, " ");
								
								//get the filesize
								getline(&line, &len, file);
								filesize = strtok(line, ": ");
								filesize = strtok(NULL, "\n");
								strcat(buf, filesize);
								//strcat(buf, " ");
								
								//skip over the file description line
								getline(&line, &len, file);
								
								//get the md5
								getline(&line, &len, file);
								md5 = strtok(line, ": ");
								md5 = strtok(NULL, "\n");
								strcat(buf, md5);
								strcat(buf, ">\n");
								
								fclose(file);
								free(line);
								
								write(ns, buf, strlen(buf));
							}
						}
					}
					memset(buf, '\0', sizeof(buf));
					strcpy(buf, "<REP LIST END>\n");
					write(ns, buf, strlen(buf));
					
					closedir(tracker_directory);
					break;
				}
			}
			else if (strncmp(buf, "<GET", strlen("<GET")) == 0)
			{
				char *tracker_filename;
				char *filename;
				//get the filename.track
				char temp[CHUNK_SIZE];
				stpcpy(temp, buf);
				tracker_filename = strtok(temp, " ");
				tracker_filename = strtok(NULL, ">");
				
				//Check to see if tracker file already exists
				memset(buf, '\0', sizeof(buf));
				strcpy(buf, "Tracker Files/");
				strcat(buf, tracker_filename);
				if (access(buf, F_OK) == 0) //If that file exists.....
				{
					//open the file
					if((file = fopen(buf, "r")) != NULL)
					{
						/*
						memset(buf, '\0', sizeof(buf));
						strcpy(buf, "<REP GET BEGIN>\n");
						//write(ns, "<REP GET BEGIN>\n", strlen("<REP GET BEGIN>\n"));
						
						//MD5_CTX md5_var;
						//unsigned char md5_sum[MD5_DIGEST_LENGTH];
						//MD5_Init(&md5_var);
						int read;
						char buf2[CHUNK_SIZE];
						while((read = fread(buf2, sizeof(char), CHUNK_SIZE, file)) > 0)
						{
							//printf("%s\n", buf);
							//write(ns, buf, read);
							//MD5_Update(&md5_var, buf, read);
							strncat(buf, buf2, read);
							memset(buf2, '\0', sizeof(buf2)); 
						}
						//MD5_Final(md5_sum, &md5_var);
						
						//write(ns, "\n<REP GET END >\n", strlen("\n<REP GET END >\n"));
						strncat(buf, "\n<REP GET END >\n", strlen("\n<REP GET END >\n"));
						write(ns, buf, strlen(buf));
						
						/*
						int i;
						char dump[(2 * MD5_DIGEST_LENGTH) + 1];
						for(i = 0; i < MD5_DIGEST_LENGTH; i++)
						{
							sprintf(dump+(i*2), "%02x", md5_sum[i]);
							write(ns, dump, sizeof(char));
							//printf("%02x", md5_sum[i]);
							printf("%s", dump);
						}
						*/
						//write(ns, ">\n", strlen(">\n"));
						
						
					//	rewind(file);
						
						
						
						
						size_t len = 0;
						char *line = NULL;
						getline(&line, &len, file);
						tracker_filename = strtok(line, " ");
						tracker_filename = strtok(NULL, "\n");
						strcat(buf, tracker_filename);
						strcpy(filename, tracker_filename);
						
						
						fclose(file);
						/////////////////////////////////////////////////////
						//break;
						
						if((file = fopen(filename, "rb")) != NULL)
						{
							memset(buf, '\0', sizeof(buf));
							int read;
							while((read = fread(buf, sizeof(char), CHUNK_SIZE, file)) > 0)
							{
								write(ns, buf, read);
								memset(buf, '\0', sizeof(buf)); 
							}
							fclose(file);
							break;
						}
						else
						{
							write(ns, "<GET invalid>", strlen("<GET invalid>"));
							break;
						}
					}
					else
					{
						write(ns, "<GET invalid>", strlen("<GET invalid>"));
						break;
					}
					
					
				}
				else
				{
					write(ns, "<GET invalid>", strlen("<GET invalid>"));
					break;
				}
				
			}
		}
	}
	
	
	
	/***************
	****************
	****************
    ****************
	****************
	//open the file to be sent
	if((file = fopen("dog.jpg", "rb")) == NULL) //r for read, b for binary
	{
		printf("Error opening file\n");
		exit(1);
	}

	//clear the buffer
	memset(buf, '\0', sizeof(buf));

	//Read a chunk, send a chunk. Repeat until entire file is read/sent.
	int read;
	while((read = fread(buf, sizeof(char), CHUNK_SIZE, file)) > 0)
	{
		//Insead of sending the entire buffer, we only send the bits that were read by fread(). This prevents us from sending garbage data.
		write(ns, buf, read);
		memset(buf, '\0', sizeof(buf)); //reclear the buffer, so we don't accidently send some tailing data.
	}
	
	fclose(file); // close the file stream
	****************
	****************
	****************
	****************/
	
	
	close(sd); //close the socket

	return (0);
}